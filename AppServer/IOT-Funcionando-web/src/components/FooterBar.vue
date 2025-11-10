<script setup>
import { computed } from 'vue'

const props = defineProps({
  boards: { type: Array, required: true },
  currentBoardId: { type: String, required: true }
})

const emit = defineEmits(['board-change'])

const selectBoard = (ev) => {
  emit('board-change', ev.target.value)
}

const currentBoard = computed(() => props.boards.find(b => b.id === props.currentBoardId))
</script>

<template>
  <footer>
    <div>
      <label>Board:</label>
      <select :value="currentBoardId" @change="selectBoard">
        <option v-for="b in boards" :key="b.id" :value="b.id">{{ b.name }}</option>
      </select>
    </div>

    <div v-if="currentBoard">
      <p>MAC: {{ currentBoard.mac }}</p>
      <p>IP: {{ currentBoard.ip }}</p>
      <p>MQTT ID: {{ currentBoard.mqtt }}</p>
    </div>
  </footer>
</template>
