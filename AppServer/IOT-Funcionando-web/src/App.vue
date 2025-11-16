<script setup>
import { ref, computed } from 'vue'
import HeaderBar from './components/HeaderBar.vue'
import ESP32Preview from './components/ESP32Preview.vue'
import SettingsPanel from './components/SettingsPanel.vue'
import PinPreview from './components/PinPreview.vue'
import SensorSetup from './components/SensorSetup.vue'
import SensorReadings from './components/SensorReadings.vue'
import SensorList from './components/SensorList.vue'
import FooterBar from './components/FooterBar.vue'
import BoardProvisioning from './components/BoardProvisioning.vue'

const activeView = ref('SettingsPanel')
const selectedPin = ref(null)
const showProvisioning = ref(false)

// Board management - start with empty boards array
const boards = ref([])
const currentBoardId = ref(null)

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
  SensorList
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

// helper (same deterministic demo rules used elsewhere) to create a pin details object
const getPinDetails = (pinNumber) => {
  const capabilities = []
  if (pinNumber % 2 === 0) capabilities.push('PWM')
  if (pinNumber % 3 === 0) capabilities.push('ADC')
  if (pinNumber % 5 === 0) capabilities.push('I2C')
  if (pinNumber % 7 === 0) capabilities.push('SPI')
  if (pinNumber % 11 === 0) capabilities.push('Touch')
  if (pinNumber === 1 || pinNumber === 2) capabilities.push('UART')
  const usable = !(pinNumber % 13 === 0)
  return { number: Number(pinNumber), capabilities, usable }
}

const handleOpenSetup = (sensorIdOrPin) => {
  // If it's a sensor ID from the list, extract the pin number
  if (typeof sensorIdOrPin === 'string' && sensorIdOrPin.startsWith('sensor_pin_')) {
    const pinNumber = parseInt(sensorIdOrPin.replace('sensor_pin_', ''))
    selectedPin.value = getPinDetails(pinNumber)
  } else if (typeof sensorIdOrPin === 'number') {
    // Direct pin number
    selectedPin.value = getPinDetails(sensorIdOrPin)
  } else {
    // Try to find in local sensors array (legacy)
    const s = sensors.value.find(x => x.id === sensorIdOrPin)
    if (s) selectedPin.value = getPinDetails(Number(s.pin))
  }
  activeView.value = 'SensorSetup'
}

const handleOpenReadings = (sensorIdOrPin) => {
  // If it's a sensor ID from the list, extract the pin number
  if (typeof sensorIdOrPin === 'string' && sensorIdOrPin.startsWith('sensor_pin_')) {
    const pinNumber = parseInt(sensorIdOrPin.replace('sensor_pin_', ''))
    selectedPin.value = getPinDetails(pinNumber)
  } else if (typeof sensorIdOrPin === 'number') {
    // Direct pin number
    selectedPin.value = getPinDetails(sensorIdOrPin)
  } else {
    // Try to find in local sensors array (legacy)
    const s = sensors.value.find(x => x.id === sensorIdOrPin)
    if (s) selectedPin.value = getPinDetails(Number(s.pin))
  }
  activeView.value = 'SensorReadings'
}

const openBoardProvisioning = () => {
  showProvisioning.value = true
}

const handleProvisionComplete = (newBoard) => {
  boards.value.push(newBoard)
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
          v-bind="{ selectedPin, sensors, deviceId: currentDeviceId }"
          @open-setup="(id) => handleOpenSetup(id)"
          @open-readings="(id) => handleOpenReadings(id)"
          @save-sensor="(s) => { saveSensor(s); onViewChange('PinPreview') }"
          @delete-sensor="(id) => { deleteSensor(id); onViewChange('PinPreview') }"
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

