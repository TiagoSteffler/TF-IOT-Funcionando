<script setup>
import { ref, computed, onMounted, watch } from 'vue'

const props = defineProps({
  sensors: { type: Array, default: () => [] },
  deviceId: { type: Number, default: 1 }
})
const emit = defineEmits(['open-setup', 'open-readings', 'delete-sensor'])

const loading = ref(false)
const error = ref(null)
const deviceSettings = ref(null)

// Fetch device settings from ingestor API
const fetchDeviceSettings = async (deviceId) => {
  loading.value = true
  error.value = null
  try {
    const mqttDeviceId = `esp32_device_${deviceId}`
    const response = await fetch(`http://localhost:5000/${mqttDeviceId}/settings/sensors/get`)
    if (!response.ok) {
      throw new Error(`Failed to fetch settings: ${response.status} ${response.statusText}`)
    }
    const data = await response.json()
    deviceSettings.value = data
  } catch (err) {
    error.value = err.message
    console.error('Error fetching device settings:', err)
  } finally {
    loading.value = false
  }
}

// Parse sensors/actuators from device settings JSON
const parsedSensors = computed(() => {
  if (!deviceSettings.value) return []
  // Expected JSON structure (adapt to actual API response):
  // { sensors: [ { id, name, protocol, pins, settings }, ... ] }
  // If structure differs, modify parsing logic here
  return deviceSettings.value.sensors || deviceSettings.value || []
})

const sensorsByPin = computed(() => {
  return parsedSensors.value.slice().sort((a, b) => {
    const pinA = a.pin
    const pinB = b.pin
    return Number(pinA) - Number(pinB)
  })
})

const openSetup = (sensor) => emit('open-setup', sensor)
const openReadings = (sensor) => emit('open-readings', sensor)
const deleteSensor = async (sensor) => {
  if (!confirm(`Delete sensor "${sensor.desc || sensor.tipo}" (ID: ${sensor.id})? This cannot be undone.`)) {
    return
  }
  
  try {
    const mqttDeviceId = `esp32_device_${props.deviceId}`
    
    const response = await fetch(`http://localhost:5000/${mqttDeviceId}/sensors/remove`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({ sensor_id: sensor.id })
    })
    
    if (!response.ok) {
      throw new Error(`Failed to delete sensor: ${response.status}`)
    }
    
    // Refresh the list after deletion
    await fetchDeviceSettings(props.deviceId)
    
  } catch (err) {
    alert(`Error deleting sensor: ${err.message}`)
    console.error('Error deleting sensor:', err)
  }
}

// Sensor type names mapping
const SENSOR_TYPE_NAMES = {
  0: 'MPU6050',
  1: 'DS18B20',
  2: 'HC-SR04',
  3: 'APDS-9960',
  4: 'SG-90 Servo',
  5: 'Relay',
  6: 'Joystick',
  7: 'Keypad 4x4',
  8: 'IR Receiver',
  9: 'Encoder'
}

// Pin type names mapping
const PIN_TYPE_NAMES = {
  0: 'Disabled',
  1: 'Digital In',
  2: 'Digital Out',
  3: 'Analog',
  4: 'SCL',
  5: 'SDA',
  6: 'OneWire'
}

const getSensorTypeName = (tipo) => {
  return SENSOR_TYPE_NAMES[tipo] || `Type ${tipo}`
}

const getPinTypeName = (tipo) => {
  return PIN_TYPE_NAMES[tipo] || `Type ${tipo}`
}

// Actuator state management
const actuatorStates = ref({})
const servoUpdateTimers = ref({})
const keypadLastValues = ref({})

// Check if sensor is an actuator
const isActuator = (tipo) => {
  return tipo === 4 || tipo === 5  // SG-90 Servo (4) or Relay (5)
}

// Fetch last sensor reading from InfluxDB
const fetchLastReading = async (sensorId) => {
  try {
    const mqttDeviceId = `esp32_device_${props.deviceId}`
    const response = await fetch(`/${mqttDeviceId}/sensors/${sensorId}/read?start=-1h`)
    
    if (!response.ok) {
      console.warn(`Failed to fetch last reading for sensor ${sensorId}`)
      return null
    }
    
    const data = await response.json()
    if (data && data.length > 0) {
      // Get the most recent value
      return data[data.length - 1].value
    }
    return null
  } catch (err) {
    console.error('Error fetching last reading:', err)
    return null
  }
}

// Publish actuator command via MQTT
const setActuatorValue = async (sensor, value) => {
  try {
    const mqttDeviceId = `esp32_device_${props.deviceId}`
    
    // Build sensor config with updated atributo1
    const sensorConfig = {
      id: sensor.id,
      desc: sensor.desc,
      tipo: sensor.tipo,
      atributo1: value,
      pinos: sensor.pinos
    }
    
    const payload = {
      sensors: [sensorConfig]
    }
    
    console.log('📤 Sending actuator update to ESP32')
    console.log('Endpoint:', `/${mqttDeviceId}/settings/sensors/set`)
    console.log('Payload:', payload)
    
    // Use the same endpoint as sensor configuration (goes through Vite proxy)
    const response = await fetch(`/${mqttDeviceId}/settings/sensors/set`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    })
    
    if (!response.ok) {
      const errorText = await response.text()
      console.error('❌ API Error:', response.status, errorText)
      // Don't throw - ESP32 might have received it even if API returns error
      // Update state optimistically since ESP32 will echo back via MQTT
    }
    
    // Update local state optimistically
    actuatorStates.value[sensor.id] = value
    console.log('✅ Actuator command sent, state updated to:', value)
    
  } catch (err) {
    console.error('Error setting actuator:', err)
    // Still update state - ESP32 might process it
    actuatorStates.value[sensor.id] = value
  }
}

// Toggle relay (0/1)
const toggleRelay = (sensor) => {
  const currentState = actuatorStates.value[sensor.id] || 0
  const newState = currentState === 0 ? 1 : 0
  setActuatorValue(sensor, newState)
}

// Set servo angle (0-180)
const setServoAngle = (sensor, angle) => {
  // Update local state immediately for responsive UI
  actuatorStates.value[sensor.id] = parseInt(angle)
  
  // Clear existing timer for this servo
  if (servoUpdateTimers.value[sensor.id]) {
    clearTimeout(servoUpdateTimers.value[sensor.id])
  }
  
  // Debounce: only send to ESP32 after 1 second of no changes
  servoUpdateTimers.value[sensor.id] = setTimeout(() => {
    console.log(`🔄 Sending debounced servo update: ${angle}°`)
    setActuatorValue(sensor, parseInt(angle))
  }, 1000)
}

// Auto-fetch on mount using device ID from props
onMounted(() => {
  fetchDeviceSettings(props.deviceId)
})

// Re-fetch when device changes
watch(() => props.deviceId, (newId) => {
  if (newId) fetchDeviceSettings(newId)
})

// Initialize actuator states from fetched sensor data
watch(parsedSensors, (sensors) => {
  sensors.forEach(sensor => {
    if (isActuator(sensor.tipo) && sensor.atributo1 !== undefined) {
      actuatorStates.value[sensor.id] = sensor.atributo1
      console.log(`🎛️ Initialized actuator ${sensor.id} state:`, sensor.atributo1)
    }
    
    // Fetch last keypad value for type 7 (TECLADO_4X4)
    if (sensor.tipo === 7) {
      fetchLastReading(sensor.id).then(value => {
        if (value !== null) {
          keypadLastValues.value[sensor.id] = value
          console.log(`🎹 Loaded last keypad value for sensor ${sensor.id}:`, value)
        }
      })
    }
  })
})
</script>

<template>
  <section>
    <h2>Configured Sensors & Actuators</h2>

    <div v-if="loading">
      <p>Loading device settings...</p>
    </div>

    <div v-else-if="error">
      <p style="color:#e53e3e">Error: {{ error }}</p>
      <p style="font-size:12px; color:#666">Make sure the ingestor is running on localhost:5000</p>
    </div>

    <div v-else-if="sensorsByPin.length">
      <ul style="list-style:none; padding:0">
        <li v-for="s in sensorsByPin" :key="s.id" style="margin-bottom:16px; padding:16px; background:rgba(255,255,255,0.05); border-radius:12px; border-left:4px solid #1890ff; backdrop-filter: blur(4px);">
          <div style="display:flex; justify-content:space-between; align-items:flex-start; gap:16px;">
            <div style="flex:1">
              <!-- Sensor Name and Type -->
              <div style="margin-bottom:8px">
                <strong style="font-size:18px; color:#fff">{{ s.desc || getSensorTypeName(s.tipo) }}</strong>
                <span style="margin-left:12px; font-size:14px; color:#8c8c8c; background:rgba(255,255,255,0.08); padding:4px 10px; border-radius:12px;">
                  {{ getSensorTypeName(s.tipo) }}
                </span>
              </div>
              
              <!-- Sensor ID -->
              <div style="font-size:14px; color:#bfbfbf; margin-bottom:8px">
                <span style="font-weight:600; color:#ffa940">ID:</span> {{ s.id }}
              </div>
              
              <!-- Pins Information -->
              <div v-if="s.pinos && s.pinos.length > 0" style="margin-top:12px">
                <div style="font-size:13px; color:#91d5ff; margin-bottom:6px; font-weight:600">📌 Pin Configuration:</div>
                <div style="display:flex; flex-wrap:wrap; gap:8px">
                  <div 
                    v-for="(pin, index) in s.pinos" 
                    :key="index"
                    style="background:rgba(24,144,255,0.15); padding:6px 12px; border-radius:8px; font-size:12px; border:1px solid rgba(24,144,255,0.3)"
                  >
                    <span style="color:#91d5ff; font-weight:600">GPIO {{ pin.pino }}</span>
                    <span style="color:#8c8c8c; margin-left:6px">({{ getPinTypeName(pin.tipo) }})</span>
                  </div>
                </div>
              </div>
              
              <!-- Actuator Controls -->
              <div v-if="isActuator(s.tipo)" style="margin-top:16px; padding:12px; background:rgba(82,196,26,0.1); border-radius:8px; border:1px solid rgba(82,196,26,0.3)">
                <!-- Relay Toggle (Type 5) -->
                <div v-if="s.tipo === 5" style="display:flex; align-items:center; gap:12px">
                  <span style="color:#95de64; font-weight:600; font-size:14px">⚡ Relay Control:</span>
                  <button 
                    @click="toggleRelay(s)"
                    :style="{
                      padding: '8px 20px',
                      borderRadius: '8px',
                      border: 'none',
                      background: (actuatorStates[s.id] || 0) === 1 ? '#52c41a' : '#8c8c8c',
                      color: 'white',
                      cursor: 'pointer',
                      fontWeight: 'bold',
                      fontSize: '13px',
                      transition: '0.2s',
                      boxShadow: (actuatorStates[s.id] || 0) === 1 ? '0 0 12px rgba(82,196,26,0.5)' : 'none'
                    }"
                  >
                    {{ (actuatorStates[s.id] || 0) === 1 ? '🟢 ON' : '⚫ OFF' }}
                  </button>
                </div>
                
                <!-- Servo Slider (Type 4) -->
                <div v-if="s.tipo === 4" style="display:flex; flex-direction:column; gap:8px">
                  <div style="display:flex; align-items:center; gap:12px">
                    <span style="color:#95de64; font-weight:600; font-size:14px">🔄 Servo Angle:</span>
                    <span style="color:#fff; font-size:16px; font-weight:bold; min-width:50px">{{ actuatorStates[s.id] || 90 }}°</span>
                  </div>
                  <div style="display:flex; align-items:center; gap:12px">
                    <input 
                      type="range" 
                      min="0" 
                      max="180" 
                      :value="actuatorStates[s.id] || 90"
                      @input="setServoAngle(s, $event.target.value)"
                      style="flex:1; height:8px; border-radius:4px; background:#d9d9d9; outline:none; cursor:pointer"
                    />
                    <input 
                      type="number" 
                      min="0" 
                      max="180" 
                      :value="actuatorStates[s.id] || 90"
                      @change="setServoAngle(s, $event.target.value)"
                      style="width:70px; padding:6px; border-radius:6px; border:1px solid #52c41a; background:rgba(0,0,0,0.3); color:#fff; font-size:13px"
                    />
                  </div>
                </div>
              </div>
              
              <!-- Keypad Last Value Display (Type 7) -->
              <div v-if="s.tipo === 7" style="margin-top:16px; padding:12px; background:rgba(250,173,20,0.1); border-radius:8px; border:1px solid rgba(250,173,20,0.3)">
                <div style="display:flex; align-items:center; gap:12px">
                  <span style="color:#ffc53d; font-weight:600; font-size:14px">🔐 Last Input:</span>
                  <span style="color:#fff; font-size:14px; font-family:monospace; background:rgba(0,0,0,0.3); padding:6px 12px; border-radius:6px; border:1px solid rgba(250,173,20,0.4)">
                    {{ keypadLastValues[s.id] || '---' }}
                  </span>
                </div>
              </div>
            </div>
            
            <!-- Action Buttons -->
            <div style="display:flex; flex-direction:column; gap:8px; min-width:120px">
              <button @click="openSetup(s)" style="padding:8px 16px; border-radius:8px; border:none; background:#1890ff; color:white; cursor:pointer; font-weight:600; font-size:13px; transition:0.2s; box-shadow:0 2px 8px rgba(24,144,255,0.3)" @mouseover="$event.target.style.background='#096dd9'" @mouseout="$event.target.style.background='#1890ff'">
                ✏️ Edit
              </button>
              <button @click="openReadings(s)" style="padding:8px 16px; border-radius:8px; border:none; background:#52c41a; color:white; cursor:pointer; font-weight:600; font-size:13px; transition:0.2s; box-shadow:0 2px 8px rgba(82,196,26,0.3)" @mouseover="$event.target.style.background='#389e0d'" @mouseout="$event.target.style.background='#52c41a'">
                📊 Readings
              </button>
              <button @click="deleteSensor(s)" style="padding:8px 16px; border-radius:8px; border:none; background:rgba(255,77,79,0.25); color:#ff7875; cursor:pointer; font-weight:600; font-size:13px; transition:0.2s; border:1px solid rgba(255,77,79,0.4)" @mouseover="$event.target.style.background='rgba(255,77,79,0.4)'" @mouseout="$event.target.style.background='rgba(255,77,79,0.25)'">
                🗑️ Delete
              </button>
            </div>
          </div>
        </li>
      </ul>
    </div>

    <div v-else>
      <p>No sensors configured on this device.</p>
    </div>
  </section>
</template>

