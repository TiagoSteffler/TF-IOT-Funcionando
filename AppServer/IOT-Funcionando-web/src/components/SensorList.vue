<script setup>
import { computed } from 'vue'

const props = defineProps({
  sensors: { type: Array, default: () => [] }
})
const emit = defineEmits(['open-setup', 'open-readings'])

const sensorsByPin = computed(() => {
  return props.sensors.slice().sort((a,b) => Number(a.pin) - Number(b.pin))
})

const openSetup = (sensor) => emit('open-setup', sensor.id)
const openReadings = (sensor) => emit('open-readings', sensor.id)
</script>

<template>
  <section>
    <h2>Configured Sensors & Actuators</h2>

    <div v-if="sensorsByPin.length">
      <ul>
        <li v-for="s in sensorsByPin" :key="s.id" style="margin-bottom:8px">
          <strong>{{ s.deviceId }}</strong> — Pin {{ s.pin }} — {{ s.type }} {{ s.model ? ('(' + s.model + ')') : '' }}
          <div style="margin-top:4px">
            <button @click="openSetup(s)">Edit</button>
            <button @click="openReadings(s)" style="margin-left:8px">Readings</button>
          </div>
        </li>
      </ul>
    </div>

    <div v-else>
      <p>No sensors configured yet.</p>
    </div>
  </section>
</template>
