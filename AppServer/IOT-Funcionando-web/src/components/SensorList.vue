<script setup>
import { ref, computed, onMounted, onUnmounted, watch } from 'vue'

const props = defineProps({
  sensors: { type: Array, default: () => [] },
  deviceId: { type: Number, default: 1 }
})
const emit = defineEmits(['open-setup', 'open-readings', 'delete-sensor'])

const loading = ref(false)
const error = ref(null)
const deviceSettings = ref(null)

// busca valores da api.py
const fetchDeviceSettings = async (deviceId) => {
  loading.value = true
  error.value = null
  try {
    const mqttDeviceId = `esp32_device_${deviceId}`
    const response = await fetch(`http://localhost:5000/${mqttDeviceId}/settings/sensors/get`)
    if (!response.ok) {
      throw new Error(`Falha ao buscar sensores: ${response.status} ${response.statusText}`)
    }
    const data = await response.json()
    deviceSettings.value = data
  } catch (err) {
    error.value = err.message
    console.error('Erro ao buscar configurações do dispositivo:', err)
  } finally {
    loading.value = false
  }
}

// cata sensores e atuadores do JSON de resposta
const parsedSensors = computed(() => {
  if (!deviceSettings.value) return []
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
  if (!confirm(`Apagar sensor "${sensor.desc || sensor.tipo}" (ID: ${sensor.id})? Esta ação não pode ser desfeita.`)) {
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
      throw new Error(`Falha ao apagar sensor: ${response.status}`)
    }
    
    // Atualiza a lista após exclusão
    await fetchDeviceSettings(props.deviceId)
    
  } catch (err) {
    alert(`Erro ao apagar sensor: ${err.message}`)
    console.error('Erro ao apagar sensor:', err)
  }
}

// enum de sensores
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

// enum de pinos
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
  return SENSOR_TYPE_NAMES[tipo] || `Tipo ${tipo}`
}

const getPinTypeName = (tipo) => {
  return PIN_TYPE_NAMES[tipo] || `Tipo ${tipo}`
}

// atuadores
const actuatorStates = ref({})
const servoUpdateTimers = ref({})
const keypadLastValues = ref({})

// Checa se o sensor é um atuador
const isActuator = (tipo) => {
  return tipo === 4 || tipo === 5  // SG-90 Servo (4) ou Relay (5)
}

// Busca a última leitura do sensor no InfluxDB
const fetchLastReading = async (sensorId) => {
  try {
    const mqttDeviceId = `esp32_device_${props.deviceId}`
    const response = await fetch(`/${mqttDeviceId}/sensors/sensor_${sensorId}/read?start=-1h`)
    
    if (!response.ok) {
      console.warn(`Falha ao buscar última leitura para o sensor ${sensorId}`)
      return null
    }
    
    const data = await response.json()
    if (data && data.length > 0) {
      // Obtém o valor mais recente
      const lastValue = data[data.length - 1].value
      // Para keypad, extrai o campo 'input' do dict
      if (typeof lastValue === 'object' && lastValue.input !== undefined) {
        return lastValue.input
      }
      return lastValue
    }
    return null
  } catch (err) {
    console.error('Erro ao buscar última leitura:', err)
    return null
  }
}

// Atualiza periodicamente os valores do keypad
const refreshKeypadValues = () => {
  parsedSensors.value.forEach(sensor => {
    if (sensor.tipo === 7) {
      fetchLastReading(sensor.id).then(value => {
        if (value !== null) {
          keypadLastValues.value[sensor.id] = value
        }
      })
    }
  })
}

let keypadRefreshInterval = null

// publica comando de atuador via MQTT
const setActuatorValue = async (sensor, value) => {
  try {
    const mqttDeviceId = `esp32_device_${props.deviceId}`
    
    // Monta configuração do sensor com atributo1 atualizado
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
    
    console.log(`Enviando atualização de atuador para ${mqttDeviceId}`)
    console.log('Endpoint:', `/${mqttDeviceId}/settings/sensors/set`)
    console.log('Payload:', payload)
    
    // mesmo endpoint e payload de configuração do atuador
    const response = await fetch(`/${mqttDeviceId}/settings/sensors/set`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    })
    
    if (!response.ok) {
      const errorText = await response.text()
      console.error('Erro na API:', response.status, errorText)
    }
    
    // Atualiza estado local pois vai que deu certo 
    actuatorStates.value[sensor.id] = value
    console.log('Estado atualizado para:', value)
    
  } catch (err) {
    console.error('Erro ao definir atuador:', err)
    // Ainda atualiza estado - ESP32 pode processar
    actuatorStates.value[sensor.id] = value
  }
}

// Toggle rele (0/1)
const toggleRelay = (sensor) => {
  const currentState = actuatorStates.value[sensor.id] || 0
  const newState = currentState === 0 ? 1 : 0
  setActuatorValue(sensor, newState)
}

// algulo do servo (0-180)
const setServoAngle = (sensor, angle) => {
  // Atualiza estado local imediatamente (se mexer muito rapido vai dar aquela engasgadinha)
  actuatorStates.value[sensor.id] = parseInt(angle)
  
  // Limpa timer de debounce para este servo
  if (servoUpdateTimers.value[sensor.id]) {
    clearTimeout(servoUpdateTimers.value[sensor.id])
  }
  
  // Debounce: envia para ESP32 somente depois de 200ms sem mudar
  servoUpdateTimers.value[sensor.id] = setTimeout(() => {
    console.log(`Enviando atualização pro servo: ${angle}°`)
    setActuatorValue(sensor, parseInt(angle))
  }, 200)
}

// Auto-fetch baseado no deviceId das props
onMounted(() => {
  fetchDeviceSettings(props.deviceId)
  // Atualiza valores do keypad a cada 5 segundos
  keypadRefreshInterval = setInterval(refreshKeypadValues, 5000)
})

// Cleanup no unmount
onUnmounted(() => {
  if (keypadRefreshInterval) {
    clearInterval(keypadRefreshInterval)
  }
})

// Re-fetch quando o deviceId mudar
watch(() => props.deviceId, (newId) => {
  if (newId) fetchDeviceSettings(newId)
})

// pega valores iniciais dos atuadores
watch(parsedSensors, (sensors) => {
  sensors.forEach(sensor => {
    if (isActuator(sensor.tipo) && sensor.atributo1 !== undefined) {
      actuatorStates.value[sensor.id] = sensor.atributo1
      console.log(`Atuador ${sensor.id} inicializado com:`, sensor.atributo1)
    }
    
    // Busca último valor do keypad para tipo 7 (TECLADO_4X4)
    if (sensor.tipo === 7) {
      fetchLastReading(sensor.id).then(value => {
        if (value !== null) {
          keypadLastValues.value[sensor.id] = value
          console.log(`Último valor do keypad carregado para ID ${sensor.id}:`, value)
        }
      })
    }
  })
})
</script>

<template>
  <section>
    <h2>Sensores e atuadores configurados</h2>

    <div v-if="loading">
      <p>Carregando configurações da placa...</p>
    </div>

    <div v-else-if="error">
      <p style="color:#e53e3e">Erro: {{ error }}</p>
      <p style="font-size:12px; color:#999">Certifique-se de que a API está rodando em localhost:5000 ou de que o dispositivo está conectado.</p>
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
                <div style="font-size:13px; color:#91d5ff; margin-bottom:6px; font-weight:600">Configuração de pinout:</div>
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
                  <span style="color:#95de64; font-weight:600; font-size:14px">Controle do relé:</span>
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
                    {{ (actuatorStates[s.id] || 0) === 1 ? 'ON' : 'OFF' }}
                  </button>
                </div>
                
                <!-- Servo Slider (Type 4) -->
                <div v-if="s.tipo === 4" style="display:flex; flex-direction:column; gap:8px">
                  <div style="display:flex; align-items:center; gap:12px">
                    <span style="color:#95de64; font-weight:600; font-size:14px">Ângulo do servo:</span>
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
                      min="1" 
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
                  <span style="color:#ffc53d; font-weight:600; font-size:14px">Última senha enviada:</span>
                  <span style="color:#fff; font-size:14px; font-family:monospace; background:rgba(0,0,0,0.3); padding:6px 12px; border-radius:6px; border:1px solid rgba(250,173,20,0.4)">
                    {{ keypadLastValues[s.id] || '---' }}
                  </span>
                </div>
              </div>
            </div>
            
            <!-- Action Buttons -->
            <div style="display:flex; flex-direction:column; gap:8px; min-width:120px">
              <button @click="openSetup(s)" style="padding:8px 16px; border-radius:8px; border:none; background:#1890ff; color:white; cursor:pointer; font-weight:600; font-size:13px; transition:0.2s; box-shadow:0 2px 8px rgba(24,144,255,0.3)" @mouseover="$event.target.style.background='#096dd9'" @mouseout="$event.target.style.background='#1890ff'">
                Editar
              </button>
              <button @click="openReadings(s)" style="padding:8px 16px; border-radius:8px; border:none; background:#52c41a; color:white; cursor:pointer; font-weight:600; font-size:13px; transition:0.2s; box-shadow:0 2px 8px rgba(82,196,26,0.3)" @mouseover="$event.target.style.background='#389e0d'" @mouseout="$event.target.style.background='#52c41a'">
                Leituras
              </button>
              <button @click="deleteSensor(s)" style="padding:8px 16px; border-radius:8px; border:none; background:rgba(255,77,79,0.25); color:#ff7875; cursor:pointer; font-weight:600; font-size:13px; transition:0.2s; border:1px solid rgba(255,77,79,0.4)" @mouseover="$event.target.style.background='rgba(255,77,79,0.4)'" @mouseout="$event.target.style.background='rgba(255,77,79,0.25)'">
                Remover
              </button>
            </div>
          </div>
        </li>
      </ul>
    </div>

    <div v-else>
      <p>Nenhum sensor configurado neste dispositivo.</p>
    </div>
  </section>
</template>

