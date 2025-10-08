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
void setup() 
{
    init_serial();
    init_pins();
    init_wifi();
    init_mqtt();
    
    Serial.println("====================================");
    Serial.println("Sistema IoT ESP32 Iniciado!");
    Serial.println("====================================");
}
  
/* Função: inicializa comunicação serial com baudrate 115200 (para fins de monitorar no terminal serial 
*          o que está acontecendo.
* Parâmetros: nenhum
* Retorno: nenhum
*/
void init_serial() 
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nIniciando sistema IoT...");
}

/* Função: inicializa os pinos do ESP32
 * Parâmetros: nenhum
 * Retorno: nenhum
 */
void init_pins(void)
{
    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    pinMode(PIN_TEMP_SENSOR, INPUT);
    
    digitalWrite(PIN_LED, LOW); // LED inicia desligado
    Serial.println("Pinos configurados!");
}
 
/* Função: inicializa e conecta-se na rede WI-FI desejada
 * Parâmetros: nenhum
 * Retorno: nenhum
 */
void init_wifi(void) 
{
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");
    reconnect_wifi();
}
  

 
/* Função: lê temperatura simulada/real
 * Parâmetros: nenhum
 * Retorno: temperatura em °C
 */
float readTemperature(void)
{
    // SIMULAÇÃO: Gera temperatura variando entre 20-35°C
    // Para sensor real (DS18B20), substituir por leitura real
    simulatedTemp += random(-10, 11) / 10.0; // Varia ±1°C
    
    // Limita entre 20-35°C
    if (simulatedTemp < 20.0) simulatedTemp = 20.0;
    if (simulatedTemp > 35.0) simulatedTemp = 35.0;
    
    return simulatedTemp;
}



/* Programa principal */
void loop() 
{   
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