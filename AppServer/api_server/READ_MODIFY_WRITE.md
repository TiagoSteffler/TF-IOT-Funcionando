# Flask API Updates - Read-Modify-Write Pattern

## 🎯 Problem Solved

**Original Issue:** When updating sensor configuration, POSTing new data would overwrite ALL sensors on ESP32, causing data loss.

**Solution:** Implemented **read-modify-write** pattern that preserves existing configuration while adding/updating sensors.

---

## 🔧 Changes Made to `api.py`

### 1. Added MQTT Subscription System

**New Imports:**
```python
import json
import threading
import time
```

**New Global Variables:**
```python
# Cache for storing ESP32 responses
config_cache = {}
config_cache_lock = threading.Lock()
```

**New MQTT Callbacks:**
```python
def on_message(client, userdata, message):
    """Stores ESP32 config responses in cache"""
    
def on_connect(client, userdata, flags, rc):
    """Subscribes to config/+/sensors and config/+/wifi"""
```

**MQTT Client Setup:**
```python
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message
```

---

### 2. Updated GET Endpoints (Request-Response Pattern)

#### `GET /{device_id}/settings/sensors/get`

**Old Behavior:**
- Sent MQTT request
- Returned "request_sent" immediately
- No actual data returned

**New Behavior:**
1. Checks cache (valid for 10s)
2. Sends MQTT request to ESP32
3. **Waits up to 5 seconds** for response
4. Returns actual sensor configuration
5. Returns 408 timeout error if ESP32 doesn't respond

**Benefits:**
- ✅ Actual data returned to Vue frontend
- ✅ Caching reduces MQTT traffic
- ✅ Graceful timeout handling
- ✅ Thread-safe cache access

#### `GET /{device_id}/settings/wifi/get`

Same pattern as sensors/get above.

---

### 3. Updated POST Endpoint (Read-Modify-Write)

#### `POST /{device_id}/settings/sensors/set`

**Old Behavior:**
```
Vue sends new sensor → API → MQTT → ESP32
(overwrites all existing sensors)
```

**New Behavior:**
```
Vue sends new sensor
    ↓
API requests current config from ESP32 (5s timeout)
    ↓
API merges new sensor with existing sensors
    ↓
API sends COMPLETE merged config to ESP32
    ↓
ESP32 saves (preserves all sensors)
```

**Merge Logic:**
```python
# Index existing sensors by ID or pin
sensor_map = {}
for sensor in existing_sensors:
    key = sensor.get('id') or f"pin_{sensor.get('pin')}"
    sensor_map[key] = sensor

# Update/add new sensors
for new_sensor in new_sensors:
    key = new_sensor.get('id') or f"pin_{new_sensor.get('pin')}"
    if key in sensor_map:
        sensor_map[key].update(new_sensor)  # Update existing
    else:
        sensor_map[key] = new_sensor  # Add new
```

**Response Includes Merged Config:**
```json
{
  "status": "config_sent",
  "sensors_count": 3,
  "merged_config": {
    "sensors": [...]  // Complete list with all sensors
  }
}
```

---

### 4. WiFi Config Endpoint

#### `POST /{device_id}/settings/wifi/set`

**Behavior:**
- WiFi settings **fully replace** (no merge needed)
- ESP32 typically restarts after WiFi config
- Password hidden in cache for security

---

## 📊 Example Workflow

### Scenario: User adds DHT22 sensor on Pin 4

**Step 1: ESP32 Current State**
```json
{
  "sensors": [
    {"id": "sensor_pin_2", "pin": 2, "model": "LDR"}
  ]
}
```

**Step 2: User submits via Vue**
```json
{
  "sensors": [
    {"id": "sensor_pin_4", "pin": 4, "model": "DHT22"}
  ]
}
```

**Step 3: API Workflow**
```
1. POST received
2. API → MQTT: "config/esp32_device_1/sensors/get"
3. ESP32 → MQTT: publishes current config
4. API receives: {"sensors": [{"id": "sensor_pin_2", ...}]}
5. API merges: 
   - sensor_pin_2 (existing)
   - sensor_pin_4 (new)
6. API → MQTT: "config/esp32_device_1/sensors/set"
   Payload: {"sensors": [pin_2, pin_4]}
7. ESP32 saves complete config
```

**Step 4: Final ESP32 State**
```json
{
  "sensors": [
    {"id": "sensor_pin_2", "pin": 2, "model": "LDR"},
    {"id": "sensor_pin_4", "pin": 4, "model": "DHT22"}
  ]
}
```

✅ **Both sensors preserved!**

---

## 🧪 Testing

### Test GET with curl
```bash
curl "http://localhost:5000/esp32_device_1/settings/sensors/get"

# Expected: Either sensor config JSON or timeout after 5s
```

### Test POST (Read-Modify-Write)
```bash
# Add a sensor
curl -X POST http://localhost:5000/esp32_device_1/settings/sensors/set \
  -H "Content-Type: application/json" \
  -d '{
    "sensors": [
      {"id": "sensor_pin_4", "pin": 4, "type": "sensor", "model": "DHT22"}
    ]
  }'

# Should return merged_config showing all sensors
```

### Monitor MQTT Traffic
```bash
# In one terminal - subscribe to all config topics
docker exec -it appserver-mosquitto-1 mosquitto_sub -v -t "config/#"

# In another terminal - make API request
curl "http://localhost:5000/esp32_device_1/settings/sensors/get"

# You should see:
# 1. API publishes: config/esp32_device_1/sensors/get (empty)
# 2. ESP32 publishes: config/esp32_device_1/sensors (JSON data)
```

---

## ⚠️ ESP32 Firmware Requirements

For this to work, your ESP32 firmware MUST:

### 1. Subscribe to Request Topics
```cpp
mqtt_client.subscribe("config/esp32_device_1/sensors/get");
mqtt_client.subscribe("config/esp32_device_1/wifi/get");
mqtt_client.subscribe("config/esp32_device_1/sensors/set");
mqtt_client.subscribe("config/esp32_device_1/wifi/set");
```

### 2. Handle GET Requests
```cpp
void on_mqtt_message(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, "config/esp32_device_1/sensors/get") == 0) {
    // Read current config from EEPROM/NVS
    String current_config = read_sensors_from_eeprom();
    
    // Publish to response topic
    mqtt_client.publish(
      "config/esp32_device_1/sensors",
      current_config.c_str()
    );
  }
}
```

### 3. Handle SET Requests
```cpp
void on_mqtt_message(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, "config/esp32_device_1/sensors/set") == 0) {
    // Parse JSON payload
    StaticJsonDocument<2048> doc;
    deserializeJson(doc, payload, length);
    
    // Save COMPLETE config to EEPROM/NVS
    save_sensors_to_eeprom(doc);
    
    // Apply configuration (initialize sensors)
    apply_sensor_config(doc);
  }
}
```

---

## 🎯 Benefits of This Approach

1. **No Data Loss**: Existing sensors are never accidentally deleted
2. **Atomic Updates**: Read-modify-write ensures consistency
3. **Fast Responses**: 10-second caching reduces MQTT traffic
4. **Graceful Degradation**: Timeouts handled with helpful error messages
5. **Thread-Safe**: Cache locking prevents race conditions
6. **Full Visibility**: Response includes complete merged config for debugging

---

## 📚 Next Steps

1. ✅ Flask API updated with read-modify-write
2. ✅ Documentation updated
3. ⏳ Update ESP32 firmware to handle MQTT topics
4. ⏳ Add DELETE endpoint for removing sensors
5. ⏳ Add erase endpoints for factory reset
6. ⏳ Test end-to-end with real hardware
