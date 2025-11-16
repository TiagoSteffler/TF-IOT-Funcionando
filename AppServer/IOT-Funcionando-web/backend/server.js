const express = require('express');
const mqtt = require('mqtt');
const cors = require('cors');
const { InfluxDB, Point } = require('@influxdata/influxdb-client');
require('dotenv').config();

const app = express();
app.use(cors());
app.use(express.json());

// ============================================
// Configuração InfluxDB
// ============================================
const influxDB = new InfluxDB({
    url: process.env.INFLUX_URL || 'http://localhost:8086',
    token: process.env.INFLUX_TOKEN || 'your-admin-token'
});

const org = process.env.INFLUX_ORG || 'esp32-org';
const bucket = process.env.INFLUX_BUCKET || 'esp32-devices';
const writeApi = influxDB.getWriteApi(org, bucket);
const queryApi = influxDB.getQueryApi(org);

// ============================================
// Configuração MQTT
// ============================================
const mqttClient = mqtt.connect(process.env.MQTT_BROKER || 'mqtt://localhost:1883', {
    clientId: process.env.MQTT_CLIENT_ID || 'esp32-manager'
});

// Estado dos dispositivos (cache em memória)
const devices = new Map(); // mac -> { id, ip, lastSeen, status }

// Controle de pareamento
let pairingMode = false;
let pairingConfig = null;

mqttClient.on('connect', () => {
    console.log('✓ Conectado ao MQTT broker');
    mqttClient.subscribe('device/+/heartbeat', (err) => {
        if (err) {
            console.error('❌ Erro ao subscrever em heartbeat:', err);
        } else {
            console.log('✓ Subscrito em device/+/heartbeat');
        }
    });
});

mqttClient.on('message', (topic, message) => {
    try {
        const payload = JSON.parse(message.toString());
        if (topic.includes('/heartbeat')) {
            const { mac, ip, id, timestamp } = payload;
            const now = Date.now();
            const existingDevice = devices.get(mac);
            const wasInPairing = existingDevice && existingDevice.status === 'pairing';

            devices.set(mac, {
                id,
                ip,
                mac,
                lastSeen: now,
                status: 'online'
            });

            const point = new Point('device_heartbeat')
                .tag('mac', mac)
                .tag('device_id', id)
                .stringField('ip', ip)
                .stringField('status', 'online')
                .timestamp(new Date());

            writeApi.writePoint(point);

            if (wasInPairing) {
                console.log(`[HEARTBEAT] ${id} (${mac}) - ${ip} ✅ PRIMEIRO HEARTBEAT APÓS PAREAMENTO`);
            } else {
                console.log(`[HEARTBEAT] ${id} (${mac}) - ${ip}`);
            }
        }
    } catch (err) {
        console.error('Erro ao processar mensagem MQTT:', err);
    }
});

// Verifica dispositivos offline
setInterval(() => {
    const now = Date.now();
    const OFFLINE_THRESHOLD = 70000;
    devices.forEach((device, mac) => {
        if (now - device.lastSeen > OFFLINE_THRESHOLD && device.status === 'online') {
            device.status = 'offline';
            console.log(`[OFFLINE] ${device.id} (${mac})`);
            const point = new Point('device_heartbeat')
                .tag('mac', mac)
                .tag('device_id', device.id)
                .stringField('ip', device.ip)
                .stringField('status', 'offline')
                .timestamp(new Date());
            writeApi.writePoint(point);
        }
    });
}, 10000);

// API
app.get('/', (req, res) => {
    res.send('ESP32 Manager - Backend Running');
});

app.get('/ping', (req, res) => {
    const mac = req.query.mac;
    const clientIP = req.socket.remoteAddress.replace('::ffff:', '');
    console.log(`\n[PING] MAC: ${mac} | IP: ${clientIP}`);

    if (!pairingMode) {
        console.log('  → Modo pareamento DESATIVADO. Retornando OK.');
        return res.send('OK');
    }

    if (!pairingConfig) {
        console.log('  → Configuração de pareamento não definida.');
        return res.status(500).send('Pairing config not set');
    }

    console.log('  → Modo pareamento ATIVADO. Enviando configurações...');

        const response = {
        mac: mac,
        ssid: pairingConfig.ssid,
        password: pairingConfig.password,
        broker: pairingConfig.broker,
        port: pairingConfig.port,
        id: pairingConfig.id || `esp32_${mac.replace(/:/g, '').slice(-6)}`
    };

    console.log('  → Resposta:', JSON.stringify(response));

    const now = Date.now();
    devices.set(mac, {
        id: response.id,
        ip: clientIP,
        mac: mac,
        lastSeen: now,
        status: 'pairing'
    });

    const pairingPoint = new Point('device_paired')
        .tag('mac', mac)
        .tag('device_id', response.id)
        .stringField('ssid', response.ssid)
        .stringField('broker', response.broker)
        .stringField('client_ip', clientIP)
        .timestamp(new Date());

    writeApi.writePoint(pairingPoint);

    const heartbeatPoint = new Point('device_heartbeat')
        .tag('mac', mac)
        .tag('device_id', response.id)
        .stringField('ip', clientIP)
        .stringField('status', 'pairing')
        .timestamp(new Date());

    writeApi.writePoint(heartbeatPoint);

    res.json(response);
});

app.get('/api/network/interfaces', (req, res) => {
    const os = require('os');
    const { exec } = require('child_process');
    const networkInterfaces = os.networkInterfaces();
    const interfaces = [];
    if (process.env.HOST_IP) {
        interfaces.push({
            address: process.env.HOST_IP,
            name: 'Host (Environment)',
            priority: 0,
            recommended: true
        });
        console.log(`[IP DETECTION] Using HOST_IP from environment: ${process.env.HOST_IP}`);
        return res.json({ interfaces });
    }

    exec("ip route show default | awk '/default/ {print $3}'", (error, stdout) => {
        if (!error && stdout.trim()) {
            const gatewayIP = stdout.trim();
            interfaces.push({
                address: gatewayIP,
                name: 'Docker Host Gateway',
                priority: 0,
                recommended: true
            });
            console.log(`[IP DETECTION] Detected Docker gateway: ${gatewayIP}`);
        }

        Object.keys(networkInterfaces).forEach(interfaceName => {
            networkInterfaces[interfaceName].forEach(iface => {
                if (iface.family === 'IPv4' && !iface.internal) {
                    const addr = iface.address;
                    const isDockerBridge = addr.match(/^172\.(1[7-9]|2[0-9]|3[0-2])\./);
                    if (!isDockerBridge) {
                        const priority = addr.startsWith('192.168.') ? 1 : addr.startsWith('10.') ? 2 : 3;
                        interfaces.push({
                            address: addr,
                            name: interfaceName,
                            priority,
                            recommended: priority === 1
                        });
                    }
                }
            });
        });

        interfaces.sort((a, b) => a.priority - b.priority);
        console.log(`[IP DETECTION] Found ${interfaces.length} interfaces:`, interfaces.map(i => `${i.name}: ${i.address}`).join(', '));
        res.json({ interfaces });
    });
});

app.get('/api/devices', async (req, res) => {
    try {
        const devicesArray = Array.from(devices.values());
        res.json(devicesArray);
    } catch (err) {
        console.error('Erro ao listar dispositivos:', err);
        res.status(500).json({ error: err.message });
    }
});

app.get('/api/devices/history', async (req, res) => {
    try {
        const query = `
            from(bucket: "${bucket}")
            |> range(start: -24h)
            |> filter(fn: (r) => r._measurement == "device_heartbeat")
            |> last()
        `;
        const result = [];
        await queryApi.queryRows(query, {
            next(row, tableMeta) {
                const record = tableMeta.toObject(row);
                result.push(record);
            },
            error(error) {
                console.error('Erro na query InfluxDB:', error);
            },
            complete() {
                res.json(result);
            }
        });
    } catch (err) {
        console.error('Erro ao consultar histórico:', err);
        res.status(500).json({ error: err.message });
    }
});

app.post('/api/pairing/start', (req, res) => {
    const { ssid, password, port, id, brokerIP } = req.body;
    if (!ssid || !password || !port || !brokerIP) {
        return res.status(400).json({ error: 'Campos obrigatórios faltando (ssid, password, port, brokerIP)' });
    }
    pairingMode = true;
    pairingConfig = {
        ssid,
        password,
        broker: brokerIP,
        port: String(port),
        id
    };

    console.log('\n🔵 MODO PAREAMENTO ATIVADO');
    console.log('Config:', pairingConfig);
    console.log(`📡 Broker: ${brokerIP}:${port}`);

    res.json({ success: true, message: 'Modo pareamento ativado', broker: brokerIP });
});

app.post('/api/pairing/stop', (req, res) => {
    pairingMode = false;
    pairingConfig = null;
    console.log('\n🔴 MODO PAREAMENTO DESATIVADO');
    res.json({ success: true, message: 'Modo pareamento desativado' });
});

app.get('/api/pairing/status', (req, res) => {
    res.json({ active: pairingMode, config: pairingMode ? pairingConfig : null });
});

const PORT = process.env.PORT || 3001;

app.listen(PORT, '0.0.0.0', () => {
    console.log('\n' + '='.repeat(50));
    console.log('🚀 ESP32 MANAGER - BACKEND (IOT-Funcionando-web)');
    console.log('='.repeat(50));
    console.log(`Porta: ${PORT}`);
    console.log(`InfluxDB: ${process.env.INFLUX_URL}`);
    console.log(`MQTT: ${process.env.MQTT_BROKER}`);
    console.log('='.repeat(50));
    console.log('\n✓ Servidor iniciado\n');
});

process.on('SIGINT', async () => {
    console.log('\n\n🛑 Encerrando servidor...');
    try {
        await writeApi.close();
        console.log('✓ InfluxDB fechado');
    } catch (err) {
        console.error('Erro ao fechar InfluxDB:', err);
    }
    mqttClient.end();
    console.log('✓ MQTT desconectado');
    process.exit(0);
});
