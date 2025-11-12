# 🐳 Docker Development Guide

## Quick Reference

### Hot Reload Status
- ✅ **Vue.js Frontend** - Hot reload enabled (no rebuild needed)
- ❌ **API Server** - Requires rebuild when Python code changes
- ❌ **Ingestor** - Requires rebuild when Python code changes
- ✅ **InfluxDB** - No rebuild needed (data persisted in volumes)
- ✅ **Mosquitto** - Config file mounted (restart to apply changes)

---

## 🚀 Quick Start

### First Time Setup
```powershell
cd AppServer
docker-compose up -d
```

### Check Status
```powershell
docker-compose ps
```

### View Logs
```powershell
# All services
docker-compose logs -f

# Specific service
docker-compose logs -f api_server
docker-compose logs -f ingestor_service
docker-compose logs -f web
```

---

## 🔄 Rebuilding Services

### Option 1: Use the Rebuild Scripts (Recommended)

**Rebuild backend services (API + Ingestor):**
```powershell
.\quick-rebuild.ps1
```

**Rebuild specific service:**
```powershell
.\rebuild-services.ps1 -Service api
.\rebuild-services.ps1 -Service ingestor
.\rebuild-services.ps1 -Service web
.\rebuild-services.ps1 -Service backend
.\rebuild-services.ps1 -Service all
```

### Option 2: Manual Docker Compose Commands

**Rebuild API server:**
```powershell
cd AppServer
docker-compose stop api_server
docker-compose build api_server
docker-compose up -d api_server
```

**Rebuild ingestor:**
```powershell
docker-compose stop ingestor_service
docker-compose build ingestor
docker-compose up -d ingestor_service
```

**Rebuild all services:**
```powershell
docker-compose down
docker-compose build
docker-compose up -d
```

**Force rebuild (ignore cache):**
```powershell
docker-compose build --no-cache api_server
docker-compose up -d api_server
```

---

## 🔥 Development Workflows

### Developing Vue.js Frontend

**No rebuild needed!** The web service uses volume mounting with Vite dev server:

1. Edit files in `AppServer/IOT-Funcionando-web/src/`
2. Save the file
3. Browser auto-refreshes (HMR - Hot Module Replacement)

**If hot reload stops working:**
```powershell
docker-compose restart web
```

**If you need to install new npm packages:**
```powershell
cd AppServer/IOT-Funcionando-web
npm install <package-name>
# Then restart the container
docker-compose restart web
```

---

### Developing Python Backend (API Server)

**After changing `api.py`:**

```powershell
# Option 1: Quick rebuild script
.\quick-rebuild.ps1

# Option 2: Manual
cd AppServer
docker-compose stop api_server
docker-compose build api_server
docker-compose up -d api_server
docker-compose logs -f api_server
```

**Enable hot reload for Flask (development only):**

You can modify the Dockerfile to add Flask's debug mode, but it's not recommended for production. For now, use the rebuild scripts.

---

### Developing Ingestor

**After changing `main.py`:**

```powershell
cd AppServer
docker-compose stop ingestor_service
docker-compose build ingestor
docker-compose up -d ingestor_service
docker-compose logs -f ingestor_service
```

---

## 🛠️ Common Tasks

### Start all services
```powershell
cd AppServer
docker-compose up -d
```

### Stop all services
```powershell
docker-compose stop
```

### Stop and remove containers (keeps data)
```powershell
docker-compose down
```

### Stop and remove everything (⚠️ deletes data)
```powershell
docker-compose down -v
```

### Restart a service
```powershell
docker-compose restart api_server
```

### View real-time logs
```powershell
docker-compose logs -f api_server
```

### Execute command inside container
```powershell
# API server
docker exec -it api_service bash

# Ingestor
docker exec -it ingestor_service sh

# Web
docker exec -it iot-web sh
```

### Check container resource usage
```powershell
docker stats
```

---

## 🐛 Debugging

### API Server not responding?

**Check if it's running:**
```powershell
docker-compose ps api_server
```

**View logs:**
```powershell
docker-compose logs api_server
```

**Check if Flask is listening:**
```powershell
docker exec -it api_service curl http://localhost:5000/health
```

**Restart:**
```powershell
docker-compose restart api_server
```

---

### Web service not hot reloading?

**Restart the container:**
```powershell
docker-compose restart web
```

**Check logs for errors:**
```powershell
docker-compose logs web
```

**Access the container and check Vite:**
```powershell
docker exec -it iot-web sh
npm run dev -- --host 0.0.0.0
```

---

### MQTT not working?

**Check Mosquitto logs:**
```powershell
docker-compose logs mosquitto
```

**Test MQTT connection:**
```powershell
# Subscribe to all topics
docker exec -it mosquitto mosquitto_sub -v -t "#"

# Publish a test message
docker exec -it mosquitto mosquitto_pub -t "test" -m "hello"
```

---

### InfluxDB not accessible?

**Check if it's running:**
```powershell
docker-compose ps influxdb
```

**Access InfluxDB UI:**
Open browser: http://localhost:8086

**Check logs:**
```powershell
docker-compose logs influxdb
```

---

## 📦 Port Reference

| Service | Port | Description |
|---------|------|-------------|
| InfluxDB | 8086 | InfluxDB UI & API |
| Mosquitto | 1883 | MQTT Broker |
| API Server | 5000 | Flask REST API |
| Web (Vite) | 5173 | Vue.js dev server |

---

## 🎯 Best Practices

1. **Use volume mounts for development** - Already configured for Vue.js
2. **Use rebuild scripts** - Faster than `docker-compose down && up`
3. **Check logs often** - `docker-compose logs -f <service>`
4. **Don't use `--no-cache` unless needed** - It's slower
5. **Keep Docker Desktop running** - Required for containers to work
6. **Use `docker-compose down` not `stop`** - Cleans up networks properly

---

## 🚨 Troubleshooting

### Error: "Port already in use"

**Solution:**
```powershell
# Find process using the port (example: port 5000)
netstat -ano | findstr :5000

# Kill the process (replace PID with actual process ID)
taskkill /PID <PID> /F

# Or change the port in docker-compose.yml
```

### Error: "Cannot connect to Docker daemon"

**Solution:**
1. Open Docker Desktop
2. Wait for it to fully start
3. Try command again

### Services won't start

**Solution:**
```powershell
# Clean rebuild
docker-compose down -v
docker-compose build --no-cache
docker-compose up -d
```

### Changes not reflected after rebuild

**Solution:**
```powershell
# Force rebuild without cache
docker-compose build --no-cache api_server
docker-compose up -d api_server
```

---

## 📝 Development Workflow Example

**Typical development session:**

```powershell
# 1. Start all services
cd AppServer
docker-compose up -d

# 2. Work on Vue.js (hot reload, no rebuild)
# Edit files in IOT-Funcionando-web/src/
# Changes appear automatically in browser

# 3. Work on API (requires rebuild)
# Edit AppServer/api_server/api.py
.\quick-rebuild.ps1

# 4. Check logs
docker-compose logs -f api_server

# 5. Done for the day
docker-compose stop
```

---

## 🎓 Learn More

- [Docker Compose Documentation](https://docs.docker.com/compose/)
- [Vite Hot Module Replacement](https://vitejs.dev/guide/features.html#hot-module-replacement)
- [Flask Development Server](https://flask.palletsprojects.com/en/2.3.x/server/)
