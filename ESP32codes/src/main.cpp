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
void setup() {
    init_serial();
    loadMQTTSettings(); // Carrega as configurações MQTT salvas da NVS
    init_wifi();
    init_mqtt();
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
}
  
/// @brief Inicializacao da comunicacao serial
void init_serial() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nIniciando sistema IoT...");
}

/// @brief Inicializacao

 
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
    verifica_conexoes_wifi_mqtt(); // <-- reativa verificação/reconexão

    // Ler e publicar dados dos sensores periodicamente
    unsigned long currentMillis = millis();
    if (currentMillis - lastSensorRead >= SENSOR_INTERVAL) {
        lastSensorRead = currentMillis;
        readAndPublishSensors(); // <-- ative se quiser publicar periodicamente
    }

    // Keep-alive da comunicação com broker MQTT (apenas se conectado)
    if (MQTT.connected()) {
        MQTT.loop();
    } else {
        // opcional: pequena espera para evitar loop apertado durante reconexão
        delay(50);
    }
    
    // Pequeno delay para não sobrecarregar
    delay(50);
}