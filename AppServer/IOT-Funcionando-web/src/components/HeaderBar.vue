<script setup>
import { ref } from 'vue'

const emit = defineEmits(['view-change', 'add-board'])

const addSensor = () => emit('view-change', 'SensorSetup')
const addBoard = () => emit('add-board')

const views = [
  { id: 'SettingsPanel', label: '⚙ CONFIGURAÇÃO' },
  { id: 'SensorPanel', label: '☄ PAINEL' },
  { id: 'SensorList', label: '🗒 SENSORES' },
  { id: 'PinPreview', label: '⚲ PINOS' },
  { id: 'SensorReadings', label: '🕮 LEITURAS' },
  { id: 'SensorSetup', label: '🛠 SETUP' }
]

const selected = ref(views[0].id)
const changeView = (id) => {
  selected.value = id
  emit('view-change', id)
}
</script>

<template>
  <header class="header">

    <div class="button-grid">
      <!-- Botões das views -->
      <button
        v-for="v in views"
        :key="v.id"
        :class="['view-btn', { active: selected === v.id }]"
        @click="changeView(v.id)"
      >
        {{ v.label }}
      </button>

      <!-- Botões extras -->
      <button class="action-btn" @click="addSensor">Add Sensor</button>
      <button class="action-btn" @click="addBoard">Add Board</button>
    </div>
  </header>
</template>

<style scoped>
.header {
  height: 80px; /* usa px */
  justify-content: center;
  align-items: center;
  color: white;
  display: flex;
  white-space: nowrap; /* impede quebra */
}

/* grade de botões */
.button-grid {
  display: flex;
  flex-wrap: nowrap; /* 🚫 impede quebrar linha */
  gap: 30px; /* px fixo */
}

.view-btn,
.action-btn {
  width: 190px;
  height: 60px;

  padding: 8px 14px;
  border: none;
  border-radius: 30px;

  font-size: 16px;
  font-weight: 600;
  color: white;
  text-shadow: 1px 1px 10px rgba(0, 0, 0, 0.6);

  cursor: pointer;
  transition: 0.15s;
  white-space: nowrap; /* texto nunca quebra */
}

/* botões das views */
.view-btn {
  background: transparent;

}

.action-btn {
  background: rgba(0, 0, 0, 0.5);

}

.view-btn:hover {
  background: rgba(0, 0, 0, 0.7);
}

.view-btn.active {
  background: rgba(0, 0, 0, 0.5);
}

.action-btn:hover {
  background: rgba(0, 0, 0, 0.7);
}
</style>
