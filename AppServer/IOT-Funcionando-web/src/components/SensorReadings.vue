<script setup>
import { ref, onMounted, watch } from 'vue'
import { Chart, registerables } from 'chart.js'

Chart.register(...registerables)

const props = defineProps({
  selectedPin: { type: Object, default: null },
  sensors: { type: Array, default: () => [] },
  deviceId: { type: Number, default: 1 }
})

const loading = ref(false)
const error = ref(null)
const readingsData = ref(null)
const chartRef = ref(null)
let chartInstance = null

// Fetch sensor readings from ingestor API (JSON response)
const fetchReadings = async (deviceId, sensorId) => {
  loading.value = true
  error.value = null
  readingsData.value = null
  
  try {
    const mqttDeviceId = `esp32_device_${deviceId}`
    const response = await fetch(`http://localhost:5000/${mqttDeviceId}/sensors/${sensorId}/read`)
    if (!response.ok) {
      throw new Error(`Failed to fetch readings: ${response.status} ${response.statusText}`)
    }
    
    const json = await response.json()
    readingsData.value = json
    
    // Render chart if we have data
    if (json && json.length && chartRef.value) {
      renderChart(json)
    }
  } catch (err) {
    error.value = err.message
    console.error('Error fetching sensor readings:', err)
  } finally {
    loading.value = false
  }
}

const renderChart = (data) => {
  if (chartInstance) chartInstance.destroy()
  
  const ctx = chartRef.value.getContext('2d')
  
  // Extract time and value from JSON structure
  const labels = data.map(row => {
    const time = new Date(row.time)
    return time.toLocaleString('en-US', { month: 'short', day: 'numeric', hour: '2-digit', minute: '2-digit' })
  })
  const values = data.map(row => parseFloat(row.value) || 0)
  
  // Use measurement name for chart label if available
  const measurementName = data[0]?.measurement || 'Sensor Reading'
  
  chartInstance = new Chart(ctx, {
    type: 'line',
    data: {
      labels,
      datasets: [{
        label: measurementName,
        data: values,
        borderColor: '#3182ce',
        backgroundColor: 'rgba(49, 130, 206, 0.1)',
        tension: 0.2
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      plugins: {
        legend: { display: true, position: 'top' }
      },
      scales: {
        y: {
          beginAtZero: false
        }
      }
    }
  })
}

// Auto-fetch on mount if selectedPin is available
onMounted(() => {
  if (props.selectedPin) {
    const sensorId = `sensor_pin_${props.selectedPin.number}`
    fetchReadings(props.deviceId, sensorId)
  }
})

// Re-fetch if selectedPin or deviceId changes
watch(() => [props.selectedPin, props.deviceId], ([newPin, newDeviceId]) => {
  if (newPin && newDeviceId) {
    const sensorId = `sensor_pin_${newPin.number}`
    fetchReadings(newDeviceId, sensorId)
  }
})
</script>

<template>
  <section>
    <h2>Sensor Readings</h2>

    <div v-if="loading">
      <p>Loading sensor readings...</p>
    </div>

    <div v-else-if="error">
      <p style="color:#e53e3e">Error: {{ error }}</p>
      <p style="font-size:12px; color:#666">Make sure the ingestor is running on localhost:5000</p>
    </div>

    <div v-else-if="readingsData && readingsData.length">
      <div style="height:300px; margin-bottom:16px">
        <canvas ref="chartRef"></canvas>
      </div>

      <details>
        <summary>Raw JSON Data ({{ readingsData.length }} rows)</summary>
        <pre style="max-height:200px; overflow:auto; background:#f7fafc; padding:8px; font-size:11px">{{ JSON.stringify(readingsData, null, 2) }}</pre>
      </details>
    </div>

    <div v-else>
      <p>No readings available. Select a sensor to view data.</p>
    </div>
  </section>
</template>

