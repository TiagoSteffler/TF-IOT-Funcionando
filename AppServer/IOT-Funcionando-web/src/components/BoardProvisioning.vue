<script setup>
import { ref } from 'vue'

const emit = defineEmits(['provision-complete', 'cancel'])

const props = defineProps({
  nextDeviceId: { type: Number, required: true }
})

const step = ref(1) // 1=instructions, 2=settings, 3=connecting, 4=success

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

const startProvisioning = () => {
  step.value = 2
}

const sendConfiguration = async () => {
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
    // TODO: Replace with actual ESP32 AP endpoint when available
    // Example: POST to http://192.168.4.1/configure
    console.log('Sending configuration to ESP32 AP:', payload)
    
    // Simulate API call
    await new Promise(resolve => setTimeout(resolve, 2000))
    
    // TODO: Detect when ESP32 restarts and disconnects
    // For now, simulate success
    step.value = 4
    
    // Emit the new board info to parent
    setTimeout(() => {
      emit('provision-complete', {
        id: `board-${props.nextDeviceId}`,
        deviceId: props.nextDeviceId,
        name: boardName.value,
        mac: espMac.value || 'Unknown',
        ip: espIp.value || 'Pending',
        mqtt: mqttDeviceId.value
      })
    }, 1000)
    
  } catch (error) {
    console.error('Provisioning failed:', error)
    alert('Failed to send configuration to ESP32. Please try again.')
    step.value = 2
  }
}

const cancel = () => {
  emit('cancel')
}
</script>

<template>
  <section>
    <h2>Add New ESP32 Board</h2>

    <!-- STEP 1: Instructions -->
    <div v-if="step === 1">

      <div class="alert info">
        <h3>Instructions</h3>
        <ol>
          <li>Power on your new ESP32 board</li>
          <li>The ESP32 will start in AP mode</li>
          <li>Connect to its WiFi (ESP32-Setup)</li>
          <li>Click “Start Setup”</li>
          <li>Fill WiFi + MQTT settings</li>
          <li>Click “Send Configuration”</li>
          <li>Reconnect to your normal WiFi</li>
        </ol>
      </div>

      <div class="field-block">
        <label>Assigned Device ID</label>
        <div class="static-text">{{ nextDeviceId }}</div>
      </div>

      <div class="field-block">
        <label>Default MQTT ID</label>
        <div class="static-text">esp32_device_{{ nextDeviceId }}</div>
      </div>

      <div style="margin-top:20px">
        <button @click="startProvisioning">Start Setup</button>
        <button @click="cancel" class="secondary">Cancel</button>
      </div>
    </div>

    <!-- STEP 2: Configuration Form -->
    <div v-if="step === 2">

      <div class="alert warn">
        <strong>⚠️ Make sure you're connected to the ESP32 AP</strong>
      </div>

      <h3 class="group-title">Board Information</h3>

      <div class="field-block">
        <label>Board Name</label>
        <input v-model="boardName" placeholder="e.g. ESP32 Living Room" />
      </div>

      <div class="field-block">
        <label>ESP32 MAC Address (optional)</label>
        <input v-model="espMac" placeholder="AA:BB:CC:DD:EE:FF" />
      </div>

      <h3 class="group-title">MQTT Settings</h3>

      <div class="field-block">
        <label>Broker IP</label>
        <input v-model="mqttBroker" placeholder="192.168.1.10 or mqtt.example.com" />
      </div>

      <div class="field-block">
        <label>Device ID (MQTT)</label>
        <input v-model="mqttDeviceId" />
      </div>

      <h3 class="group-title">WiFi Network Settings</h3>

      <div class="field-block">
        <label>WiFi Network (SSID)</label>
        <input v-model="wifiNetwork" placeholder="Your WiFi name" />
      </div>

      <div class="field-block">
        <label>Password</label>
        <input v-model="wifiPassword" type="password" placeholder="WiFi password" />
      </div>

      <div style="margin-top:20px">
        <button @click="sendConfiguration">Send Configuration</button>
        <button @click="cancel" class="secondary">Cancel</button>
      </div>
    </div>

    <!-- STEP 3: SENDING -->
    <div v-if="step === 3" class="center-area">
      <div class="spinner"></div>
      <h3>Sending Configuration…</h3>
      <p>Please wait while the ESP32 restarts.</p>
    </div>

    <!-- STEP 4: SUCCESS -->
    <div v-if="step === 4">

      <div class="alert success">
        <h3>✓ Configuration Sent Successfully!</h3>
        <p>The ESP32 is restarting and connecting to your WiFi.</p>
      </div>

      <h4 class="group-title">Next Steps</h4>

      <ol class="final-steps">
        <li>Disconnect from ESP32 AP</li>
        <li>Reconnect to your WiFi</li>
        <li>Wait a few seconds</li>
        <li>The new board will appear in your list</li>
      </ol>

      <p class="closing-hint">Closing this dialog shortly…</p>
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
