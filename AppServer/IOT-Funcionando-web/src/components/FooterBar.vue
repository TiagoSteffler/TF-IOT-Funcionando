<script setup>
import { ref } from 'vue'

const boards = ref([
  { id: 'board-1', name: 'ESP32 #1', mac: 'AA:BB:CC:11:22:33', ip: '192.168.1.100', mqtt: 'esp32_1' },
  { id: 'board-2', name: 'ESP32 #2', mac: 'AA:BB:CC:44:55:66', ip: '192.168.1.101', mqtt: 'esp32_2' }
])

const current = ref(boards.value[0].id)

const selectBoard = (ev) => {
  current.value = ev.target.value
}
</script>

<template>
  <footer>
    <div>
      <label>Board:</label>
      <select :value="current" @change="selectBoard">
        <option v-for="b in boards" :key="b.id" :value="b.id">{{ b.name }}</option>
      </select>
    </div>

    <div>
      <p>MAC: {{ boards.find(b => b.id === current)?.mac }}</p>
      <p>IP: {{ boards.find(b => b.id === current)?.ip }}</p>
      <p>MQTT ID: {{ boards.find(b => b.id === current)?.mqtt }}</p>
    </div>
  </footer>
</template>
