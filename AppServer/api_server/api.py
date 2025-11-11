import os
from threading import Lock
from flask import Flask, request, jsonify
from influxdb_client import InfluxDBClient
from influxdb_client.client.write_api import SYNCHRONOUS
import paho.mqtt.client as mqtt
import time
import json

app = Flask(__name__)

# --- Configurações (lidas do ambiente) ---
INFLUXDB_URL = os.getenv('INFLUXDB_URL')
INFLUXDB_TOKEN = os.getenv('INFLUXDB_TOKEN')
INFLUXDB_ORG = os.getenv('INFLUXDB_ORG')
INFLUXDB_BUCKET = os.getenv('INFLUXDB_BUCKET', 'sensores') # Valor padrão 'sensores'
MQTT_BROKER_HOST = os.getenv('MQTT_BROKER_HOST')
MQTT_BROKER_PORT = int(os.getenv('MQTT_BROKER_PORT'))
MQTT_TOPIC = "callback/#" 

# --- Conexão com InfluxDB ---
print("Conectando ao InfluxDB...")
try:
    influx_client = InfluxDBClient(url=INFLUXDB_URL, token=INFLUXDB_TOKEN, org=INFLUXDB_ORG)
    # Verifica se a conexão está ok (opcional, mas bom para debug)
    health = influx_client.health()
    if health.status == "pass":
        print("Conectado ao InfluxDB com sucesso!")
    else:
        print(f"Erro na saúde do InfluxDB: {health.message}")
    
    write_api = influx_client.write_api(write_options=SYNCHRONOUS)

except Exception as e:
    print(f"Erro fatal ao conectar ao InfluxDB: {e}")
    exit(1) # Sai do script se não puder conectar ao DB

# --- Funções MQTT (Atualizadas para API v2) ---

# Dicionário para armazenar as respostas MQTT por device_id
mqtt_responses = {}
mqtt_lock = Lock()

def on_connect(client, userdata, flags, reason_code, properties):
    """ Callback para quando o cliente se conecta ao broker """
    if reason_code == 0:
        print(f"Conectado ao Broker MQTT! ({MQTT_BROKER_HOST})")
        # Após conectar, se inscreve no tópico
        client.subscribe(MQTT_TOPIC)
    else:
        # reason_code 0 é sucesso. Outros valores indicam falha.
        print(f"Falha ao conectar, código de razão: {reason_code}")

def on_subscribe(client, userdata, mid, reason_codes, properties):
    """ Callback para quando o broker confirma a inscrição """
    # reason_codes é uma lista de códigos, um para cada tópico.
    # Como só nos inscrevemos em um, verificamos o primeiro.
    if reason_codes and not reason_codes[0].is_failure:
        print(f"Inscrito com sucesso no tópico: {MQTT_TOPIC}")
    else:
        print(f"Falha ao se inscrever no tópico. Código(s): {reason_codes}")

def on_disconnect(client, userdata, disconnect_flags, reason_code, properties):
    """ Callback para quando o cliente se desconecta """
    if reason_code == 0:
        print("Desconexão do Broker MQTT bem-sucedida.")
    else:
        print(f"Desconexão inesperada do Broker MQTT. Código: {reason_code}")
        print("O Paho-MQTT tentará reconectar automaticamente...")

def on_message(client, userdata, msg):
    """ Callback para quando uma mensagem é recebida (assinatura V1/V2 compatível) """
    try:
        payload = msg.payload.decode('utf-8')
        print(f"Mensagem recebida: Tópico[{msg.topic}] Payload[{payload}]")

        # Verifica se é uma resposta de callback/config
        if msg.topic.startswith("callback/") and msg.topic.endswith("/config"):
            parts = msg.topic.split('/')
            if len(parts) >= 2:
                device_id = parts[1]
                
                # Armazena a resposta com thread-safety
                with mqtt_lock:
                    mqtt_responses[device_id] = payload
                print(f"Resposta de config armazenada para {device_id}")
                return

    except Exception as e:
        print(f"Erro ao processar mensagem: {e}")

# --- Conexão com MQTT ---

# 1. MUDANÇA: Alterado de VERSION1 para VERSION2
mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

# 2. MUDANÇA: Registrando os novos callbacks
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message
mqtt_client.on_subscribe = on_subscribe
mqtt_client.on_disconnect = on_disconnect

print("Conectando ao Broker MQTT...")
try:
    mqtt_client.connect(MQTT_BROKER_HOST, MQTT_BROKER_PORT, 60)
except Exception as e:
    print(f"Não foi possível conectar ao Broker MQTT: {e}")
    exit(1)

# Loop principal para manter o script rodando
# loop_forever() gerencia reconexões automaticamente
try:
    mqtt_client.loop_start()
except KeyboardInterrupt:
    print("Script interrompido pelo usuário. Desconectando...")
    mqtt_client.disconnect()
    influx_client.close()
    print("Desconectado.")

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

@app.route('/<device_id>/settings/sensors/get')
def get_config(device_id):
    """
    Requisita a configuração do dispositivo via MQTT e aguarda a resposta.
    Timeout: 5 segundos
    """
    try:
        # Limpar resposta anterior
        with mqtt_lock:
            mqtt_responses[device_id] = None
        
        # Publicar requisição de configuração
        topic = f"config/{device_id}/get"
        result, mid = mqtt_client.publish(topic, "1", qos=1)
        
        if result != mqtt.MQTT_ERR_SUCCESS:
            return jsonify({"error": "Failed to publish to MQTT broker", "code": result}), 500
        
        print(f"Requisição de config enviada para {topic}")
        
        # Aguardar a resposta com timeout de 5 segundos
        timeout = 5
        elapsed = 0
        interval = 0.1
        
        while elapsed < timeout:
            with mqtt_lock:
                if mqtt_responses.get(device_id) is not None:
                    response = mqtt_responses[device_id]
                    mqtt_responses[device_id] = None  # Limpar
                    
                    # Tentar parsear como JSON
                    try:
                        config_data = json.loads(response)
                        return jsonify(config_data)
                    except json.JSONDecodeError:
                        return jsonify({"raw_config": response})
            
            time.sleep(interval)
            elapsed += interval
        
        # Timeout
        return jsonify({"error": "No response from device", "device_id": device_id, "timeout": timeout}), 504

    except Exception as e:
        print(f"Erro ao processar /settings/sensors/get: {e}")
        return jsonify({"error": str(e)}), 400

@app.route('/<device_id>/settings/sensors/set', methods=['POST'])
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