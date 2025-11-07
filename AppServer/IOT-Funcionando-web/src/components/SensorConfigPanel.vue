<template>
  <div class="config-modal">
    <div class="config-backdrop" @click="$emit('close')"></div>
    <div class="config-card">
      <div class="config-header">
        <h3>Configure {{ config.type === 'sensor' ? 'Sensor' : 'Actuator' }} — GPIO {{ pin.number }}</h3>
        <button class="close-btn" @click="$emit('close')">✕</button>
      </div>

      <div class="config-body">
        <div class="form-group">
          <label>Name:</label>
          <input type="text" v-model="config.name" placeholder="Enter name">
        </div>

        <div class="form-group">
          <label>Type:</label>
          <select v-model="config.type">
            <option value="sensor">Sensor</option>
            <option value="actuator">Actuator</option>
          </select>
        </div>

        <div class="form-group">
          <label>Color:</label>
          <input type="color" v-model="config.color">
        </div>

        <div class="form-group">
          <label>Mode:</label>
          <select v-model="config.mode">
            <option value="digital">Digital</option>
            <option value="analog">Analog</option>
          </select>
        </div>

        <template v-if="config.type === 'sensor'">
          <div class="form-group">
            <label>Read Interval (ms):</label>
            <input type="number" v-model.number="config.readInterval" min="100">
          </div>

          <div class="form-group">
            <label>Threshold:</label>
            <input type="number" v-model.number="config.threshold">
          </div>
        </template>

        <div class="form-group">
          <label class="checkbox-label">
            <input type="checkbox" v-model="config.enabled">
            Enabled
          </label>
        </div>

        <div class="actions">
          <button class="save-btn" @click="saveConfig">Save Configuration</button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref } from 'vue'

const props = defineProps({
  pin: { type: Object, required: true }
})

const emit = defineEmits(['close', 'save'])

const config = ref({
  name: props.pin.name || '',
  type: props.pin.type || 'sensor', // sensor or actuator
  color: props.pin.color || '#3182ce',
  readInterval: props.pin.readInterval || 1000,
  enabled: props.pin.enabled !== false,
  mode: props.pin.mode || 'digital', // digital or analog
  threshold: props.pin.threshold || 0,
})

const saveConfig = () => {
  emit('save', {
    ...props.pin,
    ...config.value
  })
  emit('close')
}
</script>

<style scoped>
.config-modal {
  position: fixed;
  inset: 0;
  z-index: 5000;
  display: flex;
  align-items: center;
  justify-content: center;
}

.config-backdrop {
  position: absolute;
  inset: 0;
  background: rgba(0, 0, 0, 0.5);
}

.config-card {
  position: relative;
  background: white;
  border-radius: 8px;
  padding: 1.5rem;
  width: 90%;
  max-width: 500px;
  box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
}

.config-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 1.5rem;
}

.config-header h3 {
  margin: 0;
  font-size: 1.25rem;
  font-weight: 600;
}

.close-btn {
  background: none;
  border: none;
  font-size: 1.5rem;
  cursor: pointer;
  padding: 0.5rem;
}

.config-body {
  display: flex;
  flex-direction: column;
  gap: 1rem;
}

.form-group {
  display: flex;
  flex-direction: column;
  gap: 0.5rem;
}

.form-group label {
  font-weight: 500;
  color: #4a5568;
}

.form-group input[type="text"],
.form-group input[type="number"],
.form-group select {
  padding: 0.5rem;
  border: 1px solid #e2e8f0;
  border-radius: 4px;
  font-size: 1rem;
}

.checkbox-label {
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.actions {
  margin-top: 1rem;
  display: flex;
  justify-content: flex-end;
}

.save-btn {
  background: #3182ce;
  color: white;
  border: none;
  padding: 0.5rem 1rem;
  border-radius: 4px;
  cursor: pointer;
  font-weight: 500;
}

.save-btn:hover {
  background: #2c5282;
}
</style>