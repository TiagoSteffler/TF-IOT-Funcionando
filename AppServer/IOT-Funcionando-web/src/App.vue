<script setup>
import { ref, computed } from 'vue'
import HeaderBar from './components/HeaderBar.vue'
import ESP32Preview from './components/ESP32Preview.vue'
import SettingsPanel from './components/SettingsPanel.vue'
import PinPreview from './components/PinPreview.vue'
import SensorSetup from './components/SensorSetup.vue'
import SensorReadings from './components/SensorReadings.vue'
import SensorList from './components/SensorList.vue'
import Rules from './components/Rules.vue'
import FooterBar from './components/FooterBar.vue'
import BoardProvisioning from './components/BoardProvisioning.vue'

const activeView = ref('SettingsPanel')
const selectedPin = ref(null)
const showProvisioning = ref(false)

// Board management - load from localStorage
const STORAGE_KEY = 'esp32_boards'

const loadBoardsFromStorage = () => {
  try {
    const stored = localStorage.getItem(STORAGE_KEY)
    return stored ? JSON.parse(stored) : []
  } catch (e) {
    console.error('Error loading boards from storage:', e)
    return []
  }
}

const saveBoardsToStorage = (boardsData) => {
  try {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(boardsData))
  } catch (e) {
    console.error('Error saving boards to storage:', e)
  }
}

const boards = ref(loadBoardsFromStorage())
const currentBoardId = ref(boards.value.length > 0 ? boards.value[0].id : null)

const currentBoard = computed(() => boards.value.find(b => b.id === currentBoardId.value))
const currentDeviceId = computed(() => currentBoard.value?.deviceId || 1)
const nextDeviceId = computed(() => {
  const maxId = boards.value.reduce((max, b) => Math.max(max, b.deviceId), 0)
  return maxId + 1
})

// store configured sensors in-memory (pinNumber, type, model, deviceId, id)
const sensors = ref([])

const saveSensor = (sensor) => {
  // if id exists, update; else create
  if (sensor.id) {
    const idx = sensors.value.findIndex(s => s.id === sensor.id)
    if (idx !== -1) sensors.value[idx] = sensor
  } else {
    sensor.id = `dev-${Date.now()}`
    sensors.value.push(sensor)
  }
}

const deleteSensor = (id) => {
  sensors.value = sensors.value.filter(s => s.id !== id)
}

const viewMap = {
  SettingsPanel,
  SensorPanel: SensorSetup,
  PinPreview,
  SensorReadings,
  SensorSetup,
  SensorList,
  Rules
}

const activeViewComponent = computed(() => viewMap[activeView.value] || SettingsPanel)

const onViewChange = (viewId) => {
  activeView.value = viewId
}

const onPinSelected = (pin) => {
  selectedPin.value = pin
  // When a pin is selected, show the PinPreview view automatically
  activeView.value = 'PinPreview'
}

// helper to create a pin details object based on actual ESP32-S3 capabilities
const getPinDetails = (pinNumber) => {
  const pin = Number(pinNumber)
  const capabilities = []
  
  // pinos touch
  const touchPins = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14]
  if (touchPins.includes(pin)) capabilities.push('Touch')
  
  // pinos ADC
  const adcPins = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20]
  if (adcPins.includes(pin)) capabilities.push('ADC')
  
  // pinos I2C
  if (pin === 18) capabilities.push('I2C (SCL)')
  if (pin === 17) capabilities.push('I2C (SDA)')
  
  // pinos USB OTG
  if (pin === 19) capabilities.push('USB D+')
  if (pin === 20) capabilities.push('USB D-')
  
  // pinos de comunicação UART
  if (pin === 43) capabilities.push('UART (TX)')
  if (pin === 44) capabilities.push('UART (RX)')
  
  // pinos de uso especial
  if (pin === 2) capabilities.push('LED builtin')
  if (pin === 48) capabilities.push('WS2812 builtin')
  
  // pinos restritos (reservados para funções do sistema/inacessíveis)
  const unusablePins = [0, 35, 36, 37, 38, 39, 40, 45, 46]
  const usable = !unusablePins.includes(pin)
  
  // Pinos para leitura de cartão SD
  if (pin === 38 || pin === 39 || pin === 40) {
    capabilities.push('SD (not recommended)')
  }
  
  return { number: pin, capabilities, usable }
}

// Store selected sensor for editing
const selectedSensor = ref(null)

const handleOpenSetup = (sensorOrId) => {
  // If it's a sensor object (from SensorList edit button)
  if (typeof sensorOrId === 'object' && sensorOrId !== null) {
    selectedSensor.value = sensorOrId
    selectedPin.value = null
  }
  // If it's a sensor ID from the list, extract the pin number
  else if (typeof sensorOrId === 'string' && sensorOrId.startsWith('sensor_pin_')) {
    const pinNumber = parseInt(sensorOrId.replace('sensor_pin_', ''))
    selectedPin.value = getPinDetails(pinNumber)
    selectedSensor.value = null
  } else if (typeof sensorOrId === 'number') {
    // Direct pin number
    selectedPin.value = getPinDetails(sensorOrId)
    selectedSensor.value = null
  } else {
    // Try to find in local sensors array (legacy)
    const s = sensors.value.find(x => x.id === sensorOrId)
    if (s) selectedPin.value = getPinDetails(Number(s.pin))
    selectedSensor.value = null
  }
  activeView.value = 'SensorSetup'
}

const handleOpenReadings = (sensor) => {
  console.log('📊 handleOpenReadings called with:', sensor)
  // Store the selected sensor object for the SensorReadings component
  if (typeof sensor === 'object' && sensor !== null) {
    selectedSensor.value = sensor
    console.log('✅ Set selectedSensor to object:', selectedSensor.value)
  } else {
    // Legacy handling: if just an ID is passed, try to find the sensor
    const s = sensors.value.find(x => x.id === sensor)
    if (s) {
      selectedSensor.value = s
      console.log('✅ Found and set selectedSensor:', selectedSensor.value)
    } else {
      console.warn('⚠️ Could not find sensor with id:', sensor)
    }
  }
  console.log('🎯 Switching to SensorReadings view')
  activeView.value = 'SensorReadings'
}

const openBoardProvisioning = () => {
  showProvisioning.value = true
}

const handleProvisionComplete = (newBoard) => {
  boards.value.push(newBoard)
  saveBoardsToStorage(boards.value)
  currentBoardId.value = newBoard.id
  showProvisioning.value = false
  activeView.value = 'SettingsPanel'
}

const handleProvisionCancel = () => {
  // Only allow cancel if there's at least one board already
  if (boards.value.length > 0) {
    showProvisioning.value = false
  }
}

const handleBoardChange = (boardId) => {
  currentBoardId.value = boardId
}

const handleEraseAll = () => {
  // Remove current board from boards array
  if (currentBoardId.value !== null) {
    boards.value = boards.value.filter(b => b.id !== currentBoardId.value)
    saveBoardsToStorage(boards.value)
    
    // If there are other boards, switch to the first one
    if (boards.value.length > 0) {
      currentBoardId.value = boards.value[0].id
    } else {
      // No boards left, reset to initial state
      currentBoardId.value = null
    }
  }
}
</script>

<template>
  <div id="app" class="app-container">
    <div class="top-area">
      <HeaderBar @view-change="onViewChange" @add-board="openBoardProvisioning" />
    </div>

    <div class="middle-area">
      <div class="left-area">
        <ESP32Preview @pin-selected="onPinSelected" />
      </div>

      <div class="right-area">
        <!-- Board Provisioning Modal or Initial Setup -->
        <BoardProvisioning 
          v-if="showProvisioning || boards.length === 0"
          :next-device-id="nextDeviceId"
          @provision-complete="handleProvisionComplete"
          @cancel="handleProvisionCancel"
        />

        <component v-else
          :is="activeViewComponent"
          v-bind="{ selectedPin, sensors, deviceId: currentDeviceId, selectedSensor, boards }"
          @open-setup="(id) => handleOpenSetup(id)"
          @open-readings="(id) => handleOpenReadings(id)"
          @save-sensor="(s) => { saveSensor(s); onViewChange('PinPreview'); selectedSensor = null }"
          @delete-sensor="(id) => { deleteSensor(id); onViewChange('PinPreview'); selectedSensor = null }"
          @erase-all="handleEraseAll"
        />
      </div>
    </div>

    <div class="footer-area">
      <FooterBar 
        :boards="boards" 
        :current-board-id="currentBoardId"
        @board-change="handleBoardChange"
      />
    </div>
  </div>
</template>


<style>
html, body {
  margin: 0 !important;
  padding: 0 !important;
  height: 100%;
  overflow: hidden;
  background: radial-gradient(circle at top left, #461248, #a93747, #f2a15f, #ffd6b6);
  background-attachment: fixed;
}
</style>

<style scoped>
/* remove qualquer borda ou fundo do container raiz */
#app {
  margin: 0 !important;
  padding: 0 !important;
  background: transparent !important;
}

/* evita qualquer fundo ocultando o degradê */
.app-container {
  display: flex;
  flex-direction: column;
  background: transparent !important;
  color: white;
  font-family: Roboto, sans-serif;
  min-height: 100vh;
  width: 100%;
}

.top-area,
.middle-area,
.left-area,
.right-area,
.footer-area {
  border: 3px solid rgba(255, 255, 255, 0);
}

.left-area,
.right-area{
  border-radius: 30px;
  margin-left: 30px;
  margin-right: 30px;
  background: rgba(0, 0, 0, 0.5);
}

/* se preferir manter bordas internas, pode colocar aqui */
.top-area {
  height: 15vh;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0 12px;
}

.middle-area {
  justify-content: center;
  display: flex;
  height: 70vh;
}

.left-area {
  width: 20%;
  overflow: auto;
  padding: 8px;
}

.right-area {
  width: 50%;
  overflow: auto;
  padding: 12px;
}

.right-area {
  overflow-y: scroll;
}

.right-area::-webkit-scrollbar {
  display: none;
}

.right-area {
  -ms-overflow-style: none;
  scrollbar-width: none;
}

footer {
  margin: 0 auto;  /* garante centralização */
}

.footer-area {
  height: 10vh;
  width: 100%;

  display: flex;
  justify-content: center;
  align-items: center;

  margin-top: auto;  /* leva para o fundo */
  padding: 0;        /* evita causar overflow */
  overflow: visible; /* garante que não adiciona scroll */
}

</style>

