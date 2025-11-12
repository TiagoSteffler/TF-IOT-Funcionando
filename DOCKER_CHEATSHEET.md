# 🐳 Docker Quick Reference Card

## ⚡ Quick Commands

```powershell
# Start development with live logs
.\dev-mode.ps1

# Rebuild backend after code changes
.\quick-rebuild.ps1

# Rebuild specific service
.\rebuild-services.ps1 -Service api
```

---

## 📝 Hot Reload Status

| Service | Hot Reload? | Action After Changes |
|---------|-------------|----------------------|
| Vue.js Frontend | ✅ YES | Just save file |
| API Server (Flask) | ❌ NO | Run `.\quick-rebuild.ps1` |
| Ingestor (Python) | ❌ NO | Run `.\quick-rebuild.ps1` |
| Mosquitto Config | ⚠️ RESTART | `docker-compose restart mosquitto` |

---

## 🎯 Common Workflows

### I changed Vue.js code
```powershell
# Nothing needed - just save the file!
# Browser will auto-refresh
```

### I changed api.py
```powershell
.\quick-rebuild.ps1
```

### I changed ingestor main.py
```powershell
.\quick-rebuild.ps1
```

### I need to see logs
```powershell
cd AppServer
docker-compose logs -f api_server
docker-compose logs -f ingestor_service
docker-compose logs -f web
```

### Something is broken
```powershell
cd AppServer
docker-compose down
docker-compose up -d
docker-compose logs -f
```

---

## 🔗 Service URLs

- **Vue.js**: http://localhost:5173
- **API**: http://localhost:5000
- **InfluxDB**: http://localhost:8086
- **MQTT**: mqtt://localhost:1883

---

## 🐛 Quick Fixes

### Vue not hot reloading?
```powershell
cd AppServer
docker-compose restart web
```

### API changes not appearing?
```powershell
.\quick-rebuild.ps1
```

### Everything is broken?
```powershell
cd AppServer
docker-compose down
docker-compose build --no-cache
docker-compose up -d
```

### Can't connect to API/MQTT?
```powershell
# Check if services are running
docker-compose ps

# View logs for errors
docker-compose logs api_server
```

---

## 📚 More Help

See `DOCKER_DEVELOPMENT.md` for comprehensive guide.
