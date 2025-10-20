#include <Trabalho.hpp>

// Definições únicas das variáveis de configuração
char BROKER_MQTT[64] = "192.168.2.100";
int BROKER_PORT = 1883;
char SSID[32] = "iot2022";
char PASSWORD[64] = "S3nhab0@";

WiFiClient espClient;
PubSubClient MQTT(espClient);

/* Função: função de callback 
 *          esta função é chamada toda vez que uma informação de 
 *          um dos tópicos subescritos chega)
 * Parâmetros: topic, payload, length
 * Retorno: nenhum
 */
static void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
    String msg = "";
 
    // Obter a string do payload recebido
    for(int i = 0; i < length; i++) 
    {
       char c = (char)payload[i];
       msg += c;
    }
    
    Serial.println("====================================");
    Serial.print("[MQTT RECEBIDO] Tópico: ");
    Serial.println(topic);
    Serial.print("[MQTT RECEBIDO] Mensagem: ");
    Serial.println(msg);
    
    // Controle do LED baseado no comando recebido
    if (msg.toInt() == 1) {
        //digitalWrite(PIN_LED, HIGH);
        Serial.println("[AÇÃO] LED LIGADO");
    } 
    else if (msg.toInt() == 0) {
        //digitalWrite(PIN_LED, LOW);
        Serial.println("[AÇÃO] LED DESLIGADO");
    }
    else {
        Serial.println("[AVISO] Comando não reconhecido");
    }
    Serial.println("====================================");
}

/// @brief Inicializacao do MQTT
void init_mqtt() {
    /* informa a qual broker e porta deve ser conectado */
    MQTT.setServer(BROKER_MQTT, BROKER_PORT); 
    /* atribui função de callback (função chamada quando qualquer informação do 
    tópico subescrito chega) */
    MQTT.setCallback(mqtt_callback);            
}
  
  
/* Função: reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
 *          em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
 * Parâmetros: nenhum
 * Retorno: nenhum
 */
void reconnect_mqtt(void) 
{
    while (!MQTT.connected()) 
    {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT)) 
        {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_SUBSCRIBE); 
            break;
        } 
        else
        {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Havera nova tentatica de conexao em 2s");
            delay(2000);
        }
    }
}
  
/* Função: reconecta-se ao WiFi
 * Parâmetros: nenhum
 * Retorno: nenhum
*/
void reconnect_wifi() 
{
    /* se já está conectado a rede WI-FI, nada é feito. 
       Caso contrário, são efetuadas tentativas de conexão */
    if (WiFi.status() == WL_CONNECTED)
        return;
         
   Serial.println();
    Serial.println("Iniciando WiFiManager (provisionamento)...");
    WiFiManager wm;
    // wm.resetSettings(); // descomente para limpar configurações disponíveis no início

    // adicionar parâmetros personalizados para editar broker/porta via portal
    WiFiManagerParameter customBroker("broker", "MQTT Broker", BROKER_MQTT, sizeof(BROKER_MQTT));
    char portStr[8];
    snprintf(portStr, sizeof(portStr), "%d", BROKER_PORT);
    WiFiManagerParameter customPort("port", "MQTT Port", portStr, sizeof(portStr));

    wm.addParameter(&customBroker);
    wm.addParameter(&customPort);

    // cria AP se não conseguir conectar; bloqueante até provisionar
    if (!wm.autoConnect("ESP32_AP")) {
        Serial.println("Falha no autoConnect. Reiniciando...");
        delay(2000);
        ESP.restart();
    }

    // após conexão / submit, copiar valores dos parâmetros para as variáveis
    strncpy(BROKER_MQTT, customBroker.getValue(), sizeof(BROKER_MQTT) - 1);
    BROKER_MQTT[sizeof(BROKER_MQTT) - 1] = '\0';
    BROKER_PORT = atoi(customPort.getValue());

    Serial.println("Conectado na rede:");
    Serial.println(WiFi.localIP());
}
 
/* Função: verifica o estado das conexões WiFI e ao broker MQTT. 
 *         Em caso de desconexão (qualquer uma das duas), a conexão
 *         é refeita.
 * Parâmetros: nenhum
 * Retorno: nenhum
 */
void verifica_conexoes_wifi_mqtt(void)
{
    /* se não há conexão com o WiFI, a conexão é refeita */
    reconnect_wifi(); 
    /* se não há conexão com o Broker, a conexão é refeita */
    if (!MQTT.connected()) 
        reconnect_mqtt(); 
} 

/* Função: lê sensores e publica dados via MQTT
 * Parâmetros: nenhum
 * Retorno: nenhum
 */
void readAndPublishSensors(void)
{
    // Criar JSON com dados dos sensores
    StaticJsonDocument<256> doc;
    
    // Ler temperatura
    float temp = 0;
    doc["temperature"] = temp;
    
    // Adicionar timestamp
    doc["timestamp"] = millis();
    
    // Serializar JSON
    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer);
    
    // Publicar via MQTT
    if (MQTT.publish(TOPICO_PUBLISH, jsonBuffer)) {
        Serial.println("------------------------------------");
        Serial.println("[SENSORES] Dados publicados:");
        Serial.print("  Temperatura: ");
        Serial.print(temp);
        Serial.println(" °C");
        Serial.print("  JSON: ");
        Serial.println(jsonBuffer);
        Serial.println("------------------------------------");
    } else {
        Serial.println("[ERRO] Falha ao publicar dados!");
    }
}