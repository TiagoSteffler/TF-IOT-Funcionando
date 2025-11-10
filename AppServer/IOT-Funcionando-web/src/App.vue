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

// Board management - starting with device ID 1
const boards = ref([
  { id: 'board-1', deviceId: 1, name: 'ESP32 #1', mac: 'AA:BB:CC:11:22:33', ip: '192.168.1.100', mqtt: 'esp32_device_1' }
])
const currentBoardId = ref('board-1')

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

const handleOpenSetup = (sensorId) => {
  if (sensorId) {
    const s = sensors.value.find(x => x.id === sensorId)
    if (s) selectedPin.value = getPinDetails(Number(s.pin))
  }
  activeView.value = 'SensorSetup'
}

const handleOpenReadings = (sensorId) => {
  if (sensorId) {
    const s = sensors.value.find(x => x.id === sensorId)
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
  showProvisioning.value = false
}

const handleBoardChange = (boardId) => {
  currentBoardId.value = boardId
}
</script>

<template>
  <div id="app">
    <!-- Top header area (10-20% of viewport) -->
    <div class="top-area" style="border:2px solid black; height:10vh; box-sizing:border-box">
      <HeaderBar @view-change="onViewChange" @add-board="openBoardProvisioning" />
    </div>

    <!-- Middle area: left 40% (preview) and right 60% (dynamic) -->
    <div class="middle-area" style="display:flex; height:75vh">
      <div class="left-area" style="width:40%; border:2px solid black; box-sizing:border-box; overflow:auto">
        <ESP32Preview @pin-selected="onPinSelected" />
      </div>

      <div class="right-area" style="width:60%; border:2px solid black; box-sizing:border-box; overflow:auto; padding:8px">
        <!-- Board Provisioning Modal -->
        <BoardProvisioning 
          v-if="showProvisioning"
          :next-device-id="nextDeviceId"
          @provision-complete="handleProvisionComplete"
          @cancel="handleProvisionCancel"
        />
        
        <!-- Regular Views -->
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

    <!-- Footer area (10% of viewport) -->
    <div class="footer-area" style="border:2px solid black; height:10vh; box-sizing:border-box">
      <FooterBar 
        :boards="boards" 
        :current-board-id="currentBoardId"
        @board-change="handleBoardChange"
      />
    </div>
  </div>
</template>



