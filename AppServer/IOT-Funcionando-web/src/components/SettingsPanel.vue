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

const eraseAll = async () => {
  if (!confirm('Erase ALL configuration (network + sensors)? This will reset the ESP32 and cannot be undone.')) {
    return
  }
  
  saving.value = true
  error.value = null
  successMessage.value = null
  
  try {
    const mqttId = `esp32_device_${props.deviceId}`
    
    // Send reset command via API
    const response = await fetch(`http://localhost:5000/${mqttId}/settings/device/reset`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      }
    })
    
    if (!response.ok) {
      throw new Error(`Failed to reset device: ${response.status}`)
    }
    
    const result = await response.json()
    successMessage.value = 'Reset command sent! Device configuration cleared.'
    console.log('Device reset:', result)
    
    // Clear local form data
    mqttBroker.value = ''
    mqttDeviceId.value = `esp32_device_${props.deviceId}`
    wifiNetwork.value = ''
    wifiPassword.value = ''
    
    // Emit to parent for additional handling
    emit('erase-all')
    
    // Clear success message after 5 seconds
    setTimeout(() => {
      successMessage.value = null
    }, 5000)
    
  } catch (err) {
    error.value = err.message
    console.error('Error resetting device:', err)
  } finally {
    saving.value = false
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


    <div class="field-block">
      <label>WiFi Network (SSID)</label>
      <input v-model="wifiNetwork" placeholder="WiFi network name" :disabled="saving" />
    </div>

    <div class="field-block">
      <label>Password</label>
      <input v-model="wifiPassword" type="password" placeholder="WiFi password" :disabled="saving" />
    </div>

    <div class="button-row">
      <button @click="saveSettings" :disabled="saving">
        {{ saving ? 'Saving...' : 'Save Settings' }}
      </button>
    </div>

    <div class="danger-zone">
      <h3>Danger Zone</h3>

      <div class="button-row">
        <button class="danger" @click="eraseNetwork">Erase Network Settings</button>
        <button class="danger-strong" @click="eraseAll" :disabled="saving">
          {{ saving ? 'Resetting...' : 'Erase All Configuration' }}
        </button>
      </div>
    </div>
  </section>
</template>

<style scoped>
section {
  background: rgba(255, 255, 255, 0.08);
  backdrop-filter: blur(6px);
  border: 1px solid rgba(255, 255, 255, 0.15);

  padding: 20px;
  border-radius: 18px;
  width: 100%;
  box-sizing: border-box;

  color: white;
  font-family: Arial, sans-serif;

  box-shadow: 0 4px 18px rgba(0,0,0,0.25);
}

/* título */
h2 {
  margin-bottom: 16px;
  font-size: 1.4rem;
  text-shadow: 0 2px 4px rgba(0,0,0,0.5);
}

h3 {
  margin-top: 18px;
  margin-bottom: 6px;
  font-size: 1.15rem;
  text-shadow: 0 1px 4px rgba(0,0,0,0.4);
}

/* separador */
.divider {
  margin: 10px 0;
  border-top: 1px solid rgba(255,255,255,0.15);
}

/* blocos uniformes */
.field-block {
  width: 100%;
  box-sizing: border-box;
  margin-top: 8px;
  padding-left: 20px;
  padding-right: 20px;
  border-radius: 10px;
}

/* labels */
label {
  font-size: 0.95rem;
  font-weight: 600;
  display: block;
  margin-bottom: 2px;
  text-shadow: 0 1px 6px rgba(0,0,0,0.5);
}

/* inputs */
.field-block input {
  width: 97%;
  padding: 10px 12px;
  border-radius: 12px;

  background: rgba(0, 0, 0, 0.35);
  border: 1px solid rgba(255, 255, 255, 0.25);
  color: white;
  font-size: 0.95rem;

  box-shadow: inset 0 0 10px rgba(0,0,0,0.3);
  transition: 0.2s;
}

.field-block input:focus {
  outline: none;
  border-color: rgba(255,255,255,0.5);
  background: rgba(0, 0, 0, 0.45);
}

/* botão padrão */
button {
  width: 200px;
  height: 55px;
  padding: 8px 14px;

  border: none;
  border-radius: 30px;

  background-color: rgba(0, 0, 0, 0.5);

  font-size: 16px;
  font-weight: 600;
  color: white;

  cursor: pointer;
  transition: 0.15s;
  white-space: nowrap;

  box-shadow: 0 3px 10px rgba(0,0,0,0.25);
}

button:hover:not(:disabled) {
  transform: translateY(-2px);
  box-shadow: 0 5px 14px rgba(0,0,0,0.35);
}

.button-row {
  margin-top: 25px;
  display: flex;
  gap: 12px;
  flex-wrap: wrap;
}

button.danger {
  background: rgba(255, 80, 80, 0.5);
}

button.danger-strong {
  background: rgba(255, 40, 40, 0.7);
}

button.danger:hover {
  background: rgba(255, 80, 80, 0.7);
}

button.danger-strong:hover {
  background: rgba(255, 40, 40, 0.85);
}
</style>
