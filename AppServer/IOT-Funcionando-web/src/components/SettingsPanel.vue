<script setup>
import { ref, onMounted, watch } from 'vue'

const props = defineProps({
  deviceId: { type: Number, default: 1 }
})

const emit = defineEmits(['save-settings', 'erase-network', 'erase-all'])

// MQTT settings
const mqttBroker = ref('')
const mqttDeviceId = ref('')

// Network settings
const wifiNetwork = ref('')
const wifiPassword = ref('')

// Loading states
const loading = ref(false)
const saving = ref(false)
const error = ref(null)
const successMessage = ref(null)

// Fetch current WiFi settings from API
const fetchWifiSettings = async (deviceId) => {
  loading.value = true
  error.value = null
  try {
    const mqttId = `esp32_device_${deviceId}`
    const response = await fetch(`http://localhost:5000/${mqttId}/settings/wifi/get`)
    if (!response.ok) {
      throw new Error(`Failed to fetch WiFi settings: ${response.status}`)
    }
    const data = await response.json()
    
    // Note: The API sends MQTT request, response comes via MQTT topic
    // For now, we'll show the request was sent
    console.log('WiFi settings request sent:', data)
  } catch (err) {
    error.value = err.message
    console.error('Error fetching WiFi settings:', err)
  } finally {
    loading.value = false
  }
}

const saveSettings = async () => {
  saving.value = true
  error.value = null
  successMessage.value = null
  
  try {
    const mqttId = `esp32_device_${props.deviceId}`
    
    // Prepare WiFi configuration payload
    const wifiPayload = {
      ssid: wifiNetwork.value,
      password: wifiPassword.value,
      mqtt_broker: mqttBroker.value,
      mqtt_device_id: mqttDeviceId.value
    }
    
    // Send WiFi configuration via API
    const response = await fetch(`http://localhost:5000/${mqttId}/settings/wifi/set`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify(wifiPayload)
    })
    
    if (!response.ok) {
      throw new Error(`Failed to save settings: ${response.status}`)
    }
    
    const result = await response.json()
    successMessage.value = 'Settings sent to device successfully!'
    console.log('Settings saved:', result)
    
    // Emit to parent for any additional handling
    emit('save-settings', {
      mqtt: {
        broker: mqttBroker.value,
        deviceId: mqttDeviceId.value
      },
      network: {
        ssid: wifiNetwork.value,
        password: wifiPassword.value
      }
    })
    
    // Clear success message after 3 seconds
    setTimeout(() => {
      successMessage.value = null
    }, 3000)
    
  } catch (err) {
    error.value = err.message
    console.error('Error saving settings:', err)
  } finally {
    saving.value = false
  }
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

// Initialize device ID when component mounts or device changes
onMounted(() => {
  mqttDeviceId.value = `esp32_device_${props.deviceId}`
  // Optionally fetch current settings
  // fetchWifiSettings(props.deviceId)
})

watch(() => props.deviceId, (newId) => {
  mqttDeviceId.value = `esp32_device_${newId}`
  // Optionally fetch current settings
  // fetchWifiSettings(newId)
})
</script>

<template>
  <section>
    <h2>ESP32 Settings</h2>

    <!-- Loading indicator -->
    <div v-if="loading" style="margin-top:12px; color:#666">
      <p>Loading settings...</p>
    </div>

    <!-- Success message -->
    <div v-if="successMessage" style="margin-top:12px; padding:12px; background:#f6ffed; border-left:4px solid #52c41a; color:#52c41a">
      {{ successMessage }}
    </div>

    <!-- Error message -->
    <div v-if="error" style="margin-top:12px; padding:12px; background:#fff2f0; border-left:4px solid #ff4d4f; color:#ff4d4f">
      Error: {{ error }}
    </div>

    <div style="margin-top:12px">
      <h3>MQTT Settings</h3>
      <div style="margin-top:8px">
        <label>Broker IP</label>
        <input v-model="mqttBroker" placeholder="e.g. 192.168.1.10 or mqtt.example.com" :disabled="saving" />
      </div>
      <div style="margin-top:8px">
        <label>Device ID</label>
        <input v-model="mqttDeviceId" placeholder="e.g. esp32_device_1" :disabled="saving" />
      </div>
    </div>

    <div style="margin-top:20px">
      <h3>Network Settings</h3>
      <div style="margin-top:8px">
        <label>WiFi Network (SSID)</label>
        <input v-model="wifiNetwork" placeholder="WiFi network name" :disabled="saving" />
      </div>
      <div style="margin-top:8px">
        <label>Password</label>
        <input v-model="wifiPassword" type="password" placeholder="WiFi password" :disabled="saving" />
      </div>
    </div>

    <div style="margin-top:20px">
      <button @click="saveSettings" :disabled="saving">
        {{ saving ? 'Saving...' : 'Save Settings' }}
      </button>
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

