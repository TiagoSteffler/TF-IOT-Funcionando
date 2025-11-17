#include <Trabalho.hpp>

// Definições únicas das variáveis de configuração
char BROKER_MQTT[64] = "192.168.0.105"; // Valor padrão
int BROKER_PORT = 1883;
char SSID[32] = "iot2022";
char PASSWORD[64] = "S3nhab0@";
char ID_DEVICE[32] = "ESP32_005"; // Valor padrão

#define NVS_NAMESPACE_MQTT "mqtt_config" // Namespace para as configurações MQTT na NVS

WiFiClient espClient;
PubSubClient MQTT(espClient);

vector<string> split(const string& s, char delimiter) {
    vector<string> tokens;
    size_t start = 0; // Início da substring atual
    size_t end = s.find(delimiter); // Posição do primeiro delimitador

    while (end != string::npos) { // std::string::npos significa "não encontrado"
        // Extrai o token do início (start) até a posição (end)
        tokens.push_back(s.substr(start, end - start));
        
        // Atualiza o início para a posição DEPOIS do delimitador
        start = end + 1;
        
        // Procura o PRÓXIMO delimitador, começando de 'start'
        end = s.find(delimiter, start);
    }

    // Pega o último token (da última posição 'start' até o fim da string)
    tokens.push_back(s.substr(start, string::npos));

    return tokens;
}

/* Função: função de callback 
 *          esta função é chamada toda vez que uma informação de 
 *          um dos tópicos subescritos chega)
 * Parâmetros: topic, payload, length
 * Retorno: nenhum
 */
static void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
    String msg = "";
 
    vector<string> tokens = split(string(topic), '/');
    if (tokens.size() < 3) {
        Serial.println("[MQTT RECEBIDO] Tópico inválido.");
        return;
    }
    string id_device = tokens[1];
    string comando = tokens[2];

    for(int i = 0; i < length; i++) 
    {
       char c = (char)payload[i];
       msg += c;
    }

    if (comando.compare("put")==0)
    {
        saveJSONSensorConfig(msg.c_str());
        ESP.restart();
    }
    else if (comando.compare("get")==0)
    {
        string callback_topic = "callback/" + id_device + "/config";
        MQTT.publish(callback_topic.c_str(), SENSOR_CONFIG);
        Serial.println(SENSOR_CONFIG);
    }

    return;
}

/* Função: Carrega as configurações MQTT salvas na NVS
 * Parâmetros: nenhum
 * Retorno: nenhum
 */
void loadMQTTSettings() {
    preferences.begin(NVS_NAMESPACE_MQTT, false); // Abre a NVS para leitura/escrita
    
    String savedBroker = preferences.getString("broker", "");
    if (savedBroker.length() > 0) {
        strncpy(BROKER_MQTT, savedBroker.c_str(), sizeof(BROKER_MQTT) - 1);
        BROKER_MQTT[sizeof(BROKER_MQTT) - 1] = '\0';
        Serial.print("[NVS] Loaded BROKER_MQTT: ");
        Serial.println(BROKER_MQTT);
    }

    int savedPort = preferences.getInt("port", 0);
    if (savedPort != 0) {
        BROKER_PORT = savedPort;
        Serial.print("[NVS] Loaded BROKER_PORT: ");
        Serial.println(BROKER_PORT);
    }

    String savedID = preferences.getString("device_id", "");
    if (savedID.length() > 0) {
        strncpy(ID_DEVICE, savedID.c_str(), sizeof(ID_DEVICE) - 1);
        ID_DEVICE[sizeof(ID_DEVICE) - 1] = '\0';
        Serial.print("[NVS] Loaded ID_DEVICE: ");
        Serial.println(ID_DEVICE);
    }
    
    preferences.end();
}

/* Função: Salva as configurações MQTT na NVS
 * Parâmetros: nenhum
 * Retorno: nenhum
 */
void saveMQTTSettings() {
    preferences.begin(NVS_NAMESPACE_MQTT, false); // Abre a NVS para leitura/escrita
    preferences.putString("broker", BROKER_MQTT);
    preferences.putInt("port", BROKER_PORT);
    preferences.putString("device_id", ID_DEVICE);
    preferences.end();
    Serial.println("[NVS] MQTT settings saved.");
}

/// @brief Inicializacao do MQTT
void init_mqtt() {
    /* informa a qual broker e porta deve ser conectado */
    MQTT.setServer(BROKER_MQTT, BROKER_PORT); 
    /* atribui função de callback (função chamada quando qualquer informação do 
    tópico subescrito chega) */
    MQTT.setCallback(mqtt_callback);
    // O ID do cliente MQTT é definido durante a reconexão
}
  
  
/* Função: reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
 *          em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
 * Parâmetros: nenhum
 * Retorno: nenhum
 */
void reconnect_mqtt(void) 
{
    int i = 0;
    while (!MQTT.connected()) 
    {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_DEVICE)) // Usa ID_DEVICE como o ID do cliente MQTT
        {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            string topico_subscribe = string(TOPICO_SUBSCRIBE_PREFIX) + "/" + string(ID_DEVICE) + "/#";
            Serial.println("Subscribing to topic: ");
            Serial.println(topico_subscribe.c_str());
            MQTT.subscribe(topico_subscribe.c_str()); // Inscreve no tópico específico do dispositivo
            return;
            break;
        } 
        else
        {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Havera nova tentatica de conexao em 2s");
            delay(2000);
            i++;
            if (i>=5)
            {
                WiFiManager wm;
                wm.resetSettings(); // Limpa configurações WiFi para forçar novo provisionamento
                Serial.println("Reiniciando dispositivo para novo provisionamento...");
                ESP.restart();
            }
            
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
    WiFiManagerParameter customID("id", "Device ID", ID_DEVICE, sizeof(ID_DEVICE));

    wm.addParameter(&customBroker);
    wm.addParameter(&customPort);
    wm.addParameter(&customID);

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
    strncpy(ID_DEVICE, customID.getValue(), sizeof(ID_DEVICE) - 1);
    ID_DEVICE[sizeof(ID_DEVICE) - 1] = '\0';
    saveMQTTSettings();

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
    float temp = 15.0; // Simulação de leitura de sensor de temperatura
    
    string topico = string(TOPICO_PUBLISH_PREFIX) + "/" + string(ID_DEVICE) + "/id_sensor" + "/temperatura";
    // Publicar via MQTT
    if (MQTT.publish(topico.c_str(), String(temp).c_str())) {
        Serial.println("------------------------------------");
        Serial.println("");
        Serial.print(topico.c_str());
        Serial.print(" -> ");
        Serial.print(temp);
        Serial.println("");
        Serial.println("------------------------------------");
    } else {
        Serial.println("[ERRO] Falha ao publicar dados!");
    }
}