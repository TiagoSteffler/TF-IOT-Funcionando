<script setup>
import { ref, onMounted, watch } from 'vue'
import { queryPin } from '../services/influxClient'
import { Chart, registerables } from 'chart.js'
Chart.register(...registerables)

const props = defineProps({ pin: { type: [String, Number], required: true } })
const emit = defineEmits(['close'])

const loading = ref(false)
const error = ref(null)
const raw = ref(null)
const chartRef = ref(null)
let chartInstance = null

const load = async () => {
  loading.value = true
  error.value = null
  raw.value = null
  try {
    const res = await queryPin(String(props.pin), { range: '-1h' })
    raw.value = res
    // If JSON and contains tables, try to parse points for chart
    const points = []
    try {
      if (res && res.results) {
        // Influx JSON shape: results[].tables[].records[] (best-effort parse)
        res.results.forEach(r => {
          if (r.tables) {
            r.tables.forEach(t => {
              if (t.records) {
                t.records.forEach(rec => {
                  const time = rec._time || rec.values && rec.values._time
                  const val = rec._value || (rec.values && rec.values._value)
                  if (time && typeof val !== 'undefined') points.push({ time, value: Number(val) })
                })
              }
            })
          }
        })
      }
    } catch (e) {
      // ignore parse errors
    }

    if (points.length && chartRef.value) {
      const ctx = chartRef.value.getContext('2d')
      if (chartInstance) chartInstance.destroy()
      chartInstance = new Chart(ctx, {
        type: 'line',
        data: {
          labels: points.map(p => new Date(p.time).toLocaleTimeString()),
          datasets: [{ label: `Pin ${props.pin}`, data: points.map(p => p.value), borderColor: '#3182ce', tension: 0.2 }]
        },
        options: { responsive: true, maintainAspectRatio: false }
      })
    }
  } catch (e) {
    error.value = e.message
  } finally {
    loading.value = false
  }
}

onMounted(load)
watch(() => props.pin, load)
</script>

<template>
  <div class="sensor-modal">
    <div class="sensor-backdrop" @click="$emit('close')"></div>
    <div class="sensor-card">
      <div class="sensor-header">
        <h3>Sensor data — GPIO {{ pin }}</h3>
        <button class="close-btn" @click="$emit('close')">✕</button>
      </div>

      <div class="sensor-body">
        <div v-if="loading">Loading data from InfluxDB...</div>
        <div v-else-if="error" class="error">Error: {{ error }}</div>
        <div v-else>
          <div class="chart-area" v-if="raw && raw.results">
            <canvas ref="chartRef"></canvas>
          </div>

          <div class="raw-area">
            <h4>Raw response (for debug)</h4>
            <pre>{{ JSON.stringify(raw, null, 2) }}</pre>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.sensor-modal {
  position: fixed;
  inset: 0;
  z-index: 5000;
}
.sensor-backdrop {
  position: absolute;
  inset: 0;
  background: rgba(0,0,0,0.45);
}
.sensor-card {
  position: absolute;
  left: 50%;
  top: 50%;
  transform: translate(-50%,-50%);
  width: 90%;
  max-width: 900px;
  max-height: 85vh;
  background: white;
  border-radius: 12px;
  overflow: auto;
  box-shadow: 0 12px 40px rgba(0,0,0,0.3);
  padding: 16px;
}
.sensor-header {
  display:flex; align-items:center; justify-content:space-between; gap:12px; margin-bottom:8px
}
.close-btn { background:transparent; border:none; font-size:18px; cursor:pointer }
.sensor-body { padding: 8px }
.chart-area { height: 300px; }
.raw-area { margin-top: 12px; background:#f7fafc; padding:12px; border-radius:8px; max-height:300px; overflow:auto }
.error { color: #c53030 }
</style>
