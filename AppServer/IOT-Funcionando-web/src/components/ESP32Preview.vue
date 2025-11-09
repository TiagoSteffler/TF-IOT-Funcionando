<script setup>
import { ref } from 'vue'

const emit = defineEmits(['pin-selected'])

// Create arrays for two columns of pins. Total 38 pins split 19/19.
const leftPins = ref(Array.from({ length: 19 }, (_, i) => i + 1))
const rightPins = ref(Array.from({ length: 19 }, (_, i) => i + 20))

const selected = ref(null)

// Simple deterministic capability assignment for demo purposes.
const getPinDetails = (pinNumber) => {
  const capabilities = []
  if (pinNumber % 2 === 0) capabilities.push('PWM')
  if (pinNumber % 3 === 0) capabilities.push('ADC')
  if (pinNumber % 5 === 0) capabilities.push('I2C')
  if (pinNumber % 7 === 0) capabilities.push('SPI')
  if (pinNumber % 11 === 0) capabilities.push('Touch')
  if (pinNumber === 1 || pinNumber === 2) capabilities.push('UART')

  // mark some pins as unavailable (reserved) for demo
  const usable = !(pinNumber % 13 === 0)

  return { number: pinNumber, capabilities, usable }
}

const selectPin = (pin) => {
  selected.value = pin
  const details = getPinDetails(pin)
  emit('pin-selected', details)
}
</script>

<template>
  <section>
    <!-- Container: left column (vertical buttons) | board image | right column (vertical buttons) -->
    <div style="display:flex; flex-direction:row; align-items:stretch; height:100%">

      <!-- Left column: vertical stack of buttons -->
      <div style="display:flex; flex-direction:column; gap:6px; padding:8px; width:84px; box-sizing:border-box">
        <button v-for="p in leftPins" :key="p" @click="selectPin(p)" style="display:block; width:100%">{{ p }}</button>
      </div>

      <!-- Board image (vertical) placeholder between the columns -->
      <div style="flex:1; display:flex; align-items:center; justify-content:center;">
        <div aria-hidden="true">BOARD IMAGE</div>
      </div>

      <!-- Right column: vertical stack of buttons -->
      <div style="display:flex; flex-direction:column; gap:6px; padding:8px; width:84px; box-sizing:border-box">
        <button v-for="p in rightPins" :key="p" @click="selectPin(p)" style="display:block; width:100%">{{ p }}</button>
      </div>

    </div>

    <div style="margin-top:8px">
      <p>Selected pin: {{ selected ?? 'none' }}</p>
    </div>
  </section>
</template>
