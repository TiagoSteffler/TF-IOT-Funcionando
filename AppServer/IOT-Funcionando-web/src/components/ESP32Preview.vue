<script setup>
import { ref } from 'vue'

const emit = defineEmits(['pin-selected'])

// ESP32-S3 pins based on pins.txt (ordered top to bottom as they appear physically)
const pinList = [
  14, 13, 12, 11, 10, 9, 46, 3, 8, 18, 17, 16, 15, 7, 6, 5, 4,
  19, 20, 21, 47, 48, 45, 0, 35, 36, 37, 38, 39, 40, 41, 42,
  2, 1, "RX", "TX"
]

// divide automaticamente em duas colunas equilibradas
const half = Math.ceil(pinList.length / 2) - 2;

// Para garantir que o **topo da coluna** seja o primeiro item do pinList,
// invertemos as fatias para que o v-for renderize na ordem visual correta.
const leftPins = ref(pinList.slice(0, half).slice().reverse())
const rightPins = ref(pinList.slice(half).slice().reverse())

const selected = ref(null)

// função existente para detalhes (mantive para compatibilidade)
const getPinDetails = (pinNumber) => {
  const capabilities = []

  if (typeof pinNumber === 'number') {
    if (pinNumber % 2 === 0) capabilities.push('PWM')
    if (pinNumber % 3 === 0) capabilities.push('ADC')
    if (pinNumber % 5 === 0) capabilities.push('I2C')
    if (pinNumber % 7 === 0) capabilities.push('SPI')
    if (pinNumber === 1 || pinNumber === 2) capabilities.push('UART')
  }

  const usable = true
  return { number: pinNumber, capabilities, usable }
}

const selectPin = (pin) => {
  selected.value = pin
  emit('pin-selected', getPinDetails(pin))
}
</script>

<template>
  <section class="pin-section">

    <div class="pins-container">

      <!-- Left column -->
      <div class="pin-column">
        <button 
          v-for="p in leftPins" 
          :key="p"
          @click="selectPin(p)"
          :class="['pin-btn', { selected: selected === p }]"
        >
          {{ p }}
        </button>
      </div>

      <!-- Board image -->
      <div class="board-area crop">
        <img class="board-image" src="/ESP32S3_Pinout.png" alt="">
      </div>

      <!-- Right column -->
      <div class="pin-column">
        <button 
          v-for="p in rightPins" 
          :key="p"
          @click="selectPin(p)"
          :class="['pin-btn', { selected: selected === p }]"
        >
          {{ p }}
        </button>
      </div>

    </div>

  </section>
</template>

<style scoped>
.pin-section {
  width: 100%;
  height: 100%;
  overflow: auto;
}

/* container geral */
.pins-container {
  display: flex;
  flex-direction: row;
  height: 100%;
  overflow: auto;
  max-width: 100%;
}

/* colunas */
.pin-column {
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  align-items: center;
  width: 84px;
  padding: 18px 8px; /* top and bottom gap */
  box-sizing: border-box;
}

/* primeira coluna puxa pra esquerda */
.pin-column:first-child {
  padding-left: 5%;
  align-items: flex-start;
}

/* última puxa pra direita */
.pin-column:last-child {
  padding-right: 5%;
  align-items: flex-end;
}

/* estilo dos botões */
.pin-btn {
  width: 25px;
  height: 25px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: rgba(0, 0, 0, 0.5);
  border: none;
  border-radius: 10px;
  color: white;
  font-size: 12px;
  font-weight: 600;
  cursor: pointer;
  transition: 0.2s ease;
}

.pin-btn:hover {
  background: rgba(0, 0, 0, 0.9);
  transform: scale(1.1);
}

.pin-btn.selected {
  background: rgba(255,255,255,0.45);
  border-color: white;
  box-shadow: 0 0 8px rgba(255,255,255,0.6);
}

/* área da imagem */
.board-area {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
}

.board-area.crop {
  overflow: hidden;
}

.board-image {
  max-width: 200%;
  object-fit: contain;
  pointer-events: none;
}
</style>
