<script setup>
import { ref, onMounted } from 'vue'

const props = defineProps({
  deviceId: { type: Number, default: 1 }
})

const rules = ref([])
const loading = ref(false)
const error = ref(null)
const showAddForm = ref(false)

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

// Load boards from localStorage
const loadBoards = () => {
  try {
    const stored = localStorage.getItem('esp32_boards')
    boards.value = stored ? JSON.parse(stored) : []
    console.log('📋 Loaded boards:', boards.value)
  } catch (e) {
    console.error('Error loading boards:', e)
    boards.value = []
  }
}

// Save sensors to localStorage
const saveSensorsToStorage = () => {
  try {
    localStorage.setItem(SENSORS_STORAGE_KEY, JSON.stringify(boardSensors.value))
    console.log('💾 Saved sensors to localStorage')
  } catch (e) {
    console.error('Error saving sensors to localStorage:', e)
  }
}

// Load sensors from localStorage
const loadSensorsFromStorage = () => {
  try {
    const stored = localStorage.getItem(SENSORS_STORAGE_KEY)
    if (stored) {
      boardSensors.value = JSON.parse(stored)
      console.log('📦 Loaded cached sensors:', Object.keys(boardSensors.value))
      return true
    }
  } catch (e) {
    console.error('Error loading sensors from localStorage:', e)
  }
  return false
}

// Fetch sensors for a specific board
const fetchSensorsForBoard = async (deviceId) => {
  const mqttDeviceId = `esp32_device_${deviceId}`
  
  if (loadingSensors.value[mqttDeviceId]) return // Already loading
  
  loadingSensors.value[mqttDeviceId] = true
  
  try {
    const response = await fetch(`http://localhost:5000/${mqttDeviceId}/settings/sensors/get`)
    
    if (!response.ok) {
      throw new Error(`Failed to fetch sensors: ${response.status}`)
    }
    
    const data = await response.json()
    console.log(`📡 Raw response for ${mqttDeviceId}:`, data)
    
    // Handle response - could be { sensors: [...] } or just [...]
    boardSensors.value[mqttDeviceId] = data.sensors || data || []
    console.log(`✅ Loaded ${boardSensors.value[mqttDeviceId].length} sensors for ${mqttDeviceId}`)
    
    // Save to localStorage
    saveSensorsToStorage()
  } catch (err) {
    console.error(`❌ Error fetching sensors for ${mqttDeviceId}:`, err)
    // Keep cached data if fetch fails
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

// Refresh all sensors for all boards
const refreshAllSensors = async () => {
  console.log('🔄 Refreshing sensors for all boards...')
  for (const board of boards.value) {
    await fetchSensorsForBoard(board.deviceId)
  }
  console.log('✅ All sensors refreshed')
}

// Fetch existing rules
const fetchRules = async () => {
  loading.value = true
  error.value = null
  try {
    const response = await fetch('/rules', {
      method: 'GET',
      headers: { 'Content-Type': 'application/json' }
    })
    
    if (!response.ok) {
      throw new Error(`Failed to fetch rules: ${response.status}`)
    }
    
    const data = await response.json()
    rules.value = data.rules || []
    console.log('📋 Loaded rules:', rules.value)
  } catch (err) {
    error.value = err.message
    console.error('Error fetching rules:', err)
  } finally {
    loading.value = false
  }
}

// Create new rule
const createRule = async () => {
  try {
    const response = await fetch('/rules', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(newRule.value)
    })
    
    if (!response.ok) {
      throw new Error(`Failed to create rule: ${response.status}`)
    }
    
    console.log('✅ Rule created successfully')
    showAddForm.value = false
    resetNewRule()
    await fetchRules()
  } catch (err) {
    alert(`Error creating rule: ${err.message}`)
    console.error('Error creating rule:', err)
  }
}

// Delete rule
const deleteRule = async (ruleId) => {
  if (!confirm(`Delete rule "${ruleId}"? This cannot be undone.`)) {
    return
  }
  
  try {
    const response = await fetch('/rules', {
      method: 'DELETE',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ id_regra: ruleId })
    })
    
    if (!response.ok) {
      throw new Error(`Failed to delete rule: ${response.status}`)
    }
    
    console.log('✅ Rule deleted successfully')
    await fetchRules()
  } catch (err) {
    alert(`Error deleting rule: ${err.message}`)
    console.error('Error deleting rule:', err)
  }
}

// Add condition to new rule
const addCondition = () => {
  newRule.value.condicao.push({
    tipo: 'limite',
    tempo: 5,
    id_device: '',
    id_sensor: '',
    medida: 'value_0',
    operador: '>',
    valor_limite: 0
  })
}

// Add password condition
const addPasswordCondition = () => {
  newRule.value.condicao.push({
    tipo: 'senha',
    id_device: '',
    id_sensor: '',
    senha: ''
  })
}

// Add action to "then" branch
const addThenAction = () => {
  newRule.value.entao.push({
    id_device: '',
    id_atuador: '',
    valor: 0,
    tempo: 0
  })
}

// Remove condition
const removeCondition = (index) => {
  newRule.value.condicao.splice(index, 1)
}

// Remove action
const removeThenAction = (index) => {
  newRule.value.entao.splice(index, 1)
}

// Add action to "else" branch
const addElseAction = () => {
  newRule.value.senao.push({
    id_device: '',
    id_atuador: '',
    valor: 0,
    tempo: 0
  })
}

// Remove else action
const removeElseAction = (index) => {
  newRule.value.senao.splice(index, 1)
}

// Reset form
const resetNewRule = () => {
  newRule.value = {
    id_regra: '',
    condicao: [],
    entao: [],
    senao: []
  }
}

onMounted(() => {
  loadBoards()
  
  // Load cached sensors first for instant UI
  const hasCachedSensors = loadSensorsFromStorage()
  
  fetchRules()
  
  // Refresh sensors in background for all boards
  boards.value.forEach(board => {
    fetchSensorsForBoard(board.deviceId)
  })
  
  if (!hasCachedSensors && boards.value.length > 0) {
    console.log('⚠️ No cached sensors found. First-time load may take a moment...')
  }
})
</script>

<template>
  <section>
    <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:24px">
      <h2 style="margin:0">Automation Rules</h2>
      <div style="display:flex; gap:12px">
        <button 
          @click="refreshAllSensors"
          :disabled="Object.values(loadingSensors).some(v => v)"
          style="padding:10px 20px; border-radius:8px; border:none; background:#52c41a; color:white; cursor:pointer; font-weight:600; font-size:14px; opacity: 1"
          :style="{ opacity: Object.values(loadingSensors).some(v => v) ? 0.5 : 1 }"
        >
          🔄 Refresh Sensors
        </button>
        <button 
          @click="showAddForm = !showAddForm"
          style="padding:10px 20px; border-radius:8px; border:none; background:#1890ff; color:white; cursor:pointer; font-weight:600; font-size:14px"
        >
          {{ showAddForm ? '❌ Cancel' : '➕ New Rule' }}
        </button>
      </div>
    </div>

    <!-- Add Rule Form -->
    <div v-if="showAddForm" style="margin-bottom:24px; padding:20px; background:rgba(24,144,255,0.1); border-radius:12px; border:1px solid rgba(24,144,255,0.3)">
      <h3 style="margin-top:0; color:#91d5ff">Create New Rule</h3>
      
      <div style="margin-bottom:16px">
        <label style="display:block; color:#91d5ff; font-weight:600; margin-bottom:8px">Rule ID:</label>
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
              ➕ Limit Condition
            </button>
            <button @click="addPasswordCondition" style="padding:6px 12px; border-radius:6px; border:none; background:#52c41a; color:white; cursor:pointer; font-weight:600; font-size:12px">
              🔑 Password Condition
            </button>
          </div>
        </div>
        
        <div v-for="(cond, index) in newRule.condicao" :key="index" style="margin-bottom:12px; padding:12px; background:rgba(82,196,26,0.1); border-radius:8px; border:1px solid rgba(82,196,26,0.3)">
          <!-- Limit Condition -->
          <div v-if="cond.tipo === 'limite'" style="display:grid; grid-template-columns: 1fr 1fr 1fr 1fr 1fr 1fr auto; gap:8px; align-items:end">
            <div>
              <label style="display:block; color:#95de64; font-size:12px; margin-bottom:4px">Board:</label>
              <select 
                v-model="cond.id_device" 
                @change="fetchSensorsForBoard(cond.id_device.replace('esp32_device_', ''))"
                style="width:100%; padding:6px; border-radius:4px; border:1px solid #52c41a; background:rgba(0,0,0,0.3); color:#fff; font-size:13px"
              >
                <option value="" disabled>Select board...</option>
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
                <option value="" disabled>{{ cond.id_device ? 'Select sensor...' : 'Select board first' }}</option>
                <option 
                  v-for="sensor in getSensorsForDevice(cond.id_device?.replace('esp32_device_', ''))"
                  :key="sensor.id" 
                  :value="sensor.id"
                >
                  {{ sensor.desc || `Sensor ${sensor.id}` }} (Type: {{ sensor.tipo }})
                </option>
              </select>
            </div>
            <div>
              <label style="display:block; color:#95de64; font-size:12px; margin-bottom:4px">Measurement:</label>
              <select v-model="cond.medida" style="width:100%; padding:6px; border-radius:4px; border:1px solid #52c41a; background:rgba(0,0,0,0.3); color:#fff; font-size:13px">
                <option value="value_0">value_0</option>
                <option value="value_1">value_1</option>
                <option value="value_2">value_2</option>
                <option value="value_3">value_3</option>
                <option value="value">value</option>
              </select>
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
              <input v-model.number="cond.valor_limite" type="number" placeholder="80" style="width:100%; padding:6px; border-radius:4px; border:1px solid #52c41a; background:rgba(0,0,0,0.3); color:#fff; font-size:13px" />
            </div>
            <div>
              <label style="display:block; color:#95de64; font-size:12px; margin-bottom:4px">Time (s):</label>
              <input v-model.number="cond.tempo" type="number" placeholder="5" style="width:100%; padding:6px; border-radius:4px; border:1px solid #52c41a; background:rgba(0,0,0,0.3); color:#fff; font-size:13px" />
            </div>
            <button @click="removeCondition(index)" style="padding:6px 10px; border-radius:4px; border:none; background:rgba(255,77,79,0.3); color:#ff7875; cursor:pointer; font-size:12px">🗑️</button>
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
                <option value="" disabled>Select board...</option>
                <option v-for="board in boards" :key="board.id" :value="`esp32_device_${board.deviceId}`">
                  {{ board.name }} (ID: {{ board.deviceId }})
                </option>
              </select>
            </div>
            <div>
              <label style="display:block; color:#95de64; font-size:12px; margin-bottom:4px">Keypad Sensor:</label>
              <select 
                v-model="cond.id_sensor" 
                :disabled="!cond.id_device"
                style="width:100%; padding:6px; border-radius:4px; border:1px solid #52c41a; background:rgba(0,0,0,0.3); color:#fff; font-size:13px"
              >
                <option value="" disabled>{{ cond.id_device ? 'Select keypad...' : 'Select board first' }}</option>
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
            <button @click="removeCondition(index)" style="padding:6px 10px; border-radius:4px; border:none; background:rgba(255,77,79,0.3); color:#ff7875; cursor:pointer; font-size:12px">🗑️</button>
          </div>
        </div>
      </div>

      <!-- Then Actions -->
      <div style="margin-bottom:20px">
        <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:12px">
          <h4 style="margin:0; color:#ffc53d">Actions (THEN)</h4>
          <button @click="addThenAction" style="padding:6px 12px; border-radius:6px; border:none; background:#faad14; color:white; cursor:pointer; font-weight:600; font-size:12px">
            ➕ Add Action
          </button>
        </div>
        
        <div v-for="(action, index) in newRule.entao" :key="index" style="margin-bottom:12px; padding:12px; background:rgba(250,173,20,0.1); border-radius:8px; border:1px solid rgba(250,173,20,0.3)">
          <div style="display:grid; grid-template-columns: 1fr 1fr 1fr 1fr auto; gap:8px; align-items:end">
            <div>
              <label style="display:block; color:#ffc53d; font-size:12px; margin-bottom:4px">Board:</label>
              <select 
                v-model="action.id_device" 
                @change="fetchSensorsForBoard(action.id_device.replace('esp32_device_', ''))"
                style="width:100%; padding:6px; border-radius:4px; border:1px solid #faad14; background:rgba(0,0,0,0.3); color:#fff; font-size:13px"
              >
                <option value="" disabled>Select board...</option>
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
                <option value="" disabled>{{ action.id_device ? 'Select actuator...' : 'Select board first' }}</option>
                <option 
                  v-for="actuator in getSensorsForDevice(action.id_device?.replace('esp32_device_', '')).filter(s => s.tipo === 4 || s.tipo === 5)"
                  :key="actuator.id" 
                  :value="actuator.id"
                >
                  {{ actuator.desc || `Actuator ${actuator.id}` }} (Type: {{ actuator.tipo === 4 ? 'Servo' : 'Relay' }})
                </option>
              </select>
            </div>
            <div>
              <label style="display:block; color:#ffc53d; font-size:12px; margin-bottom:4px">Value:</label>
              <input v-model.number="action.valor" type="number" placeholder="1" style="width:100%; padding:6px; border-radius:4px; border:1px solid #faad14; background:rgba(0,0,0,0.3); color:#fff; font-size:13px" />
            </div>
            <div>
              <label style="display:block; color:#ffc53d; font-size:12px; margin-bottom:4px">Duration (s):</label>
              <input v-model.number="action.tempo" type="number" placeholder="0" min="0" title="0 = permanent, >0 = temporary (auto-revert to 0)" style="width:100%; padding:6px; border-radius:4px; border:1px solid #faad14; background:rgba(0,0,0,0.3); color:#fff; font-size:13px" />
            </div>
            <button @click="removeThenAction(index)" style="padding:6px 10px; border-radius:4px; border:none; background:rgba(255,77,79,0.3); color:#ff7875; cursor:pointer; font-size:12px">🗑️</button>
          </div>
        </div>
      </div>

      <!-- Else Actions -->
      <div style="margin-bottom:20px">
        <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:12px">
          <h4 style="margin:0; color:#ff85c0">Actions (ELSE) - Optional</h4>
          <button @click="addElseAction" style="padding:6px 12px; border-radius:6px; border:none; background:#eb2f96; color:white; cursor:pointer; font-weight:600; font-size:12px">
            ➕ Add Action
          </button>
        </div>
        
        <div v-for="(action, index) in newRule.senao" :key="index" style="margin-bottom:12px; padding:12px; background:rgba(235,47,150,0.1); border-radius:8px; border:1px solid rgba(235,47,150,0.3)">
          <div style="display:grid; grid-template-columns: 1fr 1fr 1fr 1fr auto; gap:8px; align-items:end">
            <div>
              <label style="display:block; color:#ff85c0; font-size:12px; margin-bottom:4px">Board:</label>
              <select 
                v-model="action.id_device" 
                @change="fetchSensorsForBoard(action.id_device.replace('esp32_device_', ''))"
                style="width:100%; padding:6px; border-radius:4px; border:1px solid #eb2f96; background:rgba(0,0,0,0.3); color:#fff; font-size:13px"
              >
                <option value="" disabled>Select board...</option>
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
                <option value="" disabled>{{ action.id_device ? 'Select actuator...' : 'Select board first' }}</option>
                <option 
                  v-for="actuator in getSensorsForDevice(action.id_device?.replace('esp32_device_', '')).filter(s => s.tipo === 4 || s.tipo === 5)"
                  :key="actuator.id" 
                  :value="actuator.id"
                >
                  {{ actuator.desc || `Actuator ${actuator.id}` }} (Type: {{ actuator.tipo === 4 ? 'Servo' : 'Relay' }})
                </option>
              </select>
            </div>
            <div>
              <label style="display:block; color:#ff85c0; font-size:12px; margin-bottom:4px">Value:</label>
              <input v-model.number="action.valor" type="number" placeholder="0" style="width:100%; padding:6px; border-radius:4px; border:1px solid #eb2f96; background:rgba(0,0,0,0.3); color:#fff; font-size:13px" />
            </div>
            <div>
              <label style="display:block; color:#ff85c0; font-size:12px; margin-bottom:4px">Duration (s):</label>
              <input v-model.number="action.tempo" type="number" placeholder="0" min="0" title="0 = permanent, >0 = temporary (auto-revert to 0)" style="width:100%; padding:6px; border-radius:4px; border:1px solid #eb2f96; background:rgba(0,0,0,0.3); color:#fff; font-size:13px" />
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
        ✅ Create Rule
      </button>
    </div>

    <!-- Rules List -->
    <div v-if="loading">
      <p>Loading rules...</p>
    </div>

    <div v-else-if="error">
      <p style="color:#e53e3e">Error: {{ error }}</p>
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
                  <span v-if="cond.tipo === 'limite'">• Sensor {{ cond.id_sensor }} ({{ cond.id_device }}) {{ cond.medida }} {{ cond.operador }} {{ cond.valor_limite }} for {{ cond.tempo }}s</span>
                  <span v-else-if="cond.tipo === 'senha'">🔑 Keypad {{ cond.id_sensor }} ({{ cond.id_device }}) password: {{ cond.senha }}</span>
                </div>
              </div>
              
              <!-- Actions -->
              <div style="margin-bottom:12px">
                <strong style="color:#ffc53d">THEN:</strong>
                <div v-for="(action, idx) in rule.entao" :key="idx" style="margin-left:20px; color:#bfbfbf; font-size:14px">
                  • Set actuator {{ action.id_atuador }} ({{ action.id_device }}) to {{ action.valor }}<span v-if="action.tempo && action.tempo > 0"> for {{ action.tempo }}s</span>
                </div>
              </div>
              
              <!-- Else Actions -->
              <div v-if="rule.senao && rule.senao.length > 0">
                <strong style="color:#ff85c0">ELSE:</strong>
                <div v-for="(action, idx) in rule.senao" :key="idx" style="margin-left:20px; color:#bfbfbf; font-size:14px">
                  • Set actuator {{ action.id_atuador }} ({{ action.id_device }}) to {{ action.valor }}<span v-if="action.tempo && action.tempo > 0"> for {{ action.tempo }}s</span>
                </div>
              </div>
            </div>
            
            <button 
              @click="deleteRule(rule.id_regra)"
              style="padding:8px 16px; border-radius:8px; border:none; background:rgba(255,77,79,0.25); color:#ff7875; cursor:pointer; font-weight:600; font-size:13px"
            >
              🗑️ Delete
            </button>
          </div>
        </li>
      </ul>
    </div>

    <div v-else>
      <p>No automation rules configured. Click "New Rule" to create one.</p>
    </div>
  </section>
</template>
