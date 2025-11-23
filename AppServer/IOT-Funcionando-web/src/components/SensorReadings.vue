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
const detailsOpen = ref(false)
let chartInstance = null
let refreshInterval = null

// Fetch sensor readings from API server (JSON response)
const fetchReadings = async (deviceId, sensorId) => {
  console.log('🔍 fetchReadings called:', { deviceId, sensorId })
  loading.value = true
  error.value = null
  
  try {
    const mqttDeviceId = `esp32_device_${deviceId}`
    const url = `/${mqttDeviceId}/sensors/sensor_${sensorId}/read?start=${timeRange.value}`
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
    if (refreshInterval) clearInterval(refreshInterval)
    refreshInterval = setInterval(refreshData, 5000) // Refresh every 5 seconds
  } else {
    if (refreshInterval) {
      clearInterval(refreshInterval)
      refreshInterval = null
    }
    
    // Re-enable keypad auto-refresh if viewing keypad sensor
    if (props.selectedSensor && props.selectedSensor.tipo === 7) {
      console.log('🔐 Re-enabling keypad auto-refresh')
      refreshInterval = setInterval(refreshData, 3000)
    }
  }
}

const stringDataPoints = ref([])

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
  
  // Group data by timestamp and extract all fields
  // Structure: { "timestamp": { "x": 1127, "y": 1971, "button": 0 } }
  const timeGrouped = {}
  const fieldNames = new Set()
  const stringData = []
  
  data.forEach(row => {
    const timestamp = row.time
    const valueDict = row.value
    
    if (!timeGrouped[timestamp]) {
      timeGrouped[timestamp] = {}
    }
    
    // Merge all fields from this value dict
    if (typeof valueDict === 'object' && valueDict !== null) {
      Object.entries(valueDict).forEach(([field, value]) => {
        // Check if value is a string type (for keypad input like "123ABC" or "BABACA")
        if (typeof value === 'string') {
          stringData.push({
            time: timestamp,
            field: field,
            value: value
          })
        } else {
          // Numeric value for chart
          const parsed = parseFloat(value)
          timeGrouped[timestamp][field] = isNaN(parsed) ? 0 : parsed
          fieldNames.add(field)
        }
      })
    } else {
      // Fallback for single numeric values (shouldn't happen with new API)
      const parsed = parseFloat(valueDict)
      timeGrouped[timestamp]['value'] = isNaN(parsed) ? 0 : parsed
      fieldNames.add('value')
    }
  })
  
  // Store string data for display in table
  stringDataPoints.value = stringData.reverse() // Most recent first
  
  // Convert to sorted array by timestamp
  const sortedTimestamps = Object.keys(timeGrouped).sort()
  
  // Create labels from timestamps
  const labels = sortedTimestamps.map(timestamp => {
    const time = new Date(timestamp)
    return time.toLocaleTimeString('en-US', { hour: '2-digit', minute: '2-digit', second: '2-digit' })
  })
  
  // Create datasets for each field
  const colors = [
    { border: '#3182ce', background: 'rgba(49, 130, 206, 0.1)' },   // Blue
    { border: '#38a169', background: 'rgba(56, 161, 105, 0.1)' },   // Green
    { border: '#e53e3e', background: 'rgba(229, 62, 62, 0.1)' },    // Red
    { border: '#dd6b20', background: 'rgba(221, 107, 32, 0.1)' },   // Orange
    { border: '#9f7aea', background: 'rgba(159, 122, 234, 0.1)' },  // Purple
    { border: '#ed64a6', background: 'rgba(237, 100, 166, 0.1)' }   // Pink
  ]
  
  // Detect binary/small-range fields (button, state, bt, etc.) for secondary axis
  const binaryFields = new Set()
  fieldNames.forEach(fieldName => {
    const values = sortedTimestamps.map(ts => timeGrouped[ts][fieldName]).filter(v => v !== undefined)
    const min = Math.min(...values)
    const max = Math.max(...values)
    const range = max - min
    
    // If field has small range (0-1 or similar) or name suggests binary, use secondary axis
    if (range <= 1 || fieldName.toLowerCase().match(/button|btn|bt|state|pressed|click/)) {
      binaryFields.add(fieldName)
    }
  })
  
  const datasets = Array.from(fieldNames).map((fieldName, index) => {
    const colorIndex = index % colors.length
    const color = colors[colorIndex]
    const usesSecondaryAxis = binaryFields.has(fieldName)
    
    return {
      label: fieldName,
      data: sortedTimestamps.map(timestamp => {
        const value = timeGrouped[timestamp][fieldName]
        return value !== undefined ? value : null
      }),
      borderColor: color.border,
      backgroundColor: color.background,
      borderWidth: usesSecondaryAxis ? 3 : 2,  // Thicker lines for binary fields
      pointRadius: usesSecondaryAxis ? 4 : 3,
      pointHoverRadius: 5,
      fill: true,
      tension: 0.3,
      spanGaps: true,
      yAxisID: usesSecondaryAxis ? 'y2' : 'y'  // Assign to secondary axis if binary
    }
  })
  
  // Calculate min/max across all datasets for y-axis scaling
  const allValues = datasets.flatMap(ds => ds.data.filter(v => v !== null))
  const minValue = Math.min(...allValues)
  const maxValue = Math.max(...allValues)
  const padding = (maxValue - minValue) * 0.1 || 1
  
  const measurementName = data[0]?.measurement || 'Sensor Reading'
  
  console.log('Chart data - fields:', Array.from(fieldNames).join(', '), 'timestamps:', labels.length, 'range:', minValue, '-', maxValue)
  
  chartInstance = new Chart(ctx, {
    type: 'line',
    data: {
      labels,
      datasets
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      interaction: {
        intersect: false,
        mode: 'index'
      },
      plugins: {
        title: {
          display: true,
          text: measurementName,
          font: { size: 16, weight: 'bold' }
        },
        legend: { 
          display: true, 
          position: 'top',
          labels: {
            font: { size: 14, weight: 'bold' },
            usePointStyle: true,
            padding: 15
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
          type: 'linear',
          position: 'left',
          beginAtZero: false,
          min: minValue - padding,
          max: maxValue + padding,
          grid: { color: 'rgba(0, 0, 0, 0.05)' },
          ticks: { font: { size: 11 } },
          title: {
            display: Array.from(fieldNames).some(f => !binaryFields.has(f)),
            text: 'Value',
            font: { size: 12, weight: 'bold' }
          }
        },
        y2: {
          type: 'linear',
          position: 'right',
          beginAtZero: true,
          min: -0.2,
          max: 1.2,
          grid: { display: false },  // Don't overlap grid lines
          ticks: { 
            font: { size: 11 },
            stepSize: 1,
            callback: function(value) {
              return value === 0 || value === 1 ? value : ''  // Only show 0 and 1
            }
          },
          title: {
            display: binaryFields.size > 0,
            text: 'Binary State',
            font: { size: 12, weight: 'bold' }
          }
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
    
    // For keypad sensors (type 7), always auto-refresh every 3 seconds
    if (props.selectedSensor.tipo === 7 && !refreshInterval) {
      console.log('🔐 Keypad sensor detected - enabling auto-refresh')
      refreshInterval = setInterval(refreshData, 3000)
    }
  } else {
    console.warn('⚠️ selectedSensor is null/undefined on mount')
  }
})

// Re-fetch if selectedSensor or deviceId changes
watch(() => [props.selectedSensor, props.deviceId], ([newSensor, newDeviceId]) => {
  console.log('🔄 Props changed:', { newSensor, newDeviceId })
  
  // Clear existing auto-refresh interval
  if (refreshInterval && !autoRefresh.value) {
    clearInterval(refreshInterval)
    refreshInterval = null
  }
  
  if (newSensor && newDeviceId) {
    fetchReadings(newDeviceId, newSensor.id)
    
    // For keypad sensors (type 7), always auto-refresh every 3 seconds
    if (newSensor.tipo === 7 && !refreshInterval && !autoRefresh.value) {
      console.log('🔐 Keypad sensor detected - enabling auto-refresh')
      refreshInterval = setInterval(refreshData, 3000)
    }
  }
})

// Re-fetch when time range changes
watch(timeRange, () => {
  refreshData()
})

// Watch for readingsData changes and render chart
watch(readingsData, async (newData) => {
  if (newData && newData.length) {
    // Extract string data immediately for table display
    const stringData = []
    newData.forEach(row => {
      const valueDict = row.value
      if (typeof valueDict === 'object' && valueDict !== null) {
        Object.entries(valueDict).forEach(([field, value]) => {
          // Check if value is a string type (for keypad input)
          if (typeof value === 'string') {
            stringData.push({
              time: row.time,
              field: field,
              value: value
            })
          }
        })
      }
    })
    stringDataPoints.value = stringData.reverse() // Most recent first
    
    // Wait for Vue to render the canvas element (since it's in v-else)
    await nextTick()
    // Additional small delay to ensure canvas is fully painted
    await new Promise(resolve => setTimeout(resolve, 100))
    
    if (chartRef.value) {
      console.log('Rendering chart after data update')
      renderChart(newData)
    } else {
      console.log('No chart needed - string data only')
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
        <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:8px; flex-wrap:wrap; gap:12px; color:#2d3748">
          <span><strong>Data Points:</strong> {{ readingsData.length }}</span>
          <span><strong>Time:</strong> {{ new Date(readingsData[readingsData.length - 1]?.time).toLocaleTimeString() }}</span>
          <div style="display:flex; gap:16px; flex-wrap:wrap">
            <span v-for="(val, key) in readingsData[readingsData.length - 1]?.value" :key="key">
              <strong>{{ key }}:</strong> {{ val }}
            </span>
          </div>
        </div>
      </div>
      
      <div v-if="stringDataPoints.length > 0" style="margin-bottom:16px; background:white; padding:16px; border-radius:8px; box-shadow:0 1px 3px rgba(0,0,0,0.1)">
        <h3 style="margin-top:0; margin-bottom:12px; color:#2d3748; font-size:16px">🔐 String Data (Keypad Inputs)</h3>
        <table style="width:100%; border-collapse:collapse">
          <thead>
            <tr style="background:#edf2f7; border-bottom:2px solid #cbd5e0">
              <th style="padding:10px; text-align:left; color:#2d3748; font-weight:bold">Timestamp</th>
              <th style="padding:10px; text-align:left; color:#2d3748; font-weight:bold">Field</th>
              <th style="padding:10px; text-align:left; color:#2d3748; font-weight:bold">Value</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="(item, index) in stringDataPoints" :key="index" style="border-bottom:1px solid #e2e8f0">
              <td style="padding:10px; color:#4a5568; font-size:13px">
                {{ new Date(item.time).toLocaleString() }}
              </td>
              <td style="padding:10px; color:#4a5568; font-weight:600">
                {{ item.field }}
              </td>
              <td style="padding:10px; font-family:monospace; background:#f7fafc; color:#2d3748; font-weight:bold; font-size:14px">
                {{ item.value }}
              </td>
            </tr>
          </tbody>
        </table>
      </div>
      
      <div v-else style="position:relative; height:400px; margin-bottom:16px; background:white; padding:16px; border-radius:8px; box-shadow:0 1px 3px rgba(0,0,0,0.1)">
        <canvas ref="chartRef" style="width:100%; height:100%"></canvas>
      </div>

      <details :open="detailsOpen" @toggle="detailsOpen = $event.target.open" style="margin-top:16px">
        <summary style="cursor:pointer; padding:8px; background:#edf2f7; border-radius:4px; font-weight:bold; color:#2d3748">
          📊 Raw JSON Data ({{ readingsData.length }} rows)
        </summary>
        <pre style="max-height:300px; overflow:auto; background:#f7fafc; padding:12px; font-size:11px; border-radius:4px; margin-top:8px; border:1px solid #e2e8f0; color:#2d3748">{{ JSON.stringify(readingsData, null, 2) }}</pre>
      </details>
    </div>

    <div v-else>
      <p>No readings available. Select a sensor to view data.</p>
    </div>
  </section>
</template>

