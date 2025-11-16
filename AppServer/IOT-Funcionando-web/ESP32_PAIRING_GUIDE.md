# ESP32 Real Board Pairing Guide

## Overview

This guide explains how to pair a real ESP32 board with the IOT-Funcionando system using the automatic network discovery and pairing flow.

## Prerequisites

1. **Docker containers running:**
   ```powershell
   cd TF-IOT-Funcionando\AppServer
   docker compose up -d
   ```

2. **Mobile hotspot or router** with:
   - SSID: Any name (you'll configure this)
   - Password: `12345678` (or open network)
   - Must be accessible from both your computer and ESP32

3. **ESP32 firmware** with the pairing logic that:
   - Tries to connect to open networks first
   - Falls back to networks with password `12345678`
   - Sends GET request to gateway IP on port 3001: `/ping?mac=XX:XX:XX:XX:XX:XX`
   - Saves received configuration to SPIFFS
   - Restarts and connects to configured WiFi
   - Sends MQTT heartbeats to broker

## Pairing Flow

### 1. **Power on ESP32**
- ESP32 starts without saved WiFi credentials
- Scans for available networks
- Attempts connection to open networks
- If no open network, tries networks with password `12345678`

### 2. **ESP32 connects to network**
- Once connected, ESP32 gets IP from DHCP
- Detects gateway IP (usually `192.168.x.1` or `192.168.137.1` for Windows hotspot)

### 3. **ESP32 pings gateway**
- Sends HTTP GET request:
  ```
  GET http://<gateway-ip>:3001/ping?mac=AA:BB:CC:DD:EE:FF
  ```
- MAC address is the ESP32's WiFi MAC

### 4. **Backend responds (when pairing active)**
Backend returns JSON configuration:
```json
{
  "mac": "AA:BB:CC:DD:EE:FF",
  "ssid": "YourWiFiNetwork",
  "password": "YourWiFiPassword",
  "broker": "192.168.1.100",
  "port": "1883",
  "id": "esp32_device_1"
}
```

### 5. **ESP32 saves and restarts**
- Saves WiFi credentials and MQTT settings to SPIFFS/NVS
- Restarts automatically
- Connects to the configured WiFi network

### 6. **ESP32 connects to MQTT**
- Connects to MQTT broker at configured IP
- Publishes heartbeat every 30 seconds:
  ```
  Topic: device/<id>/heartbeat
  Payload: {"mac":"AA:BB:CC:DD:EE:FF", "ip":"192.168.1.50", "id":"esp32_device_1", "timestamp":1234567890}
  ```

### 7. **Backend detects device**
- Receives heartbeat via MQTT
- Updates device status to "online"
- Stores in InfluxDB for history

### 8. **UI confirms success**
- Frontend polls `/api/devices` every 3 seconds
- Detects new device when status becomes "online"
- Shows success message
- Stops pairing mode automatically

## Using the Web Interface

### Step-by-Step

1. **Open web interface**
   ```
   http://localhost:5173
   ```

2. **Click "Add New Board"**
   - You'll see the pairing dialog

3. **Read instructions and click "Start Setup"**

4. **Fill in configuration:**
   - **Board Name**: Friendly name (e.g., "ESP32 Living Room")
   - **MQTT Broker IP**: IP of your MQTT broker (e.g., `192.168.1.100`)
     - For Docker: use your host machine's IP on the network
     - Not `localhost` or `127.0.0.1` (ESP32 needs reachable IP)
   - **Device ID**: Auto-generated (e.g., `esp32_device_1`)
   - **WiFi SSID**: Your WiFi network name
   - **WiFi Password**: Your WiFi password

5. **Click "Start Pairing"**
   - Backend enters pairing mode
   - Waiting screen appears
   - Timeout: 2 minutes

6. **Power on ESP32** (if not already on)
   - ESP32 should connect to hotspot/open network
   - ESP32 pings gateway
   - Backend sends configuration
   - ESP32 restarts

7. **Wait for confirmation**
   - UI polls for device status
   - Success message appears when ESP32 sends first heartbeat
   - Device appears in main board list

## Network Setup Options

### Option 1: Windows Mobile Hotspot (Recommended for testing)

1. **Enable mobile hotspot:**
   - Settings → Network & Internet → Mobile hotspot
   - Set password to `12345678`

2. **Note your computer's IP on hotspot:**
   ```powershell
   ipconfig
   # Look for "Wireless LAN adapter Local Area Connection* X"
   # Usually 192.168.137.1
   ```

3. **Use this IP for MQTT Broker** in pairing form

### Option 2: Shared WiFi Router

1. Both computer and ESP32 connect to same WiFi
2. Find your computer's IP:
   ```powershell
   ipconfig
   # Look for your WiFi adapter IP (e.g., 192.168.1.100)
   ```
3. Use this IP for MQTT Broker

### Option 3: Docker with Port Forwarding (Advanced)

If running in Docker on WSL:

1. **Find WSL IP:**
   ```bash
   ip addr show eth0 | grep "inet " | awk '{print $2}' | cut -d/ -f1
   ```

2. **Set up Windows port forwarding** (PowerShell as Admin):
   ```powershell
   # Forward backend port
   netsh interface portproxy add v4tov4 listenport=3001 listenaddress=0.0.0.0 connectport=3001 connectaddress=<WSL_IP>
   
   # Forward MQTT port
   netsh interface portproxy add v4tov4 listenport=1883 listenaddress=0.0.0.0 connectport=1883 connectaddress=<WSL_IP>
   ```

3. Use Windows hotspot IP (192.168.137.1) for broker

## Troubleshooting

### ESP32 not connecting

**Check:**
- Is hotspot active with password `12345678`?
- Can ESP32 see the network? (check ESP32 serial output)
- Is ESP32 within WiFi range?

### ESP32 connects but no ping received

**Check:**
- Is backend running? `docker compose ps`
- Is port 3001 accessible? Test: `curl http://<gateway-ip>:3001/`
- Check backend logs: `docker compose logs backend`
- Is pairing mode active? Check UI shows "Waiting for ESP32..."

### Configuration sent but ESP32 doesn't connect to WiFi

**Check:**
- WiFi credentials correct?
- Is the configured WiFi network available?
- ESP32 serial output for error messages
- ESP32 saved config to SPIFFS successfully?

### ESP32 connects to WiFi but no heartbeat

**Check:**
- MQTT broker IP reachable from ESP32?
- MQTT broker running? `docker compose ps mosquitto`
- Check ESP32 can reach broker: ping from ESP32 to broker IP
- MQTT credentials correct? (default has no auth)
- Check mosquitto logs: `docker compose logs mosquitto`

### Pairing times out

**Common causes:**
- ESP32 not powered on
- ESP32 can't connect to hotspot (wrong password)
- ESP32 can't reach gateway IP (network isolation)
- Backend not in pairing mode (check logs)

## Backend API Reference

### Start Pairing Mode
```http
POST /api/pairing/start
Content-Type: application/json

{
  "ssid": "MyWiFi",
  "password": "mypassword",
  "brokerIP": "192.168.1.100",
  "port": 1883,
  "id": "esp32_device_1"
}

Response: {"success": true, "message": "Modo pareamento ativado", "broker": "192.168.1.100"}
```

### Stop Pairing Mode
```http
POST /api/pairing/stop

Response: {"success": true, "message": "Modo pareamento desativado"}
```

### Check Pairing Status
```http
GET /api/pairing/status

Response: {"active": true, "config": {...}}
```

### ESP32 Ping Endpoint
```http
GET /ping?mac=AA:BB:CC:DD:EE:FF

Response (pairing active):
{
  "mac": "AA:BB:CC:DD:EE:FF",
  "ssid": "MyWiFi",
  "password": "mypassword",
  "broker": "192.168.1.100",
  "port": "1883",
  "id": "esp32_device_1"
}

Response (pairing inactive): "OK"
```

### List Devices
```http
GET /api/devices

Response: [
  {
    "id": "esp32_device_1",
    "mac": "AA:BB:CC:DD:EE:FF",
    "ip": "192.168.1.50",
    "lastSeen": 1700000000000,
    "status": "online"
  }
]
```

## ESP32 Firmware Requirements

Your ESP32 code must implement:

1. **Network Discovery:**
   ```cpp
   // Scan for networks
   // Try open networks first
   // Try networks with password "12345678"
   ```

2. **HTTP Ping:**
   ```cpp
   // Get gateway IP
   IPAddress gateway = WiFi.gatewayIP();
   
   // Build URL
   String url = "http://" + gateway.toString() + ":3001/ping?mac=" + WiFi.macAddress();
   
   // Send GET request
   HTTPClient http;
   http.begin(url);
   int httpCode = http.GET();
   
   if (httpCode == 200) {
     String payload = http.getString();
     // Parse JSON and save to SPIFFS
   }
   ```

3. **MQTT Heartbeat:**
   ```cpp
   // Every 30 seconds
   String topic = "device/" + deviceId + "/heartbeat";
   String payload = "{\"mac\":\"" + WiFi.macAddress() + 
                    "\",\"ip\":\"" + WiFi.localIP().toString() + 
                    "\",\"id\":\"" + deviceId + 
                    "\",\"timestamp\":" + String(millis()) + "}";
   mqttClient.publish(topic.c_str(), payload.c_str());
   ```

## Example Complete Flow

```
1. User powers on ESP32
2. ESP32 scans networks → finds "MyHotspot" (open or password 12345678)
3. ESP32 connects → gets IP 192.168.137.45
4. User opens web UI → clicks "Add New Board" → fills form → clicks "Start Pairing"
5. Backend enters pairing mode
6. ESP32 sends: GET http://192.168.137.1:3001/ping?mac=AA:BB:CC:DD:EE:FF
7. Backend responds: {"ssid":"HomeWiFi","password":"secret","broker":"192.168.137.1","port":"1883","id":"esp32_device_1"}
8. ESP32 saves config → restarts
9. ESP32 connects to "HomeWiFi" → gets IP 192.168.1.75
10. ESP32 connects to MQTT broker at 192.168.137.1:1883
11. ESP32 publishes heartbeat to device/esp32_device_1/heartbeat
12. Backend receives heartbeat → marks device online
13. UI detects device online → shows success → stops pairing
14. User sees ESP32 in device list
```

## Security Notes

- Default setup has no authentication (suitable for local testing only)
- For production:
  - Add MQTT authentication
  - Use HTTPS for /ping endpoint
  - Add rate limiting to prevent abuse
  - Validate MAC addresses
  - Encrypt WiFi password in transit
  - Use WPA3 for WiFi networks
