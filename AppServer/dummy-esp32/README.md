# Dummy ESP32 Simulator (Docker Version)

A Dockerized ESP32 simulator for testing the IoT dashboard without physical hardware.

## Features

✅ **Runs in Docker**: Fully containerized, part of the docker-compose stack
✅ **Environment-based Config**: Configure device ID and MQTT settings via environment variables
✅ **Auto-reconnect**: Retries connection to MQTT broker on startup
✅ **Full MQTT Communication**: Subscribes to config topics and responds like a real ESP32
✅ **Realistic Sensor Data**: Generates appropriate garbage data based on sensor type
✅ **Multi-threaded Publishing**: Each sensor publishes on its own schedule

## Quick Start

### Start the Simulator

```powershell
cd AppServer
docker-compose up -d dummy_esp32
```

### View Logs

```powershell
docker-compose logs -f dummy_esp32
```

You should see:
```
[esp32_device_dummy1] Starting dummy ESP32 simulator...
[esp32_device_dummy1] Connecting to MQTT broker at mosquitto:1883
[esp32_device_dummy1] Connected to MQTT broker with result code 0
[esp32_device_dummy1] Subscribed to config/esp32_device_dummy1/sensors/get
[esp32_device_dummy1] Starting sensor thread for sensor_pin_34 (every 5.0s)
[esp32_device_dummy1] Published sensor_pin_34: 2341
```

### Start Everything Together

```powershell
# Start all services including the dummy ESP32
docker-compose up -d

# Or use the dev-mode script
..\dev-mode.ps1
```

## Configuration

The simulator reads configuration from environment variables in `docker-compose.yml`:

```yaml
environment:
  - DEVICE_ID=esp32_device_dummy1      # Device identifier
  - MQTT_BROKER=mosquitto              # MQTT broker hostname
  - MQTT_PORT=1883                     # MQTT broker port
```

### Run Multiple Dummy Devices

Add more instances to `docker-compose.yml`:

```yaml
  dummy_esp32_2:
    build:
      context: ./dummy-esp32
    container_name: dummy_esp32_2
    depends_on:
      - mosquitto
    environment:
      - DEVICE_ID=esp32_device_dummy2
      - MQTT_BROKER=mosquitto
      - MQTT_PORT=1883
    restart: always
    networks:
      - iot-net

  dummy_esp32_3:
    build:
      context: ./dummy-esp32
    container_name: dummy_esp32_3
    depends_on:
      - mosquitto
    environment:
      - DEVICE_ID=esp32_device_dummy3
      - MQTT_BROKER=mosquitto
      - MQTT_PORT=1883
    restart: always
    networks:
      - iot-net
```

Then start them all:
```powershell
docker-compose up -d dummy_esp32 dummy_esp32_2 dummy_esp32_3
```

## Testing

### 1. Get Current Configuration

```powershell
curl http://localhost:5000/esp32_device_dummy1/settings/sensors/get
```

### 2. Add/Update Sensors

```powershell
curl -X POST http://localhost:5000/esp32_device_dummy1/settings/sensors/set `
  -H "Content-Type: application/json" `
  -d '{
    "sensors": [
      {
        "pin": 26,
        "type": "HC-SR04",
        "enabled": true,
        "sampling_interval": 3000
      }
    ]
  }'
```

Watch the logs to see the simulator respond:
```powershell
docker-compose logs -f dummy_esp32
```

Output:
```
[esp32_device_dummy1] Received message on config/esp32_device_dummy1/sensors/set
[esp32_device_dummy1] Updated sensors config: 3 sensors
[esp32_device_dummy1] Starting sensor thread for sensor_pin_26 (every 3.0s)
[esp32_device_dummy1] Published sensor_pin_26: 156.23
```

### 3. Get Sensor Readings from InfluxDB

```powershell
curl "http://localhost:5000/esp32_device_dummy1/sensors/sensor_pin_34/read?start=-5m"
```

## Docker Commands

### Rebuild After Code Changes

```powershell
docker-compose build dummy_esp32
docker-compose up -d dummy_esp32
```

Or use the rebuild script:
```powershell
..\rebuild-services.ps1 -Service all
```

### Stop Simulator

```powershell
docker-compose stop dummy_esp32
```

### Remove Simulator

```powershell
docker-compose down dummy_esp32
```

### View Real-time Logs

```powershell
docker-compose logs -f dummy_esp32
```

## Supported Sensor Types

The simulator generates realistic data for:

- **LDR**: Light sensor (0-4095)
- **DHT22**: Temperature (15-35°C) + Humidity (30-80%)
- **MPU6050**: Accelerometer + Gyroscope
- **DS18B20**: Temperature sensor (10-40°C)
- **HC-SR04**: Ultrasonic distance (2-400cm)
- **APDS9960**: Proximity + RGB color
- **JOYSTICK**: X/Y position + button state
- **Generic**: Any unknown type returns 0-4095

## Architecture

```
┌─────────────────────────────────────────┐
│     Docker Container: dummy_esp32_1     │
│                                         │
│  ┌──────────────────────────────────┐  │
│  │    Dummy ESP32 Python Script     │  │
│  │  • Config storage (memory)       │  │
│  │  • Sensor data generation        │  │
│  │  • Multi-threaded publishing     │  │
│  └────────────┬─────────────────────┘  │
└───────────────┼─────────────────────────┘
                │
                │ MQTT Topics
                │ (via Docker network: iot-net)
                │
                ▼
┌───────────────────────────────────────────┐
│   Docker Container: mosquitto            │
│   MQTT Broker                            │
└──────────┬──────────────┬─────────────────┘
           │              │
    ┌──────▼─────┐  ┌─────▼──────┐
    │ api_server │  │  ingestor  │
    │ (Flask)    │  │  (Python)  │
    └────────────┘  └─────┬──────┘
                          │
                          ▼
                   ┌─────────────┐
                   │  influxdb   │
                   └─────────────┘
```

## Troubleshooting

### Simulator won't start

Check if mosquitto is running:
```powershell
docker-compose ps mosquitto
```

### No connection to MQTT

Check the logs:
```powershell
docker-compose logs dummy_esp32
```

If you see "Connection attempt X/5 failed", wait a bit for mosquitto to fully start.

### Data not appearing in InfluxDB

1. Check ingestor logs: `docker-compose logs ingestor`
2. Verify ingestor is subscribed to correct topics
3. Check simulator is publishing: `docker-compose logs dummy_esp32` should show "Published sensor_pin_X"

### Want to change sensor configuration

Edit `dummy_esp32.py` and rebuild:
```powershell
docker-compose build dummy_esp32
docker-compose up -d dummy_esp32
```

Or use the API to update configuration dynamically (no rebuild needed).

## Development Workflow

1. **Start all services**: `docker-compose up -d`
2. **Open Vue frontend**: http://localhost:5173
3. **Use dummy device**: Select `esp32_device_dummy1` in the UI
4. **Test configuration**: Add/remove sensors via the UI
5. **Watch live data**: See garbage sensor data streaming in real-time
6. **Ready for hardware**: Once tested, deploy to real ESP32

## Next Steps

- ✅ Test API endpoints with the dummy device
- ✅ Verify sensor data flows to InfluxDB
- ✅ Test Vue.js frontend with live data
- ✅ Add more dummy devices for multi-device testing
- 🎯 Deploy tested configuration to real ESP32 hardware
