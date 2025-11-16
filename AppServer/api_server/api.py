import os
from threading import Lock
from flask import Flask, request, jsonify
from flask_cors import CORS
import paho.mqtt.client as mqtt
from influxdb_client import InfluxDBClient
from influxdb_client.client.write_api import SYNCHRONOUS
import json
import threading
import time
import requests

app = Flask(__name__)
CORS(app)  # Enable CORS for all routes

# --- Configurações (lidas do ambiente) ---
INFLUXDB_URL = os.getenv('INFLUXDB_URL')
INFLUXDB_TOKEN = os.getenv('INFLUXDB_TOKEN')
INFLUXDB_ORG = os.getenv('INFLUXDB_ORG')
INFLUXDB_BUCKET = os.getenv('INFLUXDB_BUCKET', 'sensores') # Valor padrão 'sensores'
INFLUXDB_HEADER = {'Authorization':f'Token {INFLUXDB_TOKEN}'}
ENDPOINT_NAME = os.getenv('ENDPOINT_NAME')
MQTT_BROKER_HOST = os.getenv('MQTT_BROKER_HOST')
MQTT_BROKER_PORT = int(os.getenv('MQTT_BROKER_PORT'))
MQTT_TOPIC = "callback/#" 

# --- Cache for storing ESP32 responses ---
# Structure: { "device_id": { "sensors": {...}, "wifi": {...}, "timestamp": ... } }
config_cache = {}
config_cache_lock = threading.Lock()

# --- Cache for storing rules responses ---
# Structure: { "rules": {...}, "timestamp": ... }
rules_cache = {}
rules_cache_lock = threading.Lock()

# --- MQTT Callbacks ---
def on_message(client, userdata, message):
    """
    Callback para quando uma mensagem MQTT é recebida.
    Armazena respostas de configuração do ESP32 no cache.
    """
    topic = message.topic
    payload = message.payload.decode('utf-8')
    
    print(f"📨 Mensagem MQTT recebida no tópico: {topic}", flush=True)
    print(f"   Payload: {payload}", flush=True)  # Show full payload
    
    try:
        parts = topic.split('/')
        print(f"   Topic parts: {parts}")
        
        # Handle new response pattern: <device_id>/settings/sensors/{operation}/response
        if len(parts) >= 5 and parts[1] == 'settings' and parts[2] == 'sensors' and parts[4] == 'response':
            device_id = parts[0]
            operation = parts[3]  # 'get', 'set', or 'remove'
            
            print(f"   🔍 Detectado: device_id={device_id}, operation={operation}")
            
            # Parse response (could be JSON or simple string like "OK"/"ERROR")
            try:
                data = json.loads(payload) if payload.strip() else {}
                print(f"   📦 Dados parseados (JSON): {data}")
            except:
                data = payload  # Simple string response
                print(f"   📦 Dados parseados (string): {data}")
            
            # Store in cache with operation-specific key
            with config_cache_lock:
                if device_id not in config_cache:
                    config_cache[device_id] = {}
                cache_key = f'sensors_{operation}_response'
                config_cache[device_id][cache_key] = {
                    'data': data,
                    'timestamp': time.time()
                }
            
            print(f"✅ Resposta '{operation}' de '{device_id}' armazenada no cache com chave '{cache_key}'")
            print(f"   Cache atual para {device_id}: {list(config_cache[device_id].keys())}")
            return
        
        # Legacy: Parse do tópico: config/{device_id}/{type}
        if len(parts) >= 3 and parts[0] == 'config':
            device_id = parts[1]
            config_type = parts[2]  # 'sensors' ou 'wifi'
            
            # Parse JSON payload
            data = json.loads(payload)
            
            # Armazena no cache
            with config_cache_lock:
                if device_id not in config_cache:
                    config_cache[device_id] = {}
                config_cache[device_id][config_type] = {
                    'data': data,
                    'timestamp': time.time()
                }
            
            print(f"✅ Configuração '{config_type}' de '{device_id}' armazenada no cache")
            return
        
        # Tratar resposta de regras via callback/rules
        if topic == 'callback/rules':
            data = json.loads(payload)
            
            # Armazena no cache de regras
            with rules_cache_lock:
                rules_cache['rules'] = {
                    'data': data,
                    'timestamp': time.time()
                }
            
            print(f"✅ Regras recebidas e armazenadas no cache")
            return
    
    except Exception as e:
        print(f"❌ Erro ao processar mensagem MQTT: {e}")

def on_connect(client, userdata, flags, rc):
    """Callback quando conecta ao broker MQTT."""
    if rc == 0:
        print("✅ Conectado ao MQTT Broker com sucesso!")
        print(f"   Broker: {MQTT_BROKER_HOST}:{MQTT_BROKER_PORT}")
        print(f"   Client ID: {client._client_id.decode() if hasattr(client._client_id, 'decode') else client._client_id}")
        
        # Subscreve aos tópicos de resposta dos ESP32s
        client.subscribe("+/settings/sensors/get/response")  # New pattern
        client.subscribe("+/settings/sensors/set/response")  # New pattern
        client.subscribe("+/settings/sensors/remove/response")  # New pattern
        client.subscribe("config/+/sensors")  # Legacy support
        client.subscribe("config/+/wifi")
        client.subscribe(MQTT_TOPIC)
        print("📡 Subscrito aos tópicos de resposta de sensores e WiFi")
        print("   Tópicos subscritos:")
        print("   - +/settings/sensors/get/response")
        print("   - +/settings/sensors/set/response")
        print("   - +/settings/sensors/remove/response")
        print("   - config/+/sensors")
        print("   - config/+/wifi")
        print(f"   - {MQTT_TOPIC}")
    else:
        print(f"❌ Falha na conexão MQTT. Código de retorno: {rc}")

# --- Conexões ---
try:
    influx_client = InfluxDBClient(url=INFLUXDB_URL, token=INFLUXDB_TOKEN, org=INFLUXDB_ORG)
    query_api = influx_client.query_api()
    print("Conectado ao InfluxDB com sucesso!")

    # Conexão MQTT (para publicar configurações e receber respostas)
    print(f"Conectando ao MQTT Broker em {MQTT_BROKER_HOST}...")
    mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message
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
def health_rules():
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
        f'|> filter(fn: (r) => r["_measurement"] == "{sensor_id}")',  # sensor_id is the measurement name
        '|> filter(fn: (r) => r["_field"] == "value")',
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
def get_sensors_config(device_id):
    """
    Solicita a configuração de sensores do dispositivo via MQTT e aguarda resposta.
    Resposta esperada em: <device_id>/settings/sensors/get/response
    Timeout de 5 segundos.
    """
    try:
        cache_key = 'sensors_get_response'
        
        # Limpa cache antigo para este device
        with config_cache_lock:
            if device_id in config_cache and cache_key in config_cache[device_id]:
                del config_cache[device_id][cache_key]
        
        # Envia requisição MQTT
        request_topic = f"{device_id}/settings/sensors/get"
        mqtt_client.publish(request_topic, "", qos=1)
        print(f"📤 GET sensors solicitado: {request_topic}")
        print(f"   Aguardando resposta em: {device_id}/settings/sensors/get/response")
        print(f"   MQTT conectado: {mqtt_client.is_connected()}")
        
        # Aguarda resposta (polling no cache)
        timeout = 5
        start_time = time.time()
        
        while (time.time() - start_time) < timeout:
            with config_cache_lock:
                if device_id in config_cache and cache_key in config_cache[device_id]:
                    response_data = config_cache[device_id][cache_key]['data']
                    print(f"✅ Resposta GET recebida após {time.time() - start_time:.2f}s")
                    return jsonify(response_data)
            time.sleep(0.1)
        
        # Timeout
        print(f"⏱️ Timeout aguardando resposta de {device_id}")
        return jsonify({
            "error": "timeout",
            "message": f"ESP32 '{device_id}' não respondeu em {timeout} segundos.",
            "sensors": []
        }), 408

    except Exception as e:
        print(f"Erro ao solicitar configuração de sensores: {e}")
        return jsonify({"error": str(e)}), 500

@app.route('/<device_id>/settings/wifi/get')
def get_wifi_config(device_id):
    """
    Solicita a configuração WiFi do dispositivo via MQTT e aguarda resposta.
    Timeout de 5 segundos.
    """
    try:
        # Primeiro, verifica se temos cache recente (< 10 segundos)
        with config_cache_lock:
            if device_id in config_cache and 'wifi' in config_cache[device_id]:
                cache_age = time.time() - config_cache[device_id]['wifi']['timestamp']
                if cache_age < 10:  # Cache válido por 10 segundos
                    print(f"📦 Retornando configuração WiFi do cache (idade: {cache_age:.1f}s)")
                    return jsonify(config_cache[device_id]['wifi']['data'])
        
        # Limpa cache antigo para este device
        with config_cache_lock:
            if device_id in config_cache and 'wifi' in config_cache[device_id]:
                del config_cache[device_id]['wifi']
        
        # Envia requisição MQTT
        request_topic = f"config/{device_id}/wifi/get"
        mqtt_client.publish(request_topic, "", qos=1)
        print(f"📤 Solicitação WiFi enviada via MQTT: {request_topic}")
        
        # Aguarda resposta (polling no cache)
        timeout = 5  # segundos
        start_time = time.time()
        
        while (time.time() - start_time) < timeout:
            with config_cache_lock:
                if device_id in config_cache and 'wifi' in config_cache[device_id]:
                    print(f"✅ Resposta WiFi recebida do ESP32 após {time.time() - start_time:.2f}s")
                    return jsonify(config_cache[device_id]['wifi']['data'])
            time.sleep(0.1)  # Aguarda 100ms antes de verificar novamente
        
        # Timeout - ESP32 não respondeu
        print(f"⏱️ Timeout aguardando resposta WiFi de {device_id}")
        return jsonify({
            "error": "timeout",
            "message": f"ESP32 '{device_id}' não respondeu em {timeout} segundos. Verifique se o dispositivo está online."
        }), 408  # 408 Request Timeout

    except Exception as e:
        print(f"Erro ao solicitar configuração WiFi: {e}")
        return jsonify({"error": str(e)}), 500

@app.route('/<device_id>/settings/sensors/set', methods=['POST'])
def set_sensors_config(device_id):
    """
    Envia apenas o sensor novo/modificado para o dispositivo via MQTT.
    Espera resposta "OK" ou "ERROR" em: <device_id>/settings/sensors/set/response
    
    Espera JSON no body da requisição:
    {
      "sensors": [
        {
          "id": "sensor_pin_4",
          "pin": 4,
          "type": "sensor",
          "model": "DHT22",
          "protocol": "ADC"
        }
      ]
    }
    """
    try:
        new_config = request.get_json()
        if not new_config or 'sensors' not in new_config:
            return jsonify({"error": "Invalid payload. Expected {sensors: [...]}"}), 400
        
        new_sensors = new_config['sensors']
        if not isinstance(new_sensors, list):
            return jsonify({"error": "sensors must be an array"}), 400
        
        print(f"📝 SET sensor(es) em {device_id}: {len(new_sensors)} sensor(es)")
        
        cache_key = 'sensors_set_response'
        
        # Limpa cache de resposta
        with config_cache_lock:
            if device_id in config_cache and cache_key in config_cache[device_id]:
                del config_cache[device_id][cache_key]
        
        # Envia apenas os novos sensores (não faz merge aqui)
        topic = f"{device_id}/settings/sensors/set"
        payload = json.dumps(new_config)
        
        print(f"📤 Publicando no MQTT:", flush=True)
        print(f"   Tópico: {topic}", flush=True)
        print(f"   Payload: {payload}", flush=True)
        print(f"   Broker: {MQTT_BROKER_HOST}:{MQTT_BROKER_PORT}", flush=True)
        
        (result, mid) = mqtt_client.publish(topic, payload, qos=1)
        
        print(f"   Resultado da publicação: {result} (0=sucesso, outros=erro)", flush=True)
        print(f"   Message ID: {mid}", flush=True)
        
        if result != mqtt.MQTT_ERR_SUCCESS:
            print(f"❌ Falha ao publicar no MQTT broker: código {result}")
            return jsonify({"error": "Failed to publish to MQTT broker", "code": result}), 500
        
        print(f"📤 Sensor config enviado para {topic}")
        print(f"   Aguardando resposta em: {device_id}/settings/sensors/set/response")
        
        # Aguarda resposta OK/ERROR
        timeout = 5
        start_time = time.time()
        
        while (time.time() - start_time) < timeout:
            with config_cache_lock:
                if device_id in config_cache and cache_key in config_cache[device_id]:
                    response = config_cache[device_id][cache_key]['data']
                    elapsed = time.time() - start_time
                    
                    if response == "OK" or (isinstance(response, dict) and response.get('status') == 'OK'):
                        print(f"✅ ESP32 confirmou SET após {elapsed:.2f}s")
                        return jsonify({
                            "status": "success",
                            "message": "Sensor configuration applied successfully",
                            "device": device_id
                        })
                    else:
                        print(f"❌ ESP32 retornou erro: {response}")
                        return jsonify({
                            "status": "error",
                            "message": f"ESP32 returned error: {response}",
                            "device": device_id
                        }), 400
            time.sleep(0.1)
        
        # Timeout
        print(f"⏱️ Timeout aguardando confirmação de {device_id}")
        return jsonify({
            "error": "timeout",
            "message": f"ESP32 '{device_id}' não confirmou a operação em {timeout} segundos."
        }), 408

    except Exception as e:
        print(f"Erro ao processar SET de sensores: {e}")
        import traceback
        traceback.print_exc()
        return jsonify({"error": str(e)}), 400

@app.route('/<device_id>/sensors/remove', methods=['POST'])
def remove_sensor(device_id):
    """
    Remove um sensor do dispositivo via MQTT.
    Espera resposta "OK" ou "ERROR" em: <device_id>/settings/sensors/remove/response
    
    Espera JSON no body da requisição:
    {
      "sensor_id": "sensor_pin_4"
    }
    """
    try:
        data = request.get_json()
        if not data or 'sensor_id' not in data:
            return jsonify({"error": "Invalid payload. Expected {sensor_id: ...}"}), 400
        
        sensor_id = data['sensor_id']
        print(f"🗑️ REMOVE sensor '{sensor_id}' de {device_id}")
        
        cache_key = 'sensors_remove_response'
        
        # Limpa cache de resposta
        with config_cache_lock:
            if device_id in config_cache and cache_key in config_cache[device_id]:
                del config_cache[device_id][cache_key]
        
        # Envia requisição de remoção
        topic = f"{device_id}/settings/sensors/remove"
        payload = json.dumps({"id": sensor_id})
        
        (result, mid) = mqtt_client.publish(topic, payload, qos=1)
        
        if result != mqtt.MQTT_ERR_SUCCESS:
            return jsonify({"error": "Failed to publish to MQTT broker", "code": result}), 500
        
        print(f"📤 Remove enviado para {topic}")
        print(f"   Aguardando resposta em: {device_id}/settings/sensors/remove/response")
        
        # Aguarda resposta OK/ERROR
        timeout = 5
        start_time = time.time()
        
        while (time.time() - start_time) < timeout:
            with config_cache_lock:
                if device_id in config_cache and cache_key in config_cache[device_id]:
                    response = config_cache[device_id][cache_key]['data']
                    elapsed = time.time() - start_time
                    
                    if response == "OK" or (isinstance(response, dict) and response.get('status') == 'OK'):
                        print(f"✅ ESP32 confirmou REMOVE após {elapsed:.2f}s")
                        return jsonify({
                            "status": "success",
                            "message": f"Sensor '{sensor_id}' removed successfully",
                            "device": device_id
                        })
                    else:
                        print(f"❌ ESP32 retornou erro: {response}")
                        return jsonify({
                            "status": "error",
                            "message": f"ESP32 returned error: {response}",
                            "device": device_id
                        }), 400
            time.sleep(0.1)
        
        # Timeout
        print(f"⏱️ Timeout aguardando confirmação de {device_id}")
        return jsonify({
            "error": "timeout",
            "message": f"ESP32 '{device_id}' não confirmou a remoção em {timeout} segundos."
        }), 408

    except Exception as e:
        print(f"Erro ao processar REMOVE de sensor: {e}")
        import traceback
        traceback.print_exc()
        return jsonify({"error": str(e)}), 400

@app.route('/<device_id>/settings/wifi/set', methods=['POST'])
def set_wifi_config(device_id):
    """
    Envia configuração WiFi para o dispositivo via MQTT.
    WiFi settings geralmente substituem completamente (não fazem merge).
    
    Espera JSON no body da requisição:
    {
      "ssid": "MyWiFi",
      "password": "MyPassword",
      "mqtt_broker": "192.168.1.10",
      "mqtt_device_id": "esp32_device_1"
    }
    """
    try:
        wifi_config = request.get_json()
        if not wifi_config:
            return jsonify({"error": "Invalid payload. Expected JSON object"}), 400
        
        print(f"📝 Recebida configuração WiFi para {device_id}")
        print(f"   SSID: {wifi_config.get('ssid', 'N/A')}")
        print(f"   MQTT Broker: {wifi_config.get('mqtt_broker', 'N/A')}")
        
        topic = f"config/{device_id}/wifi/set"
        payload = json.dumps(wifi_config)
        
        (result, mid) = mqtt_client.publish(topic, payload, qos=1)
        
        if result == mqtt.MQTT_ERR_SUCCESS:
            print(f"✅ Configuração WiFi enviada para {topic} (MID: {mid})")
            
            # Atualiza cache local (sem password por segurança)
            safe_config = wifi_config.copy()
            if 'password' in safe_config:
                safe_config['password'] = '***'
            
            with config_cache_lock:
                if device_id not in config_cache:
                    config_cache[device_id] = {}
                config_cache[device_id]['wifi'] = {
                    'data': safe_config,
                    'timestamp': time.time()
                }
            
            return jsonify({
                "status": "config_sent",
                "device": device_id,
                "topic": topic,
                "note": "ESP32 will restart to apply WiFi settings"
            })
        else:
            print(f"❌ Erro ao publicar no MQTT (Código: {result})")
            return jsonify({"error": "Failed to publish to MQTT broker", "code": result}), 500

    except Exception as e:
        print(f"Erro ao processar configuração WiFi: {e}")
        import traceback
        traceback.print_exc()
        return jsonify({"error": str(e)}), 400

@app.route('/<device_id>/settings/device/reset', methods=['POST'])
def reset_device(device_id):
    """
    Envia comando de reset para o dispositivo via MQTT e limpa o cache local.
    Não espera resposta - fire and forget.
    
    Topic: <device_id>/settings/device/reset
    """
    try:
        print(f"🔄 Reset solicitado para {device_id}", flush=True)
        
        topic = f"{device_id}/settings/device/reset"
        payload = ""  # Empty payload
        
        (result, mid) = mqtt_client.publish(topic, payload, qos=1)
        
        if result == mqtt.MQTT_ERR_SUCCESS:
            print(f"✅ Comando de reset enviado para {topic} (MID: {mid})", flush=True)
            
            # Limpa cache local do dispositivo
            with config_cache_lock:
                if device_id in config_cache:
                    del config_cache[device_id]
                    print(f"🗑️ Cache do dispositivo {device_id} removido", flush=True)
            
            return jsonify({
                "status": "reset_sent",
                "device": device_id,
                "topic": topic,
                "message": "Reset command sent to device. All configuration cleared."
            })
        else:
            print(f"❌ Erro ao publicar reset no MQTT (Código: {result})", flush=True)
            return jsonify({"error": f"MQTT publish failed (code: {result})"}), 500

    except Exception as e:
        print(f"Erro ao processar reset do dispositivo: {e}", flush=True)
        import traceback
        traceback.print_exc()
        return jsonify({"error": str(e)}), 400

@app.route('/config/<device_id>', methods=['POST'])
def set_config(device_id):
    """
    [DEPRECATED] Use /<device_id>/settings/sensors/set or /<device_id>/settings/wifi/set instead.
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

@app.route('/rules', methods=['GET', 'POST', 'PUT', 'DELETE'])
def manage_rules():
    """
    Gerencia regras de automação globais (podem envolver múltiplos dispositivos)
    
    GET /rules - Lista todas as regras
    POST /rules - Cria nova regra
    PUT /rules - Atualiza regra (rule_id no JSON)
    DELETE /rules - Remove regra (rule_id no JSON)
    """
    
    if request.method == 'GET':
        return _get_rules()
    elif request.method == 'POST':
        return _create_rule()
    elif request.method == 'PUT':
        return _update_rule()
    elif request.method == 'DELETE':
        return _delete_rule()

def _get_rules():
    """
    Solicita listagem de todas as regras ao ingestor via MQTT e aguarda resposta.
    Similar a get_sensors_config().
    Tópico: rules/get
    Resposta esperada em: callback/rules
    Timeout: 5 segundos
    """
    try:
        # Primeiro, verifica se temos cache recente (< 10 segundos)
        with rules_cache_lock:
            if 'rules' in rules_cache:
                cache_age = time.time() - rules_cache['rules']['timestamp']
                if cache_age < 10:  # Cache válido por 10 segundos
                    print(f"📦 Retornando regras do cache (idade: {cache_age:.1f}s)")
                    return jsonify(rules_cache['rules']['data'])
        
        # Limpa cache antigo
        with rules_cache_lock:
            if 'rules' in rules_cache:
                del rules_cache['rules']
        
        # Envia requisição MQTT
        request_topic = "rules/get"
        mqtt_client.publish(request_topic, "{}", qos=1)
        print(f"📤 Solicitação enviada via MQTT: {request_topic}")
        
        # Aguarda resposta (polling no cache)
        timeout = 5  # segundos
        start_time = time.time()
        
        while (time.time() - start_time) < timeout:
            with rules_cache_lock:
                if 'rules' in rules_cache:
                    elapsed = time.time() - start_time
                    print(f"✅ Resposta recebida após {elapsed:.2f}s")
                    return jsonify(rules_cache['rules']['data'])
            time.sleep(0.1)  # Aguarda 100ms antes de verificar novamente
        
        # Timeout - Ingestor não respondeu
        print(f"⏱️ Timeout aguardando resposta de regras")
        return jsonify({
            "error": "timeout",
            "message": f"Ingestor não respondeu em {timeout} segundos. Verifique se o serviço está online.",
            "rules": []
        }), 408  # 408 Request Timeout

    except Exception as e:
        print(f"Erro ao solicitar regras: {e}")
        return jsonify({"error": str(e)}), 500

def _create_rule():
    """
    Cria uma nova regra de automação (pode envolver múltiplos dispositivos).
    
    Body (JSON):
{
  "id_regra": "1",
  "condicao": [
    {
      "tipo": "limite",
      "tempo": 5,
      "id_device": "12",
      "id_sensor": "23",
      "medida": "temperatura",
      "operador": ">",
      "valor_limite": 80
    }
  ],
  "entao": [
    {
      "id_device": "13",
      "id_atuador": "15",
      "tempo": 5,
      "valor": 1
    }
  ],
  "senao": []
}
    """
    try:
        rule_data = request.get_json()
        
        if not rule_data:
            return jsonify({"error": "Empty rule data"}), 400
        
        if 'id_regra' not in rule_data or 'condicao' not in rule_data or 'entao' not in rule_data or 'senao' not in rule_data:
            return jsonify({"error": "Missing required fields: name, conditions, actions"}), 400
        
        topic = "rules/add"
        payload = json.dumps(rule_data)
        
        (result, mid) = mqtt_client.publish(topic, payload, qos=1)
        
        if result == mqtt.MQTT_ERR_SUCCESS:
            return jsonify({
                "status": "rule_created",
                "rule": rule_data,
                "message": "Rule sent to ingestor for processing"
            }), 201
        else:
            return jsonify({"error": f"MQTT publish failed (code: {result})"}), 500
            
    except Exception as e:
        return jsonify({"error": str(e)}), 400

def _update_rule():
    """
    Atualiza uma regra existente.
    O rule_id deve estar no JSON do body.
    
    Body (JSON):
    {
      "id_regra": "rule_123",
      "name": "Regra atualizada",
      "enabled": false,
      ...
    }
    """
    try:
        rule_data = request.get_json()
        
        if not rule_data:
            return jsonify({"error": "Empty rule data"}), 400
        
        rule_id = rule_data.get('id_regra')
        
        if not rule_id:
            return jsonify({"error": "rule_id is required in JSON body"}), 400
        
        topic = "rules/update"
        payload = json.dumps(rule_data)
        
        (result, mid) = mqtt_client.publish(topic, payload, qos=1)
        
        if result == mqtt.MQTT_ERR_SUCCESS:
            return jsonify({
                "status": "rule_updated",
                "rule_id": rule_id,
                "message": "Rule update sent to ingestor"
            }), 200
        else:
            return jsonify({"error": f"MQTT publish failed (code: {result})"}), 500
            
    except Exception as e:
        return jsonify({"error": str(e)}), 400

def _delete_rule():
    """
    Remove uma regra.
    O id_regra deve estar no JSON do body.
    Body (JSON):
    {
      "id_regra": "rule_123"
    }
    """
    try:
        rule_data = request.get_json()
        if not rule_data:
            return jsonify({"error": "Empty rule data"}), 400

        rule_id = rule_data.get('id_regra')
        if not rule_id:
            return jsonify({"error": "id_regra is required in JSON body"}), 400

        topic = "rules/delete"
        # publica com a chave que o ingestor espera: 'id_regra'
        payload = json.dumps({"id_regra": rule_id})

        (result, mid) = mqtt_client.publish(topic, payload, qos=1)

        if result == mqtt.MQTT_ERR_SUCCESS:
            return jsonify({
                "status": "rule_deleted",
                "id_regra": rule_id,
                "message": "Rule deletion sent to ingestor"
            }), 200
        else:
            return jsonify({"error": f"MQTT publish failed (code: {result})"}), 500

    except Exception as e:
        return jsonify({"error": str(e)}), 400

    
if __name__ == '__main__':
    import sys
    sys.stdout.flush()
    sys.stderr.flush()
    print("Iniciando API server Flask...", flush=True)
    app.run(host='0.0.0.0', port=5000, debug=True, use_reloader=False) # debug=True é útil para desenvolvimento