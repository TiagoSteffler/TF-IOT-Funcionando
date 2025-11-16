# API Integration Guide

This document explains how the Vue.js frontend integrates with the Flask API server to communicate with ESP32 devices via MQTT and InfluxDB.

## 📡 API Endpoints

All endpoints use `http://localhost:5000` as the base URL.

### 1. Get Sensor Readings (InfluxDB Query)
```
GET /{device_id}/sensors/{sensor_id}/read
```

**Query Parameters:**
- `start` (optional): Time range start (default: `-1h`)
  - Examples: `-1h`, `-5m`, `-1d`, `-30d`
- `every` (optional): Aggregation window (default: raw data)
  - Examples: `1m`, `5m`, `1h`
- `measurement` (optional): Filter by measurement name (default: all)

**Response:** JSON array
```json
[
  {
    "time": "2025-10-28T13:45:00+00:00",
    "value": 2699.54,
    "measurement": "LDR"
  },
  ...
]
```

**Vue Component:** `SensorReadings.vue`
- Automatically constructs device ID as `esp32_device_{deviceId}`
- Fetches on mount and when pin/device changes
- Renders Chart.js line graph

---

### 2. Get Sensor Configuration (MQTT Request-Response)
```
GET /{device_id}/settings/sensors/get
```

**How it works:**
1. API sends MQTT request to `config/{device_id}/sensors/get`
2. ESP32 receives request and publishes current config to `config/{device_id}/sensors`
3. API subscribes to response topic and waits up to 5 seconds
4. Returns cached response or timeout error

**Response (Success):** JSON
```json
{
  "sensors": [
    {
      "id": "sensor_pin_4",
      "pin": 4,
      "type": "sensor",
      "model": "DHT22",
      "protocol": "ADC",
      "name": "DHT22 on Pin 4"
    }
  ]
}
```

**Response (Timeout - 408):** JSON
```json
{
  "error": "timeout",
  "message": "ESP32 'esp32_device_1' não respondeu em 5 segundos. Verifique se o dispositivo está online.",
  "sensors": []
}
```

**Caching:** Responses are cached for 10 seconds to reduce MQTT traffic.

**Vue Component:** `SensorList.vue`
- Calls this endpoint on mount and device change
- Displays sensors sorted by pin number
- Handles timeout gracefully with empty list

---

### 3. Get WiFi Configuration (MQTT Request-Response)
```
GET /{device_id}/settings/wifi/get
```

**How it works:**
1. API sends MQTT request to `config/{device_id}/wifi/get`
2. ESP32 receives request and publishes current config to `config/{device_id}/wifi`
3. API subscribes to response topic and waits up to 5 seconds
4. Returns cached response or timeout error

**Response (Success):** JSON
```json
{
  "ssid": "MyWiFi",
  "mqtt_broker": "192.168.1.10",
  "mqtt_device_id": "esp32_device_1"
}
```

**Note:** Password is never returned for security reasons.

**Response (Timeout - 408):** Similar to sensors/get

**Caching:** Responses are cached for 10 seconds.

**Vue Component:** `SettingsPanel.vue` (optional fetch on mount)

---

### 4. Set Sensor Configuration (Read-Modify-Write Pattern)
```
POST /{device_id}/settings/sensors/set
Content-Type: application/json
```

**⚠️ IMPORTANT - Read-Modify-Write Pattern:**
To prevent accidental deletion of existing sensors, the API implements a **read-modify-write** workflow:

1. **READ**: API requests current configuration from ESP32 (5s timeout)
2. **MODIFY**: Merges new sensor config with existing sensors
   - Updates sensor if same `id` or `pin` exists
   - Adds new sensor if not found
3. **WRITE**: Sends complete merged configuration to ESP32

**Request Body:**
```json
{
  "sensors": [
    {
      "id": "sensor_pin_4",
      "pin": 4,
      "type": "sensor",
      "model": "DHT22",
      "protocol": "ADC",
      "name": "DHT22 on Pin 4"
    }
  ]
}
```

**Response:**
```json
{
  "status": "config_sent",
  "device": "esp32_device_1",
  "topic": "config/esp32_device_1/sensors/set",
  "sensors_count": 3,
  "merged_config": {
    "sensors": [
      { "id": "sensor_pin_2", "pin": 2, ... },
      { "id": "sensor_pin_4", "pin": 4, ... },
      { "id": "sensor_pin_6", "pin": 6, ... }
    ]
  }
}
```

**Merge Logic:**
- Sensors are matched by `id` field (e.g., `"sensor_pin_4"`)
- If `id` not present, falls back to matching by `pin` number
- Existing sensor properties are updated with new values
- New sensors are appended to the list
- **No sensors are deleted** unless explicitly removed (requires separate endpoint)

**Vue Component:** `SensorSetup.vue`
- Sends configuration when user clicks "Save Settings"
- Shows success/error messages
- Emits to parent for local state sync

---

### 5. Set WiFi Configuration (MQTT Publish)
```
POST /{device_id}/settings/wifi/set
Content-Type: application/json
```

**Request Body:**
```json
{
  "ssid": "MyWiFi",
  "password": "MyPassword123",
  "mqtt_broker": "192.168.1.10",
  "mqtt_device_id": "esp32_device_1"
}
```

**Response:**
```json
{
  "status": "config_sent",
  "device": "esp32_device_1",
  "topic": "config/esp32_device_1/wifi/set",
  "note": "ESP32 will restart to apply WiFi settings"
}
```

**Note:** Unlike sensor config, WiFi settings **fully replace** existing configuration (no merge). The ESP32 will typically restart after receiving WiFi settings to apply the new network configuration.

**Vue Component:** `SettingsPanel.vue`
- POSTs WiFi and MQTT configuration
- Shows success/error messages
- Auto-populates MQTT device ID based on current board

---

## 🔌 MQTT Topics

The API server publishes/subscribes to these MQTT topics:

### Sensor Configuration
- **SET**: `config/{device_id}/sensors/set` (API publishes)
- **GET Request**: `config/{device_id}/sensors/get` (API publishes)
- **GET Response**: `config/{device_id}/sensors` (ESP32 publishes)

### WiFi Configuration
- **SET**: `config/{device_id}/wifi/set` (API publishes)
- **GET Request**: `config/{device_id}/wifi/get` (API publishes)
- **GET Response**: `config/{device_id}/wifi` (ESP32 publishes)

### Sensor Data (handled by ingestor)
- **Publish**: ESP32 publishes sensor readings to topic configured in firmware
- **Ingest**: Ingestor subscribes and writes to InfluxDB

---

## 🏗️ Architecture Flow

### Read-Modify-Write Pattern (Sensor Configuration)

**Problem:** When adding/updating a sensor, we don't want to overwrite all existing sensors on the ESP32.

**Solution:** The API implements an atomic read-modify-write workflow:

```
User clicks "Save Sensor"
    ↓
Vue Component (SensorSetup.vue)
    ↓ HTTP POST /settings/sensors/set
Flask API
    ↓
┌─────────────────────────────────────────────┐
│ STEP 1: READ (Get Current Config)          │
│ - Publishes MQTT: config/{id}/sensors/get  │
│ - Waits 5s for ESP32 response              │
│ - If timeout, assumes empty config          │
└─────────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────────┐
│ STEP 2: MODIFY (Merge Configurations)      │
│ - Indexes existing sensors by ID/pin       │
│ - Updates matching sensors (keeps others)   │
│ - Adds new sensors to list                  │
│ - Result: Complete merged config            │
└─────────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────────┐
│ STEP 3: WRITE (Send Complete Config)       │
│ - Publishes MQTT: config/{id}/sensors/set  │
│ - ESP32 receives FULL config and saves it  │
│ - Updates local cache                       │
└─────────────────────────────────────────────┘
    ↓
ESP32 saves to EEPROM/NVS
    ↓
Success response to Vue
```

**Example:**

**Current ESP32 Config:**
```json
{
  "sensors": [
    {"id": "sensor_pin_2", "pin": 2, "type": "sensor", "model": "LDR"},
    {"id": "sensor_pin_6", "pin": 6, "type": "actuator", "model": "LED"}
  ]
}
```

**User adds new sensor via UI:**
```json
{
  "sensors": [
    {"id": "sensor_pin_4", "pin": 4, "type": "sensor", "model": "DHT22"}
  ]
}
```

**API sends to ESP32 (MERGED):**
```json
{
  "sensors": [
    {"id": "sensor_pin_2", "pin": 2, "type": "sensor", "model": "LDR"},
    {"id": "sensor_pin_4", "pin": 4, "type": "sensor", "model": "DHT22"},
    {"id": "sensor_pin_6", "pin": 6, "type": "actuator", "model": "LED"}
  ]
}
```

✅ **Result:** New sensor added without losing existing ones!

---

### Reading Sensor Data
```
Vue Component (SensorReadings.vue)
    ↓ HTTP GET
Flask API (/sensors/{id}/read)
    ↓ Flux Query
InfluxDB
    ↓ JSON Response
Vue Component (renders chart)
```

### Configuring Sensors
```
Vue Component (SensorSetup.vue)
    ↓ HTTP POST
Flask API (/settings/sensors/set)
    ↓ MQTT Publish
MQTT Broker
    ↓ Subscribe
ESP32 Device
    ↓ Saves to EEPROM
ESP32 Restarts/Applies Config
```

### Getting Current Config (TODO: Full Implementation)
```
Vue Component (SensorList.vue)
    ↓ HTTP GET
Flask API (/settings/sensors/get)
    ↓ MQTT Publish Request
MQTT Broker
    ↓ Subscribe
ESP32 Device
    ↓ MQTT Publish Response
MQTT Broker
    ↓ Subscribe (needs implementation)
Flask API (should cache/return)
    ↓ JSON Response
Vue Component (displays list)
```

---

## 🔧 Component Integration Summary

### `SensorReadings.vue`
- ✅ **Integrated**: Fetches from `/{device_id}/sensors/{sensor_id}/read`
- ✅ **JSON Parsing**: Handles `{time, value, measurement}` format
- ✅ **Chart Rendering**: Chart.js line graph with formatted timestamps
- ✅ **Device ID**: Uses `esp32_device_{deviceId}` format

### `SensorList.vue`
- ✅ **Integrated**: Calls `/{device_id}/settings/sensors/get`
- ⚠️ **Limitation**: API sends MQTT request but doesn't wait for response
- 📝 **TODO**: Implement MQTT subscription in Flask to return actual config
- ✅ **Device ID**: Uses `esp32_device_{deviceId}` format

### `SensorSetup.vue`
- ✅ **Integrated**: POSTs to `/{device_id}/settings/sensors/set`
- ✅ **Success/Error Handling**: Shows feedback messages
- ✅ **Sensor ID**: Auto-generates as `sensor_pin_{pinNumber}`
- ✅ **Device ID**: Uses `esp32_device_{deviceId}` format

### `SettingsPanel.vue`
- ✅ **Integrated**: POSTs to `/{device_id}/settings/wifi/set`
- ✅ **Success/Error Handling**: Shows feedback messages
- ✅ **Auto-populate**: MQTT device ID updates with selected board
- ✅ **Device ID**: Uses `esp32_device_{deviceId}` format

---

## ⚙️ Environment Variables (Flask API)

Make sure these are set in your Docker Compose or environment:

```env
INFLUXDB_URL=http://influxdb:8086
INFLUXDB_TOKEN=your_token_here
INFLUXDB_ORG=your_org
INFLUXDB_BUCKET=sensores
MQTT_BROKER_HOST=mosquitto
MQTT_BROKER_PORT=1883
```

---

## 🚀 Running the System

### 1. Start Backend Services (Docker Compose)
```bash
cd AppServer
docker-compose up -d
```

This starts:
- InfluxDB (port 8086)
- MQTT Broker (Mosquitto, port 1883)
- Ingestor (MQTT → InfluxDB)
- API Server (Flask, port 5000)

### 2. Start Frontend (Development)
```bash
cd AppServer/IOT-Funcionando-web
npm install
npm run dev
```

Access at: `http://localhost:5173`

### 3. Configure ESP32
- Connect to ESP32's AP mode
- Use "Add Board" in the dashboard
- Send WiFi credentials + MQTT settings
- ESP32 connects to your network

---

## 📝 Known Limitations & TODO

### Flask API Features Completed ✅
1. **GET endpoints with MQTT subscription** ✅
   - API subscribes to `config/+/sensors` and `config/+/wifi`
   - Implements request-response pattern with 5s timeout
   - Caches responses for 10 seconds to reduce MQTT traffic
   - Returns helpful error messages on timeout

2. **Read-Modify-Write for sensors** ✅
   - Prevents accidental deletion of existing sensors
   - Merges new sensor config with existing configuration
   - Matches sensors by ID or pin number
   - Sends complete merged config to ESP32

3. **Configuration caching** ✅
   - Thread-safe cache with locking
   - 10-second cache validity
   - Reduces MQTT traffic for frequent requests

### Still TODO

#### API Improvements Needed
1. **Add DELETE endpoint for sensors**
   ```python
   @app.route('/<device_id>/settings/sensors/<sensor_id>/delete', methods=['POST'])
   def delete_sensor(device_id, sensor_id):
       # Implement read-modify-write to remove specific sensor
       pass
   ```

2. **Add endpoint to erase all configuration**
   ```python
   @app.route('/<device_id>/settings/erase/all', methods=['POST'])
   def erase_all_config(device_id):
       # Send empty config to ESP32
       pass
   
   @app.route('/<device_id>/settings/erase/network', methods=['POST'])
   def erase_network_config(device_id):
       # Reset WiFi to AP mode
       pass
   ```

3. **Add health check for specific device**
   ```python
   @app.route('/<device_id>/health')
   def device_health(device_id):
       # Ping device via MQTT, check if online
       pass
   ```

### ESP32 Firmware Requirements
1. **Subscribe to MQTT topics:**
   - `config/{device_id}/sensors/set`
   - `config/{device_id}/wifi/set`
   - `config/{device_id}/sensors/get`
   - `config/{device_id}/wifi/get`

2. **Publish responses:**
   - When receiving `/get` requests, publish current config to response topic
   - Example: Receive on `config/esp32_device_1/sensors/get` → Publish to `config/esp32_device_1/sensors`

3. **Handle configuration:**
   - Parse JSON payloads
   - Save to EEPROM/NVS
   - Apply settings (restart WiFi, reconfigure sensors)

---

## 🧪 Testing

### Test API Endpoints Manually

```bash
# Get sensor readings
curl "http://localhost:5000/esp32_device_1/sensors/sensor_pin_4/read?start=-1h"

# Set sensor config
curl -X POST http://localhost:5000/esp32_device_1/settings/sensors/set \
  -H "Content-Type: application/json" \
  -d '{"sensors":[{"id":"sensor_pin_4","pin":4,"type":"sensor","protocol":"ADC"}]}'

# Set WiFi config
curl -X POST http://localhost:5000/esp32_device_1/settings/wifi/set \
  -H "Content-Type: application/json" \
  -d '{"ssid":"MyWiFi","password":"pass123","mqtt_broker":"192.168.1.10"}'
```

### Monitor MQTT Traffic

```bash
# Subscribe to all config topics
docker exec -it appserver-mosquitto-1 mosquitto_sub -v -t "config/#"
```

---

## 📚 Further Reading

- [Flask Documentation](https://flask.palletsprojects.com/)
- [InfluxDB Flux Query Language](https://docs.influxdata.com/flux/)
- [Paho MQTT Python Client](https://www.eclipse.org/paho/index.php?page=clients/python/index.php)
- [Chart.js Documentation](https://www.chartjs.org/docs/)
