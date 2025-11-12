import paho.mqtt.client as mqtt
from influxdb_client import InfluxDBClient, Point
from influxdb_client.client.write_api import SYNCHRONOUS
import os
import time

# --- Configurações (lidas das variáveis de ambiente) ---
INFLUXDB_URL = os.getenv('INFLUXDB_URL')
INFLUXDB_TOKEN = os.getenv('INFLUXDB_TOKEN')
INFLUXDB_ORG = os.getenv('INFLUXDB_ORG')
INFLUXDB_BUCKET = os.getenv('INFLUXDB_BUCKET')
MQTT_BROKER_HOST = os.getenv('MQTT_BROKER_HOST')
MQTT_BROKER_PORT = int(os.getenv('MQTT_BROKER_PORT'))

# Tópico MQTT para escutar
# Pattern: {device_id}/sensors/{sensor_id}/data
MQTT_TOPIC = "+/sensors/+/data"  # + is wildcard for any device_id and sensor_id 

# --- Conexão com InfluxDB ---
print("Conectando ao InfluxDB...")
influx_client = InfluxDBClient(url=INFLUXDB_URL, token=INFLUXDB_TOKEN, org=INFLUXDB_ORG)
write_api = influx_client.write_api(write_options=SYNCHRONOUS)
print("Conectado ao InfluxDB com sucesso!")

# --- Funções MQTT ---
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"Conectado ao Broker MQTT! ({MQTT_BROKER_HOST})")
        client.subscribe(MQTT_TOPIC)
        print(f"Inscrito no tópico: {MQTT_TOPIC}")
    else:
        print(f"Falha ao conectar, código de retorno: {rc}")

def on_message(client, userdata, msg):
    try:
        # Tenta decodificar a mensagem (payload)
        payload = msg.payload.decode('utf-8')
        print(f"📨 Mensagem recebida: Tópico[{msg.topic}] Payload[{payload[:100]}...]")

        # Parse JSON payload
        import json
        data = json.loads(payload)
        
        # Extract information from topic: {device_id}/sensors/{sensor_id}/data
        parts = msg.topic.split('/')
        if len(parts) >= 4 and parts[1] == 'sensors' and parts[3] == 'data':
            device_id = data.get('device_id') or parts[0]
            sensor_id = data.get('sensor_id') or parts[2]
            sensor_type = data.get('type', 'unknown')
            value = data.get('value')
            
            # Handle different value types
            if isinstance(value, dict):
                # Multi-value sensors (e.g., DHT22 with temperature and humidity)
                for field_name, field_value in value.items():
                    point = Point(sensor_id) \
                        .tag("device_id", device_id) \
                        .tag("sensor_type", sensor_type) \
                        .tag("field", field_name) \
                        .field("value", float(field_value)) \
                        .time(time.time_ns(), write_precision='ns')
                    
                    write_api.write(bucket=INFLUXDB_BUCKET, org=INFLUXDB_ORG, record=point)
                    print(f"✅ Salvo no InfluxDB: {sensor_id}.{field_name}={field_value} ({device_id})")
            else:
                # Single-value sensors
                point = Point(sensor_id) \
                    .tag("device_id", device_id) \
                    .tag("sensor_type", sensor_type) \
                    .field("value", float(value)) \
                    .time(time.time_ns(), write_precision='ns')
                
                write_api.write(bucket=INFLUXDB_BUCKET, org=INFLUXDB_ORG, record=point)
                print(f"✅ Salvo no InfluxDB: {sensor_id}={value} ({device_id})")

    except json.JSONDecodeError as e:
        print(f"❌ Erro ao decodificar JSON: {e}")
    except Exception as e:
        print(f"❌ Erro ao processar mensagem: {e}")

# --- Conexão com MQTT ---
mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message

print("Conectando ao Broker MQTT...")
mqtt_client.connect(MQTT_BROKER_HOST, MQTT_BROKER_PORT, 60)

# Loop principal para manter o script rodando
mqtt_client.loop_forever()