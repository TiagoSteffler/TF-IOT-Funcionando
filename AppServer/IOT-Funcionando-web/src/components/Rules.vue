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
    <!-- Header -->
    <div class="flex-between" style="margin-bottom:24px">
      <h2>Regras</h2>
      <div class="flex-gap-12">
        <button
          @click="refreshAllSensors"
          :disabled="Object.values(loadingSensors).some(v => v)"
          :class="['btn', 'btn-green', { 'btn-disabled': Object.values(loadingSensors).some(v => v) }]"
        >
          Atualizar sensores
        </button>

        <button
          @click="showAddForm = !showAddForm"
          class="btn btn-blue"
        >
          {{ showAddForm ? 'Cancelar' : 'Nova regra' }}
        </button>
      </div>
    </div>

    <!-- Messages -->
    <div v-if="successMessage" class="msg-success">
      {{ successMessage }}
    </div>

    <div v-if="error" class="msg-error">
      Error: {{ error }}
    </div>

    <!-- Edit Rule Form -->
    <div v-if="showEditForm" class="form-edit">
      <h3>Edit Rule: {{ editRule.id_regra }}</h3>

      <div class="input-block">
        <label class="label-orange">ID da regra:</label>
        <input v-model="editRule.id_regra" disabled class="input input-large input-disabled" />
      </div>

      <!-- Conditions -->
      <div style="margin-bottom:20px">
        <div class="flex-between" style="margin-bottom:12px">
          <h4 class="label-green">Conditions (IF)</h4>
          <div class="flex-gap-12">
            <button @click="addEditCondition" class="btn-small btn-green">Condição de limite</button>
            <button @click="addEditPasswordCondition" class="btn-small btn-green">Condição de senha</button>
          </div>
        </div>

        <div v-for="(cond, index) in editRule.condicao" :key="index" class="block-green">
          <!-- Limit Condition -->
          <div v-if="cond.tipo === 'limite'" class="grid-cond-limit">
            <div>
              <label class="label-green">Board:</label>
              <select
                v-model="cond.id_device"
                @change="fetchSensorsForBoard(cond.id_device.replace('esp32_device_', ''))"
                class="select border-green"
              >
                <option value="" disabled>Selecione a placa...</option>
                <option v-for="board in boards" :key="board.id" :value="`esp32_device_${board.deviceId}`">
                  {{ board.name }} (ID: {{ board.deviceId }})
                </option>
              </select>
            </div>

            <div>
              <label class="label-green">Sensor:</label>
              <select
                v-model="cond.id_sensor"
                :disabled="!cond.id_device"
                class="select border-green"
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
              <label class="label-green">Nome do campo:</label>
              <input v-model="cond.medida" placeholder="e.g., x, y, bt, temperature" class="input" />
            </div>

            <div>
              <label class="label-green">Operator:</label>
              <select v-model="cond.operador" class="select border-green">
                <option value=">">></option>
                <option value=">=">>=</option>
                <option value="<"><</option>
                <option value="<="><=</option>
                <option value="==">==</option>
                <option value="!=">!=</option>
              </select>
            </div>

            <div>
              <label class="label-green">Value:</label>
              <input
                v-if="isStringSensor(cond.id_device, cond.id_sensor)"
                v-model="cond.valor_limite"
                type="text"
                placeholder="ABC123"
                class="input"
              />
              <input
                v-else
                v-model.number="cond.valor_limite"
                type="number"
                placeholder="80"
                class="input"
              />
            </div>

            <div>
              <label class="label-green">Time (s):</label>
              <input v-model.number="cond.tempo" type="number" placeholder="5" class="input" />
            </div>

            <div>
              <button @click="removeEditCondition(index)" class="btn-delete">🗑️</button>
            </div>
          </div>

          <!-- Password Condition -->
          <div v-else-if="cond.tipo === 'senha'" class="grid-cond-pass">
            <div>
              <label class="label-green">Board:</label>
              <select
                v-model="cond.id_device"
                @change="fetchSensorsForBoard(cond.id_device.replace('esp32_device_', ''))"
                class="select border-green"
              >
                <option value="" disabled>Selecione a placa...</option>
                <option v-for="board in boards" :key="board.id" :value="`esp32_device_${board.deviceId}`">
                  {{ board.name }} (ID: {{ board.deviceId }})
                </option>
              </select>
            </div>

            <div>
              <label class="label-green">Teclado:</label>
              <select
                v-model="cond.id_sensor"
                :disabled="!cond.id_device"
                class="select border-green"
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
              <label class="label-green">Password:</label>
              <input v-model="cond.senha" placeholder="1234" class="input" />
            </div>

            <div>
              <button @click="removeEditCondition(index)" class="btn-delete">🗑️</button>
            </div>
          </div>
        </div>
      </div>

      <!-- Then Actions -->
      <div style="margin-bottom:20px">
        <div class="flex-between" style="margin-bottom:12px">
          <h4 class="label-orange">Ações (ENTÃO)</h4>
          <button @click="addEditThenAction" class="btn-small btn-orange">Adicionar ação</button>
        </div>

        <div v-for="(action, index) in editRule.entao" :key="index" class="block-orange">
          <div class="grid-action">
            <div>
              <label class="label-orange">Board:</label>
              <select
                v-model="action.id_device"
                @change="fetchSensorsForBoard(action.id_device.replace('esp32_device_', ''))"
                class="select border-orange"
              >
                <option value="" disabled>Selecione a placa...</option>
                <option v-for="board in boards" :key="board.id" :value="`esp32_device_${board.deviceId}`">
                  {{ board.name }} (ID: {{ board.deviceId }})
                </option>
              </select>
            </div>

            <div>
              <label class="label-orange">Actuator:</label>
              <select v-model="action.id_atuador" :disabled="!action.id_device" class="select border-orange">
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
              <label class="label-orange">Modo:</label>
              <select v-model="action.modo" class="select border-orange">
                <option value="set">Set</option>
                <option value="toggle">Toggle</option>
              </select>
            </div>

            <div v-if="action.modo === 'set'">
              <label class="label-orange">Valor:</label>
              <input v-model.number="action.valor" type="number" placeholder="1" class="input" />
            </div>

            <div v-else>
              <label class="label-orange">Valor:</label>
              <input value="Toggle" disabled class="input input-disabled" />
            </div>

            <div>
              <label class="label-orange">Duração (s):</label>
              <input v-model.number="action.tempo" type="number" placeholder="0" min="0" class="input" />
            </div>

            <div>
              <button @click="removeEditThenAction(index)" class="btn-delete">🗑️</button>
            </div>
          </div>
        </div>
      </div>

      <!-- Else Actions -->
      <div style="margin-bottom:20px">
        <div class="flex-between" style="margin-bottom:12px">
          <h4 class="label-pink">Ações (SENÃO) - Opcional</h4>
          <button @click="addEditElseAction" class="btn-small btn-pink">Adicionar ação</button>
        </div>

        <div v-for="(action, index) in editRule.senao" :key="index" class="block-pink">
          <div class="grid-action">
            <div>
              <label class="label-pink">Board:</label>
              <select
                v-model="action.id_device"
                @change="fetchSensorsForBoard(action.id_device.replace('esp32_device_', ''))"
                class="select border-pink"
              >
                <option value="" disabled>Selecione a placa...</option>
                <option v-for="board in boards" :key="board.id" :value="`esp32_device_${board.deviceId}`">
                  {{ board.name }} (ID: {{ board.deviceId }})
                </option>
              </select>
            </div>

            <div>
              <label class="label-pink">Actuator:</label>
              <select v-model="action.id_atuador" :disabled="!action.id_device" class="select border-pink">
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
              <label class="label-pink">Modo:</label>
              <select v-model="action.modo" class="select border-pink">
                <option value="set">Set</option>
                <option value="toggle">Toggle</option>
              </select>
            </div>

            <div>
              <label class="label-pink">Valor:</label>
              <input v-model.number="action.valor" type="number" placeholder="0" class="input" />
            </div>

            <div>
              <label class="label-pink">Duração (s):</label>
              <input v-model.number="action.tempo" type="number" placeholder="0" min="0" class="input" />
            </div>

            <div>
              <button @click="removeEditElseAction(index)" class="btn-delete">🗑️</button>
            </div>
          </div>
        </div>
      </div>

      <div class="flex-gap-12">
        <button
          @click="updateRule"
          :disabled="!editRule.id_regra || editRule.condicao.length === 0 || editRule.entao.length === 0"
          class="btn btn-green"
          style="flex:1"
        >
          Atualizar regra
        </button>
        <button @click="cancelEdit" class="btn-delete">Cancelar</button>
      </div>
    </div>

    <!-- Add Rule Form -->
    <div v-if="showAddForm" class="form-add">
      <h3>Criar nova regra</h3>

      <div class="input-block">
        <label class="label-blue" style="display:block; color:#91d5ff; margin-bottom:8px; font-weight:600">ID da Regra:</label>
        <input v-model="newRule.id_regra" placeholder="e.g., rule_temp_alert" class="input input-large" />
      </div>

      <!-- Conditions (New) -->
      <div style="margin-bottom:20px">
        <div class="flex-between" style="margin-bottom:12px">
          <h4 class="label-green">Conditions (IF)</h4>
          <div class="flex-gap-12">
            <button @click="addCondition" class="btn-small btn-green">Condição de limite</button>
            <button @click="addPasswordCondition" class="btn-small btn-green">Condição de senha</button>
          </div>
        </div>

        <div v-for="(cond, index) in newRule.condicao" :key="index" class="block-green">
          <div v-if="cond.tipo === 'limite'" class="grid-cond-limit">
            <div>
              <label class="label-green">Placa:</label>
              <select
                v-model="cond.id_device"
                @change="fetchSensorsForBoard(cond.id_device.replace('esp32_device_', ''))"
                class="select border-green"
              >
                <option value="" disabled>Selecione a placa...</option>
                <option v-for="board in boards" :key="board.id" :value="`esp32_device_${board.deviceId}`">
                  {{ board.name }} (ID: {{ board.deviceId }})
                </option>
              </select>
            </div>

            <div>
              <label class="label-green">Sensor:</label>
              <select v-model="cond.id_sensor" :disabled="!cond.id_device" class="select border-green">
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
              <label class="label-green">Nome do campo:</label>
              <input v-model="cond.medida" placeholder="e.g., x, y, bt, temperature" class="input" />
            </div>

            <div>
              <label class="label-green">Operador:</label>
              <select v-model="cond.operador" class="select border-green">
                <option value=">">></option>
                <option value=">=">>=</option>
                <option value="<"><</option>
                <option value="<="><=</option>
                <option value="==">==</option>
                <option value="!=">!=</option>
              </select>
            </div>

            <div>
              <label class="label-green">Valor:</label>
              <input
                v-if="isStringSensor(cond.id_device, cond.id_sensor)"
                v-model="cond.valor_limite"
                type="text"
                placeholder="ABC123"
                class="input"
              />
              <input
                v-else
                v-model.number="cond.valor_limite"
                type="number"
                placeholder="80"
                class="input"
              />
            </div>

            <div>
              <label class="label-green">Tempo (s):</label>
              <input v-model.number="cond.tempo" type="number" placeholder="5" min="0" class="input" />
            </div>

            <div>
              <button @click="removeCondition(index)" class="btn-delete">🗑️</button>
            </div>
          </div>

          <div v-else-if="cond.tipo === 'senha'" class="grid-cond-pass">
            <div>
              <label class="label-green">Placa:</label>
              <select
                v-model="cond.id_device"
                @change="fetchSensorsForBoard(cond.id_device.replace('esp32_device_', ''))"
                class="select border-green"
              >
                <option value="" disabled>Selecione a placa...</option>
                <option v-for="board in boards" :key="board.id" :value="`esp32_device_${board.deviceId}`">
                  {{ board.name }} (ID: {{ board.deviceId }})
                </option>
              </select>
            </div>

            <div>
              <label class="label-green">Teclado:</label>
              <select v-model="cond.id_sensor" :disabled="!cond.id_device" class="select border-green">
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
              <label class="label-green">Senha:</label>
              <input v-model="cond.senha" placeholder="1234" class="input" />
            </div>

            <div>
              <button @click="removeCondition(index)" class="btn-delete">🗑️</button>
            </div>
          </div>
        </div>
      </div>

      <!-- Then Actions (New) -->
      <div style="margin-bottom:20px">
        <div class="flex-between" style="margin-bottom:12px">
          <h4 class="label-orange">Ações (ENTÃO)</h4>
          <button @click="addThenAction" class="btn-small btn-orange">Adicionar ação</button>
        </div>

        <div v-for="(action, index) in newRule.entao" :key="index" class="block-orange">
          <div class="grid-action">
            <div>
              <label class="label-orange">Placa:</label>
              <select
                v-model="action.id_device"
                @change="fetchSensorsForBoard(action.id_device.replace('esp32_device_', ''))"
                class="select border-orange"
              >
                <option value="" disabled>Selecione a placa...</option>
                <option v-for="board in boards" :key="board.id" :value="`esp32_device_${board.deviceId}`">
                  {{ board.name }} (ID: {{ board.deviceId }})
                </option>
              </select>
            </div>

            <div>
              <label class="label-orange">Atuador:</label>
              <select v-model="action.id_atuador" :disabled="!action.id_device" class="select border-orange">
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
              <label class="label-orange">Modo:</label>
              <select v-model="action.modo" class="select border-orange">
                <option value="set">Set</option>
                <option value="toggle">Toggle</option>
              </select>
            </div>

            <div v-if="action.modo === 'set'">
              <label class="label-orange">Value:</label>
              <input v-model.number="action.valor" type="number" placeholder="1" class="input" />
            </div>

            <div v-else>
              <label class="label-orange">Value:</label>
              <input value="Toggle" disabled class="input input-disabled" />
            </div>

            <div>
              <label class="label-orange">Duração (s):</label>
              <input v-model.number="action.tempo" type="number" placeholder="0" min="0" class="input" />
            </div>

            <div>
              <button @click="removeThenAction(index)" class="btn-delete">🗑️</button>
            </div>
          </div>
        </div>
      </div>

      <!-- Else Actions (New) -->
      <div style="margin-bottom:20px">
        <div class="flex-between" style="margin-bottom:12px">
          <h4 class="label-pink">Ações (SENÃO) - Opcional</h4>
          <button @click="addElseAction" class="btn-small btn-pink">Adicionar Ação</button>
        </div>

        <div v-for="(action, index) in newRule.senao" :key="index" class="block-pink">
          <div class="grid-action">
            <div>
              <label class="label-pink">Placa:</label>
              <select
                v-model="action.id_device"
                @change="fetchSensorsForBoard(action.id_device.replace('esp32_device_', ''))"
                class="select border-pink"
              >
                <option value="" disabled>Selecione a placa...</option>
                <option v-for="board in boards" :key="board.id" :value="`esp32_device_${board.deviceId}`">
                  {{ board.name }} (ID: {{ board.deviceId }})
                </option>
              </select>
            </div>

            <div>
              <label class="label-pink">Atuador:</label>
              <select v-model="action.id_atuador" :disabled="!action.id_device" class="select border-pink">
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
              <label class="label-pink">Valor:</label>
              <input v-model.number="action.valor" type="number" placeholder="0" class="input" />
            </div>

            <div>
              <label class="label-pink">Duração (s):</label>
              <input v-model.number="action.tempo" type="number" placeholder="0" min="0" class="input" />
            </div>

            <div>
              <button @click="removeElseAction(index)" class="btn-delete">🗑️</button>
            </div>
          </div>
        </div>
      </div>

      <button
        @click="createRule"
        :disabled="!newRule.id_regra || newRule.condicao.length === 0 || newRule.entao.length === 0"
        class="btn btn-green"
        style="width:100%"
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
        <li v-for="rule in rules" :key="rule.id_regra" class="block" style="margin-bottom:16px; padding:16px; background:rgba(255,255,255,0.05); border-radius:12px; border-left:4px solid #1890ff">
          <div class="flex-between">
            <div style="flex:1">
              <h3 style="margin:0 0 12px 0; color:#91d5ff">{{ rule.id_regra }}</h3>

              <!-- Conditions -->
              <div style="margin-bottom:12px">
                <strong class="label-green">IF:</strong>
                <div v-for="(cond, idx) in rule.condicao" :key="idx" style="margin-left:20px; color:#bfbfbf; font-size:14px">
                  <span v-if="cond.tipo === 'limite'">- Sensor {{ cond.id_sensor }} ({{ cond.id_device }}) campo "{{ cond.medida }}" {{ cond.operador }} {{ cond.valor_limite }} por {{ cond.tempo }}s</span>
                  <span v-else-if="cond.tipo === 'senha'">- Keypad {{ cond.id_sensor }} ({{ cond.id_device }}) senha: {{ cond.senha }}</span>
                </div>
              </div>

              <!-- Actions -->
              <div style="margin-bottom:12px">
                <strong class="label-orange">THEN:</strong>
                <div v-for="(action, idx) in rule.entao" :key="idx" style="margin-left:20px; color:#bfbfbf; font-size:14px">
                  - Atuador {{ action.id_atuador }} ({{ action.id_device }}) para {{ action.valor }}<span v-if="action.tempo && action.tempo > 0"> por {{ action.tempo }}s</span>
                </div>
              </div>

              <!-- Else Actions -->
              <div v-if="rule.senao && rule.senao.length > 0">
                <strong class="label-pink">ELSE:</strong>
                <div v-for="(action, idx) in rule.senao" :key="idx" style="margin-left:20px; color:#bfbfbf; font-size:14px">
                  - Atuador {{ action.id_atuador }} ({{ action.id_device }}) para {{ action.valor }}<span v-if="action.tempo && action.tempo > 0"> por {{ action.tempo }}s</span>
                </div>
              </div>
            </div>

            <div class="flex-gap-12">
              <button @click="startEdit(rule)" class="btn-small btn-orange">Editar</button>
              <button @click="deleteRule(rule.id_regra)" class="btn-delete">Excluir</button>
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


<style scoped>

/* ============================================================
   LAYOUTS GERAIS — mantém estrutura, atualiza visual fosco
   ============================================================ */

.flex-between {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.flex {
  display: flex;
}

.flex-gap-12 {
  display: flex;
  gap: 12px;
}

.grid-cond-limit,
.grid-cond-pass,
.grid-action {
  gap: 8px;
  align-items: end;
}

.grid-cond-limit {
  display: grid;
  grid-template-columns: repeat(6, 1fr) auto;
}

.grid-cond-pass {
  display: grid;
  grid-template-columns: repeat(3, 1fr) auto;
}

.grid-action {
  display: grid;
  grid-template-columns: 1fr 1fr 100px 1fr 1fr auto;
}


/* ============================================================
   TÍTULOS — mantendo esquema simples
   ============================================================ */

h2, h3, h4 {
  margin: 0;
  color: white;
  text-shadow: 0 2px 4px rgba(0,0,0,0.5);
}


/* ============================================================
   BOTÕES — agora idênticos ao estilo fosco do painel
   ============================================================ */

.btn, .btn-small {
  border: none;
  color: white;
  cursor: pointer;
  font-weight: 600;

  background: rgba(0,0,0,0.45);
  border: 1px solid rgba(255,255,255,0.15);

  box-shadow: 0 3px 10px rgba(0,0,0,0.25);
  transition: 0.15s;
}

.btn {
  padding: 10px 20px;
  border-radius: 10px;
  font-size: 14px;
}

.btn-small {
  padding: 6px 12px;
  border-radius: 8px;
  font-size: 12px;
}

/* Cores foscas */
.btn-green  { background: rgba(82,196,26,0.45); }
.btn-blue   { background: rgba(24,144,255,0.45); }
.btn-orange { background: rgba(250,173,20,0.45); }
.btn-pink   { background: rgba(235,47,150,0.45); }

/* delete */
.btn-delete {
  padding: 6px 10px;
  border: none;
  border-radius: 6px;
  background: rgba(199, 0, 3, 0.35);
  color: #ff7875;
  cursor: pointer;
  font-size: 12px;
}

.btn:hover:not(.btn-disabled) {
  transform: translateY(-2px);
  box-shadow: 0 5px 14px rgba(0,0,0,0.35);
}

.btn-disabled {
  opacity: 0.5;
  cursor: not-allowed;
}


/* ============================================================
   MENSAGENS — convertidas para visual fosco escuro
   ============================================================ */

.msg-success,
.msg-error {
  margin-bottom: 16px;
  padding: 12px;
  border-left-width: 4px;
  border-left-style: solid;
  border-radius: 6px;

  background: rgba(0,0,0,0.35);
  border: 1px solid rgba(255, 215, 215, 0.2);
  color: white;

  box-shadow: 0 2px 10px rgba(0,0,0,0.25);
}

.msg-success { border-left-color: #52c41a; }
.msg-error   { border-left-color: #ff4d4f; }


/* ============================================================
   FORMS — convertidos para caixas foscas uniformes
   ============================================================ */

.form-edit,
.form-add {
  margin-bottom: 24px;
  padding: 20px;
  border-radius: 12px;

  background: rgba(0,0,0,0.35);
  border: 1px solid rgba(255,255,255,0.18);

  box-shadow: 0 3px 12px rgba(0,0,0,0.25);
}

.input-block {
  width: 115%;
  margin-bottom: 16px;
}


/* ============================================================
   INPUTS E SELECTS — agora iguais aos do sensor-select
   ============================================================ */

.input,
.select,
.input-large {
  width: 85%;
  padding: 10px;

  background: rgba(0,0,0,0.35);
  border: 1px solid rgba(255,255,255,0.25);
  color: white;

  border-radius: 10px;
  font-size: 13px;

  box-shadow: inset 0 0 8px rgba(0,0,0,0.25);
}

.input-large {
  font-size: 14px;
}

.input:focus,
.select:focus {
  outline: none;
  border-color: rgba(255,255,255,0.5);
}


/* disabled */
.input-disabled {
  background: rgba(0,0,0,0.5);
  color: #8c8c8c;
  cursor: not-allowed;
}


/* tamanhos específicos */
.input-id   { width: 60%; }
.input-cond { width: 70%; }
.input-act  { width: 70%; }


/* ============================================================
   BLOCKS (condição/ação) — agora iguais aos boxes foscos
   ============================================================ */

.block-green,
.block-orange,
.block-pink {
  margin-bottom: 12px;
  padding: 16px;

  border-radius: 12px;
  background: rgba(0,0,0,0.35);
  border: 1px solid rgba(255,255,255,0.2);

  box-shadow: 0 2px 10px rgba(0,0,0,0.25);
}

/* bordas coloridas foscas */
.block-green  { border-left: 4px solid rgba(82,196,26,0.7); }
.block-orange { border-left: 4px solid rgba(250,173,20,0.7); }
.block-pink   { border-left: 4px solid rgba(235,47,150,0.7); }


/* ============================================================
   LABELS COLORIDAS — versão fosca
   ============================================================ */

.label-green  { color: rgba(149,222,100,0.9); }
.label-orange { color: rgba(255,197,61,0.9); }
.label-pink   { color: rgba(255,133,192,0.9); }

.label-green,
.label-orange,
.label-pink {
  font-size: 12px;
  margin-bottom: 4px;
  display: block;
}


/* ============================================================
   INPUTS COM BORDAS COLORIDAS (tema fosco)
   ============================================================ */

.border-green  { border-color: rgba(82,196,26,0.6); }
.border-orange { border-color: rgba(250,173,20,0.6); }
.border-pink   { border-color: rgba(235,47,150,0.6); }


/* ============================================================
   REGRAS, CONDITIONS, ACTIONS — mantém proporções internas
   ============================================================ */

.rule-id-box input {
  width: 50% !important;
}

.condition-box input,
.condition-box select,
.condition-box .row input,
.condition-box .row select,
.action-box input,
.action-box select,
.action-box .row input,
.action-box .row select {
  width: 70% !important;
  max-width: 70% !important;
  flex: 0 0 70% !important;
}

</style>

