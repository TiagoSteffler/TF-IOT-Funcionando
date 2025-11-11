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
MQTT_TOPIC = "sensor/#" 

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
            
            # Tenta converter o payload para float
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
        
        else:
            print(f"Tópico '{msg.topic}' não corresponde ao formato esperado 'sensor/device/sensor/measurement'.")

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
    mqtt_client.loop_forever()
except KeyboardInterrupt:
    print("Script interrompido pelo usuário. Desconectando...")
    mqtt_client.disconnect()
    influx_client.close()
    print("Desconectado.")