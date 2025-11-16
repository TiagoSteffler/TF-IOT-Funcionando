<script setup>
import { ref, computed, watch } from 'vue'

const props = defineProps({
  selectedPin: { type: Object, default: null },
  sensors: { type: Array, default: () => [] },
  deviceId: { type: Number, default: 1 }
})
const emit = defineEmits(['save-sensor', 'delete-sensor'])

// helper to derive capabilities for demo parity with ESP32Preview
const getPinDetails = (pinNumber) => {
  const capabilities = []
  if (pinNumber % 2 === 0) capabilities.push('PWM')
  if (pinNumber % 3 === 0) capabilities.push('ADC')
  if (pinNumber % 5 === 0) capabilities.push('I2C')
  if (pinNumber % 7 === 0) capabilities.push('SPI')
  if (pinNumber % 11 === 0) capabilities.push('Touch')
  if (pinNumber === 1 || pinNumber === 2) capabilities.push('UART')
  const usable = !(pinNumber % 13 === 0)
  return { number: pinNumber, capabilities, usable }
}

// available pins 1..38
const allPins = Array.from({ length: 38 }, (_, i) => getPinDetails(i + 1))

// form state
const mode = ref('sensor') // 'sensor' or 'actuator'
const model = ref('')
const protocol = ref('')
const pin = ref(props.selectedPin ? props.selectedPin.number : null)
const sensorDeviceId = ref('')

// API state
const saving = ref(false)
const error = ref(null)
const successMessage = ref(null)

// if editing existing sensor for the current pin, prefill
const existing = computed(() => {
  if (!props.selectedPin) return null
  return props.sensors.find(s => Number(s.pin) === Number(props.selectedPin.number)) || null
})

watch(existing, (v) => {
  if (v) {
    mode.value = v.type || 'sensor'
    model.value = v.model || ''
    protocol.value = v.protocol || ''
    pin.value = v.pin
    sensorDeviceId.value = v.deviceId || ''
  } else {
    // reset defaults when no existing
    mode.value = 'sensor'
    model.value = ''
    protocol.value = ''
    pin.value = props.selectedPin ? props.selectedPin.number : null
    sensorDeviceId.value = props.selectedPin ? `sensor_pin_${props.selectedPin.number}` : ''
  }
}, { immediate: true })

watch(() => props.selectedPin, (newPin) => {
  if (newPin && !existing.value) {
    pin.value = newPin.number
    sensorDeviceId.value = `sensor_pin_${newPin.number}`
  }
})

const allowedPins = computed(() => {
  if (!protocol.value) return allPins.filter(p => p.usable)
  // match capability strings with protocol names mapping
  const map = { 'I2C': 'I2C', 'OneWire': 'ADC', 'UART': 'UART', 'PWM': 'PWM', 'ADC': 'ADC', 'SPI': 'SPI' }
  const cap = map[protocol.value] || protocol.value
  return allPins.filter(p => p.usable && p.capabilities.includes(cap))
})

const save = async () => {
  if (!pin.value) return
  
  saving.value = true
  error.value = null
  successMessage.value = null
  
  try {
    const mqttDeviceId = `esp32_device_${props.deviceId}`
    
    // Prepare sensor configuration payload
    const sensorConfig = {
      id: sensorDeviceId.value,
      pin: pin.value,
      type: mode.value,
      model: model.value,
      protocol: protocol.value,
      name: `${model.value || mode.value} on Pin ${pin.value}`
    }
    
    // Send sensor configuration via API
    const response = await fetch(`http://localhost:5000/${mqttDeviceId}/settings/sensors/set`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({
        sensors: [sensorConfig] // Send as array for multiple sensors support
      })
    })
    
    if (!response.ok) {
      throw new Error(`Failed to save sensor: ${response.status}`)
    }
    
    const result = await response.json()
    successMessage.value = 'Sensor configuration sent to device!'
    console.log('Sensor saved:', result)
    
    // Also emit to parent for local state management
    const sensor = {
      id: existing.value ? existing.value.id : undefined,
      pin: pin.value,
      type: mode.value,
      model: model.value,
      protocol: protocol.value,
      deviceId: sensorDeviceId.value
    }
    emit('save-sensor', sensor)
    
    // Clear success message after 2 seconds
    setTimeout(() => {
      successMessage.value = null
    }, 2000)
    
  } catch (err) {
    error.value = err.message
    console.error('Error saving sensor:', err)
  } finally {
    saving.value = false
  }
}

const remove = async () => {
  if (!existing.value) return
  
  saving.value = true
  error.value = null
  successMessage.value = null
  
  try {
    const mqttDeviceId = `esp32_device_${props.deviceId}`
    
    // Send remove request to API
    const response = await fetch(`http://localhost:5000/${mqttDeviceId}/sensors/remove`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({
        id: sensorDeviceId.value
      })
    })
    
    if (!response.ok) {
      throw new Error(`Failed to remove sensor: ${response.status}`)
    }
    
    const result = await response.json()
    successMessage.value = 'Sensor removed successfully!'
    console.log('Sensor removed:', result)
    
    // Also emit to parent for local state management
    emit('delete-sensor', existing.value.id)
    
    // Clear success message after 2 seconds
    setTimeout(() => {
      successMessage.value = null
    }, 2000)
    
  } catch (err) {
    error.value = err.message
    console.error('Error removing sensor:', err)
  } finally {
    saving.value = false
  }
}
</script>

<template>
  <section>
    <h2>Sensor / Actuator Setup</h2>

    <!-- Success message -->
    <div v-if="successMessage" style="margin-bottom:12px; padding:12px; background:#f6ffed; border-left:4px solid #52c41a; color:#52c41a">
      {{ successMessage }}
    </div>

    <!-- Error message -->
    <div v-if="error" style="margin-bottom:12px; padding:12px; background:#fff2f0; border-left:4px solid #ff4d4f; color:#ff4d4f">
      Error: {{ error }}
    </div>

    <div>
      <label>Device Type</label>
      <select v-model="mode" :disabled="saving">
        <option value="sensor">Sensor</option>
        <option value="actuator">Actuator</option>
      </select>
    </div>

    <div class="field-block">
      <label>Model</label>
      <select v-model="model" :disabled="saving">
        <option value="">-- select model (populate later) --</option>
      </select>
    </div>

    <div class="field-block">
      <label>Communication / Protocol</label>
      <select v-model="protocol" :disabled="saving">
        <option value="">-- automatic / none --</option>
        <option>I2C</option>
        <option>OneWire</option>
        <option>UART</option>
        <option>PWM</option>
        <option>ADC</option>
        <option>SPI</option>
      </select>
    </div>

    <div class="field-block">
      <label>Pin</label>
      <select v-model="pin" :disabled="saving">
        <option :value="null">-- select pin --</option>
        <option
          v-for="p in allowedPins"
          :key="p.number"
          :value="p.number"
        >
          GPIO {{ p.number }} - {{ p.capabilities.join(', ') }}
        </option>
      </select>
      <div class="info-text">
        Selected pin will be validated against the chosen protocol.
      </div>
    </div>

    <div style="margin-top:8px">
      <label>Sensor ID</label>
      <input v-model="sensorDeviceId" :disabled="saving" />
    </div>

    <div style="margin-top:12px">
      <button @click="save" :disabled="saving">
        {{ saving ? 'Saving...' : 'Save Settings' }}
      </button>
      <button v-if="existing" @click="remove" style="margin-left:8px" :disabled="saving">Delete</button>
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

/* labels */
label {
  font-size: 0.95rem;
  font-weight: 600;
  display: block;
  margin-bottom: 6px;
  text-shadow: 0 1px 6px rgba(0,0,0,0.5);
}

/* blocos uniformes */
.field-block {
  width: 100%;
  box-sizing: border-box;
  margin-top: 13px;
  padding-left: 20px;
  padding-right: 20px;
  border-radius: 10px;
}

/* selects e inputs alinhados perfeitamente */
.field-block select,
.field-block input {
  width: 100%;
  box-sizing: border-box;
  padding: 10px 12px;

  border-radius: 12px;
  background: rgba(0, 0, 0, 0.35);
  border: 1px solid rgba(255, 255, 255, 0.25);
  color: white;
  font-size: 0.95rem;

  box-shadow: inset 0 0 10px rgba(0,0,0,0.3);
  transition: 0.2s;
}

.field-block select:focus,
.field-block input:focus {
  outline: none;
  border-color: rgba(255,255,255,0.5);
  background: rgba(0, 0, 0, 0.45);
}

/* texto de info */
.info-text {
  font-size: 12px;
  color: #ccc;
  margin-top: 6px;
}

/* botões */
button {
  width: 180px;
  height: 55px;

  padding: 8px 14px;
  border: none;
  border-radius: 30px;
  background-color: rgba(0, 0, 0, 0.5);

  font-size: 16px;
  font-weight: 600;
  color: white;
  text-shadow: 1px 1px 10px rgba(0, 0, 0, 0.6);

  cursor: pointer;
  transition: 0.15s;
  white-space: nowrap;

  box-shadow: 0 3px 10px rgba(0,0,0,0.25);
}

button:hover:not(:disabled) {
  transform: translateY(-2px);
  box-shadow: 0 5px 14px rgba(0,0,0,0.35);
}

button:disabled {
  background: rgba(255, 255, 255, 0.15);
  cursor: not-allowed;
  box-shadow: none;
  opacity: 0.5;
}
</style>
