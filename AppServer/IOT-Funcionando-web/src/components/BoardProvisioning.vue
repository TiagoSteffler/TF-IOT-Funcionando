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

    <!-- Step 1: Instructions -->
    <div v-if="step === 1" style="margin-top:16px">
      <div style="background:#e6f7ff; border-left:4px solid #1890ff; padding:12px; margin-bottom:16px">
        <h3 style="margin-top:0">Instructions</h3>
        <ol style="margin:8px 0; padding-left:20px">
          <li>Power on your new ESP32 board</li>
          <li>The ESP32 will start in AP mode (Access Point)</li>
          <li>Connect your computer to the ESP32's WiFi network (usually named "ESP32-Setup" or similar)</li>
          <li>Once connected, click "Start Setup" below</li>
          <li>Fill in the WiFi credentials and MQTT settings</li>
          <li>Click "Send Configuration" - the ESP32 will restart</li>
          <li>Reconnect to your regular WiFi network</li>
        </ol>
      </div>

      <div style="margin-top:16px">
        <p><strong>Board will be assigned Device ID:</strong> {{ nextDeviceId }}</p>
        <p><strong>Default MQTT ID:</strong> esp32_device_{{ nextDeviceId }}</p>
      </div>

      <div style="margin-top:20px">
        <button @click="startProvisioning" style="background:#1890ff; color:white; padding:10px 20px">
          Start Setup
        </button>
        <button @click="cancel" style="margin-left:8px">
          Cancel
        </button>
      </div>
    </div>

    <!-- Step 2: Configuration Form -->
    <div v-if="step === 2" style="margin-top:16px">
      <div style="background:#fff7e6; border-left:4px solid #fa8c16; padding:12px; margin-bottom:16px">
        <p style="margin:0"><strong>⚠️ Make sure you're connected to the ESP32's AP network</strong></p>
      </div>

      <div style="margin-top:12px">
        <h3>Board Information</h3>
        <div style="margin-top:8px">
          <label>Board Name</label>
          <input v-model="boardName" placeholder="e.g. ESP32 Living Room" />
        </div>
        <div style="margin-top:8px">
          <label>ESP32 MAC Address (optional)</label>
          <input v-model="espMac" placeholder="e.g. AA:BB:CC:DD:EE:FF" />
        </div>
      </div>

      <div style="margin-top:20px">
        <h3>MQTT Settings</h3>
        <div style="margin-top:8px">
          <label>Broker IP</label>
          <input v-model="mqttBroker" placeholder="e.g. 192.168.1.10 or mqtt.example.com" />
        </div>
        <div style="margin-top:8px">
          <label>Device ID (MQTT)</label>
          <input v-model="mqttDeviceId" placeholder="e.g. esp32_device_1" />
        </div>
      </div>

      <div style="margin-top:20px">
        <h3>WiFi Network Settings</h3>
        <div style="margin-top:8px">
          <label>WiFi Network (SSID)</label>
          <input v-model="wifiNetwork" placeholder="Your WiFi network name" />
        </div>
        <div style="margin-top:8px">
          <label>Password</label>
          <input v-model="wifiPassword" type="password" placeholder="WiFi password" />
        </div>
      </div>

      <div style="margin-top:20px">
        <button @click="sendConfiguration" style="background:#52c41a; color:white; padding:10px 20px">
          Send Configuration to ESP32
        </button>
        <button @click="cancel" style="margin-left:8px">
          Cancel
        </button>
      </div>
    </div>

    <!-- Step 3: Sending Configuration -->
    <div v-if="step === 3" style="margin-top:16px; text-align:center">
      <div style="margin:20px 0">
        <div style="display:inline-block; width:40px; height:40px; border:4px solid #1890ff; border-top-color:transparent; border-radius:50%; animation:spin 1s linear infinite"></div>
      </div>
      <h3>Sending Configuration to ESP32...</h3>
      <p>Please wait while the ESP32 receives the settings and restarts.</p>
      <p style="color:#666; font-size:14px">The ESP32 will disconnect from AP mode and connect to your WiFi.</p>
    </div>

    <!-- Step 4: Success -->
    <div v-if="step === 4" style="margin-top:16px">
      <div style="background:#f6ffed; border-left:4px solid #52c41a; padding:12px; margin-bottom:16px">
        <h3 style="margin-top:0; color:#52c41a">✓ Configuration Sent Successfully!</h3>
        <p style="margin:8px 0">The ESP32 is restarting and connecting to your WiFi network.</p>
      </div>

      <div style="margin-top:16px">
        <h4>Next Steps:</h4>
        <ol style="margin:8px 0; padding-left:20px">
          <li>Disconnect from the ESP32's AP network</li>
          <li>Reconnect to your regular WiFi network</li>
          <li>Wait a few seconds for the ESP32 to connect</li>
          <li>The new board will appear in your board list</li>
        </ol>
      </div>

      <div style="margin-top:20px">
        <p style="font-size:14px; color:#666">Closing this dialog in a moment...</p>
      </div>
    </div>
  </section>
</template>

<style scoped>
@keyframes spin {
  to { transform: rotate(360deg); }
}

label {
  display: block;
  font-weight: 500;
  margin-bottom: 4px;
}

input {
  width: 100%;
  padding: 8px;
  border: 1px solid #d9d9d9;
  border-radius: 4px;
  font-size: 14px;
}

input:focus {
  outline: none;
  border-color: #1890ff;
}

button {
  padding: 8px 16px;
  border: 1px solid #d9d9d9;
  border-radius: 4px;
  background: white;
  cursor: pointer;
  font-size: 14px;
}

button:hover {
  border-color: #1890ff;
  color: #1890ff;
}
</style>
