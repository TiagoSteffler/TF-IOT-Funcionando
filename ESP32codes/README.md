# ESP32 IoT - Código com PlatformIO

Código para ESP32 que se comunica via MQTT com o backend, enviando dados de sensores e recebendo comandos de controle.

## 🔧 Hardware Necessário

### Mínimo (para teste)
- ESP32 (qualquer modelo)
- LED conectado no pino 25
- Botão conectado no pino 32 (com pull-up)

### Completo (para produção)
- ESP32
- Sensor de Temperatura DS18B20
- Acelerômetro MPU6050
- Sensor de gestos APDS9960
- Sensor ultrassônico HC-SR04
- Relé JQC3F
- Servo SG90
- Joystick KY023
- Teclado matricial 4x4
- Controle IR

## 📦 Bibliotecas Utilizadas

```ini
WiFi (built-in)           # Conexão Wi-Fi
PubSubClient             # Cliente MQTT
ArduinoJson              # Serialização JSON
WiFiManager (opcional)   # Configuração Wi-Fi via portal
```

## 🚀 Configuração

### 1. Editar credenciais

Em `src/main.cpp`, altere:

```cpp
const char* SSID = "SEU_WIFI";
const char* PASSWORD = "SUA_SENHA";
const char* BROKER_MQTT = "IP_DO_SERVIDOR"; // Ex: "192.168.1.100"
```

### 2. Ajustar ID do dispositivo

Cada ESP32 deve ter um ID único:

```cpp
#define ID_MQTT  "ESP32_001"  // Mudar para cada dispositivo
```

### 3. Configurar pinos

Ajuste os pinos conforme seu hardware:

```cpp
#define PIN_LED 25
#define PIN_BUTTON 32
#define PIN_TEMP_SENSOR 34
```

## 📡 Protocolo de Comunicação

### Tópicos MQTT

**Publicação (ESP32 → Backend):**
```
/iot2025/sensors/70
```

**Subscrição (Backend → ESP32):**
```
/iot2025/led/70
```

### Formato das Mensagens

**Dados de sensores (JSON):**
```json
{
  "temperature": 25.3,
  "timestamp": 15234
}
```

**Comandos de controle:**
```
"1"  -> Liga LED
"0"  -> Desliga LED
```

## 🔨 Compilação e Upload

### Via PlatformIO CLI

```bash
# Compilar
pio run

# Upload
pio run --target upload

# Monitor serial
pio device monitor
```

### Via VS Code

1. Abra a pasta `ESP32codes`
2. PlatformIO: Build (Ctrl+Alt+B)
3. PlatformIO: Upload (Ctrl+Alt+U)
4. PlatformIO: Serial Monitor

## 📊 Monitoramento

O ESP32 imprime logs no monitor serial (115200 baud):

```
====================================
Sistema IoT ESP32 Iniciado!
====================================
Conectado na rede: MinhaRede
IP obtido: 192.168.1.50
Conectado ao broker MQTT!
------------------------------------
[SENSORES] Dados publicados:
  Temperatura: 25.3 °C
  JSON: {"temperature":25.3,"timestamp":15234}
------------------------------------
```

## 🧪 Modo de Teste

### Temperatura Simulada

Por padrão, o código simula leituras de temperatura entre 20-35°C para testes sem hardware adicional.

Para usar sensor real (DS18B20), substitua a função `readTemperature()`:

```cpp
#include <OneWire.h>
#include <DallasTemperature.h>

OneWire oneWire(PIN_TEMP_SENSOR);
DallasTemperature sensors(&oneWire);

float readTemperature(void) {
    sensors.requestTemperatures();
    return sensors.getTempCByIndex(0);
}
```

### Botão de Teste

Pressione o botão (pino 32) para forçar publicação imediata de dados.

## 🔄 Fluxo de Operação

1. **Inicialização:**
   - Conecta ao Wi-Fi
   - Conecta ao broker MQTT
   - Subscribe no tópico de controle

2. **Loop principal:**
   - A cada 5 segundos: lê sensores e publica via MQTT
   - Continuamente: verifica comandos recebidos
   - Botão pressionado: publica imediatamente

3. **Recebimento de comandos:**
   - Callback MQTT é acionado
   - Interpreta comando (0/1)
   - Controla LED conforme comando

## 🔧 Troubleshooting

**Não conecta ao Wi-Fi:**
- Verifique SSID e senha
- Use rede 2.4GHz (ESP32 não suporta 5GHz)

**Não conecta ao broker:**
- Verifique IP do servidor
- Confirme que backend está rodando
- Teste ping para o servidor

**Dados não chegam no backend:**
- Verifique o tópico MQTT
- Confirme formato JSON
- Veja logs do monitor serial

**LED não responde:**
- Verifique pino do LED
- Teste manualmente: `digitalWrite(25, HIGH)`
- Confirme recebimento no monitor serial

## 📚 Próximos Passos

- [ ] Adicionar sensores reais
- [ ] Implementar deep sleep para economia de energia
- [ ] Adicionar OTA (update over-the-air)
- [ ] Implementar reconexão automática melhorada
- [ ] Adicionar mais tipos de sensores

## 📖 Ver também

- `SETUP_INICIAL.md` - Guia completo de setup
- `../AppServer/Backend/` - Código do backend