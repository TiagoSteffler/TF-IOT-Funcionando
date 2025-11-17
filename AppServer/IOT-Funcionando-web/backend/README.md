# Backend (IOT-Funcionando-web)

This backend is a migrated/adapted copy of the `testeinit/backend` server that handles pairing and device heartbeats for ESP32 devices.

Quick start:

1. Install dependencies

```powershell
cd backend
npm install
```

2. Run in development

```powershell
npm run dev
```

3. Production

```powershell
npm start
```

Environment variables (optional): `INFLUX_URL`, `INFLUX_TOKEN`, `INFLUX_ORG`, `INFLUX_BUCKET`, `MQTT_BROKER`, `PORT`, `HOST_IP`.
