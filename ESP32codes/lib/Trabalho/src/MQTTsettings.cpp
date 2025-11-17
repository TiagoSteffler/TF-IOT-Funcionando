#include <Trabalho.hpp>

// ========== DEFINIÇÕES DAS VARIÁVEIS GLOBAIS ==========
// Clientes WiFi e MQTT
WiFiClient espClient;
PubSubClient MQTT(espClient);

// Configurações MQTT e WiFi
MQTTConfig mqttConfig;
WiFiConfig wifiConfig;

// Controle de heartbeat
unsigned long lastPing = 0;
const unsigned long PING_INTERVAL = 10000;

// Gerenciador I2C
I2C_Manager i2c;

bool publishGetterMQTT;
String payloadSensors;
String payloadTopicSensors;
// ======================================================
