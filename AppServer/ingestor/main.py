import paho.mqtt.client as mqtt
from influxdb_client import InfluxDBClient, Point
from influxdb_client.client.write_api import SYNCHRONOUS
import os
import time
import json
import operator
import threading
import requests

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
MQTT_REGRA_TOPIC = "regra/+"
MQTT_CALLBACK_TOPIC ="callback/regras"

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
        client.subscribe([(MQTT_TOPIC,0),(MQTT_REGRA_TOPIC,0)])
    else:
        # reason_code 0 é sucesso. Outros valores indicam falha.
        print(f"Falha ao conectar, código de razão: {reason_code}")

def on_subscribe(client, userdata, mid, reason_codes, properties):
    """ Callback para quando o broker confirma a inscrição """
    print(f"Subscription ack: mid={mid} reason_codes={reason_codes}")
    # reason_codes é uma lista de códigos quando múltiplos tópicos foram inscritos.
    # opcional: checar se algum falhou
    if reason_codes:
        for rc in reason_codes:
            print(f" - reason_code: {rc}")

def on_disconnect(client, userdata, disconnect_flags, reason_code, properties):
    """ Callback para quando o cliente se desconecta """
    if reason_code == 0:
        print("Desconexão do Broker MQTT bem-sucedida.")
    else:
        print(f"Desconexão inesperada do Broker MQTT. Código: {reason_code}")
        print("O Paho-MQTT tentará reconectar automaticamente...")


regras= {}
def cria_regra(regra):
    try:
        id = regra['id']
        for c in regra['condicao']:
            c['state']=False
            if c['tipo']=='limite':
                c['last_state']=0
                c['time_stamp']=time.time()
            if c['tipo']=='senha':
                c['buffer']=''
        regras[id]=regra
        print(f"Regra {id} criada com sucesso.")
    except Exception as e:
        print(f"❌ Erro ao adicionar regra: {e}")

def deleta_regra(regra):
    try:
        id = regra['id']
        del regras[id]
        print(f"Regra {id} deletada com sucesso.")
    except Exception as e:
        print(f"❌ Erro ao deletar regra: {e}") 

def get_regra():
    try:
        mqtt_client.publish(MQTT_CALLBACK_TOPIC,json.dumps(regras))
        print(f"Regras publicadas com sucesso.")
    except Exception as e:
        print(f"❌ Erro ao retornar regras: {e}") 


operadores = {
    '<': operator.lt,  # lt = less than (menor que)
    '<=': operator.le, # le = less than or equal (menor ou igual)
    '>': operator.gt,  # gt = greater than (maior que)
    '>=': operator.ge, # ge = greater than or equal (maior ou igual)
    '==': operator.eq, # eq = equal (igual)
    '!=': operator.ne  # ne = not equal (diferente)
}

def executar(id_device, id_atuador, tempo, valor):
    """(Função SÍNCRONA, roda em um Thread separado)"""
    try:
        url = f"http://api_server:5000/{id_device}/settings/sensors/set"
        payload ={
            "sensors" : [
                {
                    "device_id" : id_device,
                    "sensor_id" : id_atuador,
                    "value" : valor
                }
            ]
        }
        requests.post(url, json=payload)

        time.sleep(tempo) 
        
        payload ={
            "sensors" : [
                {
                    "device_id" : id_device,
                    "sensor_id" : id_atuador,
                    "value" : 0 # Reseta para 0
                }
            ]
        }
        requests.post(url, json=payload)

    except Exception as e:
        print(f"❌ Erro na thread 'executar': {e}") 


def verificar_regras(id_device, id_sensor, value):
    """(Função SÍNCRONA, roda no thread do on_message)"""
    
    # Itera sobre uma cópia das chaves, para o caso de o dict 'regras'
    # ser modificado por outro callback (boa prática)
    for regra_id in list(regras.keys()):
        try:
            # Pega a regra atual (pode ter sido deletada, por isso o try)
            if regra_id not in regras:
                continue 
                
            regra = regras[regra_id]
            resposta = True
            
            for c in regra["condicao"]:
                if c['id_device']==id_device and c['id_sensor']==id_sensor:
                    
                    if c['tipo'] == 'limite':
                        if isinstance(value, dict):
                            valor = value[c['medida']]
                        else:
                            valor = value
                            
                        state = operadores[c['operador']](valor,c['valor_limite'])
                        
                        if state != c['state']:
                            c['last_state'] = state
                            c['time_stamp'] = time.time()
                        
                        if c['tempo'] == 0:
                            resposta = resposta and state
                        else:
                            # BUG 1 CORRIGIDO: Lógica de tempo
                            # (tempo_atual - tempo_inicio) >= duracao_necessaria
                            duracao_estado_atual = time.time() - c['time_stamp']
                            
                            if state and duracao_estado_atual >= c['tempo']:
                                # A condição é Verdadeira E o tempo foi atingido
                                resposta = resposta and True
                            elif not state:
                                # Se o estado for Falso, a resposta é Falsa imediatamente
                                resposta = resposta and False
                            else:
                                # Estado é Verdadeiro, mas tempo ainda não foi atingido
                                resposta = False
                                
                    elif c['tipo'] == 'senha':
                        pass # ... sua lógica de senha ...
                
                # Se qualquer condição NÃO específica para este sensor/device for Falsa,
                # a resposta geral também deve ser Falsa.
                # (Esta parte depende de como você quer que o 'E' lógico funcione)
                # ...
                        
            # --- Fim do loop de condições ---
            
            if resposta:
                for e in regra["entao"]:
                    if e["tempo"]!=0:
                        # DISPARA EM BACKGROUND USANDO THREAD!
                        threading.Thread(target=executar, args=(e["id_device"],e["id_sensor"],e["tempo"],e["valor"])).start()
                    else:
                        url = f"http://api_server:5000/{id_device}/settings/sensors/set"
                        payload = { "sensors" : [{"device_id" : e["id_device"], "sensor_id" : e["id_sensor"], "value" : e["valor"] }]}
                        # BUG 2 CORRIGIDO:
                        requests.post(url, json=payload)
            else:
                for e in regra["senao"]:
                    if e["tempo"]!=0:
                        # DISPARA EM BACKGROUND USANDO THREAD!
                        threading.Thread(target=executar, args=(e["id_device"],e["id_sensor"],e["tempo"],e["valor"])).start()
                    else:
                        url = f"http://api_server:5000/{id_device}/settings/sensors/set"
                        payload = { "sensors" : [{"device_id" : e["id_device"], "sensor_id" : e["id_sensor"], "value" : e["valor"] }]}
                        # BUG 2 CORRIGIDO:
                        requests.post(url, json=payload)
                        
        except Exception as e:
            print(f"❌ Erro ao verificar regra {regra_id}: {e}") 

def on_message(client, userdata, msg):
    """ Callback para quando uma mensagem é recebida """
    try:
        payload = msg.payload.decode('utf-8')
        print(f"📨 Mensagem recebida: Tópico[{msg.topic}] Payload[{payload[:100]}...]")

        data = json.loads(payload)
        parts = msg.topic.split('/')

        if len(parts) >= 2 and parts[0] == 'regra':
            if parts[1] == 'add':
                cria_regra(data)
            elif parts[1] == 'delete':
                deleta_regra(data)
            elif parts[1] == 'get':
                get_regra()

        if len(parts) >= 4 and parts[1] == 'sensors' and parts[3] == 'data':
            device_id = data.get('device_id') or parts[0]
            sensor_id = data.get('sensor_id') or parts[2]
            sensor_type = data.get('type', 'unknown')
            value = data.get('value')
            
            # ✅ CORREÇÃO:
            # Simplesmente chame a função síncrona.
            # Ela é rápida. As partes lentas ('executar')
            # serão colocadas em threads por ela.
            verificar_regras(device_id, sensor_id, value)
            
            # O resto do seu código de salvamento no InfluxDB...
            if isinstance(value, dict):
                # Multi-value sensors...
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
        import traceback
        traceback.print_exc() # Imprime o stack trace completo para debug

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