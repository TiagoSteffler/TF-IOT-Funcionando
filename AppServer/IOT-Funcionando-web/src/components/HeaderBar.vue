<script setup>
import { ref } from 'vue'

const emit = defineEmits(['view-change'])

const addSensor = () => {
  // open the SensorSetup view
  emit('view-change', 'SensorSetup')
}

const addBoard = () => {
  // placeholder - action to add a board
}

const views = [
  { id: 'SettingsPanel', label: 'Settings' },
  { id: 'SensorPanel', label: 'Sensor Panel' },
  { id: 'SensorList', label: 'List Sensors' },
  { id: 'PinPreview', label: 'Pin Preview' },
  { id: 'SensorReadings', label: 'Sensor Readings' },
  { id: 'SensorSetup', label: 'Sensor / Actuator Setup' }
]

const selected = ref(views[0].id)
const onChange = () => emit('view-change', selected.value)
</script>

<template>
  <header>
    <h1>ESP32 Dashboard - IOT Funcionando</h1>

    <div>
      <label for="view-select" style="margin-right:6px">View:</label>
      <select id="view-select" v-model="selected" @change="onChange">
        <option v-for="v in views" :key="v.id" :value="v.id">{{ v.label }}</option>
      </select>

      <button @click="addSensor">Add Sensor</button>
      <button @click="addBoard">Add Board</button>
    </div>
  </header>
</template>
