<script setup>
import { ref, computed, watch, onMounted } from 'vue'

const props = defineProps({
  selectedPin: { type: Object, default: null },
  sensors: { type: Array, default: () => [] },
  deviceId: { type: Number, default: 1 },
  selectedSensor: { type: Object, default: null }
})
const emit = defineEmits(['save-sensor', 'delete-sensor'])

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
  { value: 8, name: 'IR', category: 'sensor', desc: 'Receptor IR' },
  { value: 9, name: 'ENCODER', category: 'sensor', desc: 'Encoder Rotativo' }
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
      { label: 'SDA (I2C Data)', type: 5, defaultPin: 21 },
      { label: 'SCL (I2C Clock)', type: 4, defaultPin: 22 }
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
      { label: 'SDA (I2C Data)', type: 5, defaultPin: 21 },
      { label: 'SCL (I2C Clock)', type: 4, defaultPin: 22 }
    ]
  },
  4: { // SG_90 (Servo)
    pins: [
      { label: 'PWM Control', type: 2, defaultPin: 13 }
    ]
  },
  5: { // RELE
    pins: [
      { label: 'Control', type: 2, defaultPin: 23 }
    ]
  },
  6: { // JOYSTICK
    pins: [
      { label: 'Eixo X (Analog)', type: 3, defaultPin: 34 },
      { label: 'Eixo Y (Analog)', type: 3, defaultPin: 35 },
      { label: 'Botão (Digital)', type: 1, defaultPin: 32 }
    ]
  },
  7: { // TECLADO_4X4
    pins: [
      { label: 'Linha 1', type: 2, defaultPin: 13 },
      { label: 'Linha 2', type: 2, defaultPin: 12 },
      { label: 'Linha 3', type: 2, defaultPin: 14 },
      { label: 'Linha 4', type: 2, defaultPin: 27 },
      { label: 'Coluna 1', type: 1, defaultPin: 26 },
      { label: 'Coluna 2', type: 1, defaultPin: 25 },
      { label: 'Coluna 3', type: 1, defaultPin: 33 },
      { label: 'Coluna 4', type: 1, defaultPin: 32 }
    ]
  },
  8: { // IR
    pins: [
      { label: 'Data', type: 1, defaultPin: 15 }
    ]
  },
  9: { // ENCODER
    pins: [
      { label: 'CLK', type: 1, defaultPin: 19 },
      { label: 'DT', type: 1, defaultPin: 18 }
    ]
  }
}

// Available ESP32 pins (excluding reserved/strapping pins)
const AVAILABLE_PINS = [
  0, 2, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33, 34, 35, 36, 39
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
    const mqttDeviceId = `esp32_device_${props.deviceId}`
    const response = await fetch(`http://localhost:5000/${mqttDeviceId}/settings/sensors/get`)
    if (response.ok) {
      const data = await response.json()
      const sensors = data.sensors || []
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
    console.error('Error fetching sensors for ID calculation:', err)
    // Default to 1 if fetch fails
    nextAvailableId.value = 1
    sensorId.value = 1
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

// Watch sensor type changes to initialize pins
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

// Fetch sensors on mount to get correct next ID
onMounted(() => {
  if (!props.selectedSensor) {
    fetchSensorsForId()
  }
})

// Watch selectedSensor to populate form when editing
watch(() => props.selectedSensor, (sensor) => {
  if (sensor) {
    // Populate form with existing sensor data
    sensorId.value = sensor.id
    sensorType.value = sensor.tipo
    description.value = sensor.desc || ''
    
    // Populate pins if available
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
    // Clear form and fetch new ID when no sensor is selected
    clearForm()
    fetchSensorsForId()
  }
}, { immediate: true })

const save = async () => {
  if (sensorType.value === null) {
    error.value = 'Please select a sensor/actuator type'
    return
  }
  
  // Validate all pins are selected
  const invalidPin = pins.value.find(p => !p.pino)
  if (invalidPin) {
    error.value = 'Please configure all required pins'
    return
  }
  
  saving.value = true
  error.value = null
  successMessage.value = null
  
  try {
    const mqttDeviceId = `esp32_device_${props.deviceId}`
    
    // Prepare sensor configuration payload matching ESP32 format
    const sensorConfig = {
      id: sensorId.value,
      desc: description.value,
      tipo: sensorType.value,
      pinos: pins.value.map(p => ({
        pino: p.pino,
        tipo: p.tipo
      }))
    }
    
    const payload = { sensors: [sensorConfig] }
    
    console.log('═══════════════════════════════════════════════════')
    console.log('📤 SENDING TO ESP32 BOARD')
    console.log('═══════════════════════════════════════════════════')
    console.log('Device ID:', mqttDeviceId)
    console.log('Endpoint:', `http://localhost:5000/${mqttDeviceId}/settings/sensors/set`)
    console.log('Sensor Type:', selectedSensorInfo.value.name, `(tipo: ${sensorType.value})`)
    console.log('Number of Pins:', pins.value.length)
    console.log('---------------------------------------------------')
    console.log('Full Payload:')
    console.log(JSON.stringify(payload, null, 2))
    console.log('═══════════════════════════════════════════════════')
    
    // Send sensor configuration via API
    const response = await fetch(`http://localhost:5000/${mqttDeviceId}/settings/sensors/set`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify(payload)
    })
    
    if (!response.ok) {
      const errorData = await response.json().catch(() => ({}))
      console.error('❌ ESP32 Response Error:', response.status, errorData)
      throw new Error(errorData.message || `Failed to save sensor: ${response.status}`)
    }
    
    const result = await response.json()
    console.log('✅ ESP32 Response Success:', result)
    successMessage.value = `${selectedSensorInfo.value.name} configured successfully!`
    console.log('Sensor saved:', result)
    
    // Also emit to parent for local state management
    emit('save-sensor', sensorConfig)
    
    // Clear form after success and increment ID
    setTimeout(() => {
      successMessage.value = null
      sensorType.value = null
      description.value = ''
      pins.value = []
      // ID will auto-increment via the watcher when sensors prop updates
    }, 2000)
    
  } catch (err) {
    error.value = err.message
    console.error('Error saving sensor:', err)
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
    const mqttDeviceId = `esp32_device_${props.deviceId}`
    
    console.log('═══════════════════════════════════════════════════')
    console.log('🗑️ REMOVING SENSOR FROM ESP32 BOARD')
    console.log('═══════════════════════════════════════════════════')
    console.log('Device ID:', mqttDeviceId)
    console.log('Sensor ID to Remove:', sensorId.value)
    console.log('Endpoint:', `http://localhost:5000/${mqttDeviceId}/sensors/remove`)
    console.log('═══════════════════════════════════════════════════')
    
    // Send remove request to API
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
      console.error('❌ Remove Sensor Error:', response.status, errorData)
      throw new Error(errorData.message || `Failed to remove sensor: ${response.status}`)
    }
    
    const result = await response.json()
    console.log('✅ Sensor Removed Successfully:', result)
    successMessage.value = 'Sensor removed successfully!'
    console.log('Sensor removed:', result)
    
    // Also emit to parent for local state management
    emit('delete-sensor', sensorId.value)
    
    // Reset form
    setTimeout(() => {
      successMessage.value = null
      sensorType.value = null
      description.value = ''
      pins.value = []
    }, 2000)
    
  } catch (err) {
    error.value = err.message
    console.error('Error removing sensor:', err)
  } finally {
    saving.value = false
  }
}

</script>

<template>
  <section>
    <h2>Sensor / Actuator Setup</h2>

    <!-- Success message -->
    <div v-if="successMessage" class="alert alert-success">
      ✓ {{ successMessage }}
    </div>

    <!-- Error message -->
    <div v-if="error" class="alert alert-error">
      ✗ {{ error }}
    </div>

    <!-- Sensor ID -->
    <div class="field-block">
      <label>Sensor ID</label>
      <input v-model.number="sensorId" type="number" :disabled="saving" />
      <div class="info-text">Unique identifier for this sensor/actuator</div>
    </div>

    <!-- Sensor Type Selection -->
    <div class="field-block">
      <label>Sensor/Actuator Type</label>
      <select v-model.number="sensorType" :disabled="saving">
        <option :value="null">-- Select Type --</option>
        <optgroup label="Sensors">
          <option 
            v-for="sensor in SENSOR_TYPES.filter(s => s.category === 'sensor')" 
            :key="sensor.value" 
            :value="sensor.value"
          >
            {{ sensor.name }} - {{ sensor.desc }}
          </option>
        </optgroup>
        <optgroup label="Actuators">
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

    <!-- Description -->
    <div v-if="sensorType !== null" class="field-block">
      <label>Description</label>
      <input v-model="description" :disabled="saving" placeholder="Optional description" />
    </div>

    <!-- Dynamic Pin Configuration -->
    <div v-if="sensorType !== null && requiredPins.length > 0" class="pins-config">
      <h3>Pin Configuration</h3>
      <div class="info-box">
        <strong>{{ selectedSensorInfo.name }}</strong> requires {{ requiredPins.length }} pin(s)
      </div>

      <div v-for="(pinConfig, index) in pins" :key="index" class="pin-row">
        <div class="pin-label">
          <span class="pin-number">{{ index + 1 }}</span>
          {{ pinConfig.label }}
        </div>
        <div class="pin-select-wrapper">
          <label class="pin-type-label">
            Pin Type: <strong>{{ PIN_TYPES.find(t => t.value === pinConfig.tipo)?.desc }}</strong>
          </label>
          <select v-model.number="pinConfig.pino" :disabled="saving">
            <option :value="null">-- Select GPIO --</option>
            <option v-for="pin in AVAILABLE_PINS" :key="pin" :value="pin">
              GPIO {{ pin }}
            </option>
          </select>
        </div>
      </div>
    </div>

    <!-- Preview -->
    <div v-if="sensorType !== null && pins.length > 0" class="preview-box">
      <h4>Configuration Preview</h4>
      <pre>{{ JSON.stringify({
  id: sensorId,
  desc: description,
  tipo: sensorType,
  pinos: pins.map(p => ({ pino: p.pino, tipo: p.tipo }))
}, null, 2) }}</pre>
    </div>

    <!-- Action Buttons -->
    <div class="button-group">
      <button @click="save" :disabled="saving || sensorType === null" class="btn-primary">
        {{ saving ? 'Saving...' : 'Save Configuration' }}
      </button>
      <button @click="clearForm" :disabled="saving" class="btn-secondary">
        Clear
      </button>
      <button @click="remove" :disabled="saving" class="btn-danger">
        Remove Sensor
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
