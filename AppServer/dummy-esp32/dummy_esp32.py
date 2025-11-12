"""
Dummy ESP32 Simulator
Simulates an ESP32 device that:
- Subscribes to MQTT config topics
- Responds to configuration requests (GET)
- Stores configuration from API (SET)
- Publishes sensor readings with garbage data based on sensor type
"""

import paho.mqtt.client as mqtt
import json
import time
import random
import threading
import os
from datetime import datetime

class DummyESP32:
    def __init__(self, device_id=None, mqtt_broker=None, mqtt_port=None):
        # Allow environment variable overrides
        self.device_id = device_id or os.getenv("DEVICE_ID", "esp32_device_1")
        self.mqtt_broker = mqtt_broker or os.getenv("MQTT_BROKER", "mosquitto")
        self.mqtt_port = int(mqtt_port or os.getenv("MQTT_PORT", "1883"))
        
        # Simulated configuration storage (like ESP32 EEPROM/Flash)
        self.sensors_config = {
            "sensors": [
                {
                    "id": "sensor_pin_34",
                    "pin": 34,
                    "type": "LDR",
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
        
        # MQTT client setup
        self.client = mqtt.Client(client_id=self.device_id)
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        
        # Threading control
        self.running = False
        self.sensor_threads = []
        
    def on_connect(self, client, userdata, flags, rc):
        """Called when connected to MQTT broker"""
        print(f"[{self.device_id}] Connected to MQTT broker with result code {rc}")
        
        # Subscribe to configuration topics
        config_topics = [
            f"config/{self.device_id}/sensors/get",
            f"config/{self.device_id}/sensors/set",
            f"config/{self.device_id}/wifi/get",
            f"config/{self.device_id}/wifi/set"
        ]
        
        for topic in config_topics:
            client.subscribe(topic)
            print(f"[{self.device_id}] Subscribed to {topic}")
    
    def on_message(self, client, userdata, message):
        """Handle incoming MQTT messages"""
        topic = message.topic
        payload = message.payload.decode()
        
        print(f"[{self.device_id}] Received message on {topic}")
        
        try:
            # Handle sensors GET request
            if topic == f"config/{self.device_id}/sensors/get":
                response_topic = f"config/{self.device_id}/sensors"
                self.client.publish(response_topic, json.dumps(self.sensors_config))
                print(f"[{self.device_id}] Published sensors config to {response_topic}")
            
            # Handle sensors SET request
            elif topic == f"config/{self.device_id}/sensors/set":
                new_config = json.loads(payload)
                self.sensors_config = new_config
                print(f"[{self.device_id}] Updated sensors config: {len(new_config.get('sensors', []))} sensors")
                
                # Restart sensor publishing threads with new config
                self.restart_sensor_threads()
            
            # Handle wifi GET request
            elif topic == f"config/{self.device_id}/wifi/get":
                response_topic = f"config/{self.device_id}/wifi"
                # Don't send actual password
                safe_wifi = self.wifi_config.copy()
                safe_wifi["password"] = "********"
                self.client.publish(response_topic, json.dumps(safe_wifi))
                print(f"[{self.device_id}] Published wifi config to {response_topic}")
            
            # Handle wifi SET request
            elif topic == f"config/{self.device_id}/wifi/set":
                new_wifi = json.loads(payload)
                self.wifi_config = new_wifi
                print(f"[{self.device_id}] Updated wifi config: SSID={new_wifi.get('ssid')}")
        
        except json.JSONDecodeError as e:
            print(f"[{self.device_id}] Error parsing JSON: {e}")
        except Exception as e:
            print(f"[{self.device_id}] Error handling message: {e}")
    
    def generate_sensor_data(self, sensor):
        """Generate realistic garbage data based on sensor type"""
        sensor_type = sensor.get("type", "UNKNOWN")
        
        # Generate data based on sensor type
        if sensor_type == "LDR":
            # Light sensor: 0-4095 (ADC range)
            return random.randint(100, 4000)
        
        elif sensor_type == "DHT22":
            # Temperature/Humidity sensor
            # Return dict for multi-value sensors
            return {
                "temperature": round(random.uniform(15.0, 35.0), 2),
                "humidity": round(random.uniform(30.0, 80.0), 2)
            }
        
        elif sensor_type == "MPU6050":
            # Accelerometer/Gyroscope
            return {
                "accel_x": round(random.uniform(-2.0, 2.0), 3),
                "accel_y": round(random.uniform(-2.0, 2.0), 3),
                "accel_z": round(random.uniform(-2.0, 2.0), 3),
                "gyro_x": round(random.uniform(-250.0, 250.0), 2),
                "gyro_y": round(random.uniform(-250.0, 250.0), 2),
                "gyro_z": round(random.uniform(-250.0, 250.0), 2)
            }
        
        elif sensor_type == "DS18B20":
            # Temperature sensor
            return round(random.uniform(10.0, 40.0), 2)
        
        elif sensor_type == "HC-SR04":
            # Ultrasonic distance sensor (cm)
            return round(random.uniform(2.0, 400.0), 2)
        
        elif sensor_type == "APDS9960":
            # Gesture/Color/Proximity sensor
            return {
                "proximity": random.randint(0, 255),
                "red": random.randint(0, 255),
                "green": random.randint(0, 255),
                "blue": random.randint(0, 255)
            }
        
        elif sensor_type == "JOYSTICK":
            # Joystick with X, Y, button
            return {
                "x": random.randint(0, 4095),
                "y": random.randint(0, 4095),
                "button": random.choice([0, 1])
            }
        
        else:
            # Generic analog sensor
            return random.randint(0, 4095)
    
    def publish_sensor_reading(self, sensor):
        """Continuously publish sensor readings"""
        sensor_id = sensor.get("id", f"sensor_pin_{sensor.get('pin')}")
        pin = sensor.get("pin")
        sampling_interval = sensor.get("sampling_interval", 5000) / 1000.0  # Convert to seconds
        
        print(f"[{self.device_id}] Starting sensor thread for {sensor_id} (every {sampling_interval}s)")
        
        while self.running:
            if sensor.get("enabled", True):
                # Generate sensor data
                value = self.generate_sensor_data(sensor)
                
                # Publish to sensor reading topic
                topic = f"{self.device_id}/sensors/{sensor_id}/data"
                
                # Create payload matching the expected format
                payload = {
                    "device_id": self.device_id,
                    "sensor_id": sensor_id,
                    "pin": pin,
                    "type": sensor.get("type"),
                    "value": value,
                    "timestamp": datetime.utcnow().isoformat() + "Z"
                }
                
                self.client.publish(topic, json.dumps(payload))
                print(f"[{self.device_id}] Published {sensor_id}: {value}")
            
            time.sleep(sampling_interval)
    
    def restart_sensor_threads(self):
        """Stop old sensor threads and start new ones with updated config"""
        # Stop all running threads
        self.running = False
        for thread in self.sensor_threads:
            thread.join(timeout=2.0)
        
        self.sensor_threads = []
        self.running = True
        
        # Start new threads for each enabled sensor
        for sensor in self.sensors_config.get("sensors", []):
            if sensor.get("enabled", True):
                thread = threading.Thread(target=self.publish_sensor_reading, args=(sensor,), daemon=True)
                thread.start()
                self.sensor_threads.append(thread)
    
    def start(self):
        """Start the dummy ESP32 simulator"""
        print(f"[{self.device_id}] Starting dummy ESP32 simulator...")
        print(f"[{self.device_id}] Connecting to MQTT broker at {self.mqtt_broker}:{self.mqtt_port}")
        
        # Connect to MQTT broker with retry logic
        max_retries = 5
        retry_delay = 2
        
        for attempt in range(max_retries):
            try:
                self.client.connect(self.mqtt_broker, self.mqtt_port, 60)
                break
            except Exception as e:
                print(f"[{self.device_id}] Connection attempt {attempt + 1}/{max_retries} failed: {e}")
                if attempt < max_retries - 1:
                    print(f"[{self.device_id}] Retrying in {retry_delay} seconds...")
                    time.sleep(retry_delay)
                else:
                    print(f"[{self.device_id}] Failed to connect after {max_retries} attempts. Exiting.")
                    return
        
        self.client.loop_start()
        
        # Wait for connection
        time.sleep(2)
        
        # Start sensor publishing threads
        self.running = True
        self.restart_sensor_threads()
        
        print(f"[{self.device_id}] Dummy ESP32 is running! Press Ctrl+C to stop.")
        
        try:
            while True:
                time.sleep(1)
        except KeyboardInterrupt:
            print(f"\n[{self.device_id}] Stopping dummy ESP32...")
            self.stop()
    
    def stop(self):
        """Stop the dummy ESP32 simulator"""
        self.running = False
        
        # Stop all sensor threads
        for thread in self.sensor_threads:
            thread.join(timeout=2.0)
        
        # Disconnect from MQTT
        self.client.loop_stop()
        self.client.disconnect()
        
        print(f"[{self.device_id}] Dummy ESP32 stopped.")


if __name__ == "__main__":
    # Configuration from environment variables
    dummy = DummyESP32()
    dummy.start()
