<script setup>
import { ref, onMounted, watch, onUnmounted, nextTick } from 'vue'
import { Chart, registerables } from 'chart.js'

Chart.register(...registerables)

const props = defineProps({
  selectedSensor: { type: Object, default: null },
  sensors: { type: Array, default: () => [] },
  deviceId: { type: Number, default: 1 }
})

const loading = ref(false)
const error = ref(null)
const readingsData = ref(null)
const chartRef = ref(null)
const timeRange = ref('-5m') // ultimos 5 minutos padrao
const autoRefresh = ref(false)
const detailsOpen = ref(false)
let chartInstance = null
let refreshInterval = null

// Local sensor selection and available sensors
const localSelectedSensor = ref(null)
const availableSensors = ref([])

// Fetch available sensors for the current device
const fetchAvailableSensors = async () => {
  try {
    const mqttDeviceId = `esp32_device_${props.deviceId}`
    const response = await fetch(`http://localhost:5000/${mqttDeviceId}/settings/sensors/get`)
    if (response.ok) {
      const data = await response.json()
      availableSensors.value = data.sensors || []
    }
  } catch (err) {
    console.error('Error fetching sensors:', err)
  }
}

// busca leituras gravadas via api.py
const fetchReadings = async (deviceId, sensorId) => {
  console.log('Buscando leituras para:', { deviceId, sensorId })
  loading.value = true
  error.value = null
  
  try {
    const mqttDeviceId = `esp32_device_${deviceId}`
    const url = `/${mqttDeviceId}/sensors/sensor_${sensorId}/read?start=${timeRange.value}`
    console.log('Buscando de ', url)
    const response = await fetch(url)
    if (!response.ok) {
      throw new Error(`Falha ao buscar leituras: ${response.status} ${response.statusText}`)
    }
    
    const json = await response.json()
    readingsData.value = json
    
    console.log('Dados do sensor buscados:', json.length, 'dados')
  } catch (err) {
    error.value = err.message
    console.error('Erro ao buscar leituras do sensor:', err)
  } finally {
    loading.value = false
  }
}

const refreshData = () => {
  const sensor = localSelectedSensor.value || props.selectedSensor
  if (sensor && props.deviceId) {
    fetchReadings(props.deviceId, sensor.id)
  }
}

const toggleAutoRefresh = () => {
  autoRefresh.value = !autoRefresh.value
  if (autoRefresh.value) {
    refreshData() // refresh
    if (refreshInterval) clearInterval(refreshInterval)
    refreshInterval = setInterval(refreshData, 5000) // atualiza a cada 5 segundos (pisca a tela n tem jeito)
  } else {
    if (refreshInterval) {
      clearInterval(refreshInterval)
      refreshInterval = null
    }
    
    // busca sempre valores do teclado
    if (props.selectedSensor && props.selectedSensor.tipo === 7) {
      refreshInterval = setInterval(refreshData, 5000)
    }
  }
}

const stringDataPoints = ref([])

const renderChart = (data) => {
  if (!chartRef.value) {
    console.error('Gráfico não encontrado')
    return
  }
  
  if (chartInstance) {
    console.log('Destruindo instância anterior do gráfico')
    chartInstance.destroy()
  }
  
  console.log('Renderizando gráfico com', data.length, 'pontos')
  
  const ctx = chartRef.value.getContext('2d')
  
  // extrari dados de dict (infelizmente nao terao todos o mesmo tempo pq o influx grava eles com milesimos de diferenca)
  // estrutura teórica: { "timestamp": { "x": 1127, "y": 1971, "button": 0 } }
  const timeGrouped = {}
  const fieldNames = new Set()
  const stringData = []
  
  data.forEach(row => {
    const timestamp = row.time
    const valueDict = row.value
    
    if (!timeGrouped[timestamp]) {
      timeGrouped[timestamp] = {}
    }
    
    // busca todos os campos dict
    if (typeof valueDict === 'object' && valueDict !== null) {
      Object.entries(valueDict).forEach(([field, value]) => {
        // Checagem para teclado(retorno string)
        if (typeof value === 'string') {
          stringData.push({
            time: timestamp,
            field: field,
            value: value
          })
        } else {
          // Valor numérico para gráfico
          const parsed = parseFloat(value)
          timeGrouped[timestamp][field] = isNaN(parsed) ? 0 : parsed
          fieldNames.add(field)
        }
      })
    } else {
      const parsed = parseFloat(valueDict)
      timeGrouped[timestamp]['value'] = isNaN(parsed) ? 0 : parsed
      fieldNames.add('value')
    }
  })
  
  // Armazena dados de string para exibição em tabela
  stringDataPoints.value = stringData.reverse() // ordem cronologica decrescente
  
  // Converte para array ordenado por timestamp
  const sortedTimestamps = Object.keys(timeGrouped).sort()
  
  // Cria labels a partir dos timestamps
  const labels = sortedTimestamps.map(timestamp => {
    const time = new Date(timestamp)
    return time.toLocaleTimeString('en-US', { hour: '2-digit', minute: '2-digit', second: '2-digit' })
  })
  
  // cores para cada tipo de medida
  const colors = [
    { border: '#3182ce', background: 'rgba(49, 130, 206, 0.1)' },   // Blue
    { border: '#38a169', background: 'rgba(56, 161, 105, 0.1)' },   // Green
    { border: '#e53e3e', background: 'rgba(229, 62, 62, 0.1)' },    // Red
    { border: '#dd6b20', background: 'rgba(221, 107, 32, 0.1)' },   // Orange
    { border: '#9f7aea', background: 'rgba(159, 122, 234, 0.1)' },  // Purple
    { border: '#ed64a6', background: 'rgba(237, 100, 166, 0.1)' },  // Pink
    { border: '#00b5ad', background: 'rgba(0, 181, 173, 0.1)' },    // Teal
    { border: '#d69e2e', background: 'rgba(214, 158, 46, 0.1)' }    // Yellow
  ]
  
  // pressupoe alguns tipos de medidas a usarem escala menor tipo botoes e cia.
  const binaryFields = new Set()
  fieldNames.forEach(fieldName => {
    const values = sortedTimestamps.map(ts => timeGrouped[ts][fieldName]).filter(v => v !== undefined)
    const min = Math.min(...values)
    const max = Math.max(...values)
    const range = max - min
    
    // Se o campo tem pequeno intervalo (0-1 ou similar) usa eixo secundario
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
      borderWidth: usesSecondaryAxis ? 3 : 2,  // danonão grosso
      pointRadius: usesSecondaryAxis ? 4 : 3,
      pointHoverRadius: 5,
      fill: true,
      tension: 0.3,
      spanGaps: true,
      yAxisID: usesSecondaryAxis ? 'y2' : 'y'  // Atribui ao eixo secundário se binário
    }
  })
  
  // Calcula min/max entre todos os datasets para escala do eixo y (quse certo que fica entre 0 e 4096 para ADC, 0-180 servo e 0-1 rele/botao)
  const allValues = datasets.flatMap(ds => ds.data.filter(v => v !== null))
  const minValue = Math.min(...allValues)
  const maxValue = Math.max(...allValues)
  const padding = (maxValue - minValue) * 0.1 || 1
  
  const measurementName = data[0]?.measurement || 'Leitura de sensor'
  
  console.log('Campos do gráfico:', Array.from(fieldNames).join(', '), 'timestamps:', labels.length, 'range:', minValue, '-', maxValue)
  
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
          grid: { display: false },  
          ticks: { 
            font: { size: 11 },
            stepSize: 1,
            callback: function(value) {
              return value === 0 || value === 1 ? value : ''  // binario
            }
          },
          title: {
            display: binaryFields.size > 0,
            text: 'Estado binário',
            font: { size: 12, weight: 'bold' }
          }
        }
      },
      animation: {
        duration: autoRefresh.value ? 500 : 1000
      }
    }
  })
  
  console.log('Gráfico renderizado com sucesso')
}

// Auto-fetch ao aparecer se acessado a partir do proprio sensor em SensorList.vue
onMounted(async () => {
  console.log('SensorReadings montado com props:', { selectedSensor: props.selectedSensor, deviceId: props.deviceId })
  
  // Fetch available sensors if no sensor is selected
  if (!props.selectedSensor) {
    await fetchAvailableSensors()
  }
  
  if (props.selectedSensor) {
    localSelectedSensor.value = props.selectedSensor
    fetchReadings(props.deviceId, props.selectedSensor.id)
    
    // sempre atualiza entrada de teclado a cada 5 segundos
    if (props.selectedSensor.tipo === 7 && !refreshInterval) {
      refreshInterval = setInterval(refreshData, 5000)
    }
  } else {
    console.warn('selectedSensor nulo')
  }
})

// Re-fetch ao mudar sensorID ou deviceID
watch(() => [props.selectedSensor, props.deviceId], ([newSensor, newDeviceId]) => {
  console.log('Props modificados:', { newSensor, newDeviceId })
  
  // Update local selection if prop changes
  if (newSensor) {
    localSelectedSensor.value = newSensor
  }
  
  // desliga auto refresh
  if (refreshInterval && !autoRefresh.value) {
    clearInterval(refreshInterval)
    refreshInterval = null
  }
  
  if (newSensor && newDeviceId) {
    fetchReadings(newDeviceId, newSensor.id)
    
    // teclado
    if (newSensor.tipo === 7 && !refreshInterval && !autoRefresh.value) {
      refreshInterval = setInterval(refreshData, 5000)
    }
  }
})

// Watch local sensor selection changes
watch(localSelectedSensor, (newSensor) => {
  if (newSensor && props.deviceId) {
    // Clear existing intervals
    if (refreshInterval) {
      clearInterval(refreshInterval)
      refreshInterval = null
    }
    
    fetchReadings(props.deviceId, newSensor.id)
    
    // Auto-refresh for keypad sensors
    if (newSensor.tipo === 7) {
      refreshInterval = setInterval(refreshData, 5000)
    }
  }
})

// Re-fetch quando acabar o tempo de refresh
watch(timeRange, () => {
  refreshData()
})

// atualiza grafico/tabela assim que receber resposta da api
watch(readingsData, async (newData) => {
  if (newData && newData.length) {
    // teclado
    const stringData = []
    newData.forEach(row => {
      const valueDict = row.value
      if (typeof valueDict === 'object' && valueDict !== null) {
        Object.entries(valueDict).forEach(([field, value]) => {
          // teclado
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
    stringDataPoints.value = stringData.reverse() // Mais recentes primeiro
    
    // Espera o Vue renderizar o elemento canvas (já que está em v-else)
    await nextTick()
    // Pequeno atraso adicional pra garantir
    await new Promise(resolve => setTimeout(resolve, 100))
    
    if (chartRef.value) {
      console.log('recarregando gráfico...')
      renderChart(newData)
    } else {
      console.log('Dados de string')
    }
  }
})

// Cleanup no unmount
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
  <section class="sensor-section">
    
    <!-- HEADER -->
    <div class="sensor-header">
      <h2 class="sensor-title">
        Leituras de sensores/atuadores
        <span v-if="localSelectedSensor || selectedSensor">
          - {{ (localSelectedSensor || selectedSensor).desc || `Sensor ${(localSelectedSensor || selectedSensor).id}` }}
          (ID: {{ (localSelectedSensor || selectedSensor).id }})
        </span>
      </h2>

      <!-- CONTROLES -->
      <div style="display:flex; gap:8px; align-items:center">
        <select class="time-select" v-model="timeRange">
          <option value="-1m">Último minuto</option>
          <option value="-5m">Últimos 5 minutos</option>
          <option value="-15m">Últimos 15 minutos</option>
          <option value="-1h">Última 1 hora</option>
          <option value="-6h">Últimas 6 horas</option>
          <option value="-24h">Últimas 24 horas</option>
        </select>

        <button
          @click="toggleAutoRefresh"
          class="btn"
          :class="autoRefresh ? 'btn-auto-on' : 'btn-auto-off'"
        >
          {{ autoRefresh ? '🔄 Auto-refresh ON' : '⏸️ Auto-refresh OFF' }}
        </button>

        <button
          @click="refreshData"
          class="btn btn-refresh"
          :disabled="loading"
        >
          {{ loading ? 'Carregando...' : 'Refresh' }}
        </button>
      </div>
    </div>


    <!-- ESTADOS: LOADING / ERROR -->
    <div v-if="loading">
      <p>Carregando leituras...</p>
    </div>

    <div v-else-if="error">
      <p style="color:#e53e3e">Erro: {{ error }}</p>
      <p style="font-size:12px; color:#666">Certifique-se de que a API está rodando em localhost:5000.</p>
    </div>


    <!-- DADOS DISPONÍVEIS -->
    <div v-else-if="readingsData && readingsData.length">

      <!-- RESUMO -->
      <div class="summary-box">
        <div class="summary-header">
          <span><strong>Pontos de dados:</strong> {{ readingsData.length }}</span>
          <span><strong>Hora:</strong> {{ new Date(readingsData[readingsData.length - 1]?.time).toLocaleTimeString() }}</span>

          <div style="display:flex; gap:16px; flex-wrap:wrap">
            <span
              v-for="(val, key) in readingsData[readingsData.length - 1]?.value"
              :key="key"
            >
              <strong>{{ key }}:</strong> {{ val }}
            </span>
          </div>
        </div>
      </div>


      <!-- STRING DATA (TECLADO) -->
      <div v-if="stringDataPoints.length > 0" class="string-table-wrapper">
        <h3 class="string-table-title">Entradas do teclado:</h3>

        <table class="string-table">
          <thead>
            <tr>
              <th>Timestamp</th>
              <th>Campo</th>
              <th>Valor</th>
            </tr>
          </thead>

          <tbody>
            <tr v-for="(item, index) in stringDataPoints" :key="index">
              <td>{{ new Date(item.time).toLocaleString() }}</td>
              <td style="font-weight:600">{{ item.field }}</td>
              <td>{{ item.value }}</td>
            </tr>
          </tbody>
        </table>
      </div>


      <!-- GRÁFICO -->
      <div
        v-else
        class="chart-box"
      >
        <canvas ref="chartRef"></canvas>
      </div>


      <!-- RAW JSON -->
      <details class="raw-details" :open="detailsOpen" @toggle="detailsOpen = $event.target.open">
        <summary>Dados JSON raw: ({{ readingsData.length }} linhas)</summary>

        <pre class="raw-json-box">
{{ JSON.stringify(readingsData, null, 2) }}
        </pre>
      </details>

    </div>


    <!-- NENHUM SENSOR SELECIONADO -->
    <div v-else>
      <div v-if="!props.selectedSensor && !localSelectedSensor" class="sensor-select-container">
        <p class="no-sensor-text">Nenhum sensor selecionado. Escolha um sensor para visualizar os dados:</p>

        <div v-if="availableSensors.length > 0" style="max-width:400px; margin:0 auto">
          <select
            v-model="localSelectedSensor"
            class="sensor-select"
          >
            <option :value="null" disabled>-- Selecione um sensor --</option>

            <option
              v-for="sensor in availableSensors"
              :key="sensor.id"
              :value="sensor"
            >
              {{ sensor.desc || `Sensor ${sensor.id}` }} (ID: {{ sensor.id }}, Tipo: {{ sensor.tipo }})
            </option>
          </select>
        </div>

        <p v-else class="no-sensor-text">
          Nenhum sensor encontrado para este dispositivo.
        </p>
      </div>

      <p v-else>Nenhuma leitura disponível para o sensor selecionado.</p>
    </div>

  </section>
</template>


<style scoped>
/* ============================================================
   SEÇÃO PRINCIPAL (mesmo vidro fosco do style original)
   ============================================================ */
.sensor-section {
  display: block;
  padding: 20px;

  background: rgba(255, 255, 255, 0.08);
  backdrop-filter: blur(6px);
  border: 1px solid rgba(255, 255, 255, 0.15);
  border-radius: 18px;

  color: white;
  font-family: Arial, sans-serif;

  box-shadow: 0 4px 18px rgba(0,0,0,0.25);
}


/* ============================================================
   TÍTULO + BARRA DE CONTROLES (seguindo o estilo dos h2/h3)
   ============================================================ */
.sensor-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 12px;
  margin-bottom: 16px;
}

.sensor-title {
  margin: 0;
  font-size: 1.4rem;
  text-shadow: 0 2px 4px rgba(0,0,0,0.5);
  color: white;
}

.sensor-title span {
  font-size: 0.9em;
  font-weight: normal;
  color: rgba(255,255,255,0.7);
}


/* ============================================================
   SELECTOR DE TEMPO (convertido para estilo escuro fosco)
   ============================================================ */
.time-select {
  padding: 10px 14px;
  border-radius: 12px;

  background: rgba(0, 0, 0, 0.35);
  border: 1px solid rgba(255,255,255,0.25);
  color: white;

  font-size: 0.95rem;
  cursor: pointer;
  box-shadow: inset 0 0 8px rgba(0,0,0,0.25);
  transition: 0.2s;
}

.time-select:focus {
  outline: none;
  border: 1px solid rgba(255,255,255,0.45);
  background: rgba(0, 0, 0, 0.45);
}


/* ============================================================
   BOTÕES (convertidos para o mesmo padrão do botão principal)
   ============================================================ */
.btn {
  width: auto;
  height: 45px;

  padding: 8px 16px;
  border: none;
  border-radius: 30px;

  background-color: rgba(0, 0, 0, 0.5);
  color: white;

  font-size: 15px;
  font-weight: 600;
  cursor: pointer;
  white-space: nowrap;

  transition: 0.15s;
  box-shadow: 0 3px 10px rgba(0,0,0,0.25);
}

.btn:hover:not(:disabled) {
  transform: translateY(-2px);
  box-shadow: 0 5px 14px rgba(0,0,0,0.35);
}

/* auto-refresh ON (verde suave fosco) */
.btn-auto-on {
  background: rgba(80, 200, 120, 0.5);
}

.btn-auto-on:hover {
  background: rgba(80, 200, 120, 0.7);
}

/* auto-refresh OFF (cinza neutro fosco) */
.btn-auto-off {
  background: rgba(200, 200, 200, 0.35);
  color: white;
}

.btn-auto-off:hover {
  background: rgba(200, 200, 200, 0.55);
}

/* refresh (vermelho estilo danger) */
.btn-refresh {
  background: rgba(255, 60, 60, 0.5);
}

.btn-refresh:hover:not(:disabled) {
  background: rgba(255, 60, 60, 0.7);
}

.btn-refresh:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}


/* ============================================================
   PAINEL DE RESUMO
   ============================================================ */
.summary-box {
  background: rgba(0,0,0,0.35);
  border: 1px solid rgba(255,255,255,0.15);

  padding: 18px;
  border-radius: 14px;
  margin-bottom: 20px;

  box-shadow: 0 2px 10px rgba(0,0,0,0.25);
  color: white;
}

.summary-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 12px;
}


/* ============================================================
   TABELA (STRING VALUES)
   ============================================================ */
.string-table-wrapper {
  background: rgba(0,0,0,0.35);
  border: 1px solid rgba(255,255,255,0.15);

  padding: 20px;
  border-radius: 14px;
  margin-bottom: 20px;

  box-shadow: 0 2px 10px rgba(0,0,0,0.25);
  color: white;
}

.string-table-title {
  margin: 0 0 12px;
  font-size: 1.1rem;
  font-weight: bold;
  text-shadow: 0 1px 4px rgba(0,0,0,0.4);
}

.string-table {
  width: 100%;
  border-collapse: collapse;
}

.string-table thead tr {
  background: rgba(255,255,255,0.08);
}

.string-table th {
  padding: 10px;
  font-weight: bold;
  color: white;
  text-shadow: 0 1px 3px rgba(0,0,0,0.4);
}

.string-table td {
  padding: 10px;
  color: rgba(255,255,255,0.85);
  border-bottom: 1px solid rgba(255,255,255,0.15);
  font-size: 13px;
}

.string-table td:last-child {
  font-family: monospace;
  font-weight: bold;
  background: rgba(255,255,255,0.05);
  color: white;
  border-radius: 6px;
}


/* ============================================================
   ÁREA DO GRÁFICO
   ============================================================ */
.chart-box {
  position: relative;
  height: 400px;

  background: rgba(0,0,0,0.35);
  border: 1px solid rgba(255,255,255,0.15);
  padding: 20px;
  border-radius: 14px;
  margin-bottom: 20px;

  box-shadow: 0 2px 10px rgba(0,0,0,0.25);
}


/* ============================================================
   RAW JSON
   ============================================================ */
.raw-details summary {
  cursor: pointer;
  padding: 10px;

  background: rgba(0,0,0,0.35);
  border: 1px solid rgba(255,255,255,0.2);
  border-radius: 10px;

  font-weight: bold;
  color: white;
  text-shadow: 0 1px 3px rgba(0,0,0,0.3);

  box-shadow: 0 2px 8px rgba(0,0,0,0.25);
}

.raw-json-box {
  max-height: 300px;
  overflow: auto;

  background: rgba(0,0,0,0.35);
  border: 1px solid rgba(255,255,255,0.2);
  padding: 14px;
  border-radius: 10px;

  color: white;
  font-size: 11px;
  margin-top: 10px;
  white-space: pre-wrap;
}


/* ============================================================
   SELETOR DE SENSOR
   ============================================================ */
.sensor-select-container {
  background: rgba(0,0,0,0.35);
  border: 1px solid rgba(255,255,255,0.15);

  padding: 24px;
  text-align: center;
  border-radius: 14px;

  box-shadow: 0 2px 10px rgba(0,0,0,0.25);
  color: white;
}

.sensor-select {
  width: 100%;
  padding: 12px;

  border-radius: 12px;
  background: rgba(0,0,0,0.35);
  border: 1px solid rgba(255,255,255,0.25);
  color: white;

  font-size: 0.95rem;

  box-shadow: inset 0 0 8px rgba(0,0,0,0.25);
}

.sensor-select:focus {
  outline: none;
  border-color: rgba(255,255,255,0.5);
}

.no-sensor-text {
  color: rgba(255,255,255,0.75);
  font-size: 14px;
  margin-top: 16px;
}
</style>
