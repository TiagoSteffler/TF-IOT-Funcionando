import asyncio
import aiomqtt
from influxdb_client.client.influxdb_client_async import InfluxDBClientAsync
from influxdb_client import Point
import os
import json
import operator
import time
import traceback

# --- Configurações (lidas das variáveis de ambiente) ---
INFLUXDB_URL = os.getenv('INFLUXDB_URL')
INFLUXDB_TOKEN = os.getenv('INFLUXDB_TOKEN')
INFLUXDB_ORG = os.getenv('INFLUXDB_ORG')
INFLUXDB_BUCKET = os.getenv('INFLUXDB_BUCKET')
MQTT_BROKER_HOST = os.getenv('MQTT_BROKER_HOST')
MQTT_BROKER_PORT = int(os.getenv('MQTT_BROKER_PORT'))

# --- Tópicos MQTT ---
MQTT_SENSOR_DATA_TOPIC = "+/sensors/+/data"
MQTT_RULES_TOPIC = "rules/+"
MQTT_RULES_CALLBACK_TOPIC = "callback/rules"

# Lista de tipos de sensores que DEVEM ser salvos como String
STRING_SENSOR_TYPES = ["TECLADO_4X4"]

# --- Armazenamento de Regras (em memória) ---
regras = {}
RULES_CONFIG_FILE = 'rules_config.json' # <-- ADICIONE AQUI

def salvar_regras_no_arquivo():
    """Salva o dicionário 'regras' atual no arquivo JSON."""
    global regras
    try:
        with open(RULES_CONFIG_FILE, 'w') as f:
            json.dump(regras, f, indent=4)
        print(f"✅ Regras salvas com sucesso em {RULES_CONFIG_FILE}")
    except Exception as e:
        print(f"❌ Erro ao salvar regras no arquivo: {e}")

def carregar_regras_do_arquivo():
    """Carrega as regras do arquivo JSON para o dicionário 'regras'."""
    global regras
    if os.path.exists(RULES_CONFIG_FILE):
        try:
            with open(RULES_CONFIG_FILE, 'r') as f:
                # Evita erro se o arquivo estiver vazio
                content = f.read()
                if not content:
                    print(f"ℹ️ Arquivo {RULES_CONFIG_FILE} está vazio. Começando com regras vazias.")
                    regras = {}
                else:
                    regras = json.loads(content)
                    print(f"✅ Regras carregadas com sucesso de {RULES_CONFIG_FILE}. Total: {len(regras)}")
        except Exception as e:
            print(f"⚠️ Erro ao carregar {RULES_CONFIG_FILE}: {e}. Começando com regras vazias.")
            regras = {}
    else:
        print(f"ℹ️ Arquivo {RULES_CONFIG_FILE} não encontrado. Criando arquivo vazio...")
        regras = {}
        # Cria o arquivo vazio para garantir que ele exista
        try:
            with open(RULES_CONFIG_FILE, 'w') as f:
                json.dump({}, f) # Escreve um JSON vazio
            print(f"✅ Arquivo {RULES_CONFIG_FILE} criado com sucesso.")
        except Exception as e:
            print(f"❌ Erro ao criar {RULES_CONFIG_FILE}: {e}")

# --- Operadores para Regras ---
operadores = {
    '<': operator.lt,
    '<=': operator.le,
    '>': operator.gt,
    '>=': operator.ge,
    '==': operator.eq,
    '!=': operator.ne
}

# --- Funções de Gerenciamento de Regras (Síncronas) ---
# (Estas funções manipulam o dict 'regras' e são chamadas pelo loop principal)

def cria_regra(regra):
    try:
        id = regra['id_regra']
        for c in regra['condicao']:
            if c['tipo'] == 'limite':
                c['last_state'] = False
                c['time_stamp'] = time.time()
            if c['tipo'] == 'senha':
                c['buffer'] = ''
        regras[id] = regra
        print(f"✅ Regra {id} criada com sucesso.")
        salvar_regras_no_arquivo()
    except Exception as e:
        print(f"❌ Erro ao adicionar regra: {e}")

def atualiza_regra(regra):
    try:
        id = regra.get('id_regra')
        if not id:
            print(f"❌ Erro ao atualizar regra: 'id_regra' não fornecido")
            return
        
        if id in regras:
            regras[id].update(regra)
            # Reinicializa o estado das condições
            for c in regras[id]['condicao']:
                if c['tipo'] == 'limite':
                    c['last_state'] = False
                    c['time_stamp'] = time.time()
                if c['tipo'] == 'senha':
                    c['buffer'] = ''
            print(f"✅ Regra {id} atualizada com sucesso.")
            salvar_regras_no_arquivo()
        else:
            print(f"⚠️ Regra {id} não encontrada. Criando como nova...")
            cria_regra(regra)
    except Exception as e:
        print(f"❌ Erro ao atualizar regra: {e}")

def deleta_regra(regra):
    try:
        id = regra.get('id_regra')
        if not id:
            print(f"❌ Erro ao deletar regra: 'id_regra' não fornecido")
            return
        
        if id in regras:
            del regras[id]
            print(f"✅ Regra {id} deletada com sucesso.")
            salvar_regras_no_arquivo()
        else:
            print(f"⚠️ Regra {id} não encontrada para deletar.")
    except Exception as e:
        print(f"❌ Erro ao deletar regra: {e}") 

async def async_get_regra(client):
    try:
        await client.publish(MQTT_RULES_CALLBACK_TOPIC, json.dumps(regras))
        print(f"✅ Regras publicadas com sucesso no {MQTT_RULES_CALLBACK_TOPIC}.")
    except Exception as e:
        print(f"❌ Erro ao retornar regras: {e}")

# --- Funções de Execução de Regras (Assíncronas) ---

async def async_executar_comando(client, id_device, id_atuador, valor):
    """Publica um único comando para um atuador."""
    try:
        # IMPORTANTE: Novo padrão de tópico para comandos de atuador
        topic = f"config/{id_device}/actuators/{id_atuador}/set"
        payload = json.dumps({"value": valor})
        await client.publish(topic, payload)
        print(f"✅ Regra (Comando): Publicado em {topic} -> {payload}")
    except Exception as e:
        print(f"❌ Erro em 'async_executar_comando': {e}")

async def async_executar_temporizado(client, id_device, id_atuador, tempo, valor):
    """(Função ASSÍNCRONA) Executa um comando e o reverte após 'tempo'."""
    try:
        # Tópico para o atuador receber o comando
        topic = f"config/{id_device}/actuators/{id_atuador}/set"
        
        # Publica o valor inicial
        payload_on = json.dumps({"value": valor})
        await client.publish(topic, payload_on)
        print(f"✅ Regra (ON): Publicado em {topic} -> {payload_on}")

        # Aguarda o tempo definido
        await asyncio.sleep(tempo) 
        
        # Publica o valor de reset (0)
        payload_off = json.dumps({"value": 0})
        await client.publish(topic, payload_off)
        print(f"✅ Regra (OFF): Publicado em {topic} -> {payload_off}")

    except Exception as e:
        print(f"❌ Erro na task 'async_executar_temporizado': {e}") 

async def async_verificar_regras(client, id_device, id_sensor, value):
    """Verifica todas as regras com base em um novo dado de sensor."""
    
    for regra_id in list(regras.keys()):
        try:
            if regra_id not in regras:
                continue 
                
            regra = regras[regra_id]
            # Inicia assumindo que a condição é Falsa até que seja provada Verdadeira
            resposta_final_condicao = True
            
            # 1. Avalia as Condições
            condicao_atendida = False
            for c in regra["condicao"]:
                # Verifica se a condição é para este sensor/device
                if c.get('id_device') == id_device and c.get('id_sensor') == id_sensor:
                    condicao_atendida = True # Marcamos que este sensor é relevante para esta regra
                    if c['tipo'] == 'limite':
                        # Extrai o valor correto se for um dict (ex: DHT22)
                        try:
                            valor_sensor = float(value[c['medida']]) if isinstance(value, dict) else float(value)
                        except (KeyError, ValueError, TypeError):
                            print(f"  [Regra {regra_id}] Medida '{c.get('medida')}' não encontrada ou valor inválido em {value}")
                            resposta_final_condicao = False
                            break # Se uma condição falha, a resposta_final é Falsa
                        
                        # Compara o valor
                        state = operadores[c['operador']](valor_sensor, c['valor_limite'])
                        
                        if state != c.get('last_state', not state):
                            c['last_state'] = state
                            c['time_stamp'] = time.time()
                        
                        if c['tempo'] == 0:
                            # Regra sem tempo, só checa o estado
                            if not state:
                                resposta_final_condicao = False
                                break
                        else:
                            # Regra com tempo
                            duracao_estado_atual = time.time() - c['time_stamp']
                            if not (state and duracao_estado_atual >= c['tempo']):
                                # Se estado for Falso, ou se for Verdadeiro mas tempo não atingido
                                resposta_final_condicao = False
                                break
                                
                    elif c['tipo'] == 'senha':
                        if value == '*':
                            c['buffer']=''
                        else:
                            c['buffer']=f"{c['buffer']}{value}"
                        if len(c['buffer']) == len(c['senha']):
                            if c['buffer'] == c['senha']:
                                resposta_final_condicao = True
                            else:
                                resposta_final_condicao = False
                        else:
                            condicao_atendida = False

            # Se o sensor não era relevante para nenhuma condição da regra, não faz nada
            if not condicao_atendida:
                continue

            # 2. Executa Ações (ENTAO / SENAO)
            if resposta_final_condicao:
                # Executa o bloco "ENTAO"
                for e in regra.get("entao", []):
                    if e["tempo"] != 0:
                        # Dispara em background como uma nova Task
                        asyncio.create_task(async_executar_temporizado(
                            client, e["id_device"], e["id_atuador"], e["tempo"], e["valor"]
                        ))
                    else:
                        # Executa comando simples
                        await async_executar_comando(
                            client, e["id_device"], e["id_atuador"], e["valor"]
                        )
            else:
                # Executa o bloco "SENAO"
                for e in regra.get("senao", []):
                    if e["tempo"] != 0:
                        asyncio.create_task(async_executar_temporizado(
                            client, e["id_device"], e["id_atuador"], e["tempo"], e["valor"]
                        ))
                    else:
                        await async_executar_comando(
                            client, e["id_device"], e["id_atuador"], e["valor"]
                        )
                        
        except Exception as e:
            print(f"❌ Erro ao verificar regra {regra_id}: {e}")
            traceback.print_exc()

# --- Função Principal (Main) ---

async def main():
    print("Iniciando Ingestor Assíncrono...")
    
    # Conecta ao InfluxDB (Async)
    # Conecta ao InfluxDB (Async)
    try:
        influx_client = InfluxDBClientAsync(url=INFLUXDB_URL, token=INFLUXDB_TOKEN, org=INFLUXDB_ORG)
        write_api = influx_client.write_api()
        
        # Tenta pingar o banco de dados para verificar a conexão
        if await influx_client.ping(): # <--- LINHA CORRIGIDA
            print("✅ Conectado ao InfluxDB com sucesso!")
        else:
            raise Exception("Erro ao pingar o InfluxDB. Verifique a URL ou token.")
            
    except Exception as e:
        print(f"❌ Erro fatal ao conectar ao InfluxDB: {e}")
        return

    # Conecta ao MQTT (Async)
    try:
        print(f"Conectando ao Broker MQTT em {MQTT_BROKER_HOST}...")
        async with aiomqtt.Client(hostname=MQTT_BROKER_HOST, port=MQTT_BROKER_PORT) as client:
            print("✅ Conectado ao Broker MQTT!")
            
            # Inscreve-se nos tópicos
            await client.subscribe(MQTT_SENSOR_DATA_TOPIC)
            await client.subscribe(MQTT_RULES_TOPIC)
            print(f"  Inscrito em: {MQTT_SENSOR_DATA_TOPIC}")
            print(f"  Inscrito em: {MQTT_RULES_TOPIC}")

            # Loop principal de mensagens
            async for message in client.messages:
                try:
                    payload_str = message.payload.decode('utf-8')
                    topic = message.topic.value
                    print(f"📨 Mensagem recebida: Tópico[{topic}] Payload[{payload_str[:100]}...]")
                    
                    data = json.loads(payload_str)
                    parts = topic.split('/')

                    # --- Roteador de Tópicos ---

                    # 1. Tópicos de Regras (rules/+)
                    if parts[0] == 'rules':
                        if parts[1] == 'add':
                            cria_regra(data)
                        elif parts[1] == 'update':
                            atualiza_regra(data)
                        elif parts[1] == 'delete':
                            deleta_regra(data)
                        elif parts[1] == 'get':
                            await async_get_regra(client)
                    
                    # 2. Tópicos de Dados de Sensores (+/sensors/+/data)
                    elif len(parts) >= 4 and parts[1] == 'sensors' and parts[3] == 'data':
                        device_id = data.get('device_id') or parts[0]
                        sensor_id = data.get('sensor_id') or parts[2]
                        sensor_type = data.get('type', 'unknown')
                        value = data.get('value')
                        
                        # 2a. Verifica regras (não bloqueante)
                        await async_verificar_regras(client, device_id, sensor_id, value)
                        
                        # 2b. Salva no InfluxDB (não bloqueante)
                        if isinstance(value, dict):
                            # Múltiplos valores (ex: DHT22)
                            points = []
                            for field_name, field_value in value.items():
                                try:
                                    point = Point(sensor_id) \
                                        .tag("device_id", device_id) \
                                        .tag("sensor_type", sensor_type) \
                                        .tag("field", field_name) \
                                        .field("value", float(field_value)) \
                                        .time(time.time_ns(), write_precision='ns')
                                    points.append(point)
                                except (ValueError, TypeError) as e:
                                    print(f"  [Influx] Ignorando valor inválido: {field_name}={field_value} ({e})")
                            
                            if points:
                                await write_api.write(bucket=INFLUXDB_BUCKET, org=INFLUXDB_ORG, record=points)
                                print(f"  ✅ Salvo no InfluxDB: {sensor_id} (múltiplos valores) ({device_id})")
                                
                        else:
                        # Valor único
                            point = Point(sensor_id) \
                                .tag("device_id", device_id) \
                                .tag("sensor_type", sensor_type)

                            # --- [LÓGICA CORRIGIDA] ---
                            # Verifica se o TIPO de sensor está na lista de strings
                            if sensor_type in STRING_SENSOR_TYPES:
                                # Se for um teclado, SEMPRE salva como string
                                point.field("value", str(value))
                                save_type = "String"
                            else:
                                # Para todos os outros sensores, tenta salvar como float
                                save_type = "Float"
                                try:
                                    point.field("value", float(value))
                                except (ValueError, TypeError):
                                    # Se falhar (ex: "nan"), salva como string
                                    point.field("value", str(value))
                                    save_type = "String (fallback)"

                            point.time(time.time_ns(), write_precision='ns')

                            try:
                                await write_api.write(bucket=INFLUXDB_BUCKET, org=INFLUXDB_ORG, record=point)
                                print(f"  ✅ Salvo no InfluxDB: {sensor_id}={value} (Tipo: {save_type}) ({device_id})")
                            except Exception as e:
                                print(f"  [Influx] Erro ao salvar ponto: {e}")
                                
                except json.JSONDecodeError as e:
                    print(f"❌ Erro ao decodificar JSON: {e}")
                except Exception as e:
                    print(f"❌ Erro ao processar mensagem: {e}")
                    traceback.print_exc()

    except aiomqtt.MqttError as e:
        print(f"❌ Erro de conexão MQTT: {e}. O ingestor será encerrado.")
    except (asyncio.CancelledError, KeyboardInterrupt):
        print("\n🛑 Ingestor interrompido. Desconectando...")
    finally:
        if 'influx_client' in locals() and influx_client:
            await influx_client.close()
            print("✅ Conexão com InfluxDB fechada.")
        print("✅ Ingestor encerrado.")

if __name__ == "__main__":
    carregar_regras_do_arquivo()
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("🛑 Processo principal interrompido.")