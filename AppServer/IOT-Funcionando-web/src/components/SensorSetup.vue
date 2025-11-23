<script setup>
import { ref, computed, watch, onMounted } from 'vue'

const props = defineProps({
  selectedPin: { type: Object, default: null },
  sensors: { type: Array, default: () => [] },
  deviceId: { type: Number, default: 1 },
  selectedSensor: { type: Object, default: null },
  boards: { type: Array, default: () => [] }
})
const emit = defineEmits(['save-sensor', 'delete-sensor'])

// Target device for sensor configuration (defaults to current board)
const targetDeviceId = ref(props.deviceId)

// Sensor/Actuator types matching ESP32 enum tipo_sensor_t
const SENSOR_TYPES = [
  { value: 0, name: 'MPU6050', category: 'sensor', desc: 'Acelerômetro/Giroscópio' },
  { value: 1, name: 'DS18B20', category: 'sensor', desc: 'Sensor de Temperatura' },
  { value: 2, name: 'HC_SR04', category: 'sensor', desc: 'Sensor Ultrassônico' },
  { value: 3, name: 'APDS_9960', category: 'sensor', desc: 'Sensor de Gestos/Cor' },
  { value: 4, name: 'SG_90', category: 'actuator', desc: 'Servo Motor' },
  { value: 5, name: 'RELE', category: 'actuator', desc: 'Módulo Relé' },
  { value: 6, name: 'JOYSTICK', category: 'sensor', desc: 'Joystick Analógico' },
  { value: 7, name: 'TECLADO_4X4', category: 'sensor', desc: 'Teclado Matricial 4x4' },
  { value: 8, name: 'ENCODER', category: 'sensor', desc: 'Encoder Rotativo' },
  { value: 9, name: 'DHT11', category: 'sensor', desc: 'Sensor de Temperatura/Umidade' }
]

// Pin types matching ESP32 enum estado_pino_t
const PIN_TYPES = [
  { value: 0, name: 'DESATIVADO', desc: 'Desativado' },
  { value: 1, name: 'DIGITAL_INPUT', desc: 'Digital Input' },
  { value: 2, name: 'DIGITAL_OUTPUT', desc: 'Digital Output' },
  { value: 3, name: 'ANALOGICO', desc: 'Analógico' },
  { value: 4, name: 'SCL_', desc: 'I2C SCL' },
  { value: 5, name: 'SDA_', desc: 'I2C SDA' },
  { value: 6, name: 'ONE_WIRE', desc: 'OneWire' }
]

// Pin requirements for each sensor type
const SENSOR_PIN_CONFIG = {
  0: { // MPU6050
    pins: [
      { label: 'SDA (I2C Data)', type: 5, defaultPin: 17 },
      { label: 'SCL (I2C Clock)', type: 4, defaultPin: 18 }
    ]
  },
  1: { // DS18B20
    pins: [
      { label: 'Data (OneWire)', type: 6, defaultPin: 4 }
    ]
  },
  2: { // HC_SR04
    pins: [
      { label: 'Trigger', type: 2, defaultPin: 5 },
      { label: 'Echo', type: 1, defaultPin: 18 }
    ]
  },
  3: { // APDS_9960
    pins: [
      { label: 'SDA (I2C Data)', type: 5, defaultPin: 17 },
      { label: 'SCL (I2C Clock)', type: 4, defaultPin: 18 }
    ]
  },
  4: { // SG_90 (Servo)
    pins: [
      { label: 'Controle PWM', type: 2, defaultPin: 13 }
    ]
  },
  5: { // RELE
    pins: [
      { label: 'Controle', type: 2, defaultPin: 23 }
    ]
  },
  6: { // JOYSTICK
    pins: [
      { label: 'Eixo X (Analógico)', type: 3, defaultPin: 1 },
      { label: 'Eixo Y (Analógico)', type: 3, defaultPin: 2 },
      { label: 'Botão (Digital)', type: 1, defaultPin: 3 }
    ]
  },
  7: { // TECLADO_4X4
    pins: [
      { label: 'Linha 1', type: 2, defaultPin: 8 },
      { label: 'Linha 2', type: 2, defaultPin: 9 },
      { label: 'Linha 3', type: 2, defaultPin: 10 },
      { label: 'Linha 4', type: 2, defaultPin: 11 },
      { label: 'Coluna 1', type: 1, defaultPin: 12 },
      { label: 'Coluna 2', type: 1, defaultPin: 13 },
      { label: 'Coluna 3', type: 1, defaultPin: 14 },
      { label: 'Coluna 4', type: 1, defaultPin: 21 }
    ]
  },
  8: { // ENCODER
    pins: [
      { label: 'CLK', type: 1, defaultPin: 19 },
      { label: 'DT', type: 1, defaultPin: 18 }
    ]
  },
  9: { // DHT11
    pins: [
      { label: 'Data', type: 1, defaultPin: 15 }
    ]
  }
}

// Available ESP32-S3 pins (including all usable and some debug pins)
const AVAILABLE_PINS = [
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 38, 39, 40, 41, 42, 47, 48
]

// Form state
const sensorId = ref(1)
const nextAvailableId = ref(1)
const sensorType = ref(null)
const description = ref('')
const pins = ref([])

// API state
const saving = ref(false)
const error = ref(null)
const successMessage = ref(null)
const warningMessage = ref(null)
const existingSensorIds = ref([])

// Helper function to clear form - defined early so watchers can use it
const clearForm = () => {
  sensorType.value = null
  description.value = ''
  pins.value = []
  error.value = null
  successMessage.value = null
}

// Fetch current sensors to calculate next ID
const fetchSensorsForId = async () => {
  try {
    const mqttDeviceId = `esp32_device_${targetDeviceId.value}`
    const response = await fetch(`http://localhost:5000/${mqttDeviceId}/settings/sensors/get`)
    if (response.ok) {
      const data = await response.json()
      const sensors = data.sensors || []
      
      // busca IDs de sensores se exsitirem
      existingSensorIds.value = sensors.map(s => s.id)
      
      if (sensors.length > 0) {
        const maxId = Math.max(...sensors.map(s => s.id || 0))
        nextAvailableId.value = maxId + 1
        sensorId.value = maxId + 1
      } else {
        nextAvailableId.value = 1
        sensorId.value = 1
      }
    }
  } catch (err) {
    console.error('Não foi possível buscar IDs de sensores já adicionados:', err)
    // defaulta pra 1
    nextAvailableId.value = 1
    sensorId.value = 1
    existingSensorIds.value = []
  }
}

// Computed
const selectedSensorInfo = computed(() => {
  if (sensorType.value === null) return null
  return SENSOR_TYPES.find(s => s.value === sensorType.value)
})

const requiredPins = computed(() => {
  if (sensorType.value === null) return []
  return SENSOR_PIN_CONFIG[sensorType.value]?.pins || []
})

// modifica pinos para selecao quando o sensor mudar
watch(sensorType, (newType) => {
  if (newType !== null) {
    const config = SENSOR_PIN_CONFIG[newType]
    if (config) {
      description.value = SENSOR_TYPES.find(s => s.value === newType)?.name || ''
      pins.value = config.pins.map(pinConfig => ({
        pino: pinConfig.defaultPin,
        tipo: pinConfig.type,
        label: pinConfig.label
      }))
    }
  } else {
    pins.value = []
    description.value = ''
  }
})

// busca proximo ID (quase certo que nao esta funcionando direito)
onMounted(() => {
  if (!props.selectedSensor) {
    fetchSensorsForId()
  }
})

// compara se ID selecionado ja esta sendo usado (acho que não ta funcionando direito)
watch(sensorId, (newId) => {
  // besteirinha pra limpar aviso
  if (props.selectedSensor && props.selectedSensor.id === newId) {
    warningMessage.value = null
    return
  }
  
  // verifica se ID já esta em uso
  if (existingSensorIds.value.includes(newId)) {
    warningMessage.value = `⚠️ Aviso: O ID do sensor ${newId} já está em uso! Salvar irá sobrescrever a configuração existente. Considere deletar o sensor antigo primeiro ou usar o ID ${nextAvailableId.value}.`
  } else {
    warningMessage.value = null
  }
})

// preenche opcoes baseado no sensor
watch(() => props.selectedSensor, (sensor) => {
  if (sensor) {
    // preenche formulario com dados do sensor existente
    sensorId.value = sensor.id
    sensorType.value = sensor.tipo
    description.value = sensor.desc || ''
    
    // preenche pinos se disponíveis
    if (sensor.pinos && Array.isArray(sensor.pinos)) {
      const config = SENSOR_PIN_CONFIG[sensor.tipo]
      if (config) {
        pins.value = sensor.pinos.map((pino, index) => ({
          pino: pino.pino,
          tipo: pino.tipo,
          label: config.pins[index]?.label || `Pin ${index + 1}`
        }))
      }
    }
  } else {
    // Limpa formulário e busca novo ID quando nenhum sensor está selecionado
    clearForm()
    fetchSensorsForId()
  }
}, { immediate: true })

// observa mudanca de device para refazer busca de sensores
watch(targetDeviceId, () => {
  if (!props.selectedSensor) {
    fetchSensorsForId()
  }
})

const save = async () => {
  if (sensorType.value === null) {
    error.value = 'Por favor, selecione um tipo de sensor/atuador'
    return
  }
  
  // valida se todos os pinos estão configurados
  const invalidPin = pins.value.find(p => !p.pino)
  if (invalidPin) {
    error.value = 'Por favor, configure todos os pinos necessários'
    return
  }
  
  saving.value = true
  error.value = null
  successMessage.value = null
  
  try {
    const mqttDeviceId = `esp32_device_${targetDeviceId.value}`
    
    // Prepara payload de configuração do sensor no formato esperado pelo ESP32
    const sensorConfig = {
      id: sensorId.value,
      desc: description.value,
      tipo: sensorType.value,
      pinos: pins.value.map(p => ({
        pino: p.pino,
        tipo: p.tipo
      }))
    }
    
    // Adiciona atributo1 para atuadores (relé e servo) para definir estado inicial
    if (sensorType.value === 4) {
      // Servo: padrão para 90 graus (posição média)
      sensorConfig.atributo1 = 90
    } else if (sensorType.value === 5) {
      // Relay: padrão para desligado (0)
      sensorConfig.atributo1 = 0
    }
    
    const payload = { sensors: [sensorConfig] }
    
    console.log('═══════════════════════════════════════════════════')
    console.log(`Envio de dados para ${mqttDeviceId}`)
    console.log('═══════════════════════════════════════════════════')
    console.log('ID do dispositivo:', mqttDeviceId)
    console.log('Endpoint:', `http://localhost:5000/${mqttDeviceId}/settings/sensors/set`)
    console.log('Tipo de sensor:', selectedSensorInfo.value.name, `(tipo: ${sensorType.value})`)
    console.log('Número de pinos:', pins.value.length)
    console.log('---------------------------------------------------')
    console.log('Payload:')
    console.log(JSON.stringify(payload, null, 2))
    console.log('═══════════════════════════════════════════════════')
    
    // envia pra api.py
    const response = await fetch(`http://localhost:5000/${mqttDeviceId}/settings/sensors/set`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify(payload)
    })
    
    if (!response.ok) {
      const errorData = await response.json().catch(() => ({}))
      console.error('Erro na resposta do ESP32:', response.status, errorData)
      throw new Error(errorData.message || `Falha ao salvar sensor: ${response.status}`)
    }
    
    const result = await response.json()
    console.log('Resposta do ESP32:', result)
    
    // verifica mensagem de resposta (é pra ter OK se ta tudo certo, depois da pra implementar eco na configuração completa)
    if (result.status === 'error' && !result.message?.includes('OK')) {
      console.error('ESP32 retornou erro:', result.message)
      throw new Error(`ESP32 retornou erro: ${result.message}`)
    }
    
    successMessage.value = `${selectedSensorInfo.value.name} configurado com sucesso!`
    console.log('Sensor salvo:', result)
    
    // armazena localmente so pq sim
    emit('save-sensor', sensorConfig)
    
    // limpa tela
    setTimeout(() => {
      successMessage.value = null
      sensorType.value = null
      description.value = ''
      pins.value = []
    }, 2000)
    
  } catch (err) {
    error.value = err.message
    console.error('Erro ao salvar sensor:', err)
  } finally {
    saving.value = false
  }
}

const remove = async () => {
  if (!sensorId.value) return
  
  saving.value = true
  error.value = null
  successMessage.value = null
  
  try {
    const mqttDeviceId = `esp32_device_${targetDeviceId.value}`
    
    console.log('═══════════════════════════════════════════════════')
    console.log(`Remoção de sensor de ${mqttDeviceId}`)
    console.log('═══════════════════════════════════════════════════')
    console.log('ID do dispositivo:', mqttDeviceId)
    console.log('ID do sensor para remover:', sensorId.value)
    console.log('Endpoint:', `http://localhost:5000/${mqttDeviceId}/sensors/remove`)
    console.log('═══════════════════════════════════════════════════')
    
    // envia pedido de remoção para api.py
    const response = await fetch(`http://localhost:5000/${mqttDeviceId}/sensors/remove`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({
        sensor_id: sensorId.value
      })
    })
    
    if (!response.ok) {
      const errorData = await response.json().catch(() => ({}))
      console.error('Erro ao remover sensor:', response.status, errorData)
      throw new Error(errorData.message || `Falha ao remover sensor: ${response.status}`)
    }
    
    const result = await response.json()
    console.log('✅ Sensor removido com sucesso:', result)
    successMessage.value = 'Sensor removido com sucesso!'
    console.log('Sensor removido:', result)
    
    // apagar localmente pq sim
    emit('delete-sensor', sensorId.value)
    
    // limpa tela
    setTimeout(() => {
      successMessage.value = null
      sensorType.value = null
      description.value = ''
      pins.value = []
    }, 2000)
    
  } catch (err) {
    error.value = err.message
    console.error('Erro ao remover sensor:', err)
  } finally {
    saving.value = false
  }
}

</script>

<template>
  <section>
    <h2>Configuração de Sensor / Atuador</h2>

    <!-- Seleção de Placa -->
    <div v-if="boards.length > 1" class="field-block board-selector">
      <label>Dispositivo-alvo:</label>
      <select v-model.number="targetDeviceId" :disabled="saving">
        <option v-for="board in boards" :key="board.id" :value="board.deviceId">
          {{ board.name }} (ID: {{ board.deviceId }})
        </option>
      </select>
      <div class="info-text">Selecione para qual placa adicionar este sensor.</div>
    </div>

    <!-- Mensagem de sucesso -->
    <div v-if="successMessage" class="alert alert-success">
      ✓ {{ successMessage }}
    </div>

    <!-- Mensagem de erro -->
    <div v-if="error" class="alert alert-error">
      ✗ {{ error }}
    </div>

    <!-- Mensagem de aviso -->
    <div v-if="warningMessage" class="alert alert-warning">
      {{ warningMessage }}
    </div>

    <!-- ID do sensor -->
    <div class="field-block">
      <label>ID do sensor</label>
      <input v-model.number="sensorId" type="number" :disabled="saving" />
    <div class="info-text">Identificador único para este sensor/atuador (InfluxDB salvará esse sensor como sensor_{{ sensorId }}).</div>
    </div>

    <!-- modelo de sensor ou atuador -->
    <div class="field-block">
      <label>Tipo de Sensor/Atuador</label>
      <select v-model.number="sensorType" :disabled="saving">
        <option :value="null">-- Selecione modelo --</option>
        <optgroup label="Sensores">
          <option 
            v-for="sensor in SENSOR_TYPES.filter(s => s.category === 'sensor')" 
            :key="sensor.value" 
            :value="sensor.value"
          >
            {{ sensor.name }} - {{ sensor.desc }}
          </option>
        </optgroup>
        <optgroup label="Atuadores">
          <option 
            v-for="sensor in SENSOR_TYPES.filter(s => s.category === 'actuator')" 
            :key="sensor.value" 
            :value="sensor.value"
          >
            {{ sensor.name }} - {{ sensor.desc }}
          </option>
        </optgroup>
      </select>
    </div>

    <!-- Descrição -->
    <div v-if="sensorType !== null" class="field-block">
      <label>Descrição (label a ser salva)</label>
      <input v-model="description" :disabled="saving" placeholder="Descrição opcional" />
    </div>

    <!-- Configuração dos pinos -->
    <div v-if="sensorType !== null && requiredPins.length > 0" class="pins-config">
      <h3>Configuração do pinout</h3>
      <div class="info-box">
        <strong>{{ selectedSensorInfo.name }}</strong> precisa utilizar {{ requiredPins.length }} pino(s)
      </div>

      <div v-for="(pinConfig, index) in pins" :key="index" class="pin-row">
        <div class="pin-label">
          <span class="pin-number">{{ index + 1 }}</span>
          {{ pinConfig.label }}
        </div>
        <div class="pin-select-wrapper">
          <label class="pin-type-label">
            Tipo: <strong>{{ PIN_TYPES.find(t => t.value === pinConfig.tipo)?.desc }}</strong>
          </label>
          <select v-model.number="pinConfig.pino" :disabled="saving">
            <option :value="null">-- Selecione GPIO --</option>
            <option v-for="pin in AVAILABLE_PINS" :key="pin" :value="pin">
              GPIO {{ pin }}
            </option>
          </select>
        </div>
      </div>
    </div>

    <!-- Preview -->
    <div v-if="sensorType !== null && pins.length > 0" class="preview-box">
      <h4>Visualização do payload de configuração</h4>
      <pre>{{ JSON.stringify({
  id: sensorId,
  desc: description,
  tipo: sensorType,
  pinos: pins.map(p => ({ pino: p.pino, tipo: p.tipo }))
}, null, 2) }}</pre>
    </div>

    <!-- botoes -->
    <div class="button-group">
      <button @click="save" :disabled="saving || sensorType === null" class="btn-primary">
        {{ saving ? 'Salvando...' : 'Salvar configuração' }}
      </button>
      <button @click="clearForm" :disabled="saving" class="btn-secondary">
        Cancelar
      </button>
      <button @click="remove" :disabled="saving" class="btn-danger">
        Remover sensor
      </button>
    </div>
  </section>
</template>



<style scoped>
section {
  background: rgba(255, 255, 255, 0.08);
  backdrop-filter: blur(6px);
  border: 1px solid rgba(255, 255, 255, 0.15);
  padding: 20px;
  border-radius: 18px;
  width: 100%;
  box-sizing: border-box;
  color: white;
  font-family: Arial, sans-serif;
  box-shadow: 0 4px 18px rgba(0,0,0,0.25);
}

h2 {
  margin-bottom: 16px;
  font-size: 1.4rem;
  text-shadow: 0 2px 4px rgba(0,0,0,0.5);
}

h3 {
  margin-top: 20px;
  margin-bottom: 12px;
  font-size: 1.2rem;
  color: #ffa940;
  text-shadow: 0 1px 3px rgba(0,0,0,0.5);
}

h4 {
  margin-top: 10px;
  margin-bottom: 8px;
  font-size: 1rem;
  color: #91d5ff;
}

/* Alerts */
.alert {
  padding: 12px 16px;
  border-radius: 10px;
  margin-bottom: 16px;
  font-size: 15px;
  font-weight: 500;
}

.alert-success {
  background: rgba(82, 196, 26, 0.15);
  border-left: 4px solid #52c41a;
  color: #95de64;
}

.alert-error {
  background: rgba(255, 77, 79, 0.15);
  border-left: 4px solid #ff4d4f;
  color: #ff7875;
}

.alert-warning {
  background: rgba(250, 173, 20, 0.15);
  border-left: 4px solid #ffa940;
  color: #ffc53d;
}

/* Field blocks */
.field-block {
  margin-top: 16px;
}

label {
  font-size: 0.95rem;
  font-weight: 600;
  display: block;
  margin-bottom: 6px;
  text-shadow: 0 1px 6px rgba(0,0,0,0.5);
}

input, select {
  width: 100%;
  padding: 10px 12px;
  border-radius: 12px;
  background: rgba(0, 0, 0, 0.35);
  border: 1px solid rgba(255, 255, 255, 0.25);
  color: white;
  font-size: 0.95rem;
  box-shadow: inset 0 0 10px rgba(0,0,0,0.3);
  transition: 0.2s;
}

input:focus, select:focus {
  outline: none;
  border-color: rgba(255,255,255,0.5);
  background: rgba(0, 0, 0, 0.45);
}

input:disabled, select:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.info-text {
  font-size: 12px;
  color: #bfbfbf;
  margin-top: 6px;
}

/* Board Selector */
.board-selector {
  padding: 14px;
  background: rgba(24, 144, 255, 0.08);
  border: 1px solid rgba(24, 144, 255, 0.25);
  border-radius: 14px;
  margin-bottom: 20px;
}

.board-selector label {
  color: #91d5ff;
  font-size: 1rem;
}

.board-selector select {
  background: rgba(0, 0, 0, 0.45);
  border-color: rgba(24, 144, 255, 0.35);
}

.board-selector select:focus {
  border-color: rgba(24, 144, 255, 0.7);
  box-shadow: 0 0 0 2px rgba(24, 144, 255, 0.1);
}

/* Pins Configuration */
.pins-config {
  margin-top: 24px;
  padding: 16px;
  background: rgba(0, 0, 0, 0.2);
  border-radius: 12px;
  border: 1px solid rgba(255, 255, 255, 0.1);
}

.info-box {
  padding: 10px 12px;
  background: rgba(24, 144, 255, 0.15);
  border-left: 4px solid #1890ff;
  border-radius: 8px;
  margin-bottom: 16px;
  font-size: 14px;
  color: #91d5ff;
}

.pin-row {
  display: grid;
  grid-template-columns: 200px 1fr;
  gap: 12px;
  align-items: center;
  margin-bottom: 12px;
  padding: 12px;
  background: rgba(255, 255, 255, 0.03);
  border-radius: 8px;
  border: 1px solid rgba(255, 255, 255, 0.08);
}

.pin-label {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 14px;
  font-weight: 500;
}

.pin-number {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 24px;
  height: 24px;
  background: rgba(255, 169, 64, 0.25);
  border-radius: 50%;
  font-size: 12px;
  font-weight: 700;
  color: #ffa940;
}

.pin-select-wrapper {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.pin-type-label {
  font-size: 11px;
  color: #8c8c8c;
  text-transform: uppercase;
  margin: 0;
}

.pin-type-label strong {
  color: #d9d9d9;
}

/* Preview Box */
.preview-box {
  margin-top: 20px;
  padding: 14px;
  background: rgba(0, 0, 0, 0.4);
  border-radius: 10px;
  border: 1px solid rgba(255, 255, 255, 0.1);
}

.preview-box pre {
  margin: 8px 0 0 0;
  padding: 12px;
  background: rgba(0, 0, 0, 0.5);
  border-radius: 8px;
  color: #95de64;
  font-size: 12px;
  overflow-x: auto;
  font-family: 'Courier New', monospace;
}

/* Buttons */
.button-group {
  margin-top: 20px;
  display: flex;
  gap: 10px;
  flex-wrap: wrap;
}

button {
  padding: 12px 24px;
  border: none;
  border-radius: 30px;
  font-size: 15px;
  font-weight: 600;
  color: white;
  cursor: pointer;
  box-shadow: 0 3px 10px rgba(0,0,0,0.25);
  transition: 0.2s;
  white-space: nowrap;
}

.btn-primary {
  background: linear-gradient(135deg, #1890ff, #096dd9);
  min-width: 180px;
}

.btn-primary:hover:not(:disabled) {
  transform: translateY(-2px);
  box-shadow: 0 5px 14px rgba(24, 144, 255, 0.4);
}

.btn-secondary {
  background: rgba(255, 255, 255, 0.15);
}

.btn-secondary:hover:not(:disabled) {
  background: rgba(255, 255, 255, 0.25);
  transform: translateY(-2px);
}

.btn-danger {
  background: rgba(255, 77, 79, 0.25);
}

.btn-danger:hover:not(:disabled) {
  background: rgba(255, 77, 79, 0.4);
  transform: translateY(-2px);
  box-shadow: 0 5px 14px rgba(255, 77, 79, 0.3);
}

button:disabled {
  opacity: 0.5;
  cursor: not-allowed;
  box-shadow: none;
}

button:disabled:hover {
  transform: none;
}

/* Responsive */
@media (max-width: 768px) {
  .pin-row {
    grid-template-columns: 1fr;
  }
}
</style>
