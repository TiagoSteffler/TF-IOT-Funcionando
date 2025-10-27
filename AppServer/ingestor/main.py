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

# Tópico MQTT para escutar (ex: todos os tópicos dentro de 'grupo1')
MQTT_TOPIC = "sensor/#" 

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
        print(f"Mensagem recebida: Tópico[{msg.topic}] Payload[{payload}]")

        # Extrai o "sensor" e o "dispositivo" do tópico
        # Ex: "sensor/esp32-ttgo-1/dht11/temperatura"
        parts = msg.topic.split('/')
        if len(parts) >= 4:
            device_id = parts[1]
            sensor_name = parts[2]
            measurement_name = parts[3]
            
            # Tenta converter o payload para float (aqui você pode fazer filtros/agregações) 
            try:
                value = float(payload)
            except ValueError:
                print(f"Payload não é um número: {payload}")
                return

            # Cria o Ponto de dado para o InfluxDB
            point = Point(measurement_name) \
                .tag("device_id", device_id) \
                .tag("sensor", sensor_name) \
                .field("value", value) \
                .time(time.time_ns(), write_precision='ns')
            
            # Escreve o ponto no InfluxDB
            write_api.write(bucket=INFLUXDB_BUCKET, org=INFLUXDB_ORG, record=point)
            print(f"Dado salvo no InfluxDB: {measurement_name}={value} para {device_id}")

    except Exception as e:
        print(f"Erro ao processar mensagem: {e}")

# --- Conexão com MQTT ---
mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message

print("Conectando ao Broker MQTT...")
mqtt_client.connect(MQTT_BROKER_HOST, MQTT_BROKER_PORT, 60)

# Loop principal para manter o script rodando
mqtt_client.loop_forever()