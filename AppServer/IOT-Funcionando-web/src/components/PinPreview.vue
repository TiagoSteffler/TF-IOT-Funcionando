<script setup>
import { toRefs, computed } from 'vue'

const props = defineProps({
  selectedPin: { type: Object, default: null },
  sensors: { type: Array, default: () => [] }
})
const emit = defineEmits(['open-setup', 'open-readings'])

const { selectedPin, sensors } = toRefs(props)

const attachedSensor = computed(() => {
  if (!selectedPin.value) return null
  // Sensors from API have 'pin' property (not 'pins')
  // { id: "sensor_pin_34", pin: 34, type: "LDR", enabled: true, sampling_interval: 5000 }
  return sensors.value.find(s => Number(s.pin) === Number(selectedPin.value.number)) || null
})

const openSetup = () => {
  if (!selectedPin.value) return
  if (!selectedPin.value.usable) return // unusable pins cannot setup
  emit('open-setup')
}

const openReadings = () => {
  if (!selectedPin.value) return
  if (!selectedPin.value.usable) return
  if (!attachedSensor.value) return
  emit('open-readings')
}
</script>

<template>
  <section>
    <h2>Pin Preview</h2>

    <div v-if="selectedPin">
      <p><strong>Pin:</strong> {{ selectedPin.number }}</p>
      <p><strong>Usable:</strong> {{ selectedPin.usable ? 'Yes' : 'No' }}</p>
      <p><strong>Capabilities:</strong></p>
      <ul>
        <li v-for="cap in selectedPin.capabilities" :key="cap">{{ cap }}</li>
      </ul>

      <div style="margin-top:8px">
        <button @click="openSetup" :disabled="!selectedPin.usable">Add / Edit Sensor</button>
        <button @click="openReadings" :disabled="!selectedPin.usable || !attachedSensor" style="margin-left:8px">Preview Data</button>
      </div>

      <div style="margin-top:8px" v-if="attachedSensor">
        <p><strong>Sensor ID:</strong> {{ attachedSensor.id }}</p>
        <p><strong>Type:</strong> {{ attachedSensor.type }}</p>
        <p><strong>Enabled:</strong> {{ attachedSensor.enabled ? 'Yes' : 'No' }}</p>
        <p><strong>Sampling Interval:</strong> {{ attachedSensor.sampling_interval }}ms</p>
      </div>
    </div>

    <div v-else>
      <p>No pin selected. Click a pin on the left to preview it.</p>
    </div>
  </section>
</template>
