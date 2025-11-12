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
    const pinA = a.pin
    const pinB = b.pin
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
      <ul style="list-style:none; padding:0">
        <li v-for="s in sensorsByPin" :key="s.id" style="margin-bottom:12px; padding:12px; background:#f7fafc; border-radius:8px; border-left:4px solid #3182ce">
          <div style="display:flex; justify-content:space-between; align-items:center">
            <div>
              <strong style="font-size:16px">{{ s.type || 'Unknown Sensor' }}</strong>
              <div style="font-size:14px; color:#718096; margin-top:4px">
                <span>📍 Pin {{ s.pin }}</span>
                <span style="margin-left:16px">🔌 {{ s.id }}</span>
                <span style="margin-left:16px" :style="{ color: s.enabled ? '#48bb78' : '#cbd5e0' }">
                  {{ s.enabled ? '✅ Enabled' : '⏸️ Disabled' }}
                </span>
                <span style="margin-left:16px">⏱️ {{ s.sampling_interval }}ms</span>
              </div>
            </div>
            <div style="display:flex; gap:8px">
              <button @click="openSetup(s)" style="padding:6px 12px; border-radius:4px; border:none; background:#4299e1; color:white; cursor:pointer; font-weight:bold">
                ✏️ Edit
              </button>
              <button @click="openReadings(s)" style="padding:6px 12px; border-radius:4px; border:none; background:#48bb78; color:white; cursor:pointer; font-weight:bold">
                📊 Readings
              </button>
            </div>
          </div>
        </li>
      </ul>
    </div>

    <div v-else>
      <p>No sensors configured on this device.</p>
    </div>
  </section>
</template>

