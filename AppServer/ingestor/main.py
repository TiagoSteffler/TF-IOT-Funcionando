import asyncio
import aiomqtt
import aiohttp
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
API_SERVER_URL = os.getenv('API_SERVER_URL', 'http://api_server:5000')

# --- Tópicos MQTT ---
MQTT_SENSOR_DATA_TOPIC = "+/sensors/+/data"
MQTT_RULES_TOPIC = "rules/+"
MQTT_RULES_CALLBACK_TOPIC = "callback/rules"

# Sensor type enum mapping (from ESP32 Trabalho.hpp)
SENSOR_TYPES = {
    0: "MPU_6050",
    1: "DS18_B20",
    2: "HC_SR04",
    3: "APDS_9960",
    4: "SG_90",
    5: "RELE",
    6: "JOYSTICK",
    7: "TECLADO_4X4",
    8: "ENCODER",
    9: "DHT_11"
}

# Lista de tipos de sensores que DEVEM ser salvos como String (usando o ID numérico)
STRING_SENSOR_TYPES = [7]  # TECLADO_4X4

# --- Armazenamento de Regras (em memória) ---
regras = {}
RULES_CONFIG_FILE = 'rules_config.json' # <-- ADICIONE AQUI

# --- Armazenamento de Configurações de Sensores (em memória) ---
# Estrutura: {device_id: {sensor_id: {id, desc, tipo, pinos, atributo1, ...}}}
sensor_configs = {}

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
            elif c['tipo'] == 'senha':
                # Password conditions don't need time tracking, but initialize if needed
                c['last_state'] = False
                c['time_stamp'] = time.time()
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
                elif c['tipo'] == 'senha':
                    c['last_state'] = False
                    c['time_stamp'] = time.time()
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
        # Convert regras dict to array format expected by API
        rules_array = list(regras.values())
        response_payload = {"rules": rules_array}
        
        print(f"📤 GET RULES: Enviando {len(rules_array)} regras para {MQTT_RULES_CALLBACK_TOPIC}")
        print(f"   Regras: {json.dumps(response_payload, indent=2)}")
        
        await client.publish(MQTT_RULES_CALLBACK_TOPIC, json.dumps(response_payload))
        print(f"✅ Regras publicadas com sucesso no {MQTT_RULES_CALLBACK_TOPIC}.")
    except Exception as e:
        print(f"❌ Erro ao retornar regras: {e}")
        traceback.print_exc()

# --- Funções de Execução de Regras (Assíncronas) ---

async def async_executar_comando(client, id_device, id_atuador, valor, modo='set'):
    """Envia comando para atuador via API HTTP.
    
    Args:
        modo: 'set' (default) = set to specified value, 'toggle' = flip current state
    """
    try:
        # Get cached sensor configuration if available
        sensor_config = None
        if id_device in sensor_configs and id_atuador in sensor_configs[id_device]:
            # Use full cached config - EXACTLY like SensorList.vue
            cached = sensor_configs[id_device][id_atuador]
            
            # Handle toggle mode
            if modo == 'toggle':
                current_value = cached.get("atributo1", 0)
                # Toggle: 0 -> 1, any non-zero -> 0
                valor = 0 if current_value else 1
                print(f"🔄 Toggle mode: {current_value} -> {valor}")
            
            sensor_config = {
                "id": cached.get("id", id_atuador),
                "desc": cached.get("desc", ""),
                "tipo": cached.get("tipo"),
                "atributo1": valor,
                "pinos": cached.get("pinos", [])
            }
        else:
            # Minimal config if not cached
            print(f"  ⚠️ Configuração do sensor {id_atuador} não encontrada no cache. Usando config mínima.")
            if modo == 'toggle':
                print(f"  ⚠️ Toggle mode requires cached state - defaulting to valor=1")
                valor = 1
            sensor_config = {
                "id": id_atuador,
                "atributo1": valor
            }
        
        # Send HTTP POST to API server - EXACTLY like SensorList.vue
        url = f"{API_SERVER_URL}/{id_device}/settings/sensors/set"
        payload = {"sensors": [sensor_config]}
        
        print(f"📤 Regra (Comando): Enviando HTTP POST para {url}")
        print(f"   Payload: {json.dumps(payload)}")
        
        async with aiohttp.ClientSession() as session:
            async with session.post(url, json=payload, headers={'Content-Type': 'application/json'}) as response:
                if response.status != 200:
                    error_text = await response.text()
                    print(f"❌ API Error: {response.status} - {error_text}")
                else:
                    print(f"✅ Regra (Comando): Atuador {id_atuador} atualizado para {valor}")
    except Exception as e:
        print(f"❌ Erro em 'async_executar_comando': {e}")
        traceback.print_exc()

async def async_executar_temporizado(client, id_device, id_atuador, tempo, valor):
    """(Função ASSÍNCRONA) Executa um comando via HTTP e o reverte após 'tempo'."""
    try:
        # Get cached sensor configuration if available
        base_config = {}
        if id_device in sensor_configs and id_atuador in sensor_configs[id_device]:
            cached = sensor_configs[id_device][id_atuador]
            base_config = {
                "id": cached.get("id", id_atuador),
                "desc": cached.get("desc", ""),
                "tipo": cached.get("tipo"),
                "pinos": cached.get("pinos", [])
            }
        else:
            base_config = {"id": id_atuador}
        
        # Send HTTP POST to turn ON
        url = f"{API_SERVER_URL}/{id_device}/settings/sensors/set"
        sensor_config_on = {**base_config, "atributo1": valor}
        payload_on = {"sensors": [sensor_config_on]}
        
        print(f"📤 Regra (ON): Enviando HTTP POST para {url}")
        print(f"   Payload: {json.dumps(payload_on)}")
        
        async with aiohttp.ClientSession() as session:
            async with session.post(url, json=payload_on, headers={'Content-Type': 'application/json'}) as response:
                if response.status == 200:
                    print(f"✅ Regra (ON): Atuador {id_atuador} ativado com valor {valor}")
                else:
                    error_text = await response.text()
                    print(f"❌ API Error (ON): {response.status} - {error_text}")

        # Aguarda o tempo definido
        await asyncio.sleep(tempo) 
        
        # Send HTTP POST to turn OFF
        sensor_config_off = {**base_config, "atributo1": 0}
        payload_off = {"sensors": [sensor_config_off]}
        
        print(f"📤 Regra (OFF): Enviando HTTP POST para {url}")
        print(f"   Payload: {json.dumps(payload_off)}")
        
        async with aiohttp.ClientSession() as session:
            async with session.post(url, json=payload_off, headers={'Content-Type': 'application/json'}) as response:
                if response.status == 200:
                    print(f"✅ Regra (OFF): Atuador {id_atuador} desativado")
                else:
                    error_text = await response.text()
                    print(f"❌ API Error (OFF): {response.status} - {error_text}")

    except Exception as e:
        print(f"❌ Erro na task 'async_executar_temporizado': {e}")
        traceback.print_exc() 

async def async_verificar_regras(client, id_device, id_sensor, value):
    """Verifica todas as regras com base em um novo dado de sensor.
    
    Executa ações apenas em TRANSIÇÕES de estado (false→true ou true→false)
    para evitar execuções repetidas enquanto a condição permanece verdadeira.
    """
    
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
                    
                    # Handle different condition types
                    if c.get('tipo') == 'senha':
                        # Password condition - compare entire input string
                        try:
                            if isinstance(value, dict):
                                # For keypad, the string is in value['input']
                                valor_sensor = str(value.get('input', ''))
                            else:
                                valor_sensor = str(value)
                            
                            senha_esperada = c.get('senha', '')
                            state = (valor_sensor == senha_esperada)
                            
                            print(f"  [Regra {regra_id}] Password check: '{valor_sensor}' == '{senha_esperada}' → {state}")
                            
                        except (KeyError, ValueError, TypeError) as e:
                            print(f"  [Regra {regra_id}] Erro ao verificar senha em {value} ({type(value).__name__}): {e}")
                            resposta_final_condicao = False
                            break
                    
                    else:  # tipo == 'limite' or default
                        # Limit condition - compare specific field value
                        try:
                            medida = c['medida']
                            if isinstance(value, dict):
                                # Dict access for named fields (e.g., {"x": 1951, "y": 1981, "bt": 0})
                                valor_sensor = value[medida]
                            else:
                                # Single value (for actuators)
                                valor_sensor = value
                            
                            # Determine if we need to compare as strings or numbers
                            valor_limite = c['valor_limite']
                            
                            # If valor_limite is a string, compare as strings
                            if isinstance(valor_limite, str):
                                valor_sensor = str(valor_sensor)
                            else:
                                # Otherwise, compare as numbers
                                valor_sensor = float(valor_sensor)
                                valor_limite = float(valor_limite)
                                
                        except (KeyError, ValueError, TypeError) as e:
                            print(f"  [Regra {regra_id}] Medida '{c.get('medida')}' não encontrada ou valor inválido em {value} ({type(value).__name__}): {e}")
                            resposta_final_condicao = False
                            break # Se uma condição falha, a resposta_final é Falsa
                        
                        # Compara o valor (works for both strings and numbers)
                        state = operadores[c['operador']](valor_sensor, valor_limite)
                    
                    # Track state changes for transitions
                    if state != c.get('last_state', not state):
                        c['last_state'] = state
                        c['time_stamp'] = time.time()
                    else:
                        if state==False:
                            condicao_atendida = False
                            break
                    
                    # Time tracking - only for limit conditions with tempo field
                    if c.get('tipo') == 'senha':
                        # Password conditions are instant - no time delay
                        if not state:
                            resposta_final_condicao = False
                            break
                    else:
                        # Limit conditions may have time requirements
                        tempo = c.get('tempo', 0)
                        if tempo == 0:
                            # Regra sem tempo, só checa o estado
                            if not state:
                                resposta_final_condicao = False
                                break
                        else:
                            # Regra com tempo
                            duracao_estado_atual = time.time() - c['time_stamp']
                            if not (state and duracao_estado_atual >= tempo):
                                # Se estado for Falso, ou se for Verdadeiro mas tempo não atingido
                                resposta_final_condicao = False
                                break

            # Se o sensor não era relevante para nenhuma condição da regra, não faz nada
            if not condicao_atendida:
                continue

            # Verifica se houve mudança de estado da regra (transição)
            last_rule_state = regra.get('_last_triggered_state', None)
            state_changed = (last_rule_state != resposta_final_condicao)
            
            # Só executa ações se houve mudança de estado (debouncing)
            if not state_changed:
                # Estado não mudou - não executa ações novamente
                continue
            
            # Atualiza o estado anterior da regra
            regra['_last_triggered_state'] = resposta_final_condicao
            
            # 2. Executa Ações (ENTAO / SENAO) - APENAS EM TRANSIÇÕES
            if resposta_final_condicao:
                print(f"  🔔 [Regra {regra_id}] Transição FALSE → TRUE: Executando bloco THEN")
                # Executa o bloco "ENTAO"
                for e in regra.get("entao", []):
                    modo = e.get("modo", "set")  # Default to 'set' for backward compatibility
                    if e["tempo"] != 0:
                        # Dispara em background como uma nova Task
                        asyncio.create_task(async_executar_temporizado(
                            client, e["id_device"], e["id_atuador"], e["tempo"], e["valor"]
                        ))
                    else:
                        # Executa comando simples
                        await async_executar_comando(
                            client, e["id_device"], e["id_atuador"], e["valor"], modo
                        )
            else:
                print(f"  🔔 [Regra {regra_id}] Transição TRUE → FALSE: Executando bloco ELSE")
                # Executa o bloco "SENAO"
                for e in regra.get("senao", []):
                    modo = e.get("modo", "set")  # Default to 'set' for backward compatibility
                    if e["tempo"] != 0:
                        asyncio.create_task(async_executar_temporizado(
                            client, e["id_device"], e["id_atuador"], e["tempo"], e["valor"]
                        ))
                    else:
                        await async_executar_comando(
                            client, e["id_device"], e["id_atuador"], e["valor"], modo
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
            await client.subscribe("+/settings/sensors/get/response")
            print(f"  Inscrito em: {MQTT_SENSOR_DATA_TOPIC}")
            print(f"  Inscrito em: {MQTT_RULES_TOPIC}")
            print(f"  Inscrito em: +/settings/sensors/get/response")

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
                        print(f"🔀 ROTEADOR DE REGRAS: Ação = {parts[1]}")
                        if parts[1] == 'add':
                            print(f"  ➕ ADD RULE: {data.get('id_regra', 'SEM_ID')}")
                            cria_regra(data)
                        elif parts[1] == 'update':
                            print(f"  ✏️ UPDATE RULE: {data.get('id_regra', 'SEM_ID')}")
                            atualiza_regra(data)
                        elif parts[1] == 'delete':
                            print(f"  🗑️ DELETE RULE: {data.get('id_regra', 'SEM_ID')}")
                            deleta_regra(data)
                        elif parts[1] == 'get':
                            print(f"  📋 GET RULES: Retornando todas as regras")
                            await async_get_regra(client)
                    
                    # 2. Tópicos de Configuração de Sensores (+/settings/sensors/get/response)
                    elif len(parts) >= 5 and parts[1] == 'settings' and parts[2] == 'sensors' and parts[3] == 'get' and parts[4] == 'response':
                        device_id = parts[0]
                        sensors_list = data.get('sensors', [])
                        
                        print(f"📥 Configuração de sensores recebida para {device_id}: {len(sensors_list)} sensores")
                        
                        # Cache all sensor configurations
                        if device_id not in sensor_configs:
                            sensor_configs[device_id] = {}
                        
                        for sensor in sensors_list:
                            sensor_id = sensor.get('id')
                            if sensor_id is not None:
                                sensor_configs[device_id][sensor_id] = {
                                    "id": sensor_id,
                                    "desc": sensor.get('desc', ''),
                                    "tipo": sensor.get('tipo', -1),
                                    "pinos": sensor.get('pinos', []),
                                    "atributo1": sensor.get('atributo1', 0)
                                }
                                print(f"  ✅ Cached config for sensor {sensor_id}: {sensor.get('desc', 'N/A')}")
                    
                    # 3. Tópicos de Dados de Sensores (+/sensors/+/data)
                    elif len(parts) >= 4 and parts[1] == 'sensors' and parts[3] == 'data':
                        device_id = data.get('device_id') or parts[0]
                        sensor_id = data.get('sensor_id') or data.get('id') or parts[2]
                        sensor_type_id = data.get('type') if data.get('type') is not None else data.get('tipo', -1)
                        sensor_type_name = SENSOR_TYPES.get(sensor_type_id, 'unknown')
                        
                        # Actuators (RELE, SG_90) handle both formats:
                        # - Old format: atributo1 (backwards compatibility)
                        # - New format: values.state (RELE) or values.angle (SG_90)
                        # Types 4 (SG_90) and 5 (RELE)
                        if sensor_type_id in [4, 5]:  # SG_90 or RELE
                            # Try old format first (backwards compatibility)
                            value = data.get('atributo1')
                            
                            # Fall back to new format if old not present
                            if value is None:
                                values_dict = data.get('values')
                                if isinstance(values_dict, dict):
                                    # Type 5 (RELE) uses 'state', Type 4 (SG_90) uses 'angle'
                                    if sensor_type_id == 5:
                                        value = values_dict.get('state')
                                    elif sensor_type_id == 4:
                                        value = values_dict.get('angle')
                                
                                if value is None:
                                    field_name = 'state' if sensor_type_id == 5 else 'angle'
                                    print(f"  ⚠️ Actuator message missing both 'atributo1' and 'values.{field_name}': {data}")
                                    continue
                            
                            field_name = 'state' if sensor_type_id == 5 else 'angle'
                            print(f"  🎛️ Actuator {sensor_type_name}: {field_name}={value}")
                            
                            # Cache sensor configuration for later use in rules
                            if device_id not in sensor_configs:
                                sensor_configs[device_id] = {}
                            sensor_configs[device_id][sensor_id] = {
                                "id": sensor_id,
                                "desc": data.get('desc', ''),
                                "tipo": sensor_type_id,
                                "pinos": data.get('pinos', []),
                                "atributo1": value
                            }
                            
                            # 2a. Verifica regras (não bloqueante) - actuators use single value
                            await async_verificar_regras(client, device_id, sensor_id, value)
                            
                            # 2b. Salva no InfluxDB (não bloqueante) - actuators save single value
                            measurement_name = f"sensor_{sensor_id}"
                            point = Point(measurement_name) \
                                .tag("device_id", device_id) \
                                .tag("sensor_type", sensor_type_name) \
                                .tag("sensor_type_id", str(sensor_type_id)) \
                                .field("value", float(value)) \
                                .time(time.time_ns(), write_precision='ns')
                            
                            try:
                                await write_api.write(bucket=INFLUXDB_BUCKET, org=INFLUXDB_ORG, record=point)
                                print(f"  ✅ Salvo no InfluxDB: {measurement_name} ({sensor_type_name}) = {value} (Atuador)")
                            except Exception as e:
                                print(f"  [Influx] Erro ao salvar ponto: {e}")
                            
                            continue  # Skip the sensor dict processing below
                        
                        # Sensors now always send 'values' as a dictionary (e.g., {"x": 1951, "y": 1981, "bt": 0})
                        value = data.get('values')
                        if value is None:
                            print(f"  ⚠️ Mensagem sem campo 'values': {data}")
                            continue
                        if not isinstance(value, dict):
                            print(f"  ⚠️ Campo 'values' deve ser um dicionário, recebido: {type(value).__name__}")
                            continue
                        
                        # 2a. Verifica regras (não bloqueante)
                        await async_verificar_regras(client, device_id, sensor_id, value)
                        
                        # 2b. Salva no InfluxDB (não bloqueante)
                        # Use sensor_id as measurement name (each sensor gets its own "table")
                        measurement_name = f"sensor_{sensor_id}"
                        
                        # Dictionary values with named fields (e.g., {"x": 1951, "y": 1981, "bt": 0})
                        points = []
                        for field_name, field_value in value.items():
                            try:
                                point = Point(measurement_name) \
                                    .tag("device_id", device_id) \
                                    .tag("sensor_type", sensor_type_name) \
                                    .tag("sensor_type_id", str(sensor_type_id)) \
                                    .tag("field", field_name)
                                
                                # Save as string for keyboard types, float for others
                                if sensor_type_id in STRING_SENSOR_TYPES:
                                    point.field(field_name, str(field_value))
                                else:
                                    point.field(field_name, float(field_value))
                                
                                point.time(time.time_ns(), write_precision='ns')
                                points.append(point)
                            except (ValueError, TypeError) as e:
                                print(f"  [Influx] Ignorando valor inválido: {field_name}={field_value} ({e})")
                        
                        if points:
                            await write_api.write(bucket=INFLUXDB_BUCKET, org=INFLUXDB_ORG, record=points)
                            print(f"  ✅ Salvo no InfluxDB: {measurement_name} ({sensor_type_name}) dict com {len(points)} campos ({device_id})")
                                
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