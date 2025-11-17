<script setup>
import { ref, onMounted, watch, onUnmounted, nextTick } from 'vue'
import { Chart, registerables } from 'chart.js'

Chart.register(...registerables)

const props = defineProps({
  selectedSensor: { type: Object, default: null }, // Changed from selectedPin to selectedSensor
  sensors: { type: Array, default: () => [] },
  deviceId: { type: Number, default: 1 }
})

const loading = ref(false)
const error = ref(null)
const readingsData = ref(null)
const chartRef = ref(null)
const timeRange = ref('-5m') // Default: last 5 minutes
const autoRefresh = ref(false)
let chartInstance = null
let refreshInterval = null

// Fetch sensor readings from API server (JSON response)
const fetchReadings = async (deviceId, sensorId) => {
  console.log('🔍 fetchReadings called:', { deviceId, sensorId })
  loading.value = true
  error.value = null
  
  try {
    const mqttDeviceId = `esp32_device_${deviceId}`
    const url = `/${mqttDeviceId}/sensors/${sensorId}/read?start=${timeRange.value}`
    console.log('📡 Fetching from:', url)
    const response = await fetch(url)
    if (!response.ok) {
      throw new Error(`Failed to fetch readings: ${response.status} ${response.statusText}`)
    }
    
    const json = await response.json()
    readingsData.value = json
    
    console.log('Fetched sensor data:', json.length, 'data points')
  } catch (err) {
    error.value = err.message
    console.error('Error fetching sensor readings:', err)
  } finally {
    loading.value = false
  }
}

const refreshData = () => {
  if (props.selectedSensor && props.deviceId) {
    fetchReadings(props.deviceId, props.selectedSensor.id)
  }
}

const toggleAutoRefresh = () => {
  autoRefresh.value = !autoRefresh.value
  if (autoRefresh.value) {
    refreshData() // Immediate refresh
    refreshInterval = setInterval(refreshData, 5000) // Refresh every 5 seconds
  } else {
    if (refreshInterval) {
      clearInterval(refreshInterval)
      refreshInterval = null
    }
  }
}

const renderChart = (data) => {
  if (!chartRef.value) {
    console.error('Canvas element not found!')
    return
  }
  
  if (chartInstance) {
    console.log('Destroying previous chart instance')
    chartInstance.destroy()
  }
  
  console.log('Rendering chart with', data.length, 'data points')
  
  const ctx = chartRef.value.getContext('2d')
  
  // Extract time and value from JSON structure
  const labels = data.map(row => {
    const time = new Date(row.time)
    return time.toLocaleTimeString('en-US', { hour: '2-digit', minute: '2-digit', second: '2-digit' })
  })
  const values = data.map(row => parseFloat(row.value) || 0)
  
  // Use measurement name for chart label if available
  const measurementName = data[0]?.measurement || 'Sensor Reading'
  
  // Calculate min/max for better y-axis scaling
  const minValue = Math.min(...values)
  const maxValue = Math.max(...values)
  const padding = (maxValue - minValue) * 0.1 || 1
  
  console.log('Chart data - labels:', labels.length, 'values:', values.length, 'range:', minValue, '-', maxValue)
  
  chartInstance = new Chart(ctx, {
    type: 'line',
    data: {
      labels,
      datasets: [{
        label: measurementName,
        data: values,
        borderColor: '#3182ce',
        backgroundColor: 'rgba(49, 130, 206, 0.1)',
        borderWidth: 2,
        pointRadius: 3,
        pointHoverRadius: 5,
        fill: true,
        tension: 0.3
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      interaction: {
        intersect: false,
        mode: 'index'
      },
      plugins: {
        legend: { 
          display: true, 
          position: 'top',
          labels: {
            font: { size: 14, weight: 'bold' }
          }
        },
        tooltip: {
          backgroundColor: 'rgba(0, 0, 0, 0.8)',
          titleFont: { size: 14 },
          bodyFont: { size: 13 },
          padding: 12
        }
      },
      scales: {
        x: {
          grid: { display: false },
          ticks: { 
            maxTicksLimit: 10,
            font: { size: 11 }
          }
        },
        y: {
          beginAtZero: false,
          min: minValue - padding,
          max: maxValue + padding,
          grid: { color: 'rgba(0, 0, 0, 0.05)' },
          ticks: { font: { size: 11 } }
        }
      },
      animation: {
        duration: autoRefresh.value ? 500 : 1000
      }
    }
  })
  
  console.log('Chart instance created successfully')
}

// Auto-fetch on mount if selectedSensor is available
onMounted(() => {
  console.log('📍 SensorReadings mounted with props:', { selectedSensor: props.selectedSensor, deviceId: props.deviceId })
  if (props.selectedSensor) {
    fetchReadings(props.deviceId, props.selectedSensor.id)
  } else {
    console.warn('⚠️ selectedSensor is null/undefined on mount')
  }
})

// Re-fetch if selectedSensor or deviceId changes
watch(() => [props.selectedSensor, props.deviceId], ([newSensor, newDeviceId]) => {
  console.log('🔄 Props changed:', { newSensor, newDeviceId })
  if (newSensor && newDeviceId) {
    fetchReadings(newDeviceId, newSensor.id)
  }
})

// Re-fetch when time range changes
watch(timeRange, () => {
  refreshData()
})

// Watch for readingsData changes and render chart
watch(readingsData, async (newData) => {
  if (newData && newData.length) {
    // Wait for Vue to render the canvas element (since it's in v-else-if)
    await nextTick()
    // Additional small delay to ensure canvas is fully painted
    await new Promise(resolve => setTimeout(resolve, 100))
    
    if (chartRef.value) {
      console.log('Rendering chart after data update')
      renderChart(newData)
    } else {
      console.error('Chart canvas ref still not available - canvas not in DOM')
    }
  }
})

// Cleanup on unmount
onUnmounted(() => {
  if (refreshInterval) {
    clearInterval(refreshInterval)
  }
  if (chartInstance) {
    chartInstance.destroy()
  }
})
</script>

<template>
  <section>
    <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:16px">
      <h2 style="margin:0">
        Sensor Readings
        <span v-if="selectedSensor" style="font-size:0.9em; color:#718096; font-weight:normal">
          - {{ selectedSensor.desc || `Sensor ${selectedSensor.id}` }} (ID: {{ selectedSensor.id }})
        </span>
      </h2>
      
      <div style="display:flex; gap:8px; align-items:center">
        <select v-model="timeRange" style="padding:6px 12px; border-radius:4px; border:1px solid #cbd5e0">
          <option value="-1m">Last 1 minute</option>
          <option value="-5m">Last 5 minutes</option>
          <option value="-15m">Last 15 minutes</option>
          <option value="-1h">Last 1 hour</option>
          <option value="-6h">Last 6 hours</option>
          <option value="-24h">Last 24 hours</option>
        </select>
        
        <button 
          @click="toggleAutoRefresh" 
          :style="{
            padding: '6px 12px',
            borderRadius: '4px',
            border: 'none',
            backgroundColor: autoRefresh ? '#48bb78' : '#cbd5e0',
            color: autoRefresh ? 'white' : '#2d3748',
            fontWeight: 'bold',
            cursor: 'pointer'
          }"
        >
          {{ autoRefresh ? '🔄 Auto-refresh ON' : '⏸️ Auto-refresh OFF' }}
        </button>
        
        <button 
          @click="refreshData" 
          :disabled="loading"
          style="padding:6px 12px; border-radius:4px; border:none; background-color:#3182ce; color:white; font-weight:bold; cursor:pointer"
        >
          {{ loading ? '⏳ Loading...' : '🔄 Refresh' }}
        </button>
      </div>
    </div>

    <div v-if="loading">
      <p>Loading sensor readings...</p>
    </div>

    <div v-else-if="error">
      <p style="color:#e53e3e">Error: {{ error }}</p>
      <p style="font-size:12px; color:#666">Make sure the ingestor is running on localhost:5000</p>
    </div>

    <div v-else-if="readingsData && readingsData.length">
      <div style="background:#f7fafc; padding:16px; border-radius:8px; margin-bottom:16px">
        <div style="display:flex; justify-content:space-between; margin-bottom:8px">
          <span><strong>Data Points:</strong> {{ readingsData.length }}</span>
          <span><strong>Latest Value:</strong> {{ readingsData[readingsData.length - 1]?.value }}</span>
          <span><strong>Time:</strong> {{ new Date(readingsData[readingsData.length - 1]?.time).toLocaleTimeString() }}</span>
        </div>
      </div>
      
      <div style="position:relative; height:400px; margin-bottom:16px; background:white; padding:16px; border-radius:8px; box-shadow:0 1px 3px rgba(0,0,0,0.1)">
        <canvas ref="chartRef" style="width:100%; height:100%"></canvas>
      </div>

      <details style="margin-top:16px">
        <summary style="cursor:pointer; padding:8px; background:#edf2f7; border-radius:4px; font-weight:bold">
          📊 Raw JSON Data ({{ readingsData.length }} rows)
        </summary>
        <pre style="max-height:300px; overflow:auto; background:#f7fafc; padding:12px; font-size:11px; border-radius:4px; margin-top:8px; border:1px solid #e2e8f0">{{ JSON.stringify(readingsData, null, 2) }}</pre>
      </details>
    </div>

    <div v-else>
      <p>No readings available. Select a sensor to view data.</p>
    </div>
  </section>
</template>

