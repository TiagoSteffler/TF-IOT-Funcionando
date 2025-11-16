"""
Dummy ESP32 Simulator (Asyncio Version)
Simula um dispositivo ESP32 que:
- Se inscreve em tópicos de configuração MQTT (GET/SET)
- [NOVO] Se inscreve em tópicos de comando de atuador (config/+/actuators/+/set)
- Publica leituras de sensores com dados fictícios
"""

import asyncio
import aiomqtt
import json
import random
import os
from datetime import datetime

class DummyESP32:
    def __init__(self, device_id=None, mqtt_broker=None, mqtt_port=None):
        self.device_id = device_id or os.getenv("DEVICE_ID", "esp32_device_1")
        self.mqtt_broker = mqtt_broker or os.getenv("MQTT_BROKER", "mosquitto")
        self.mqtt_port = int(mqtt_port or os.getenv("MQTT_PORT", "1883"))
        
        # Armazenamento de configuração simulado (como ESP32 EEPROM/Flash)
        self.sensors_config = {
            "sensors": [
                {
                    "id": "sensor_pin_34",
                    "pin": 34,
                    "type": "TECLADO_4X4",
                    "enabled": True,
                    "sampling_interval": 5000
                },
                {
                    "id": "sensor_pin_35",
                    "pin": 35,
                    "type": "DHT22",
                    "enabled": True,
                    "sampling_interval": 10000
                }
            ]
        }
        
        self.wifi_config = {
            "ssid": "DummyNetwork",
            "password": "********",
            "mqtt_broker": self.mqtt_broker,
            "mqtt_port": self.mqtt_port,
            "mqtt_device_id": self.device_id
        }
        
        self.client = None
        self.sensor_tasks = [] # Armazena asyncio.Task em vez de threads

    def generate_sensor_data(self, sensor):
        """Gera dados fictícios realistas com base no tipo de sensor"""
        sensor_type = sensor.get("type", "UNKNOWN")
        
        if sensor_type == "LDR":
            return random.randint(100, 4000)
        
        elif sensor_type == "DHT22":
            return {
                "temperature": round(random.uniform(15.0, 35.0), 2),
                "humidity": round(random.uniform(30.0, 80.0), 2)
            }
        
        elif sensor_type == "MPU6050":
            return {
                "accel_x": round(random.uniform(-2.0, 2.0), 3),
                "accel_y": round(random.uniform(-2.0, 2.0), 3),
                "accel_z": round(random.uniform(-2.0, 2.0), 3),
                "gyro_x": round(random.uniform(-250.0, 250.0), 2),
                "gyro_y": round(random.uniform(-250.0, 250.0), 2),
                "gyro_z": round(random.uniform(-250.0, 250.0), 2)
            }
        
        elif sensor_type == "DS18B20":
            return round(random.uniform(10.0, 40.0), 2)
        
        elif sensor_type == "HC-SR04":
            return round(random.uniform(2.0, 400.0), 2)
        
        elif sensor_type == "APDS9960":
            return {
                "proximity": random.randint(0, 255),
                "red": random.randint(0, 255),
                "green": random.randint(0, 255),
                "blue": random.randint(0, 255)
            }
        
        elif sensor_type == "JOYSTICK":
            return {
                "x": random.randint(0, 4095),
                "y": random.randint(0, 4095),
                "button": random.choice([0, 1])
            }
        
        elif sensor_type == "TECLADO_4X4":
            teclas_possiveis = ['1', '2', '3', 'A', '4', '5', '6', 'B', '7', '8', '9', 'C', '*', '0', '#', 'D']
            return random.choice(teclas_possiveis)

        else:
            return random.randint(0, 4095)

    async def publish_sensor_reading(self, sensor):
        """Publica continuamente leituras de sensores (como uma asyncio.Task)"""
        try:
            sensor_id = sensor.get("id", f"sensor_pin_{sensor.get('pin')}")
            pin = sensor.get("pin")
            sampling_interval = sensor.get("sampling_interval", 5000) / 1000.0  # Converte para segundos
            
            print(f"[{self.device_id}] Iniciando tarefa de sensor para {sensor_id} (a cada {sampling_interval}s)")
            
            while True:
                if sensor.get("enabled", True):
                    value = self.generate_sensor_data(sensor)
                    
                    topic = f"{self.device_id}/sensors/{sensor_id}/data"
                    payload = {
                        "device_id": self.device_id,
                        "sensor_id": sensor_id,
                        "pin": pin,
                        "type": sensor.get("type"),
                        "value": value,
                        "timestamp": datetime.utcnow().isoformat() + "Z"
                    }
                    
                    if self.client:
                        try:
                            await self.client.publish(topic, json.dumps(payload))
                            print(f"[{self.device_id}] Publicado {sensor_id}: {value}")
                        except Exception as e:
                            print(f"[{self.device_id}] Falha ao publicar: {e}")
                
                await asyncio.sleep(sampling_interval)
        
        except asyncio.CancelledError:
            print(f"[{self.device_id}] Tarefa do sensor {sensor_id} cancelada.")
        except Exception as e:
            print(f"[{self.device_id}] Erro na tarefa do sensor {sensor_id}: {e}")

    async def stop_all_sensor_tasks(self):
        """Para e cancela todas as tarefas de sensores em execução"""
        if not self.sensor_tasks:
            return
            
        print(f"[{self.device_id}] Cancelando {len(self.sensor_tasks)} tarefas de sensores...")
        for task in self.sensor_tasks:
            task.cancel()
        
        await asyncio.gather(*self.sensor_tasks, return_exceptions=True)
        self.sensor_tasks = []
        print(f"[{self.device_id}] Todas as tarefas de sensores paradas.")

    async def restart_sensor_tasks(self):
        """Para tarefas antigas e inicia novas com base na configuração atual"""
        await self.stop_all_sensor_tasks()
        
        print(f"[{self.device_id}] Iniciando novas tarefas de sensores com base na configuração...")
        for sensor in self.sensors_config.get("sensors", []):
            if sensor.get("enabled", True):
                task = asyncio.create_task(self.publish_sensor_reading(sensor))
                self.sensor_tasks.append(task)

    async def message_handler(self):
        """Processa todas as mensagens MQTT recebidas"""
        print(f"[{self.device_id}] Manipulador de mensagens iniciado.")
        async for message in self.client.messages:
            topic = message.topic.value
            payload = message.payload.decode()
            
            print(f"[{self.device_id}] Mensagem recebida em {topic}")
            
            try:
                # Lidar com GET de sensores
                if topic == f"config/{self.device_id}/sensors/get":
                    response_topic = f"config/{self.device_id}/sensors"
                    await self.client.publish(response_topic, json.dumps(self.sensors_config))
                    print(f"[{self.device_id}] Configuração de sensores publicada em {response_topic}")
                
                # Lidar com SET de sensores
                elif topic == f"config/{self.device_id}/sensors/set":
                    new_config = json.loads(payload)
                    self.sensors_config = new_config
                    print(f"[{self.device_id}] Configuração de sensores atualizada: {len(new_config.get('sensors', []))} sensores")
                    
                    # Reinicia tarefas de sensores com nova config
                    await self.restart_sensor_tasks()
                
                # Lidar com GET de wifi
                elif topic == f"config/{self.device_id}/wifi/get":
                    response_topic = f"config/{self.device_id}/wifi"
                    safe_wifi = self.wifi_config.copy()
                    safe_wifi["password"] = "********" # Não enviar senha
                    await self.client.publish(response_topic, json.dumps(safe_wifi))
                    print(f"[{self.device_id}] Configuração de wifi publicada em {response_topic}")
                
                # Lidar com SET de wifi
                elif topic == f"config/{self.device_id}/wifi/set":
                    new_wifi = json.loads(payload)
                    self.wifi_config = new_wifi
                    print(f"[{self.device_id}] Configuração de wifi atualizada: SSID={new_wifi.get('ssid')}")

                # *** [NOVO] Lidar com comando de atuador vindo do Ingestor ***
                elif topic.startswith(f"config/{self.device_id}/actuators/") and topic.endswith("/set"):
                    actuator_id = topic.split('/')[-2]
                    command = json.loads(payload)
                    value = command.get("value")
                    print(f"")
                    print(f"[{self.device_id}] ⚡️ COMANDO ATUADOR RECEBIDO ⚡️")
                    print(f"    Atuador: {actuator_id}")
                    print(f"    Valor:   {value}")
                    print(f"")
            
            except json.JSONDecodeError as e:
                print(f"[{self.device_id}] Erro ao decodificar JSON: {e}")
            except Exception as e:
                print(f"[{self.device_id}] Erro ao manipular mensagem: {e}")

    async def start(self):
        """Inicia o simulador dummy ESP32"""
        print(f"[{self.device_id}] Iniciando simulador dummy ESP32 (Async)...")
        print(f"[{self.device_id}] Conectando ao broker MQTT em {self.mqtt_broker}:{self.mqtt_port}")
        
        while True: # Adiciona um loop de reconexão
            try:
                # *** [CORREÇÃO APLICADA AQUI] ***
                # O argumento 'client_id' foi renomeado para 'identifier' no aiomqtt
                async with aiomqtt.Client(
                    hostname=self.mqtt_broker,
                    port=self.mqtt_port,
                    identifier=self.device_id  # <--- Esta é a correção
                ) as client:
                    self.client = client
                    print(f"[{self.device_id}] Conectado ao broker com sucesso.")
                    
                    # Tópicos para se inscrever
                    config_topics = [
                        f"config/{self.device_id}/sensors/get",
                        f"config/{self.device_id}/sensors/set",
                        f"config/{self.device_id}/wifi/get",
                        f"config/{self.device_id}/wifi/set",
                        f"config/{self.device_id}/actuators/+/set" # <-- Tópico do atuador
                    ]
                    
                    for topic in config_topics:
                        await self.client.subscribe(topic)
                        print(f"[{self.device_id}] Inscrito em {topic}")
                    
                    # Inicia as tarefas de publicação de sensores
                    await self.restart_sensor_tasks()
                    
                    # Inicia o manipulador de mensagens
                    await self.message_handler()

            except (asyncio.CancelledError, KeyboardInterrupt):
                print(f"\n[{self.device_id}] Parando o simulador...")
                await self.stop_all_sensor_tasks()
                print(f"[{self.device_id}] Simulador dummy ESP32 parado.")
                break # Sai do loop de reconexão
            except aiomqtt.MqttError as e:
                print(f"[{self.device_id}] Erro de conexão MQTT: {e}. Tentando reconectar em 5 segundos...")
                await self.stop_all_sensor_tasks() # Para tarefas antes de reconectar
                await asyncio.sleep(5)
            except Exception as e:
                print(f"[{self.device_id}] Erro inesperado: {e}")
                traceback.print_exc()
                await self.stop_all_sensor_tasks()
                await asyncio.sleep(5)


if __name__ == "__main__":
    dummy = DummyESP32()
    try:
        asyncio.run(dummy.start())
    except KeyboardInterrupt:
        print(f"\n[{dummy.device_id}] Processo principal interrompido. Desligando.")