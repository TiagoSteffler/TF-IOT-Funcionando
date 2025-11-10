<script setup>
import { ref } from 'vue'

const emit = defineEmits(['save-settings', 'erase-network', 'erase-all'])

// MQTT settings
const mqttBroker = ref('')
const mqttDeviceId = ref('')

// Network settings
const wifiNetwork = ref('')
const wifiPassword = ref('')

const saveSettings = () => {
  const settings = {
    mqtt: {
      broker: mqttBroker.value,
      deviceId: mqttDeviceId.value
    },
    network: {
      ssid: wifiNetwork.value,
      password: wifiPassword.value
    }
  }
  emit('save-settings', settings)
}

const eraseNetwork = () => {
  if (confirm('Erase network configuration? The ESP32 will lose WiFi connection settings.')) {
    emit('erase-network')
  }
}

const eraseAll = () => {
  if (confirm('Erase ALL configuration (network + pins)? This cannot be undone.')) {
    emit('erase-all')
  }
}
</script>

<template>
  <section>
    <h2>ESP32 Settings</h2>

    <div style="margin-top:12px">
      <h3>MQTT Settings</h3>
      <div style="margin-top:8px">
        <label>Broker IP</label>
        <input v-model="mqttBroker" placeholder="e.g. 192.168.1.10 or mqtt.example.com" />
      </div>
      <div style="margin-top:8px">
        <label>Device ID</label>
        <input v-model="mqttDeviceId" placeholder="e.g. esp32_device_1" />
      </div>
    </div>

    <div style="margin-top:20px">
      <h3>Network Settings</h3>
      <div style="margin-top:8px">
        <label>WiFi Network (SSID)</label>
        <input v-model="wifiNetwork" placeholder="WiFi network name" />
      </div>
      <div style="margin-top:8px">
        <label>Password</label>
        <input v-model="wifiPassword" type="password" placeholder="WiFi password" />
      </div>
    </div>

    <div style="margin-top:20px">
      <button @click="saveSettings">Save Settings</button>
    </div>

    <div style="margin-top:20px; border-top:1px solid #ccc; padding-top:12px">
      <h3>Danger Zone</h3>
      <div style="margin-top:8px">
        <button @click="eraseNetwork" style="background:#e53e3e; color:white">Erase Network Settings</button>
        <button @click="eraseAll" style="margin-left:8px; background:#c53030; color:white">Erase All Configuration</button>
      </div>
    </div>
  </section>
</template>

