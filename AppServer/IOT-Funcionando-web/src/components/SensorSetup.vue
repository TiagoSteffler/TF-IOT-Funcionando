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

    <div class="field-block">
      <label>Device Type</label>
      <select v-model="mode">
        <option value="sensor">Sensor</option>
        <option value="actuator">Actuator</option>
      </select>
    </div>

    <div class="field-block">
      <label>Model</label>
      <select v-model="model">
        <option value="">-- select model (populate later) --</option>
      </select>
    </div>

    <div class="field-block">
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

    <div class="field-block">
      <label>Pin</label>
      <select v-model="pin">
        <option :value="null">-- select pin --</option>
        <option
          v-for="p in allowedPins"
          :key="p.number"
          :value="p.number"
        >
          GPIO {{ p.number }} - {{ p.capabilities.join(', ') }}
        </option>
      </select>
      <div class="info-text">
        Selected pin will be validated against the chosen protocol.
      </div>
    </div>

    <div class="field-block">
      <label>Device ID</label>
      <input v-model="deviceId" />
    </div>

    <div style="margin-top:12px">
      <button @click="save">Save Settings</button>
      <button v-if="existing" @click="remove" style="margin-left:8px">Delete</button>
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

  box-shadow: 0 4px 18px rgba(0,0,0,0.25);
}

/* título */
h2 {
  margin-bottom: 16px;
  font-size: 1.4rem;
  text-shadow: 0 2px 4px rgba(0,0,0,0.5);
}

/* labels */
label {
  font-size: 0.95rem;
  font-weight: 600;
  display: block;
  margin-bottom: 6px;
  text-shadow: 0 1px 6px rgba(0,0,0,0.5);
}

/* blocos uniformes */
.field-block {
  width: 100%;
  box-sizing: border-box;
  margin-top: 13px;
  padding-left: 20px;
  padding-right: 20px;
  border-radius: 10px;
}

/* selects e inputs alinhados perfeitamente */
.field-block select,
.field-block input {
  width: 100%;
  box-sizing: border-box;
  padding: 10px 12px;

  border-radius: 12px;
  background: rgba(0, 0, 0, 0.35);
  border: 1px solid rgba(255, 255, 255, 0.25);
  color: white;
  font-size: 0.95rem;

  box-shadow: inset 0 0 10px rgba(0,0,0,0.3);
  transition: 0.2s;
}

.field-block select:focus,
.field-block input:focus {
  outline: none;
  border-color: rgba(255,255,255,0.5);
  background: rgba(0, 0, 0, 0.45);
}

/* texto de info */
.info-text {
  font-size: 12px;
  color: #ccc;
  margin-top: 6px;
}

/* botões */
button {
  width: 180px;
  height: 55px;

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
  white-space: nowrap;

  box-shadow: 0 3px 10px rgba(0,0,0,0.25);
}

button:hover:not(:disabled) {
  transform: translateY(-2px);
  box-shadow: 0 5px 14px rgba(0,0,0,0.35);
}

button:disabled {
  background: rgba(255, 255, 255, 0.15);
  cursor: not-allowed;
  box-shadow: none;
  opacity: 0.5;
}
</style>
