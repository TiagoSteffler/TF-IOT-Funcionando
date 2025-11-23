<script setup>
import { toRefs, computed, ref, watch, onMounted } from 'vue'

const props = defineProps({
  selectedPin: { type: Object, default: null },
  sensors: { type: Array, default: () => [] },
  deviceId: { type: Number, default: 1 }
})
const emit = defineEmits(['open-setup', 'open-readings'])

const { selectedPin, sensors, deviceId } = toRefs(props)

// busca algum sensor da API que use esse pino
const apiSensors = ref([])
const loading = ref(false)

const fetchSensors = async () => {
  if (!deviceId.value) return
  loading.value = true
  try {
    const mqttDeviceId = `esp32_device_${deviceId.value}`
    const response = await fetch(`http://localhost:5000/${mqttDeviceId}/settings/sensors/get`)
    if (response.ok) {
      const data = await response.json()
      // API returns array directly, not wrapped in .sensors
      apiSensors.value = Array.isArray(data) ? data : []
    }
  } catch (err) {
    console.error('Erro ao buscar sensores:', err)
  } finally {
    loading.value = false
  }
}

// cata o sensor usando o pino
const attachedSensor = computed(() => {
  if (!selectedPin.value) return null
  
  // Verifica sensores da API primeiro
  for (const sensor of apiSensors.value) {
    if (sensor.pinos && Array.isArray(sensor.pinos)) {
      // Verifica se algum pino deste sensor corresponde ao pino selecionado
      const hasPin = sensor.pinos.some(p => Number(p.pino) === Number(selectedPin.value.number))
      if (hasPin) return sensor
    }
  }
  
  return null
})

// sensores
const SENSOR_TYPE_NAMES = {
  0: 'MPU6050 (Acelerômetro/Giroscópio)',
  1: 'DS18B20 (Temperatura)',
  2: 'HC-SR04 (Ultrassônico)',
  3: 'APDS-9960 (Gesto/Cor)',
  4: 'SG-90 (Servo)',
  5: 'Relay',
  6: 'Joystick',
  7: 'Keypad 4x4',
  8: 'Encoder',
  9: 'DHT11 (Temperatura/Umidade)'
}

const getSensorTypeName = (tipo) => {
  return SENSOR_TYPE_NAMES[tipo] || `Desconhecido: (${tipo})`
}

const openSetup = () => {
  if (!selectedPin.value) return
  if (!selectedPin.value.usable) return
  
  // se tiver algum sensor no pino pode editar
  if (attachedSensor.value) {
    emit('open-setup', attachedSensor.value)
  } else {
    // se nao tiver nada abre pra adicionar novo
    emit('open-setup', selectedPin.value.number)
  }
}

const openReadings = () => {
  if (!selectedPin.value) return
  if (!selectedPin.value.usable) return
  if (!attachedSensor.value) return
  emit('open-readings', attachedSensor.value)
}

// busca novos sensores e pinos
watch([selectedPin, deviceId], () => {
  if (selectedPin.value && deviceId.value) {
    fetchSensors()
  }
}, { immediate: true })

onMounted(() => {
  if (selectedPin.value && deviceId.value) {
    fetchSensors()
  }
})
</script>

<template>
  <section>
    <h2>Dados do pino</h2>

    <div v-if="loading" style="text-align:center; padding:20px">
      <p>Carregando dados de sensores...</p>
    </div>

    <div v-else-if="selectedPin">
      <div class="pin-info">
        <p><strong>Número do pino:</strong> GPIO {{ selectedPin.number }}</p>
        <p><strong>Status: </strong> 
          <span :style="{ color: selectedPin.usable ? '#52c41a' : '#ff4d4f' }">
            {{ selectedPin.usable ? 'Disponível' : 'Indisponível/Utilizado' }}
          </span>
        </p>
        <p><strong>Recursos:</strong></p>
        <ul v-if="selectedPin.capabilities && selectedPin.capabilities.length > 0">
          <li v-for="cap in selectedPin.capabilities" :key="cap">{{ cap }}</li>
        </ul>
        <p v-else style="margin-left:20px; opacity:0.7">Sem recursos especiais</p>
      </div>

      <div v-if="attachedSensor" class="sensor-info">
        <h3 style="margin-top:16px; margin-bottom:8px; color:#ffa940">Sensor/Atuador conectado</h3>
        <p><strong>ID do sensor/atuador:</strong> {{ attachedSensor.id }}</p>
        <p><strong>Descrição:</strong> {{ attachedSensor.desc || 'N/A' }}</p>
        <p><strong>Tipo:</strong> {{ getSensorTypeName(attachedSensor.tipo) }}</p>
        <p><strong>Pinos usados:</strong></p>
        <ul v-if="attachedSensor.pinos && attachedSensor.pinos.length > 0">
          <li v-for="(pin, idx) in attachedSensor.pinos" :key="idx">
            GPIO {{ pin.pino }} (Tipo: {{ pin.tipo }})
          </li>
        </ul>
        <div v-if="attachedSensor.atributo1 !== undefined" style="margin-top:8px; padding:8px; background:rgba(24,144,255,0.1); border-radius:6px">
          <p style="margin:0"><strong>Estado atual:</strong> {{ attachedSensor.atributo1 }}</p>
        </div>
      </div>

      <div v-else class="no-sensor-info">
        <p style="opacity:0.7; margin-top:16px">Nenhum sensor/atuador configurado neste pino</p>
      </div>

      <div class="button-group">
        <button @click="openSetup" :disabled="!selectedPin.usable">
          {{ attachedSensor ? 'Editar Sensor' : 'Adicionar Sensor' }}
        </button>
        <button @click="openReadings" :disabled="!selectedPin.usable || !attachedSensor" style="margin-left:8px">Ver Dados</button>
      </div>
    </div>

    <div v-else class="no-pin">
      <p>Nenhum pino selecionado. Clique em um pino à esquerda para visualizá-lo.</p>
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

  /* sombra suave */
  box-shadow: 0 4px 18px rgba(0,0,0,0.25);
}

/* título */
h2 {
  margin-bottom: 12px;
  font-size: 1.4rem;
  text-shadow: 0 2px 4px rgba(0,0,0,0.5);
}

h3 {
  font-size: 1.1rem;
  text-shadow: 0 1px 3px rgba(0,0,0,0.5);
}

/* parágrafos */
p {
  margin: 4px 0;
  font-size: 0.95rem;
}

/* lista de capacidades */
ul {
  margin-top: 4px;
  padding-left: 20px;
}
li {
  opacity: 0.9;
  margin: 2px 0;
}

/* Info sections */
.pin-info {
  padding: 12px;
  background: rgba(255, 255, 255, 0.05);
  border-radius: 10px;
  margin-bottom: 16px;
  border-left: 3px solid rgba(255, 255, 255, 0.3);
}

.sensor-info {
  padding: 12px;
  background: rgba(255, 169, 64, 0.1);
  border-radius: 10px;
  margin-bottom: 16px;
  border-left: 3px solid #ffa940;
}

.no-sensor-info {
  padding: 12px;
  background: rgba(255, 255, 255, 0.03);
  border-radius: 10px;
  margin-bottom: 16px;
  text-align: center;
}

.no-pin {
  text-align: center;
  padding: 40px 20px;
  opacity: 0.7;
}

.button-group {
  margin-top: 20px;
  display: flex;
  gap: 10px;
  flex-wrap: wrap;
}

/* botões */
button {
  width: 190px;
  height: 60px;

  padding: 8px 14px;
  border: none;
  border-radius: 30px;
  background-color: rgba(0, 0, 0, 0.5);

  font-size: 16px;
  font-weight: 600;
  color: white;
  text-shadow: 1px 1px 10px rgba(0, 0, 0, 0.6);

  cursor: pointer;
  transition: 0.15s;
  white-space: nowrap; /* texto nunca quebra */


  box-shadow: 0 3px 10px rgba(0,0,0,0.25);
}

/* hover */
button:hover:not(:disabled) {
  transform: translateY(-2px);
  box-shadow: 0 5px 14px rgba(0,0,0,0.35);
}

/* botões desabilitados */
button:disabled {
  background: rgba(255, 255, 255, 0.15);
  cursor: not-allowed;
  box-shadow: none;
  opacity: 0.5;
}

button:disabled:hover {
  transform: none;
}
</style>
