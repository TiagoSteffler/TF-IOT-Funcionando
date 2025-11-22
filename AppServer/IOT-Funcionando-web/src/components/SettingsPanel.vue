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

// Clear InfluxDB data
const clearingInfluxDB = ref(false)

const clearInfluxDB = async () => {
  const confirmed = confirm(
    '⚠️ WARNING: This will DELETE ALL sensor data from InfluxDB!\n\n' +
    'This includes historical readings from all sensors and boards.\n' +
    'This action CANNOT be undone.\n\n' +
    'Are you absolutely sure you want to continue?'
  )
  
  if (!confirmed) {
    return
  }
  
  // Double confirmation
  const doubleConfirm = confirm(
    'FINAL CONFIRMATION\n\n' +
    'All sensor data will be permanently deleted.\n\n' +
    'Type YES in your mind and click OK to proceed, or Cancel to abort.'
  )
  
  if (!doubleConfirm) {
    return
  }
  
  clearingInfluxDB.value = true
  error.value = null
  successMessage.value = null
  
  try {
    const response = await fetch('/influxdb/clear', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      }
    })
    
    if (!response.ok) {
      throw new Error(`Failed to clear InfluxDB: ${response.status}`)
    }
    
    const result = await response.json()
    successMessage.value = '✅ InfluxDB cleared successfully! All sensor data has been deleted.'
    console.log('InfluxDB cleared:', result)
    
    setTimeout(() => {
      successMessage.value = null
    }, 5000)
    
  } catch (err) {
    error.value = err.message
    console.error('Error clearing InfluxDB:', err)
  } finally {
    clearingInfluxDB.value = false
  }
}

</script>

<template>
  <section>
    <h2>ESP32 Settings</h2>

    <!-- MQTT SETTINGS -->
    <div class="field-block">
      <label>Broker IP</label>
      <input v-model="mqttBroker" placeholder="e.g. 192.168.1.10 or mqtt.example.com" />
    </div>

    <div class="field-block">
      <label>Device ID</label>
      <input v-model="mqttDeviceId" placeholder="e.g. esp32_device_1" />
    </div>


    <div class="field-block">
      <label>WiFi Network (SSID)</label>
      <input v-model="wifiNetwork" placeholder="WiFi network name" />
    </div>

    <div class="field-block">
      <label>Password</label>
      <input v-model="wifiPassword" type="password" placeholder="WiFi password" />
    </div>

    <div class="button-row">
      <button @click="saveSettings">Save Settings</button>
    </div>

    <div class="danger-zone">
      <h3>Danger Zone</h3>

      <div class="button-row">
        <button class="danger" @click="eraseNetwork">Erase Network Settings</button>
        <button class="danger-strong" @click="eraseAll" :disabled="saving">
          {{ saving ? 'Resetting...' : 'Erase All Configuration' }}
        </button>
      </div>

      <div class="button-row" style="margin-top:16px">
        <button class="danger-strong" @click="clearInfluxDB" :disabled="clearingInfluxDB">
          {{ clearingInfluxDB ? 'Clearing Database...' : '🗑️ Clear All InfluxDB Data' }}
        </button>
      </div>
      <p style="font-size:12px; color:#ff7875; margin-top:8px">
        ⚠️ This will permanently delete ALL sensor readings from InfluxDB. Use this to clean up after changing sensor configurations.
      </p>
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
