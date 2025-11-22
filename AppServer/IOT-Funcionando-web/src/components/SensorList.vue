<script setup>
import { ref, computed, onMounted, watch } from 'vue'

const props = defineProps({
  sensors: { type: Array, default: () => [] },
  deviceId: { type: Number, default: 1 }
})
const emit = defineEmits(['open-setup', 'open-readings'])

const loading = ref(false)
const error = ref(null)
const deviceSettings = ref(null)

// Fetch device settings from ingestor API
const fetchDeviceSettings = async (deviceId) => {
  loading.value = true
  error.value = null
  try {
    const mqttDeviceId = `esp32_device_${deviceId}`
    const response = await fetch(`http://localhost:5000/${mqttDeviceId}/settings/sensors/get`)
    if (!response.ok) {
      throw new Error(`Failed to fetch settings: ${response.status} ${response.statusText}`)
    }
    const data = await response.json()
    deviceSettings.value = data
  } catch (err) {
    error.value = err.message
    console.error('Error fetching device settings:', err)
  } finally {
    loading.value = false
  }
}

// Parse sensors/actuators from device settings JSON
const parsedSensors = computed(() => {
  if (!deviceSettings.value) return []
  // Expected JSON structure (adapt to actual API response):
  // { sensors: [ { id, name, protocol, pins, settings }, ... ] }
  // If structure differs, modify parsing logic here
  return deviceSettings.value.sensors || deviceSettings.value || []
})

const sensorsByPin = computed(() => {
  return parsedSensors.value.slice().sort((a, b) => {
    const pinA = Array.isArray(a.pins) ? a.pins[0] : a.pins
    const pinB = Array.isArray(b.pins) ? b.pins[0] : b.pins
    return Number(pinA) - Number(pinB)
  })
})

const openSetup = (sensor) => emit('open-setup', sensor.id)
const openReadings = (sensor) => emit('open-readings', sensor.id)

// Auto-fetch on mount using device ID from props
onMounted(() => {
  fetchDeviceSettings(props.deviceId)
})

// Re-fetch when device changes
watch(() => props.deviceId, (newId) => {
  if (newId) fetchDeviceSettings(newId)
})
</script>

<template>
  <section>
    <h2>Configured Sensors & Actuators</h2>

    <div v-if="loading">
      <p>Loading device settings...</p>
    </div>

    <div v-else-if="error">
      <p style="color:#e53e3e">Error: {{ error }}</p>
      <p style="font-size:12px; color:#666">Make sure the ingestor is running on localhost:5000</p>
    </div>

    <div v-else-if="sensorsByPin.length">
      <ul>
        <li v-for="s in sensorsByPin" :key="s.id" style="margin-bottom:8px">
          <strong>{{ s.name || s.id }}</strong> — Pin {{ Array.isArray(s.pins) ? s.pins.join(', ') : s.pins }} — {{ s.protocol }}
          <div style="margin-top:4px">
            <button @click="openSetup(s)">Edit</button>
            <button @click="openReadings(s)" style="margin-left:8px">Readings</button>
          </div>
        </li>
      </ul>
    </div>

    <div v-else>
      <p>No sensors configured on this device.</p>
    </div>
  </section>
</template>

