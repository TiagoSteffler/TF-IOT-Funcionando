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
  <section>
    <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:16px">
      <h2 style="margin:0">
       Leituras de sensores/atuadores
        <span v-if="localSelectedSensor || selectedSensor" style="font-size:0.9em; color:#718096; font-weight:normal">
          - {{ (localSelectedSensor || selectedSensor).desc || `Sensor ${(localSelectedSensor || selectedSensor).id}` }} (ID: {{ (localSelectedSensor || selectedSensor).id }})
        </span>
      </h2>
      
      <div style="display:flex; gap:8px; align-items:center">
        <select v-model="timeRange" style="padding:6px 12px; border-radius:4px; border:1px solid #cbd5e0">
          <option value="-1m">Último minuto</option>
          <option value="-5m">Últimos 5 minutos</option>
          <option value="-15m">Últimos 15 minutos</option>
          <option value="-1h">Última 1 hora</option>
          <option value="-6h">Últimas 6 horas</option>
          <option value="-24h">Últimas 24 horas</option>
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
          {{ loading ? 'Carregando...' : 'Refresh' }}
        </button>
      </div>
    </div>

    <div v-if="loading">
      <p>Carregando leituras...</p>
    </div>

    <div v-else-if="error">
      <p style="color:#e53e3e">Erro: {{ error }}</p>
      <p style="font-size:12px; color:#666">Certifique-se de que a API está rodando em localhost:5000.</p>
    </div>

    <div v-else-if="readingsData && readingsData.length">
      <div style="background:#f7fafc; padding:16px; border-radius:8px; margin-bottom:16px">
        <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:8px; flex-wrap:wrap; gap:12px; color:#2d3748">
          <span><strong>Pontos de dados:</strong> {{ readingsData.length }}</span>
          <span><strong>Hora:</strong> {{ new Date(readingsData[readingsData.length - 1]?.time).toLocaleTimeString() }}</span>
          <div style="display:flex; gap:16px; flex-wrap:wrap">
            <span v-for="(val, key) in readingsData[readingsData.length - 1]?.value" :key="key">
              <strong>{{ key }}:</strong> {{ val }}
            </span>
          </div>
        </div>
      </div>
      
      <div v-if="stringDataPoints.length > 0" style="margin-bottom:16px; background:white; padding:16px; border-radius:8px; box-shadow:0 1px 3px rgba(0,0,0,0.1)">
        <h3 style="margin-top:0; margin-bottom:12px; color:#2d3748; font-size:16px">Entradas do teclado:</h3>
        <table style="width:100%; border-collapse:collapse">
          <thead>
            <tr style="background:#edf2f7; border-bottom:2px solid #cbd5e0">
              <th style="padding:10px; text-align:left; color:#2d3748; font-weight:bold">Timestamp</th>
              <th style="padding:10px; text-align:left; color:#2d3748; font-weight:bold">Campo</th>
              <th style="padding:10px; text-align:left; color:#2d3748; font-weight:bold">Valor</th>
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
          Dados JSON raw: ({{ readingsData.length }} linhas)
        </summary>
        <pre style="max-height:300px; overflow:auto; background:#f7fafc; padding:12px; font-size:11px; border-radius:4px; margin-top:8px; border:1px solid #e2e8f0; color:#2d3748">{{ JSON.stringify(readingsData, null, 2) }}</pre>
      </details>
    </div>

    <div v-else>
      <div v-if="!props.selectedSensor && !localSelectedSensor" style="background:#f7fafc; padding:24px; border-radius:8px; text-align:center">
        <p style="margin-bottom:16px; color:#2d3748; font-size:16px">Nenhum sensor selecionado. Escolha um sensor para visualizar os dados:</p>
        
        <div v-if="availableSensors.length > 0" style="max-width:400px; margin:0 auto">
          <select 
            v-model="localSelectedSensor" 
            style="width:100%; padding:12px; border-radius:8px; border:1px solid #cbd5e0; font-size:14px; background:white; color:#2d3748"
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
        
        <p v-else style="color:#718096; font-size:14px; margin-top:16px">
          Nenhum sensor encontrado para este dispositivo.
        </p>
      </div>
      
      <p v-else>Nenhuma leitura disponível para o sensor selecionado.</p>
    </div>
  </section>
</template>

