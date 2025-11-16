//Programa: Sistema IoT ESP32 com MQTT - Sensores e Automação
//Adaptado para trabalho final IoT 2025
#include <Trabalho.hpp>

/* Variáveis globais */

unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 5000; // Intervalo de leitura dos sensores (5 segundos)

vector<Sensor> sensores;
  
//Prototypes
void init_serial(void);
void init_pins(void);
void init_wifi(void);

 
/* 
 *  Implementações das funções
 */
I2C_Manager i2c; // Objeto gerenciador do I2C


void setup() {
    // ok init_serial();
    // ok loadMQTTSettings(); // Carrega as configurações MQTT salvas da NVS
    // ok init_wifi();
    // ok init_mqtt();
    try
    {
        loadJSONSensorConfig();
    }
    catch(const exception& e)
    {
        Serial.println("[NVS] Erro ao carregar configuração dos sensores da NVS.");
    }
    
    sensores = init_sensor_config(SENSOR_CONFIG);

    Serial.println("====================================");
    Serial.println("Sistema IoT ESP32 Iniciado!");
    Serial.println("====================================");
    i2c.begin();
}
  
/// @brief Inicializacao da comunicacao serial
void init_serial() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nIniciando sistema IoT...");
}

/// @brief Inicializacao
void init_pins() {
    //pinMode(PIN_LED, OUTPUT);
    //pinMode(PIN_BUTTON, INPUT_PULLUP);
    //pinMode(PIN_TEMP_SENSOR, INPUT);
    //digitalWrite(PIN_LED, LOW); // LED inicia desligado
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
    
    /* Teste manual com botão (publicar imediatamente)
    if (digitalRead(PIN_BUTTON) == LOW) {
        delay(50); // Debounce
        if (digitalRead(PIN_BUTTON) == LOW) {
            Serial.println("[BOTÃO] Publicação manual acionada!");
            readAndPublishSensors();
            while(digitalRead(PIN_BUTTON) == LOW); // Aguarda soltar
        }
    }*/
    
    // Keep-alive da comunicação com broker MQTT
    if (MQTT.connected())
    {
        MQTT.loop();
    }
    
    // Pequeno delay para não sobrecarregar
    delay(50);
}