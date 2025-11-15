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

<style scoped>
footer {
  width: 70%;
  height: 110px;
  padding: 20px 26px 40px 26px;

  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 32px;

  background: rgba(0, 0, 0, 0.5);
  backdrop-filter: blur(8px);

  border-radius: 30px;
  box-shadow: 0 -4px 18px rgba(0,0,0,0.25);

  color: white;
  font-family: Arial, sans-serif;

  position: relative;      /* permite deslocamento com bottom */
  bottom: -35px;           /* ultrapassa 40px o limite inferior da página */
}


/* blocos */
footer div {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

/* label "Board" */
label {
  font-size: 0.95rem;
  font-weight: 600;
  text-shadow: 0 1px 6px rgba(0,0,0,0.5);
}

/* select */
select {
  padding: 8px 12px;
  border-radius: 12px;

  background: rgba(0, 0, 0, 0.35);
  border: 1px solid rgba(255, 255, 255, 0.25);
  color: white;

  font-size: 0.95rem;
  cursor: pointer;
  transition: 0.2s;

  box-shadow: inset 0 0 10px rgba(0,0,0,0.3);
}

select:hover {
  background: rgba(0,0,0,0.45);
}

select:focus {
  outline: none;
  border-color: rgba(255,255,255,0.5);
}

/* textos das infos */
p {
  margin: 0;
  font-size: 0.95rem;
  opacity: 0.9;
  text-shadow: 0 1px 4px rgba(0,0,0,0.5);
}

/* responsivo */
@media (max-width: 750px) {
  footer {
    flex-direction: column;
    align-items: flex-start;
  }
}
</style>
