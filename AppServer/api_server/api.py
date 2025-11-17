import os
from flask import Flask, request, jsonify
import paho.mqtt.client as mqtt
from influxdb_client import InfluxDBClient
from influxdb_client.client.write_api import SYNCHRONOUS

app = Flask(__name__)

# --- Configurações (lidas do ambiente) ---
INFLUXDB_URL = os.getenv('INFLUXDB_URL')
INFLUXDB_TOKEN = os.getenv('INFLUXDB_TOKEN')
INFLUXDB_ORG = os.getenv('INFLUXDB_ORG')
INFLUXDB_BUCKET = os.getenv('INFLUXDB_BUCKET', 'sensores') # Valor padrão 'sensores'
MQTT_BROKER_HOST = os.getenv('MQTT_BROKER_HOST')
MQTT_BROKER_PORT = int(os.getenv('MQTT_BROKER_PORT'))

# --- Conexões ---
try:
    # Conexão InfluxDB
    print(f"Conectando ao InfluxDB em {INFLUXDB_URL}...")
    influx_client = InfluxDBClient(url=INFLUXDB_URL, token=INFLUXDB_TOKEN, org=INFLUXDB_ORG)
    query_api = influx_client.query_api()
    print("Conectado ao InfluxDB com sucesso!")

    # Conexão MQTT (para publicar configurações)
    print(f"Conectando ao MQTT Broker em {MQTT_BROKER_HOST}...")
    mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)
    mqtt_client.connect(MQTT_BROKER_HOST, MQTT_BROKER_PORT, 60)
    mqtt_client.loop_start() # Roda em thread de background
    print("Conectado ao MQTT com sucesso!")

except Exception as e:
    print(f"Erro fatal na inicialização: {e}")
    # Em um cenário real, você poderia querer que o container falhasse se não puder conectar.

# --- Rotas da API ---

@app.route('/health')
def health_check():
    """Verifica se a API está no ar."""
    return jsonify({"status": "API Server is running"})

@app.route('/<device_id>/sensors/<sensor_id>/read')
def get_data(device_id, sensor_id):
    """
    Busca dados históricos do InfluxDB.
    Parâmetros de Query (URL):
    ?start= : Período de início (ex: -1h, -5m, -1d). Padrão: -1h
    ?every= : Intervalo de agregação (ex: 1m, 5s, 10m). Padrão: Retorna dados brutos.
    ?measurement = : Medida que vai ser utilizada. Padrão: Todas as medidas.
    """
    
    # Obter parâmetros da query string (ex: ?start=-1h&every=5m)
    start_range = request.args.get('start', '-1h') # Padrão: última hora
    every_window = request.args.get('every') # Padrão: null (sem agregação)
    measurement = request.args.get('measurement') # Padrão: null (sem filtro)

    # Montar a query Flux dinamicamente
    q_influx_parts = [
        f'from(bucket: "{INFLUXDB_BUCKET}")',
        f'|> range(start: {start_range})',
        f'|> filter(fn: (r) => r["device_id"] == "{device_id}")',
        f'|> filter(fn: (r) => r["sensor"] == "{sensor_id}")',
        '|> filter(fn: (r) => r["_field"] == "value")', # Como definido no seu ingestor
    ]

    if measurement:
        q_influx_parts.append(f'|> filter(fn: (r) => r["_measurement"] == "{measurement}")')

    # Adicionar agregação (média) se 'every' foi fornecido
    if every_window:
        q_influx_parts.append(f'|> aggregateWindow(every: {every_window}, fn: mean, createEmpty: false)')
        q_influx_parts.append('|> yield(name: "mean")')
    else:
        # Se não agregar, apenas retorna os valores brutos
        q_influx_parts.append('|> yield(name: "raw")')

    # Junta as partes da query
    q_influx = "\n".join(q_influx_parts)
    
    print(f"--- Executando Query Influx ---\n{q_influx}\n---------------------------------")

    # Executar a query e processar o resultado
    try:
        result = query_api.query(org=INFLUXDB_ORG, query=q_influx)
        
        # Converter o resultado em um JSON simples
        data_points = []
        for table in result:
            for record in table.records:
                point = {
                    "time": record.get_time().isoformat(), # Converte data/hora para string ISO
                    "value": record.get_value(),
                    "measurement": record.get_measurement(),
                }
                data_points.append(point)
        
        # Retornar o JSON
        return jsonify(data_points)

    except Exception as e:
        print(f"Erro ao consultar InfluxDB: {e}")
        return jsonify({"error": str(e)}), 500

@app.route('/config/<device_id>', methods=['POST'])
def set_config(device_id):
    """
    Envia uma nova configuração (JSON) para um dispositivo via MQTT.
    """
    try:
        # Pega o JSON enviado no corpo (body) da requisição
        config_json = request.data.decode('utf-8')
        
        # Tópico MQTT que o ESP32 está escutando para 'put'
        # Baseado em: ESP32codes/lib/Trabalho/src/MQTTsettings.cpp
        topic = f"config/{device_id}/put" 
        
        # Publica a nova configuração
        (result, mid) = mqtt_client.publish(topic, config_json, qos=1) # QoS 1 para garantir entrega
        
        if result == mqtt.MQTT_ERR_SUCCESS:
            print(f"Publicada nova config para {topic} (MID: {mid})")
            return jsonify({"status": "config_sent", "device": device_id, "topic": topic})
        else:
            print(f"Erro ao publicar no MQTT (Código: {result})")
            return jsonify({"error": "Failed to publish to MQTT broker", "code": result}), 500

    except Exception as e:
        print(f"Erro ao processar /config: {e}")
        return jsonify({"error": str(e)}), 400 # 400 Bad Request

if __name__ == '__main__':
    print("Iniciando API server Flask...")
    app.run(host='0.0.0.0', port=5000, debug=True) # debug=True é útil para desenvolvimento