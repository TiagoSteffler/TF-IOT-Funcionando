<script setup>
import { ref, computed, watch } from 'vue'

const props = defineProps({
  selectedPin: { type: Object, default: null },
  sensors: { type: Array, default: () => [] }
})
const emit = defineEmits(['save-sensor', 'delete-sensor'])

// helper to derive capabilities for demo parity with ESP32Preview
const getPinDetails = (pinNumber) => {
  const capabilities = []
  if (pinNumber % 2 === 0) capabilities.push('PWM')
  if (pinNumber % 3 === 0) capabilities.push('ADC')
  if (pinNumber % 5 === 0) capabilities.push('I2C')
  if (pinNumber % 7 === 0) capabilities.push('SPI')
  if (pinNumber % 11 === 0) capabilities.push('Touch')
  if (pinNumber === 1 || pinNumber === 2) capabilities.push('UART')
  const usable = !(pinNumber % 13 === 0)
  return { number: pinNumber, capabilities, usable }
}

// available pins 1..38
const allPins = Array.from({ length: 38 }, (_, i) => getPinDetails(i + 1))

// form state
const mode = ref('sensor') // 'sensor' or 'actuator'
const model = ref('')
const protocol = ref('')
const pin = ref(props.selectedPin ? props.selectedPin.number : null)
const deviceId = ref('')

// if editing existing sensor for the current pin, prefill
const existing = computed(() => {
  if (!props.selectedPin) return null
  return props.sensors.find(s => Number(s.pin) === Number(props.selectedPin.number)) || null
})

watch(existing, (v) => {
  if (v) {
    mode.value = v.type || 'sensor'
    model.value = v.model || ''
    protocol.value = v.protocol || ''
    pin.value = v.pin
    deviceId.value = v.deviceId || ''
  } else {
    // reset defaults when no existing
    mode.value = 'sensor'
    model.value = ''
    protocol.value = ''
    pin.value = props.selectedPin ? props.selectedPin.number : null
    deviceId.value = props.selectedPin ? `dev-${props.selectedPin.number}` : ''
  }
}, { immediate: true })

const allowedPins = computed(() => {
  if (!protocol.value) return allPins.filter(p => p.usable)
  // match capability strings with protocol names mapping
  const map = { 'I2C': 'I2C', 'OneWire': 'ADC', 'UART': 'UART', 'PWM': 'PWM', 'ADC': 'ADC', 'SPI': 'SPI' }
  const cap = map[protocol.value] || protocol.value
  return allPins.filter(p => p.usable && p.capabilities.includes(cap))
})

const save = () => {
  if (!pin.value) return
  const sensor = {
    id: existing.value ? existing.value.id : undefined,
    pin: pin.value,
    type: mode.value,
    model: model.value,
    protocol: protocol.value,
    deviceId: deviceId.value
  }
  emit('save-sensor', sensor)
}

const remove = () => {
  if (!existing.value) return
  emit('delete-sensor', existing.value.id)
}
</script>

<template>
  <section>
    <h2>Sensor / Actuator Setup</h2>

    <div>
      <label>Device Type</label>
      <select v-model="mode">
        <option value="sensor">Sensor</option>
        <option value="actuator">Actuator</option>
      </select>
    </div>

    <div style="margin-top:8px">
      <label>Model</label>
      <select v-model="model">
        <option value="">-- select model (populate later) --</option>
      </select>
    </div>

    <div style="margin-top:8px">
      <label>Communication / Protocol</label>
      <select v-model="protocol">
        <option value="">-- automatic / none --</option>
        <option>I2C</option>
        <option>OneWire</option>
        <option>UART</option>
        <option>PWM</option>
        <option>ADC</option>
        <option>SPI</option>
      </select>
    </div>

    <div style="margin-top:8px">
      <label>Pin</label>
      <select v-model="pin">
        <option :value="null">-- select pin --</option>
        <option v-for="p in allowedPins" :key="p.number" :value="p.number">GPIO {{ p.number }} - {{ p.capabilities.join(', ') }}</option>
      </select>
      <div style="font-size:12px; color:#666; margin-top:4px">Selected pin will be validated against the chosen protocol.</div>
    </div>

    <div style="margin-top:8px">
      <label>Device ID</label>
      <input v-model="deviceId" />
    </div>

    <div style="margin-top:12px">
      <button @click="save">Save Settings</button>
      <button v-if="existing" @click="remove" style="margin-left:8px">Delete</button>
    </div>
  </section>
</template>
