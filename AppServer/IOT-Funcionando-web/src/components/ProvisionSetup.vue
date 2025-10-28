<script setup>
import { ref } from 'vue'

const emit = defineEmits(['close'])

const ssid = ref('')
const password = ref('')
const hostname = ref('esp32-device')

const mqttServer = ref('')
const mqttPort = ref(1883)
const mqttClientId = ref('esp32_device')
const mqttBaseTopic = ref('devices/esp32')

const status = ref('')
const busy = ref(false)

const connectToDevice = async () => {
  // No-op helper for UI; we cannot switch networks from the browser
}

const sendConfig = async () => {
  busy.value = true
  status.value = ''

  try {
    // First send WiFi config
    const wifiPayload = {
      ssid: ssid.value,
      password: password.value,
      hostname: hostname.value
    }

    // Try POST to common ESP32 AP address
    const base = 'http://192.168.4.1'

    // Send WiFi
    const wifiResp = await fetch(base + '/api/config/wifi', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(wifiPayload)
    })

    if (!wifiResp.ok) {
      const txt = await wifiResp.text().catch(() => '')
      throw new Error('WiFi config failed: ' + (txt || wifiResp.status))
    }

    // Send MQTT config
    const mqttPayload = {
      enabled: !!mqttServer.value,
      server: mqttServer.value,
      port: Number(mqttPort.value) || 1883,
      clientId: mqttClientId.value,
      baseTopic: mqttBaseTopic.value
    }

    const mqttResp = await fetch(base + '/api/config/mqtt', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(mqttPayload)
    })

    if (!mqttResp.ok) {
      const txt = await mqttResp.text().catch(() => '')
      throw new Error('MQTT config failed: ' + (txt || mqttResp.status))
    }

    status.value = 'Configuration sent. Device should restart and attempt to connect.'
  } catch (err) {
    status.value = err.message || String(err)
  } finally {
    busy.value = false
  }
}
</script>

<template>
  <div class="provision-overlay">
    <div class="provision-modal">
      <header>
        <h3>ESP32 Quick Setup</h3>
        <button class="close" @click="emit('close')">✕</button>
      </header>

      <section class="note">
        <p>Please connect your host to the ESP32's AP (SSID: <strong>ESP32-Setup-XXXX</strong>) before sending settings.</p>
        <p>If your host must remain online, use ethernet or a second Wi-Fi adapter — the browser cannot change networks programmatically.</p>
      </section>

      <form @submit.prevent="sendConfig">
        <h4>Wi-Fi</h4>
        <label>SSID</label>
        <input v-model="ssid" placeholder="Wi-Fi SSID" />

        <label>Password</label>
        <input v-model="password" type="password" placeholder="Wi-Fi password" />

        <label>Hostname</label>
        <input v-model="hostname" placeholder="esp32-device" />

        <h4>MQTT (optional)</h4>
        <label>Server</label>
        <input v-model="mqttServer" placeholder="broker.local or 192.168.x.x" />

        <div class="row">
          <div>
            <label>Port</label>
            <input v-model="mqttPort" type="number" />
          </div>
          <div>
            <label>Client ID</label>
            <input v-model="mqttClientId" />
          </div>
        </div>

        <label>Base Topic</label>
        <input v-model="mqttBaseTopic" />

        <div class="actions">
          <button type="submit" class="menu-button" :disabled="busy">Send to device</button>
          <button type="button" class="menu-button" @click="emit('close')">Cancel</button>
        </div>
      </form>

      <footer class="status">
        <div v-if="busy">Sending configuration…</div>
        <div v-else-if="status">{{ status }}</div>
      </footer>
    </div>
  </div>
</template>

<style scoped>
.provision-overlay {
  position: fixed;
  left: 0;
  top: 0;
  right: 0;
  bottom: 0;
  background: rgba(0,0,0,0.45);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 9999;
}
.provision-modal {
  width: 720px;
  max-width: 95%;
  background: white;
  border-radius: 10px;
  padding: 18px;
  box-shadow: 0 8px 40px rgba(0,0,0,0.2);
}
.provision-modal header {
  display:flex; align-items:center; justify-content:space-between; margin-bottom:10px;
}
.provision-modal .note { font-size:13px; color:#555; margin-bottom:12px }
.provision-modal label { display:block; font-weight:600; margin-top:8px }
.provision-modal input { width:100%; padding:8px 10px; margin-top:6px; border-radius:6px; border:1px solid #ddd }
.provision-modal .row { display:flex; gap:12px }
.provision-modal .row > div { flex:1 }
.provision-modal .actions { display:flex; gap:10px; margin-top:12px }
.provision-modal .close { background:transparent; border:none; font-size:18px; cursor:pointer }
.provision-modal footer.status { margin-top:10px; color:#333 }

.menu-button {
  padding: 6px 10px;
  background: #4a5568;
  color: white;
  border: none;
  border-radius: 4px;
  cursor: pointer;
  font-size: 12px;
  font-weight: 600;
  transition: all 0.2s;
  min-width: 45px;
  text-align: center;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.2);
}
</style>
