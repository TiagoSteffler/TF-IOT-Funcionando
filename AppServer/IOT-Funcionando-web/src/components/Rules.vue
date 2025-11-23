<script setup>
import { ref, onMounted } from 'vue'

const props = defineProps({
  deviceId: { type: Number, default: 1 }
})

const rules = ref([])
const loading = ref(false)
const error = ref(null)
const showAddForm = ref(false)
const showEditForm = ref(false)
const editingRuleId = ref(null)

// Boards and sensors data
const boards = ref([])
const boardSensors = ref({}) // Map of device_id -> sensors array
const loadingBoards = ref(false)
const loadingSensors = ref({})
const SENSORS_STORAGE_KEY = 'esp32_board_sensors'

// New rule form
const newRule = ref({
  id_regra: '',
  condicao: [],
  entao: [],
  senao: []
})

// Edit rule form
const editRule = ref({
  id_regra: '',
  condicao: [],
  entao: [],
  senao: []
})

// placas carregadas em cache
const loadBoards = () => {
  try {
    const stored = localStorage.getItem('esp32_boards')
    boards.value = stored ? JSON.parse(stored) : []
    console.log('Placas carregadas:', boards.value)
  } catch (e) {
    console.error('Erro ao carregar placas:', e)
    boards.value = []
  }
}

// Salva sensores no localStorage
const saveSensorsToStorage = () => {
  try {
    localStorage.setItem(SENSORS_STORAGE_KEY, JSON.stringify(boardSensors.value))
    console.log('Sensores salvos no localStorage')
  } catch (e) {
    console.error('Erro ao salvar sensores no localStorage:', e)
  }
}

// Carrega sensores do localStorage
const loadSensorsFromStorage = () => {
  try {
    const stored = localStorage.getItem(SENSORS_STORAGE_KEY)
    if (stored) {
      boardSensors.value = JSON.parse(stored)
      console.log('Sensores carregados:', Object.keys(boardSensors.value))
      return true
    }
  } catch (e) {
    console.error('Erro ao carregar sensores do cache:', e)
  }
  return false
}

// busca os sensores ao escolher a placa
const fetchSensorsForBoard = async (deviceId) => {
  const mqttDeviceId = `esp32_device_${deviceId}`
  
  if (loadingSensors.value[mqttDeviceId]) return // Já está carregando
  
  loadingSensors.value[mqttDeviceId] = true
  
  try {
    const response = await fetch(`http://localhost:5000/${mqttDeviceId}/settings/sensors/get`)
    
    if (!response.ok) {
      throw new Error(`Falha ao buscar sensores: ${response.status}`)
    }
    
    const data = await response.json()
    console.log(`Resposta para ${mqttDeviceId}:`, data)
    
    // Handle response - could be { sensors: [...] } or just [...]
    boardSensors.value[mqttDeviceId] = data.sensors || data || []
    console.log(`Carregados ${boardSensors.value[mqttDeviceId].length} sensores para ${mqttDeviceId}`)
    
    // salva no cache
    saveSensorsToStorage()
  } catch (err) {
    console.error(`Erro ao buscar sensores para ${mqttDeviceId}:`, err)
    // nao apaga o cache
    if (!boardSensors.value[mqttDeviceId]) {
      boardSensors.value[mqttDeviceId] = []
    }
  } finally {
    loadingSensors.value[mqttDeviceId] = false
  }
}

// Get sensors for a device ID
const getSensorsForDevice = (deviceId) => {
  const mqttDeviceId = `esp32_device_${deviceId}`
  return boardSensors.value[mqttDeviceId] || []
}

// busca tudo tudo
const refreshAllSensors = async () => {
  console.log('Buscando sensores para todas as placas')
  for (const board of boards.value) {
    await fetchSensorsForBoard(board.deviceId)
  }
  console.log('Todos os sensores atualizados')
}

// busca as regras existentes
const fetchRules = async () => {
  loading.value = true
  error.value = null
  try {
    const response = await fetch('/rules', {
      method: 'GET',
      headers: { 'Content-Type': 'application/json' }
    })
    
    if (!response.ok) {
      throw new Error(`Falha ao buscar regras: ${response.status}`)
    }
    
    const data = await response.json()
    rules.value = data.rules || []
    console.log('Regras carregadas:', rules.value)
  } catch (err) {
    error.value = err.message
    console.error('Erro ao buscar regras:', err)
  } finally {
    loading.value = false
  }
}

// cria uma regra
const createRule = async () => {
  try {
    const response = await fetch('/rules', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(newRule.value)
    })
    
    if (!response.ok) {
      throw new Error(`Falha ao criar regra: ${response.status}`)
    }
    
    console.log('Regra criada com sucesso')
    showAddForm.value = false
    resetNewRule()
    await fetchRules()
  } catch (err) {
    alert(`Erro ao criar regra: ${err.message}`)
    console.error('Erro ao criar regra:', err)
  }
}

// edita a regra
const startEdit = (rule) => {
  editingRuleId.value = rule.id_regra
  editRule.value = {
    id_regra: rule.id_regra,
    condicao: JSON.parse(JSON.stringify(rule.condicao || [])),
    entao: JSON.parse(JSON.stringify(rule.entao || [])).map(a => ({ ...a, modo: a.modo || 'set' })),
    senao: JSON.parse(JSON.stringify(rule.senao || [])).map(a => ({ ...a, modo: a.modo || 'set' }))
  }
  showEditForm.value = true
  showAddForm.value = false
  
  // Carrega sensores para todos os dispositivos usados na regra (capaz que tem sensor/placa apagada)
  const deviceIds = new Set()
  rule.condicao?.forEach(c => c.id_device && deviceIds.add(c.id_device))
  rule.entao?.forEach(a => a.id_device && deviceIds.add(a.id_device))
  rule.senao?.forEach(a => a.id_device && deviceIds.add(a.id_device))
  
  deviceIds.forEach(deviceId => {
    fetchSensorsForBoard(deviceId.replace('esp32_device_', ''))
  })
}

// atualiza regra
const updateRule = async () => {
  try {
    const response = await fetch('/rules', {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(editRule.value)
    })
    
    if (!response.ok) {
      throw new Error(`Falha ao atualizar regra: ${response.status}`)
    }
    
    console.log('Regra atualizada com sucesso')
    showEditForm.value = false
    editingRuleId.value = null
    resetEditRule()
    await fetchRules()
  } catch (err) {
    alert(`Error updating rule: ${err.message}`)
    console.error('Error updating rule:', err)
  }
}

// cancela pq vai dar cagada
const cancelEdit = () => {
  showEditForm.value = false
  editingRuleId.value = null
  resetEditRule()
}

// apaga regra pois isto é uma democracia
const deleteRule = async (ruleId) => {
  if (!confirm(`Excluir regra "${ruleId}"? Isso não pode ser desfeito.`)) {
    return
  }
  
  try {
    const response = await fetch('/rules', {
      method: 'DELETE',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ id_regra: ruleId })
    })
    
    if (!response.ok) {
      throw new Error(`Falha ao apagar regra: ${response.status}`)
    }
    
    console.log('Regra apagada com sucesso')
    await fetchRules()
  } catch (err) {
    alert(`Erro ao apagar regra: ${err.message}`)
    console.error('Erro ao apagar regra:', err)
  }
}

// adiciona condição de leitura de sensor
const addCondition = () => {
  newRule.value.condicao.push({
    tipo: 'limite',
    tempo: 5,
    id_device: '',
    id_sensor: '',
    medida: '',
    operador: '>',
    valor_limite: 0
  })
}

// adiciona condição de senha
const addPasswordCondition = () => {
  newRule.value.condicao.push({
    tipo: 'senha',
    id_device: '',
    id_sensor: '',
    senha: ''
  })
}

// adiciona o 'se'
const addThenAction = () => {
  newRule.value.entao.push({
    id_device: '',
    id_atuador: '',
    valor: 0,
    tempo: 0,
    modo: 'set' // 'set' ou 'toggle'
  })
}

// apaga condição
const removeCondition = (index) => {
  newRule.value.condicao.splice(index, 1)
}

// apaga ação 'se'
const removeThenAction = (index) => {
  newRule.value.entao.splice(index, 1)
}

// adiciona o 'senao'
const addElseAction = () => {
  newRule.value.senao.push({
    id_device: '',
    id_atuador: '',
    valor: 0,
    tempo: 0,
    modo: 'set' // 'set' ou 'toggle'
  })
}

// apaga ação 'senao'
const removeElseAction = (index) => {
  newRule.value.senao.splice(index, 1)
}

// Reseta regra
const resetNewRule = () => {
  newRule.value = {
    id_regra: '',
    condicao: [],
    entao: [],
    senao: []
  }
}

// Helpers para editar regra
const addEditCondition = () => {
  editRule.value.condicao.push({
    tipo: 'limite',
    tempo: 5,
    id_device: '',
    id_sensor: '',
    medida: '',
    operador: '>',
    valor_limite: 0
  })
}

const addEditPasswordCondition = () => {
  editRule.value.condicao.push({
    tipo: 'senha',
    id_device: '',
    id_sensor: '',
    senha: ''
  })
}

const addEditThenAction = () => {
  editRule.value.entao.push({
    id_device: '',
    id_atuador: '',
    valor: 0,
    tempo: 0,
    modo: 'set' // 'set' ou 'toggle'
  })
}

const addEditElseAction = () => {
  editRule.value.senao.push({
    id_device: '',
    id_atuador: '',
    valor: 0,
    tempo: 0,
    modo: 'set' // 'set' ou 'toggle'
  })
}

const removeEditCondition = (index) => {
  editRule.value.condicao.splice(index, 1)
}

const removeEditThenAction = (index) => {
  editRule.value.entao.splice(index, 1)
}

const removeEditElseAction = (index) => {
  editRule.value.senao.splice(index, 1)
}

const resetEditRule = () => {
  editRule.value = {
    id_regra: '',
    condicao: [],
    entao: [],
    senao: []
  }
}

const successMessage = ref(null)

// verifica se o sensor é de string (teclado)
const isStringSensor = (deviceId, sensorId) => {
  if (!deviceId || !sensorId) return false
  
  const sensors = boardSensors.value[deviceId]
  if (!sensors || !Array.isArray(sensors)) return false
  
  const sensor = sensors.find(s => s.id === sensorId)
  if (!sensor) return false
  
  // Tipos de sensor que usam strings: 7 = Teclado 4x4
  const STRING_SENSOR_TYPES = [7]
  return STRING_SENSOR_TYPES.includes(sensor.tipo)
}

onMounted(() => {
  loadBoards()
  
  // carrega cache
  const hasCachedSensors = loadSensorsFromStorage()
  
  fetchRules()
  
  // atualiza todos os sensores de todas as placas
  boards.value.forEach(board => {
    fetchSensorsForBoard(board.deviceId)
  })
  
  if (!hasCachedSensors && boards.value.length > 0) {
    console.log('Nenhum sensor em cache encontrado, buscando todos os sensores')
  }
})
</script>

<template>
  <section>
    <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:24px">
      <h2 style="margin:0">Regras</h2>
      <div style="display:flex; gap:12px">

        <button 
          @click="refreshAllSensors"
          :disabled="Object.values(loadingSensors).some(v => v)"
          style="padding:10px 20px; border-radius:8px; border:none; background:#52c41a; color:white; cursor:pointer; font-weight:600; font-size:14px; opacity: 1"
          :style="{ opacity: Object.values(loadingSensors).some(v => v) ? 0.5 : 1 }"
        >
          Atualizar sensores
        </button>
        <button 
          @click="showAddForm = !showAddForm"
          style="padding:10px 20px; border-radius:8px; border:none; background:#1890ff; color:white; cursor:pointer; font-weight:600; font-size:14px"
        >
          {{ showAddForm ? 'Cancelar' : 'Nova regra' }}
        </button>
      </div>
    </div>

    <!-- Success message -->
    <div v-if="successMessage" style="margin-bottom:16px; padding:12px; background:#f6ffed; border-left:4px solid #52c41a; color:#52c41a; border-radius:4px">
      {{ successMessage }}
    </div>

    <!-- Error message -->
    <div v-if="error" style="margin-bottom:16px; padding:12px; background:#fff2f0; border-left:4px solid #ff4d4f; color:#ff4d4f; border-radius:4px">
      Error: {{ error }}
    </div>

    <!-- Edit Rule Form -->
    <div v-if="showEditForm" style="margin-bottom:24px; padding:20px; background:rgba(250,173,20,0.1); border-radius:12px; border:1px solid rgba(250,173,20,0.3)">
      <h3 style="margin-top:0; color:#ffc53d">Edit Rule: {{ editRule.id_regra }}</h3>
      
      <div style="margin-bottom:16px">
        <label style="display:block; color:#ffc53d; font-weight:600; margin-bottom:8px">ID da regra:</label>
        <input 
          v-model="editRule.id_regra" 
          disabled
          style="width:100%; padding:10px; border-radius:6px; border:1px solid #434343; background:rgba(0,0,0,0.5); color:#8c8c8c; font-size:14px; cursor:not-allowed"
        />
      </div>

      <!-- Conditions -->
      <div style="margin-bottom:20px">
        <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:12px">
          <h4 style="margin:0; color:#95de64">Conditions (IF)</h4>
          <div style="display:flex; gap:8px">
            <button @click="addEditCondition" style="padding:6px 12px; border-radius:6px; border:none; background:#52c41a; color:white; cursor:pointer; font-weight:600; font-size:12px">
              Condição de limite
            </button>
            <button @click="addEditPasswordCondition" style="padding:6px 12px; border-radius:6px; border:none; background:#52c41a; color:white; cursor:pointer; font-weight:600; font-size:12px">
              Condição de senha
            </button>
          </div>
        </div>
        
        <div v-for="(cond, index) in editRule.condicao" :key="index" style="margin-bottom:12px; padding:12px; background:rgba(82,196,26,0.1); border-radius:8px; border:1px solid rgba(82,196,26,0.3)">
          <!-- Limit Condition -->
          <div v-if="cond.tipo === 'limite'" style="display:grid; grid-template-columns: 1fr 1fr 1fr 1fr 1fr 1fr auto; gap:8px; align-items:end">
            <div>
              <label style="display:block; color:#95de64; font-size:12px; margin-bottom:4px">Board:</label>
              <select 
                v-model="cond.id_device" 
                @change="fetchSensorsForBoard(cond.id_device.replace('esp32_device_', ''))"
                style="width:100%; padding:6px; border-radius:4px; border:1px solid #52c41a; background:rgba(0,0,0,0.3); color:#fff; font-size:13px"
              >
                <option value="" disabled>Selecione a placa...</option>
                <option v-for="board in boards" :key="board.id" :value="`esp32_device_${board.deviceId}`">
                  {{ board.name }} (ID: {{ board.deviceId }})
                </option>
              </select>
            </div>
            <div>
              <label style="display:block; color:#95de64; font-size:12px; margin-bottom:4px">Sensor:</label>
              <select 
                v-model="cond.id_sensor" 
                :disabled="!cond.id_device"
                style="width:100%; padding:6px; border-radius:4px; border:1px solid #52c41a; background:rgba(0,0,0,0.3); color:#fff; font-size:13px"
              >
                <option value="" disabled>{{ cond.id_device ? 'Selecione o sensor...' : 'Selecione a placa primeiro' }}</option>
                <option 
                  v-for="sensor in getSensorsForDevice(cond.id_device?.replace('esp32_device_', ''))"
                  :key="sensor.id" 
                  :value="sensor.id"
                >
                  {{ sensor.desc || `Sensor ${sensor.id}` }} (Tipo: {{ sensor.tipo }})
                </option>
              </select>
            </div>
            <div>
              <label style="display:block; color:#95de64; font-size:12px; margin-bottom:4px">Nome do campo:</label>
              <input 
                v-model="cond.medida" 
                placeholder="e.g., x, y, bt, temperature"
                style="width:100%; padding:6px; border-radius:4px; border:1px solid #52c41a; background:rgba(0,0,0,0.3); color:#fff; font-size:13px"
              />
            </div>
            <div>
              <label style="display:block; color:#95de64; font-size:12px; margin-bottom:4px">Operator:</label>
              <select v-model="cond.operador" style="width:100%; padding:6px; border-radius:4px; border:1px solid #52c41a; background:rgba(0,0,0,0.3); color:#fff; font-size:13px">
                <option value=">">></option>
                <option value=">=">>=</option>
                <option value="<"><</option>
                <option value="<="><=</option>
                <option value="==">==</option>
                <option value="!=">!=</option>
              </select>
            </div>
            <div>
              <label style="display:block; color:#95de64; font-size:12px; margin-bottom:4px">Value:</label>
              <input 
                v-if="isStringSensor(cond.id_device, cond.id_sensor)"
                v-model="cond.valor_limite" 
                type="text" 
                placeholder="ABC123" 
                style="width:100%; padding:6px; border-radius:4px; border:1px solid #52c41a; background:rgba(0,0,0,0.3); color:#fff; font-size:13px" 
              />
              <input 
                v-else
                v-model.number="cond.valor_limite" 
                type="number" 
                placeholder="80" 
                style="width:100%; padding:6px; border-radius:4px; border:1px solid #52c41a; background:rgba(0,0,0,0.3); color:#fff; font-size:13px" 
              />
            </div>
            <div>
              <label style="display:block; color:#95de64; font-size:12px; margin-bottom:4px">Time (s):</label>
              <input v-model.number="cond.tempo" type="number" placeholder="5" style="width:100%; padding:6px; border-radius:4px; border:1px solid #52c41a; background:rgba(0,0,0,0.3); color:#fff; font-size:13px" />
            </div>
            <button @click="removeEditCondition(index)" style="padding:6px 10px; border-radius:4px; border:none; background:rgba(255,77,79,0.3); color:#ff7875; cursor:pointer; font-size:12px">🗑️</button>
          </div>
          
          <!-- Password Condition -->
          <div v-else-if="cond.tipo === 'senha'" style="display:grid; grid-template-columns: 1fr 1fr 1fr auto; gap:8px; align-items:end">
            <div>
              <label style="display:block; color:#95de64; font-size:12px; margin-bottom:4px">Board:</label>
              <select 
                v-model="cond.id_device" 
                @change="fetchSensorsForBoard(cond.id_device.replace('esp32_device_', ''))"
                style="width:100%; padding:6px; border-radius:4px; border:1px solid #52c41a; background:rgba(0,0,0,0.3); color:#fff; font-size:13px"
              >
                <option value="" disabled>Selecione a placa...</option>
                <option v-for="board in boards" :key="board.id" :value="`esp32_device_${board.deviceId}`">
                  {{ board.name }} (ID: {{ board.deviceId }})
                </option>
              </select>
            </div>
            <div>
              <label style="display:block; color:#95de64; font-size:12px; margin-bottom:4px">Teclado:</label>
              <select 
                v-model="cond.id_sensor" 
                :disabled="!cond.id_device"
                style="width:100%; padding:6px; border-radius:4px; border:1px solid #52c41a; background:rgba(0,0,0,0.3); color:#fff; font-size:13px"
              >
                <option value="" disabled>{{ cond.id_device ? 'Selecione o teclado...' : 'Selecione a placa primeiro' }}</option>
                <option 
                  v-for="sensor in getSensorsForDevice(cond.id_device?.replace('esp32_device_', '')).filter(s => s.tipo === 7)"
                  :key="sensor.id" 
                  :value="sensor.id"
                >
                  {{ sensor.desc || `Keypad ${sensor.id}` }}
                </option>
              </select>
            </div>
            <div>
              <label style="display:block; color:#95de64; font-size:12px; margin-bottom:4px">Password:</label>
              <input v-model="cond.senha" placeholder="1234" style="width:100%; padding:6px; border-radius:4px; border:1px solid #52c41a; background:rgba(0,0,0,0.3); color:#fff; font-size:13px" />
            </div>
            <button @click="removeEditCondition(index)" style="padding:6px 10px; border-radius:4px; border:none; background:rgba(255,77,79,0.3); color:#ff7875; cursor:pointer; font-size:12px">🗑️</button>
          </div>
        </div>
      </div>

      <!-- Then Actions -->
      <div style="margin-bottom:20px">
        <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:12px">
          <h4 style="margin:0; color:#ffc53d">Ações (ENTÃO)</h4>
          <button @click="addEditThenAction" style="padding:6px 12px; border-radius:6px; border:none; background:#faad14; color:white; cursor:pointer; font-weight:600; font-size:12px">
            Adicionar ação
          </button>
        </div>
        
        <div v-for="(action, index) in editRule.entao" :key="index" style="margin-bottom:12px; padding:12px; background:rgba(250,173,20,0.1); border-radius:8px; border:1px solid rgba(250,173,20,0.3)">
          <div style="display:grid; grid-template-columns: 1fr 1fr 100px 1fr 1fr auto; gap:8px; align-items:end">
            <div>
              <label style="display:block; color:#ffc53d; font-size:12px; margin-bottom:4px">Board:</label>
              <select 
                v-model="action.id_device" 
                @change="fetchSensorsForBoard(action.id_device.replace('esp32_device_', ''))"
                style="width:100%; padding:6px; border-radius:4px; border:1px solid #faad14; background:rgba(0,0,0,0.3); color:#fff; font-size:13px"
              >
                <option value="" disabled>Selecione a placa...</option>
                <option v-for="board in boards" :key="board.id" :value="`esp32_device_${board.deviceId}`">
                  {{ board.name }} (ID: {{ board.deviceId }})
                </option>
              </select>
            </div>
            <div>
              <label style="display:block; color:#ffc53d; font-size:12px; margin-bottom:4px">Actuator:</label>
              <select 
                v-model="action.id_atuador" 
                :disabled="!action.id_device"
                style="width:100%; padding:6px; border-radius:4px; border:1px solid #faad14; background:rgba(0,0,0,0.3); color:#fff; font-size:13px"
              >
                <option value="" disabled>{{ action.id_device ? 'Selecione o atuador...' : 'Selecione a placa primeiro' }}</option>
                <option 
                  v-for="actuator in getSensorsForDevice(action.id_device?.replace('esp32_device_', '')).filter(s => s.tipo === 4 || s.tipo === 5)"
                  :key="actuator.id" 
                  :value="actuator.id"
                >
                  {{ actuator.desc || `Atuador ${actuator.id}` }} (Tipo: {{ actuator.tipo === 4 ? 'Servo' : 'Relé' }})
                </option>
              </select>
            </div>
            <div>
              <label style="display:block; color:#ffc53d; font-size:12px; margin-bottom:4px">Modo:</label>
              <select v-model="action.modo" style="width:100%; padding:6px; border-radius:4px; border:1px solid #faad14; background:rgba(0,0,0,0.3); color:#fff; font-size:13px">
                <option value="set">Set</option>
                <option value="toggle">Toggle</option>
              </select>
            </div>
            <div v-if="action.modo === 'set'">
              <label style="display:block; color:#ffc53d; font-size:12px; margin-bottom:4px">Valor:</label>
              <input v-model.number="action.valor" type="number" placeholder="1" style="width:50%; padding:6px; border-radius:4px; border:1px solid #faad14; background:rgba(0,0,0,0.3); color:#fff; font-size:13px" />
            </div>
            <div v-else>
              <label style="display:block; color:#ffc53d; font-size:12px; margin-bottom:4px">Valor:</label>
              <input value="Toggle" disabled style="width:50%; padding:6px; border-radius:4px; border:1px solid #faad14; background:rgba(0,0,0,0.2); color:#999; font-size:13px" />
            </div>
            <div>
              <label style="display:block; color:#ffc53d; font-size:12px; margin-bottom:4px">Duração (s):</label>
              <input v-model.number="action.tempo" type="number" placeholder="0" min="0" title="0 = permanente, >0 = temporário (auto-reverter para 0)" style="width:100%; padding:6px; border-radius:4px; border:1px solid #faad14; background:rgba(0,0,0,0.3); color:#fff; font-size:13px" />
            </div>
            <button @click="removeEditThenAction(index)" style="padding:6px 10px; border-radius:4px; border:none; background:rgba(255,77,79,0.3); color:#ff7875; cursor:pointer; font-size:12px">🗑️</button>
          </div>
        </div>
      </div>

      <!-- Else Actions -->
      <div style="margin-bottom:20px">
        <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:12px">
          <h4 style="margin:0; color:#ff85c0">Ações (SENÃO) - Opcional</h4>
          <button @click="addEditElseAction" style="padding:6px 12px; border-radius:6px; border:none; background:#eb2f96; color:white; cursor:pointer; font-weight:600; font-size:12px">
            Adicionar ação
          </button>
        </div>
        
        <div v-for="(action, index) in editRule.senao" :key="index" style="margin-bottom:12px; padding:12px; background:rgba(235,47,150,0.1); border-radius:8px; border:1px solid rgba(235,47,150,0.3)">
          <div style="display:grid; grid-template-columns: 1fr 1fr 100px 1fr 1fr auto; gap:8px; align-items:end">
            <div>
              <label style="display:block; color:#ff85c0; font-size:12px; margin-bottom:4px">Board:</label>
              <select 
                v-model="action.id_device" 
                @change="fetchSensorsForBoard(action.id_device.replace('esp32_device_', ''))"
                style="width:100%; padding:6px; border-radius:4px; border:1px solid #eb2f96; background:rgba(0,0,0,0.3); color:#fff; font-size:13px"
              >
                <option value="" disabled>Selecione a placa...</option>
                <option v-for="board in boards" :key="board.id" :value="`esp32_device_${board.deviceId}`">
                  {{ board.name }} (ID: {{ board.deviceId }})
                </option>
              </select>
            </div>
            <div>
              <label style="display:block; color:#ff85c0; font-size:12px; margin-bottom:4px">Actuator:</label>
              <select 
                v-model="action.id_atuador" 
                :disabled="!action.id_device"
                style="width:100%; padding:6px; border-radius:4px; border:1px solid #eb2f96; background:rgba(0,0,0,0.3); color:#fff; font-size:13px"
              >
                <option value="" disabled>{{ action.id_device ? 'Selecione o atuador...' : 'Selecione a placa primeiro' }}</option>
                <option 
                  v-for="actuator in getSensorsForDevice(action.id_device?.replace('esp32_device_', '')).filter(s => s.tipo === 4 || s.tipo === 5)"
                  :key="actuator.id" 
                  :value="actuator.id"
                >
                  {{ actuator.desc || `Atuador ${actuator.id}` }} (Tipo: {{ actuator.tipo === 4 ? 'Servo' : 'Relé' }})
                </option>
              </select>
            </div>
            <div>
              <label style="display:block; color:#ff85c0; font-size:12px; margin-bottom:4px">Modo:</label>
              <select v-model="action.modo" style="width:100%; padding:6px; border-radius:4px; border:1px solid #eb2f96; background:rgba(0,0,0,0.3); color:#fff; font-size:13px">
                <option value="set">Set</option>
                <option value="toggle">Toggle</option>
              </select>
            </div>
            <div v-if="action.modo === 'set'">
              <label style="display:block; color:#ff85c0; font-size:12px; margin-bottom:4px">Valor:</label>
              <input v-model.number="action.valor" type="number" placeholder="0" style="width:50%; padding:6px; border-radius:4px; border:1px solid #eb2f96; background:rgba(0,0,0,0.3); color:#fff; font-size:13px" />
            </div>
            <div v-else>
              <label style="display:block; color:#ff85c0; font-size:12px; margin-bottom:4px">Valor:</label>
              <input value="Toggle" disabled style="width:50%; padding:6px; border-radius:4px; border:1px solid #eb2f96; background:rgba(0,0,0,0.2); color:#999; font-size:13px" />
            </div>
            <div>
              <label style="display:block; color:#ff85c0; font-size:12px; margin-bottom:4px">Duração (s):</label>
              <input v-model.number="action.tempo" type="number" placeholder="0" min="0" title="0 = permanente, >0 = temporário (auto-reverte para 0 após duração)" style="width:100%; padding:6px; border-radius:4px; border:1px solid #eb2f96; background:rgba(0,0,0,0.3); color:#fff; font-size:13px" />
            </div>
            <button @click="removeEditElseAction(index)" style="padding:6px 10px; border-radius:4px; border:none; background:rgba(255,77,79,0.3); color:#ff7875; cursor:pointer; font-size:12px">🗑️</button>
          </div>
        </div>
      </div>

      <div style="display:flex; gap:12px">
        <button 
          @click="updateRule"
          :disabled="!editRule.id_regra || editRule.condicao.length === 0 || editRule.entao.length === 0"
          style="padding:12px 24px; border-radius:8px; border:none; background:#52c41a; color:white; cursor:pointer; font-weight:600; font-size:14px; flex:1"
        >
          Atualizar regra
        </button>
        <button 
          @click="cancelEdit"
          style="padding:12px 24px; border-radius:8px; border:none; background:rgba(255,77,79,0.3); color:#ff7875; cursor:pointer; font-weight:600; font-size:14px"
        >
          Cancelar
        </button>
      </div>
    </div>

    <!-- Add Rule Form -->
    <div v-if="showAddForm" style="margin-bottom:24px; padding:20px; background:rgba(24,144,255,0.1); border-radius:12px; border:1px solid rgba(24,144,255,0.3)">
      <h3 style="margin-top:0; color:#91d5ff">Criar nova regra</h3>
      
      <div style="margin-bottom:16px">
        <label style="display:block; color:#91d5ff; font-weight:600; margin-bottom:8px">ID da Regra:</label>
        <input 
          v-model="newRule.id_regra" 
          placeholder="e.g., rule_temp_alert"
          style="width:100%; padding:10px; border-radius:6px; border:1px solid #434343; background:rgba(0,0,0,0.3); color:#fff; font-size:14px"
        />
      </div>

      <!-- Conditions -->
      <div style="margin-bottom:20px">
        <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:12px">
          <h4 style="margin:0; color:#95de64">Conditions (IF)</h4>
          <div style="display:flex; gap:8px">
            <button @click="addCondition" style="padding:6px 12px; border-radius:6px; border:none; background:#52c41a; color:white; cursor:pointer; font-weight:600; font-size:12px">
              Condição de limite
            </button>
            <button @click="addPasswordCondition" style="padding:6px 12px; border-radius:6px; border:none; background:#52c41a; color:white; cursor:pointer; font-weight:600; font-size:12px">
              Condição de senha
            </button>
          </div>
        </div>
        
        <div v-for="(cond, index) in newRule.condicao" :key="index" style="margin-bottom:12px; padding:12px; background:rgba(82,196,26,0.1); border-radius:8px; border:1px solid rgba(82,196,26,0.3)">
          <!-- Limit Condition -->
          <div v-if="cond.tipo === 'limite'" style="display:grid; grid-template-columns: 1fr 1fr 1fr 1fr 1fr 1fr auto; gap:8px; align-items:end">
            <div>
              <label style="display:block; color:#95de64; font-size:12px; margin-bottom:4px">Placa:</label>
              <select 
                v-model="cond.id_device" 
                @change="fetchSensorsForBoard(cond.id_device.replace('esp32_device_', ''))"
                style="width:100%; padding:6px; border-radius:4px; border:1px solid #52c41a; background:rgba(0,0,0,0.3); color:#fff; font-size:13px"
              >
                <option value="" disabled>Selecione a placa...</option>
                <option v-for="board in boards" :key="board.id" :value="`esp32_device_${board.deviceId}`">
                  {{ board.name }} (ID: {{ board.deviceId }})
                </option>
              </select>
            </div>
            <div>
              <label style="display:block; color:#95de64; font-size:12px; margin-bottom:4px">Sensor:</label>
              <select 
                v-model="cond.id_sensor" 
                :disabled="!cond.id_device"
                style="width:100%; padding:6px; border-radius:4px; border:1px solid #52c41a; background:rgba(0,0,0,0.3); color:#fff; font-size:13px"
              >
                <option value="" disabled>{{ cond.id_device ? 'Selecione o sensor...' : 'Selecione a placa primeiro' }}</option>
                <option 
                  v-for="sensor in getSensorsForDevice(cond.id_device?.replace('esp32_device_', ''))"
                  :key="sensor.id" 
                  :value="sensor.id"
                >
                  {{ sensor.desc || `Sensor ${sensor.id}` }} (Tipo: {{ sensor.tipo }})
                </option>
              </select>
            </div>
            <div>
              <label style="display:block; color:#95de64; font-size:12px; margin-bottom:4px">Nome do campo:</label>
              <input 
                v-model="cond.medida" 
                placeholder="e.g., x, y, bt, temperature"
                style="width:100%; padding:6px; border-radius:4px; border:1px solid #52c41a; background:rgba(0,0,0,0.3); color:#fff; font-size:13px"
              />
            </div>
            <div>
              <label style="display:block; color:#95de64; font-size:12px; margin-bottom:4px">Operador:</label>
              <select v-model="cond.operador" style="width:100%; padding:6px; border-radius:4px; border:1px solid #52c41a; background:rgba(0,0,0,0.3); color:#fff; font-size:13px">
                <option value=">">></option>
                <option value=">=">>=</option>
                <option value="<"><</option>
                <option value="<="><=</option>
                <option value="==">==</option>
                <option value="!=">!=</option>
              </select>
            </div>
            <div>
              <label style="display:block; color:#95de64; font-size:12px; margin-bottom:4px">Valor:</label>
              <input 
                v-if="isStringSensor(cond.id_device, cond.id_sensor)"
                v-model="cond.valor_limite" 
                type="text" 
                placeholder="ABC123" 
                style="width:100%; padding:6px; border-radius:4px; border:1px solid #52c41a; background:rgba(0,0,0,0.3); color:#fff; font-size:13px" 
              />
              <input 
                v-else
                v-model.number="cond.valor_limite" 
                type="number" 
                placeholder="80" 
                style="width:100%; padding:6px; border-radius:4px; border:1px solid #52c41a; background:rgba(0,0,0,0.3); color:#fff; font-size:13px" 
              />
            </div>
            <div>
              <label style="display:block; color:#95de64; font-size:12px; margin-bottom:4px">Tempo (s):</label>
              <input v-model.number="cond.tempo" type="number" placeholder="5" min="0" title="0 = instantâneo, >0 = condição deve ser verdadeira por essa duração" style="width:100%; padding:6px; border-radius:4px; border:1px solid #52c41a; background:rgba(0,0,0,0.3); color:#fff; font-size:13px" />
            </div>
            <button @click="removeCondition(index)" style="padding:6px 10px; border-radius:4px; border:none; background:rgba(255,77,79,0.3); color:#ff7875; cursor:pointer; font-size:12px">🗑️</button>
          </div>
          
          <!-- Password Condition -->
          <div v-else-if="cond.tipo === 'senha'" style="display:grid; grid-template-columns: 1fr 1fr 1fr auto; gap:8px; align-items:end">
            <div>
              <label style="display:block; color:#95de64; font-size:12px; margin-bottom:4px">Placa:</label>
              <select 
                v-model="cond.id_device" 
                @change="fetchSensorsForBoard(cond.id_device.replace('esp32_device_', ''))"
                style="width:100%; padding:6px; border-radius:4px; border:1px solid #52c41a; background:rgba(0,0,0,0.3); color:#fff; font-size:13px"
              >
                <option value="" disabled>Selecione a placa...</option>
                <option v-for="board in boards" :key="board.id" :value="`esp32_device_${board.deviceId}`">
                  {{ board.name }} (ID: {{ board.deviceId }})
                </option>
              </select>
            </div>
            <div>
              <label style="display:block; color:#95de64; font-size:12px; margin-bottom:4px">Teclado:</label>
              <select 
                v-model="cond.id_sensor" 
                :disabled="!cond.id_device"
                style="width:100%; padding:6px; border-radius:4px; border:1px solid #52c41a; background:rgba(0,0,0,0.3); color:#fff; font-size:13px"
              >
                <option value="" disabled>{{ cond.id_device ? 'Selecione o teclado...' : 'Selecione a placa primeiro' }}</option>
                <option 
                  v-for="sensor in getSensorsForDevice(cond.id_device?.replace('esp32_device_', '')).filter(s => s.tipo === 7)"
                  :key="sensor.id" 
                  :value="sensor.id"
                >
                  {{ sensor.desc || `Keypad ${sensor.id}` }}
                </option>
              </select>
            </div>
            <div>
              <label style="display:block; color:#95de64; font-size:12px; margin-bottom:4px">Senha:</label>
              <input v-model="cond.senha" placeholder="1234" style="width:100%; padding:6px; border-radius:4px; border:1px solid #52c41a; background:rgba(0,0,0,0.3); color:#fff; font-size:13px" />
            </div>
            <button @click="removeCondition(index)" style="padding:6px 10px; border-radius:4px; border:none; background:rgba(255,77,79,0.3); color:#ff7875; cursor:pointer; font-size:12px">🗑️</button>
          </div>
        </div>
      </div>

      <!-- Then Actions -->
      <div style="margin-bottom:20px">
        <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:12px">
          <h4 style="margin:0; color:#ffc53d">Ações (ENTÃO)</h4>
          <button @click="addThenAction" style="padding:6px 12px; border-radius:6px; border:none; background:#faad14; color:white; cursor:pointer; font-weight:600; font-size:12px">
            Adicionar ação
          </button>
        </div>
        
        <div v-for="(action, index) in newRule.entao" :key="index" style="margin-bottom:12px; padding:12px; background:rgba(250,173,20,0.1); border-radius:8px; border:1px solid rgba(250,173,20,0.3)">
          <div style="display:grid; grid-template-columns: 1fr 1fr 100px 1fr 1fr auto; gap:8px; align-items:end">
            <div>
              <label style="display:block; color:#ffc53d; font-size:12px; margin-bottom:4px">Placa:</label>
              <select 
                v-model="action.id_device" 
                @change="fetchSensorsForBoard(action.id_device.replace('esp32_device_', ''))"
                style="width:100%; padding:6px; border-radius:4px; border:1px solid #faad14; background:rgba(0,0,0,0.3); color:#fff; font-size:13px"
              >
                <option value="" disabled>Selecione a placa...</option>
                <option v-for="board in boards" :key="board.id" :value="`esp32_device_${board.deviceId}`">
                  {{ board.name }} (ID: {{ board.deviceId }})
                </option>
              </select>
            </div>
            <div>
              <label style="display:block; color:#ffc53d; font-size:12px; margin-bottom:4px">Atuador:</label>
              <select 
                v-model="action.id_atuador" 
                :disabled="!action.id_device"
                style="width:100%; padding:6px; border-radius:4px; border:1px solid #faad14; background:rgba(0,0,0,0.3); color:#fff; font-size:13px"
              >
                <option value="" disabled>{{ action.id_device ? 'Selecione o atuador...' : 'Selecione a placa primeiro' }}</option>
                <option 
                  v-for="actuator in getSensorsForDevice(action.id_device?.replace('esp32_device_', '')).filter(s => s.tipo === 4 || s.tipo === 5)"
                  :key="actuator.id" 
                  :value="actuator.id"
                >
                  {{ actuator.desc || `Atuador ${actuator.id}` }} (Tipo: {{ actuator.tipo === 4 ? 'Servo' : 'Relé' }})
                </option>
              </select>
            </div>
            <div>
              <label style="display:block; color:#ffc53d; font-size:12px; margin-bottom:4px">Modo:</label>
              <select v-model="action.modo" style="width:100%; padding:6px; border-radius:4px; border:1px solid #faad14; background:rgba(0,0,0,0.3); color:#fff; font-size:13px">
                <option value="set">Set</option>
                <option value="toggle">Toggle</option>
              </select>
            </div>
            <div v-if="action.modo === 'set'">
              <label style="display:block; color:#ffc53d; font-size:12px; margin-bottom:4px">Value:</label>
              <input v-model.number="action.valor" type="number" placeholder="1" style="width:100%; padding:6px; border-radius:4px; border:1px solid #faad14; background:rgba(0,0,0,0.3); color:#fff; font-size:13px" />
            </div>
            <div v-else>
              <label style="display:block; color:#ffc53d; font-size:12px; margin-bottom:4px">Value:</label>
              <input value="Toggle" disabled style="width:100%; padding:6px; border-radius:4px; border:1px solid #faad14; background:rgba(0,0,0,0.2); color:#999; font-size:13px" />
            </div>
            <div>
              <label style="display:block; color:#ffc53d; font-size:12px; margin-bottom:4px">Duração (s):</label>
              <input v-model.number="action.tempo" type="number" placeholder="0" min="0" title="0 = permanente, >0 = temporário (auto-reverter para 0)" style="width:100%; padding:6px; border-radius:4px; border:1px solid #faad14; background:rgba(0,0,0,0.3); color:#fff; font-size:13px" />
            </div>
            <button @click="removeThenAction(index)" style="padding:6px 10px; border-radius:4px; border:none; background:rgba(255,77,79,0.3); color:#ff7875; cursor:pointer; font-size:12px">🗑️</button>
          </div>
        </div>
      </div>

      <!-- Else Actions -->
      <div style="margin-bottom:20px">
        <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:12px">
          <h4 style="margin:0; color:#ff85c0">Ações (SENÃO) - Opcional</h4>
          <button @click="addElseAction" style="padding:6px 12px; border-radius:6px; border:none; background:#eb2f96; color:white; cursor:pointer; font-weight:600; font-size:12px">
            Adicionar Ação
          </button>
        </div>
        
        <div v-for="(action, index) in newRule.senao" :key="index" style="margin-bottom:12px; padding:12px; background:rgba(235,47,150,0.1); border-radius:8px; border:1px solid rgba(235,47,150,0.3)">
          <div style="display:grid; grid-template-columns: 1fr 1fr 100px 1fr 1fr auto; gap:8px; align-items:end">
            <div>
              <label style="display:block; color:#ff85c0; font-size:12px; margin-bottom:4px">Placa:</label>
              <select 
                v-model="action.id_device" 
                @change="fetchSensorsForBoard(action.id_device.replace('esp32_device_', ''))"
                style="width:100%; padding:6px; border-radius:4px; border:1px solid #eb2f96; background:rgba(0,0,0,0.3); color:#fff; font-size:13px"
              >
                <option value="" disabled>Selecione a placa...</option>
                <option v-for="board in boards" :key="board.id" :value="`esp32_device_${board.deviceId}`">
                  {{ board.name }} (ID: {{ board.deviceId }})
                </option>
              </select>
            </div>
            <div>
              <label style="display:block; color:#ff85c0; font-size:12px; margin-bottom:4px">Atuador:</label>
              <select 
                v-model="action.id_atuador" 
                :disabled="!action.id_device"
                style="width:100%; padding:6px; border-radius:4px; border:1px solid #eb2f96; background:rgba(0,0,0,0.3); color:#fff; font-size:13px"
              >
                <option value="" disabled>{{ action.id_device ? 'Selecione o atuador...' : 'Selecione a placa primeiro' }}</option>
                <option 
                  v-for="actuator in getSensorsForDevice(action.id_device?.replace('esp32_device_', '')).filter(s => s.tipo === 4 || s.tipo === 5)"
                  :key="actuator.id" 
                  :value="actuator.id"
                >
                  {{ actuator.desc || `Atuador ${actuator.id}` }} (Tipo: {{ actuator.tipo === 4 ? 'Servo' : 'Relé' }})
                </option>
              </select>
            </div>
            <div>
              <label style="display:block; color:#ff85c0; font-size:12px; margin-bottom:4px">Valor:</label>
              <input v-model.number="action.valor" type="number" placeholder="0" style="width:100%; padding:6px; border-radius:4px; border:1px solid #eb2f96; background:rgba(0,0,0,0.3); color:#fff; font-size:13px" />
            </div>
            <div>
              <label style="display:block; color:#ff85c0; font-size:12px; margin-bottom:4px">Duração (s):</label>
              <input v-model.number="action.tempo" type="number" placeholder="0" min="0" title="0 = set and stay (no auto-revert), >0 = temporary (auto-revert to 0 after duration)" style="width:100%; padding:6px; border-radius:4px; border:1px solid #eb2f96; background:rgba(0,0,0,0.3); color:#fff; font-size:13px" />
            </div>
            <button @click="removeElseAction(index)" style="padding:6px 10px; border-radius:4px; border:none; background:rgba(255,77,79,0.3); color:#ff7875; cursor:pointer; font-size:12px">🗑️</button>
          </div>
        </div>
      </div>

      <button 
        @click="createRule"
        :disabled="!newRule.id_regra || newRule.condicao.length === 0 || newRule.entao.length === 0"
        style="padding:12px 24px; border-radius:8px; border:none; background:#52c41a; color:white; cursor:pointer; font-weight:600; font-size:14px; width:100%"
      >
        Criar regra
      </button>
    </div>

    <!-- Rules List -->
    <div v-if="loading">
      <p>Carregando regras...</p>
    </div>

    <div v-else-if="error">
      <p style="color:#e53e3e">Erro: {{ error }}</p>
    </div>

    <div v-else-if="rules.length">
      <ul style="list-style:none; padding:0">
        <li v-for="rule in rules" :key="rule.id_regra" style="margin-bottom:16px; padding:16px; background:rgba(255,255,255,0.05); border-radius:12px; border-left:4px solid #1890ff">
          <div style="display:flex; justify-content:space-between; align-items:start">
            <div style="flex:1">
              <h3 style="margin:0 0 12px 0; color:#91d5ff">{{ rule.id_regra }}</h3>
              
              <!-- Conditions -->
              <div style="margin-bottom:12px">
                <strong style="color:#95de64">IF:</strong>
                <div v-for="(cond, idx) in rule.condicao" :key="idx" style="margin-left:20px; color:#bfbfbf; font-size:14px">
                  <span v-if="cond.tipo === 'limite'">- Sensor {{ cond.id_sensor }} ({{ cond.id_device }}) campo "{{ cond.medida }}" {{ cond.operador }} {{ cond.valor_limite }} por {{ cond.tempo }}s</span>
                  <span v-else-if="cond.tipo === 'senha'">- Keypad {{ cond.id_sensor }} ({{ cond.id_device }}) senha: {{ cond.senha }}</span>
                </div>
              </div>
              
              <!-- Actions -->
              <div style="margin-bottom:12px">
                <strong style="color:#ffc53d">THEN:</strong>
                <div v-for="(action, idx) in rule.entao" :key="idx" style="margin-left:20px; color:#bfbfbf; font-size:14px">
                  - Atuador {{ action.id_atuador }} ({{ action.id_device }}) para {{ action.valor }}<span v-if="action.tempo && action.tempo > 0"> por {{ action.tempo }}s</span>
                </div>
              </div>
              
              <!-- Else Actions -->
              <div v-if="rule.senao && rule.senao.length > 0">
                <strong style="color:#ff85c0">ELSE:</strong>
                <div v-for="(action, idx) in rule.senao" :key="idx" style="margin-left:20px; color:#bfbfbf; font-size:14px">
                  - Atuador {{ action.id_atuador }} ({{ action.id_device }}) para {{ action.valor }}<span v-if="action.tempo && action.tempo > 0"> por {{ action.tempo }}s</span>
                </div>
              </div>
            </div>
            
            <div style="display:flex; gap:8px">
              <button 
                @click="startEdit(rule)"
                style="padding:8px 16px; border-radius:8px; border:none; background:rgba(250,173,20,0.25); color:#ffc53d; cursor:pointer; font-weight:600; font-size:13px">Editar</button>
              <button 
                @click="deleteRule(rule.id_regra)"
                style="padding:8px 16px; border-radius:8px; border:none; background:rgba(255,77,79,0.25); color:#ff7875; cursor:pointer; font-weight:600; font-size:13px">Excluir</button>
            </div>
          </div>
        </li>
      </ul>
    </div>

    <div v-else>
      <p>Nenhuma regra configurada. Clique em "Nova Regra" para criar uma.</p>
    </div>
  </section>
</template>
