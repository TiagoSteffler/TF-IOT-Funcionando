<script setup>
import { ref, onMounted } from 'vue'

const emit = defineEmits(['provision-complete', 'cancel'])

const props = defineProps({
  nextDeviceId: { type: Number, required: true }
})

const step = ref(1) // 1=instructions, 2=settings, 3=pairing-waiting, 4=success

// MQTT settings
const mqttBroker = ref('')
const mqttDeviceId = ref(`esp32_device_${props.nextDeviceId}`)

// Network settings
const wifiNetwork = ref('')
const wifiPassword = ref('')

// Board info
const boardName = ref(`ESP32 #${props.nextDeviceId}`)
const espMac = ref('')
const espIp = ref('')

// Pairing state
const pairingActive = ref(false)

const startProvisioning = () => {
  step.value = 2
}

const startPairing = async () => {
  // Validate required fields
  if (!wifiNetwork.value || !wifiPassword.value || !mqttBroker.value) {
    alert('Favor preencher todas as configurações antes de iniciar o emparelhamento.')
    return
  }

  step.value = 3
  
  const payload = {
    mqtt: {
      broker: mqttBroker.value,
      deviceId: mqttDeviceId.value
    },
    network: {
      ssid: wifiNetwork.value,
      password: wifiPassword.value
    }
  }

  try {
    // Start pairing mode on the backend
    // Backend will now respond to ESP32 GET /ping requests with config
    const apiPayload = {
      ssid: wifiNetwork.value,
      password: wifiPassword.value,
      brokerIP: mqttBroker.value,
      port: 1883,
      id: mqttDeviceId.value
    }

    const resp = await fetch('/api/pairing/start', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(apiPayload)
    })

    if (!resp.ok) {
      const text = await resp.text().catch(() => '');
      throw new Error(`Falha ao iniciar emparelhamento: ${resp.status} ${text}`)
    }

    const json = await resp.json()
    console.log('Modo de emparelhamento ativado no backend:', json)
    pairingActive.value = true

    // Now waiting for ESP32 to:
    // 1. Connect to open network or hotspot
    // 2. Send GET /ping?mac=XX:XX:XX:XX:XX:XX
    // 3. Receive config and restart
    // 4. Connect to configured WiFi
    // 5. Send MQTT heartbeat
    
    // Poll for devices to detect when ESP32 connects
    pollForNewDevice()

  } catch (error) {
    console.error('Falha ao iniciar modo de emparelhamento:', error)
    const errorMsg = error.message || 'Erro desconhecido'
    alert(`Falha ao iniciar modo de emparelhamento: ${errorMsg}\n\nCertifique-se de que o servidor backend está rodando na porta 3001.`)
    step.value = 2
  }
}

const pollForNewDevice = async () => {
  // Poll backend for new devices (check every 3 seconds for up to 2 minutes)
  let attempts = 0
  const maxAttempts = 40 // 2 minutes

  const checkInterval = setInterval(async () => {
    attempts++
    
    try {
      const resp = await fetch('/api/devices')
      if (resp.ok) {
        const devices = await resp.json()
        
        // Look for our device ID
        const ourDevice = devices.find(d => d.id === mqttDeviceId.value)
        
        if (ourDevice && ourDevice.status === 'online') {
          console.log('ESP32 conectado!', ourDevice)
          clearInterval(checkInterval)
          
          // Update board info with real data
          espMac.value = ourDevice.mac
          espIp.value = ourDevice.ip
          
          // Stop pairing mode
          await stopPairing()
          
          // Show success
          step.value = 4
          
          // Emit to parent
          setTimeout(() => {
            emit('provision-complete', {
              id: `board-${props.nextDeviceId}`,
              deviceId: props.nextDeviceId,
              name: boardName.value,
              mac: ourDevice.mac,
              ip: ourDevice.ip,
              mqtt: mqttDeviceId.value
            })
          }, 2000)
        }
      }
    } catch (err) {
      console.error('Erro ao consultar dispositivos:', err)
    }
    
    if (attempts >= maxAttempts) {
      console.warn('⏱️ Tempo esgotado para emparelhamento - nenhum dispositivo conectado')
      clearInterval(checkInterval)
      await stopPairing()
      alert('Tempo esgotado para emparelhamento. ESP32 não se conectou dentro de 2 minutos. Por favor, tente novamente.')
      step.value = 2
    }
  }, 3000)
}

const stopPairing = async () => {
  if (!pairingActive.value) return
  
  try {
    const resp = await fetch('/api/pairing/stop', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' }
    })
    
    if (resp.ok) {
      console.log('Modo de emparelhamento desativado')
      pairingActive.value = false
    }
  } catch (error) {
    console.error('Erro ao desativar modo de emparelhamento:', error)
  }
}

const cancel = async () => {
  await stopPairing()
  emit('cancel')
}
</script>

<template>
  <section>
    <h2>Add New ESP32 Board</h2>

    <!-- STEP 1: Instructions -->
    <div v-if="step === 1">

      <div class="alert info">
        <h3>Instruções</h3>
        <ol>
          <li>Ligue sua nova placa ESP32</li>
          <li>ESP32 tentará se conectar a redes abertas ou hotspot com senha "12345678"</li>
          <li>Clique em "Iniciar Configuração" abaixo</li>
          <li>Preencha as configurações de WiFi + MQTT</li>
          <li>Clique em "Iniciar Emparelhamento" - o backend ouvirá o ESP32</li>
          <li>ESP32 enviará um ping para o gateway e receberá a configuração</li>
          <li>ESP32 reiniciará e se conectará ao seu WiFi</li>
          <li>Espere pela confirmação (dispositivo aparece na lista)</li>
        </ol>
      </div>

      <div class="field-block">
        <label>ID do dispositivo atribuído</label>
        <div class="static-text">{{ nextDeviceId }}</div>
      </div>

      <div class="field-block">
        <label>ID MQTT padrão</label>
        <div class="static-text">esp32_device_{{ nextDeviceId }}</div>
      </div>

      <div style="margin-top:20px">
        <button @click="startProvisioning">Iniciar configuração</button>
        <button @click="cancel" class="secondary">Cancelar</button>
      </div>
    </div>

    <!-- STEP 2: Configuration Form -->
    <div v-if="step === 2">

      <div class="alert warn">
        <strong>Certifique-se de que o ESP32 está ligado e procurando redes.</strong>
      </div>

      <h3 class="group-title">Informações da placa</h3>

      <div class="field-block">
        <label>Nome da placa</label>
        <input v-model="boardName" placeholder="e.g. ESP32 do galego" />
      </div>

      <h3 class="group-title">Configurações MQTT</h3>
      <div class="field-block">
        <label>Endereço IP do broker</label>
        <input v-model="mqttBroker" placeholder="192.168.1.10 ou mqtt.example.com" />
      </div>

      <div class="field-block">
        <label>ID do dispositivo (MQTT)</label>
        <input v-model="mqttDeviceId" />
      </div>

      <h3 class="group-title">Configurações da rede WiFi</h3>
      <div class="field-block">
        <label>Rede WiFi (SSID)</label>
        <input v-model="wifiNetwork" placeholder="Nome do WiFi" />
      </div>

      <div class="field-block">
        <label>Senha</label>
        <input v-model="wifiPassword" type="password" placeholder="Senha do WiFi" />
      </div>

      <div style="margin-top:20px">
        <button @click="startPairing">Iniciar emparelhamento</button>
        <button @click="cancel" class="secondary">Cancel</button>
      </div>
    </div>

    <!-- STEP 3: WAITING FOR ESP32 -->
    <div v-if="step === 3" class="center-area">
      <div class="spinner"></div>
      <h3>Aguardando ESP32…</h3>
      <p>Modo de emparelhamento ativo. ESP32 deve se conectar à rede e enviar uma solicitação de ping.</p>
      <p class="hint">Isso pode levar até 2 minutos. Certifique-se de que o ESP32 pode alcançar o gateway.</p>
    </div>

    <!-- STEP 4: SUCCESS -->
    <div v-if="step === 4">

      <div class="alert success">
        <h3>Configuração enviada com sucesso!</h3>
        <p>O ESP32 está reiniciando e conectando ao seu WiFi.</p>
      </div>

      <h4 class="group-title">O que aconteceu</h4>
      <ol class="final-steps">
        <li>ESP32 conectado à rede</li>
        <li>ESP32 enviou ping para o gateway e recebeu configuração</li>
        <li>ESP32 reiniciou e conectou ao seu WiFi</li>
        <li>ESP32 agora está enviando sinais vitais via MQTT</li>
      </ol>

      <p class="closing-hint">Fechando este diálogo em breve…</p>
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

h2 {
  margin-bottom: 16px;
  font-size: 1.4rem;
  text-shadow: 0 2px 4px rgba(0,0,0,0.5);
}

.group-title {
  margin-top: 24px;
  margin-bottom: 10px;
  font-size: 1.2rem;
  font-weight: 600;
}

/* Alerts */
.alert {
  padding: 12px 16px;
  font-size: 15px;
  border-radius: 10px;
  margin-top: 10px;
  margin-bottom: 20px;
}
.alert.info {
  background: rgba(24,144,255,0.15);
  border-left: 4px solid #1890ff;
}
.alert.warn {
  background: rgba(250,140,22,0.15);
  border-left: 4px solid #fa8c16;
}
.alert.success {
  background: rgba(82,196,26,0.15);
  border-left: 4px solid #52c41a;
}

/* Blocks */
.field-block {
  width: 95%;
  margin-top: 13px;
  padding-left: 20px;
  padding-right: 20px;
}

label {
  font-size: 0.95rem;
  font-weight: 600;
  display: block;
  margin-bottom: 6px;
  text-shadow: 0 1px 6px rgba(0,0,0,0.5);
}

input {
  width: 100%;
  padding: 10px 12px;
  border-radius: 12px;
  background: rgba(0, 0, 0, 0.35);
  border: 1px solid rgba(255, 255, 255, 0.25);
  color: white;
  font-size: 0.95rem;
  box-shadow: inset 0 0 10px rgba(0,0,0,0.3);
}
input:focus {
  outline: none;
  border-color: rgba(255,255,255,0.5);
  background: rgba(0, 0, 0, 0.45);
}

.static-text {
  padding: 10px 12px;
  border-radius: 12px;
  background: rgba(255,255,255,0.07);
  border: 1px solid rgba(255,255,255,0.15);
  font-size: 0.95rem;
}

/* Buttons */
button {
  width: 200px;
  height: 55px;
  border: none;
  border-radius: 30px;
  background: rgba(0, 0, 0, 0.45);
  font-size: 16px;
  font-weight: 600;
  color: white;
  cursor: pointer;
  box-shadow: 0 3px 10px rgba(0,0,0,0.25);
  transition: 0.2s;
  margin-top: 10px;
}
button.secondary {
  background: rgba(255,255,255,0.15);
}
button:hover {
  transform: translateY(-2px);
  box-shadow: 0 5px 14px rgba(0,0,0,0.35);
}

/* Step 3 */
.center-area {
  text-align: center;
  margin-top: 30px;
}
.spinner {
  width: 45px;
  height: 45px;
  border: 4px solid #1890ff;
  border-top-color: transparent;
  border-radius: 50%;
  margin: 20px auto;
  animation: spin 1s linear infinite;
}
@keyframes spin {
  to { transform: rotate(360deg); }
}
.hint {
  font-size: 14px;
  opacity: 0.8;
  margin-top: 10px;
}

/* Step 4 */
.final-steps {
  padding-left: 20px;
}
.closing-hint {
  margin-top: 18px;
  font-size: 14px;
  opacity: 0.8;
}
</style>
