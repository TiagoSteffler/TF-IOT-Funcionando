import { fileURLToPath, URL } from 'node:url'

import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import vueDevTools from 'vite-plugin-vue-devtools'

// https://vite.dev/config/
export default defineConfig({
  plugins: [
    vue(),
    vueDevTools(),
  ],
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url))
    },
  },
  server: {
    proxy: {
      '/api': {
        target: process.env.VITE_BACKEND_URL || 'http://backend:3001',
        changeOrigin: true,
        secure: false
      },
      '/ping': {
        target: process.env.VITE_BACKEND_URL || 'http://backend:3001',
        changeOrigin: true,
        secure: false
      },
      // Proxy sensor data requests to api_server (Flask on port 5000)
      '/esp32_': {
        target: 'http://api_server:5000',
        changeOrigin: true,
        secure: false
      },
      // Proxy rules endpoint to api_server (Flask on port 5000)
      '/rules': {
        target: 'http://api_server:5000',
        changeOrigin: true,
        secure: false
      },
      // Proxy InfluxDB management endpoints to api_server
      '/influxdb': {
        target: 'http://api_server:5000',
        changeOrigin: true,
        secure: false
      }
    }
  }
})
