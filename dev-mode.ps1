# dev-mode.ps1
# Start development environment with live logs

param(
    [Parameter(Mandatory=$false)]
    [switch]$Rebuild
)

Set-Location "$PSScriptRoot\AppServer"

Write-Host "`n🚀 Starting Development Environment" -ForegroundColor Cyan
Write-Host "===================================" -ForegroundColor Cyan

if ($Rebuild) {
    Write-Host "`n🔨 Rebuilding backend services..." -ForegroundColor Yellow
    docker-compose build api_server ingestor
}

Write-Host "`n▶️  Starting all services..." -ForegroundColor Green
docker-compose up -d

Start-Sleep -Seconds 2

Write-Host "`n✅ Services started!" -ForegroundColor Green
Write-Host "`n📊 Container status:" -ForegroundColor Cyan
docker-compose ps

Write-Host "`n🌐 Service URLs:" -ForegroundColor Cyan
Write-Host "  • Vue.js Dev:    http://localhost:5173" -ForegroundColor White
Write-Host "  • API Server:    http://localhost:5000" -ForegroundColor White
Write-Host "  • InfluxDB UI:   http://localhost:8086" -ForegroundColor White
Write-Host "  • MQTT Broker:   mqtt://localhost:1883" -ForegroundColor White

Write-Host "`n📋 Live logs (Ctrl+C to exit, services keep running):" -ForegroundColor Yellow
Write-Host "─────────────────────────────────────────────────────" -ForegroundColor Gray

# Follow logs for API and Ingestor (most relevant for backend dev)
docker-compose logs -f api_server ingestor_service
