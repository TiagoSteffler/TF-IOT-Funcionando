<script setup>
import { ref, onMounted, watch } from 'vue'

const props = defineProps({
  deviceId: { type: Number, default: 1 }
})

const emit = defineEmits(['save-settings', 'erase-network', 'erase-all'])

// MQTT settings
const mqttBroker = ref('')
const mqttDeviceId = ref('')

// Network settings
const wifiNetwork = ref('')
const wifiPassword = ref('')

// Loading states
const loading = ref(false)
const saving = ref(false)
const error = ref(null)
const successMessage = ref(null)

// Fetch current WiFi settings from API
const fetchWifiSettings = async (deviceId) => {
  loading.value = true
  error.value = null
  try {
    const mqttId = `esp32_device_${deviceId}`
    const response = await fetch(`http://localhost:5000/${mqttId}/settings/wifi/get`)
    if (!response.ok) {
      throw new Error(`Falha ao buscar configurações WiFi: ${response.status}`)
    }
    const data = await response.json()
    
    // Note: The API sends MQTT request, response comes via MQTT topic
    // For now, we'll show the request was sent
    console.log('Solicitação de configurações WiFi enviada:', data)
  } catch (err) {
    error.value = err.message
    console.error('Erro ao buscar configurações WiFi:', err)
  } finally {
    loading.value = false
  }
}

const saveSettings = async () => {
  saving.value = true
  error.value = null
  successMessage.value = null
  
  try {
    const mqttId = `esp32_device_${props.deviceId}`
    
    // Prepare WiFi configuration payload
    const wifiPayload = {
      ssid: wifiNetwork.value,
      password: wifiPassword.value,
      mqtt_broker: mqttBroker.value,
      mqtt_device_id: mqttDeviceId.value
    }
    
    // Send WiFi configuration via API
    const response = await fetch(`http://localhost:5000/${mqttId}/settings/wifi/set`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify(wifiPayload)
    })
    
    if (!response.ok) {
      throw new Error(`Falha ao salvar configurações: ${response.status}`)
    }
    
    const result = await response.json()
    successMessage.value = 'Configurações enviadas para o dispositivo com sucesso!'
    console.log('Configurações salvas:', result)
    
    // Emit to parent for any additional handling
    emit('save-settings', {
      mqtt: {
        broker: mqttBroker.value,
        deviceId: mqttDeviceId.value
      },
      network: {
        ssid: wifiNetwork.value,
        password: wifiPassword.value
      }
    })
    
    // Clear success message after 3 seconds
    setTimeout(() => {
      successMessage.value = null
    }, 3000)
    
  } catch (err) {
    error.value = err.message
    console.error('Erro ao salvar configurações:', err)
  } finally {
    saving.value = false
  }
}

const eraseNetwork = () => {
  if (confirm('Apagar configuração de rede? O ESP32 perderá as configurações de conexão WiFi.')) {
    emit('erase-network')
  }
}

const eraseAll = async () => {
  if (!confirm('Apagar TODAS as configurações (rede + sensores)? Isso irá resetar o ESP32 e não pode ser desfeito.')) {
    return
  }
  
  saving.value = true
  error.value = null
  successMessage.value = null
  
  try {
    const mqttId = `esp32_device_${props.deviceId}`
    
    // Send reset command via API
    const response = await fetch(`http://localhost:5000/${mqttId}/settings/device/reset`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      }
    })
    
    if (!response.ok) {
      throw new Error(`Falha ao resetar dispositivo: ${response.status}`)
    }
    
    const result = await response.json()
    successMessage.value = 'Comando de reset enviado! Configuração do dispositivo apagada.'
    console.log('Dispositivo resetado:', result)
    
    // Clear local form data
    mqttBroker.value = ''
    mqttDeviceId.value = `esp32_device_${props.deviceId}`
    wifiNetwork.value = ''
    wifiPassword.value = ''
    
    // Emit to parent for additional handling
    emit('erase-all')
    
    // Clear success message after 5 seconds
    setTimeout(() => {
      successMessage.value = null
    }, 5000)
    
  } catch (err) {
    error.value = err.message
    console.error('Erro ao resetar dispositivo:', err)
  } finally {
    saving.value = false
  }
}

// Initialize device ID when component mounts or device changes
onMounted(() => {
  mqttDeviceId.value = `esp32_device_${props.deviceId}`
})

watch(() => props.deviceId, (newId) => {
  mqttDeviceId.value = `esp32_device_${newId}`
})

// Clear InfluxDB data
const clearingInfluxDB = ref(false)

const clearInfluxDB = async () => {
  const confirmed = confirm(
    'AVISO: Isso irá APAGAR TODOS os dados dos sensores do InfluxDB!\n\n' +
    'Isso inclui leituras históricas de todos os sensores e placas.\n' +
    'Esta ação não poderá ser desfeita.\n\n' +
    'Você tem certeza absoluta de que deseja continuar?'
  )
  
  if (!confirmed) {
    return
  }
  
  // Double confirmation
  const doubleConfirm = confirm(
    'CONFIRMAÇÃO FINAL\n\n' +
    'Todos os dados dos sensores serão permanentemente apagados.\n\n' +
    'Clique em OK para continuar, ou Cancelar para abortar.'
  )
  
  if (!doubleConfirm) {
    return
  }
  
  clearingInfluxDB.value = true
  error.value = null
  successMessage.value = null
  
  try {
    const response = await fetch('/influxdb/clear', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      }
    })
    
    if (!response.ok) {
      throw new Error(`Failed to clear InfluxDB: ${response.status}`)
    }
    
    const result = await response.json()
    successMessage.value = 'InfluxDB apagado com sucesso! Todos os dados dos sensores foram deletados.'
    console.log('InfluxDB cleared:', result)
    
    setTimeout(() => {
      successMessage.value = null
    }, 5000)
    
  } catch (err) {
    error.value = err.message
    console.error('Erro ao apagar InfluxDB:', err)
  } finally {
    clearingInfluxDB.value = false
  }
}

</script>

<template>
  <section>
    <h2>Configurações da placa</h2>

    <!-- Loading indicator -->
    <div v-if="loading" style="margin-top:12px; color:#666">
      <p>Carregando configurações...</p>
    </div>

    <!-- Success message -->
    <div v-if="successMessage" style="margin-top:12px; padding:12px; background:#f6ffed; border-left:4px solid #52c41a; color:#52c41a">
      {{ successMessage }}
    </div>

    <!-- Error message -->
    <div v-if="error" style="margin-top:12px; padding:12px; background:#fff2f0; border-left:4px solid #ff4d4f; color:#ff4d4f">
      Erro: {{ error }}
    </div>

    <div style="margin-top:12px">
      <h3>Configurações MQTT</h3>
      <div class="field-block">
        <label>Endereço IP do broker:</label>
        <input v-model="mqttBroker" placeholder="e.g. 192.168.1.10 ou mqtt.example.com" :disabled="saving" />
      </div>
      <div class="field-block">
        <label>ID do dispositivo:</label>
        <input v-model="mqttDeviceId" placeholder="ex.: esp32_device_1" :disabled="saving" />
      </div>
    </div>

    <div style="margin-top:12px">
      <h3>Configurações de rede</h3>
      <div class="field-block">
        <label>Nome da rede (SSID):</label>
        <input v-model="wifiNetwork" placeholder="Nome da rede WiFi" :disabled="saving" />
      </div>

      <div class="field-block">
        <label>Senha:</label>
        <input v-model="wifiPassword" type="password" placeholder="Senha WiFi" :disabled="saving" />
      </div>
    </div>

    <div class="button-row">
      <button @click="saveSettings" :disabled="saving">
        {{ saving ? 'Salvando...' : 'Salvar Configurações' }}
      </button>
    </div>

    <div class="danger-zone">
      <h3>Danger Zone</h3>

      <div class="button-row">
        <button class="danger-strong" @click="eraseNetwork">Apagar configurações de conexão</button>
        <button class="danger-strong" @click="eraseAll" :disabled="saving">
          {{ saving ? 'Resetando...' : 'Apagar todas as configurações' }}
        </button>
        <button class="danger-strong" @click="clearInfluxDB" :disabled="clearingInfluxDB">
          {{ clearingInfluxDB ? 'Apagando banco de dados...' : 'Apagar todos os dados do InfluxDB' }}
        </button>
      </div>
      <p style="font-size:12px; color:#ff7875; margin-top:8px">
        ⚠️ Isso irá apagar permanentemente TODAS as leituras dos sensores do InfluxDB. Use isso apenas em casos de desconfiguração de dispositivos previamente cadastrados.
      </p>
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

h3 {
  margin-top: 18px;
  margin-bottom: 6px;
  font-size: 1.15rem;
  text-shadow: 0 1px 4px rgba(0,0,0,0.4);
}

/* separador */
.divider {
  margin: 10px 0;
  border-top: 1px solid rgba(255,255,255,0.15);
}

/* blocos uniformes */
.field-block {
  width: 100%;
  box-sizing: border-box;
  margin-top: 8px;
  padding-left: 20px;
  padding-right: 20px;
  border-radius: 10px;
}

/* labels */
label {
  font-size: 0.95rem;
  font-weight: 600;
  display: block;
  margin-bottom: 2px;
  text-shadow: 0 1px 6px rgba(0,0,0,0.5);
}

/* inputs */
.field-block input {
  width: 97%;
  padding: 10px 12px;
  border-radius: 12px;

  background: rgba(0, 0, 0, 0.35);
  border: 1px solid rgba(255, 255, 255, 0.25);
  color: white;
  font-size: 0.95rem;

  box-shadow: inset 0 0 10px rgba(0,0,0,0.3);
  transition: 0.2s;
}

.field-block input:focus {
  outline: none;
  border-color: rgba(255,255,255,0.5);
  background: rgba(0, 0, 0, 0.45);
}

/* botão padrão */
button {
  width: 200px;
  height: 55px;
  padding: 8px 14px;

  border: none;
  border-radius: 30px;

  background-color: rgba(0, 0, 0, 0.5);

  font-size: 16px;
  font-weight: 600;
  color: white;

  cursor: pointer;
  transition: 0.15s;
  white-space: nowrap;

  box-shadow: 0 3px 10px rgba(0,0,0,0.25);
}

button:hover:not(:disabled) {
  transform: translateY(-2px);
  box-shadow: 0 5px 14px rgba(0,0,0,0.35);
}

.button-row {
  margin-top: 25px;
  display: flex;
  gap: 12px;
  flex-wrap: wrap;
}

button.danger {
  background: rgba(255, 80, 80, 0.5);
}

button.danger-strong {
  background: rgba(255, 40, 40, 0.7);
}

button.danger:hover {
  background: rgba(255, 80, 80, 0.7);
}

button.danger-strong:hover {
  background: rgba(255, 40, 40, 0.85);
}
</style>
