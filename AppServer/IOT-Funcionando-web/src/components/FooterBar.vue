<script setup>
import { computed, ref, onMounted, onBeforeUnmount } from 'vue'

const props = defineProps({
  boards: { type: Array, required: true },
  currentBoardId: { type: String, required: true }
})

const emit = defineEmits(['board-change'])

const selectBoard = (ev) => {
  emit('board-change', ev.target.value)
}

const currentBoard = computed(() => props.boards.find(b => b.id === props.currentBoardId))
const deviceStatus = ref({ online: false, timeSinceLastSeen: null })
let statusCheckInterval = null

const checkDeviceStatus = async () => {
  if (!currentBoard.value?.mac) {
    deviceStatus.value = { online: false, timeSinceLastSeen: null }
    return
  }
  
  try {
    const response = await fetch(`http://localhost:3001/api/devices/${currentBoard.value.mac}/status`)
    if (response.ok) {
      const data = await response.json()
      deviceStatus.value = data
    } else {
      deviceStatus.value = { online: false, timeSinceLastSeen: null }
    }
  } catch (err) {
    console.error('Error checking device status:', err)
    deviceStatus.value = { online: false, timeSinceLastSeen: null }
  }
}

const statusText = computed(() => {
  if (!deviceStatus.value.timeSinceLastSeen) return 'Desconhecido'
  const seconds = Math.floor(deviceStatus.value.timeSinceLastSeen / 1000)
  return `${seconds}s atrás`
})

onMounted(() => {
  checkDeviceStatus()
  statusCheckInterval = setInterval(checkDeviceStatus, 5000) // Check every 5 seconds
})

onBeforeUnmount(() => {
  if (statusCheckInterval) {
    clearInterval(statusCheckInterval)
  }
})
</script>

<template>
  <footer>
    <div>
      <label>Placa:</label>
      <select :value="currentBoardId" @change="selectBoard">
        <option v-for="b in boards" :key="b.id" :value="b.id">{{ b.name }}</option>
      </select>
    </div>

    <div v-if="currentBoard">
      <p>MAC: {{ currentBoard.mac }}</p>
      <p>IP: {{ currentBoard.ip }}</p>
      <p>MQTT ID: {{ currentBoard.mqtt }}</p>
    </div>

    <div v-if="currentBoard" class="status-indicator">
      <div class="status-row">
        <div class="status-dot" :class="{ online: deviceStatus.online, offline: !deviceStatus.online }"></div>
        <span class="status-text">{{ deviceStatus.online ? 'Online' : 'Offline' }}</span>
      </div>
      <p class="last-seen">{{ deviceStatus.online ? 'Heartbeat: ' + statusText : 'Sem heartbeat' }}</p>
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

/* Status indicator */
.status-indicator {
  display: flex;
  flex-direction: column;
  gap: 6px;
  align-items: flex-end;
}

.status-row {
  display: flex;
  align-items: center;
  gap: 8px;
}

.status-dot {
  width: 12px;
  height: 12px;
  border-radius: 50%;
  box-shadow: 0 0 8px currentColor;
  transition: all 0.3s ease;
}

.status-dot.online {
  background: #52c41a;
  color: #52c41a;
  animation: pulse 2s ease-in-out infinite;
}

.status-dot.offline {
  background: #ff4d4f;
  color: #ff4d4f;
}

@keyframes pulse {
  0%, 100% {
    opacity: 1;
    transform: scale(1);
  }
  50% {
    opacity: 0.7;
    transform: scale(1.1);
  }
}

.status-text {
  font-size: 0.95rem;
  font-weight: 600;
  text-shadow: 0 1px 4px rgba(0,0,0,0.5);
}

.last-seen {
  font-size: 0.8rem;
  opacity: 0.7;
  margin: 0;
}

/* responsivo */
@media (max-width: 750px) {
  footer {
    flex-direction: column;
    align-items: flex-start;
  }
}
</style>
