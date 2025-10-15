//Programa: Sistema IoT ESP32 com MQTT - Sensores e Automação
//Adaptado para trabalho final IoT 2025
#include <Trabalho.hpp>

/* Variáveis globais */

unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 5000; // Intervalo de leitura dos sensores (5 segundos)
float simulatedTemp = 25.0; // Temperatura simulada inicial
  
//Prototypes
void init_serial(void);
void init_pins(void);
void init_wifi(void);
void init_mqtt(void);
void reconnect_wifi(void); 

float readTemperature(void);
 
/* 
 *  Implementações das funções
 */
void setup() {
    init_serial();
    //init_pins();
    init_wifi();
    init_mqtt();
    
    Serial.println("====================================");
    Serial.println("Sistema IoT ESP32 Iniciado!");
    Serial.println("====================================");
}
  
/// @brief Inicializacao da comunicacao serial
void init_serial() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nIniciando sistema IoT...");
}

/// @brief Inicializacao
void init_pins() {
    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    pinMode(PIN_TEMP_SENSOR, INPUT);
    
    digitalWrite(PIN_LED, LOW); // LED inicia desligado
    Serial.println("Pinos configurados!");
}
 
/// @brief Inicializacao da conexao Wi-Fi
void init_wifi(void) {
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");
    reconnect_wifi();
}
  

 

void loop() {   
    // Garante funcionamento das conexões WiFi e ao broker MQTT
    verifica_conexoes_wifi_mqtt();
    
    // Ler e publicar dados dos sensores periodicamente
    unsigned long currentMillis = millis();
    if (currentMillis - lastSensorRead >= SENSOR_INTERVAL) {
        lastSensorRead = currentMillis;
        readAndPublishSensors();
    }
    
    // Teste manual com botão (publicar imediatamente)
    if (digitalRead(PIN_BUTTON) == LOW) {
        delay(50); // Debounce
        if (digitalRead(PIN_BUTTON) == LOW) {
            Serial.println("[BOTÃO] Publicação manual acionada!");
            readAndPublishSensors();
            while(digitalRead(PIN_BUTTON) == LOW); // Aguarda soltar
        }
    }
    
    // Keep-alive da comunicação com broker MQTT
    MQTT.loop();
    
    // Pequeno delay para não sobrecarregar
    delay(100);
}