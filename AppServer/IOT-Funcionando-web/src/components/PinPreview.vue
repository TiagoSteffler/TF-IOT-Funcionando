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
        <p><strong>Configured Device ID:</strong> {{ attachedSensor.deviceId }}</p>
        <p><strong>Type:</strong> {{ attachedSensor.type }}</p>
        <p><strong>Model:</strong> {{ attachedSensor.model }}</p>
      </div>
    </div>

    <div v-else>
      <p>No pin selected. Click a pin on the left to preview it.</p>
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

/* bloco de informações extras */
div[style*="margin-top:8px"] {
  margin-top: 12px !important;
  padding: 8px;
  background: rgba(255,255,255,0.05);
  border-radius: 10px;
}
</style>
