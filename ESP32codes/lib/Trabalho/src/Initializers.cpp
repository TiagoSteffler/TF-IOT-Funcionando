
#include <Trabalho.hpp>

/*
=======================================================================
BIBLIOTECA DE INICIALIZACAO DO SISTEMA

Possui rotinas para inicializar conecxoes e setup de redes WiFi e MQTT, assim como
funcoes de pareamento com servidor local via HTTP.
=======================================================================
*/

/// @brief Inicializa a comunicacao serial
void initSerial() {
  Serial.begin(115200);
  delay(50);
  Serial.println("ESP32 Ligado -- Serial iniciada =======================");
}


/// @brief Inicializa o sistema de arquivos SPIFFS
/// @return 0 se sucedido, -1 se falhou
int initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("Erro ao montar SPIFFS");
    return -1;
  }
  Serial.println("SPIFFS montado com sucesso");
  return 0;
}


/// @brief Cria arquivo no SPIFFS se ele nao existir
/// @param path Caminho do arquivo
/// @return 0 se sucedido, -1 se falhou
int createFileIfNotExists(const char* path) {
  if (!SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "w");
    if (!file) {
      Serial.print("Erro ao criar arquivo: ");
      Serial.println(path);
      return -1;
    }

    file.close();
    Serial.print("Arquivo criado: ");
    Serial.println(path);
  }
  return 0;
}


/// @brief Salva credenciais WiFi no SPIFFS
/// @param ssid SSID da rede
/// @param password Senha da rede
/// @return 0 se sucedido, -1 se falhou 
int saveWifiConfig(const char* ssid, const char* password) {
  File file = SPIFFS.open("/wifi.txt", "w");
  if (!file) {
    Serial.println("Erro ao abrir arquivo de credenciais para escrita");
    return -1;
  }
  
  file.println(ssid);
  file.println(password);
  file.close();

  wifiConfig.ssid = String(ssid);
  wifiConfig.password = String(password);

  Serial.println("Credenciais WiFi salvas com sucesso");
  return 0;
}


/// @brief Salva configuracoes MQTT no SPIFFS
/// @param broker IP do broker
/// @param port Porta do broker
/// @param id ID do dispositivo
/// @return 0 se sucedido, -1 se falhou
int saveMQTTConfig(const char* broker, const char* port, const char* id) {
  File file = SPIFFS.open("/mqtt.txt", "w");
  if (!file) {
    Serial.println("Erro ao abrir arquivo de configuracoes MQTT para escrita");
    return -1;
  }
  
  file.println(broker);
  file.println(port);
  file.println(id);
  file.close();

  mqttConfig.broker = String(broker);
  mqttConfig.port = String(port);
  mqttConfig.id = String(id);

  Serial.println("Configuracoes MQTT salvas com sucesso");
  return 0;
}


/// @brief Responsavel pela inicializacao geral do sistema
/// @return 0 se sucedido, -1 se falhou
int initSetup(){
    initSerial();
    if (i2c.begin() == false) {
        Serial.println("Erro ao iniciar I2C");
        return -1;
    }

    // Inicializa o SPIFFS
    if (initSPIFFS() != 0) return -1;
    // Verifica o arquivo de configuracao do WiFi
    if (createFileIfNotExists("/wifi.txt") != 0) return -1;
    // Verifica o arquivo de configuracoes do MQTT 
    if (createFileIfNotExists("/mqtt.txt") != 0) return -1;
    // Verifica o arquivo de topicos do MQTT
    if (createFileIfNotExists("/topics.txt") != 0) return -1;
    // Verifica o arquivo de configuracao de sensores e atuadores
    if (createFileIfNotExists("/devices.txt") != 0) return -1;

    return 0;
}


/// @brief Inicializa a conexao WiFi com credenciais do SPIFFS
/// @return 0 se conectado, -1 se falhou
int initWiFi() {
  // Le credenciais
  File file = SPIFFS.open("/wifi.txt", "r");
  if (!file) {
    Serial.println("Erro ao abrir arquivo de credenciais");
    return -1;
  }
  
  String ssid = file.readStringUntil('\n');
  String password = file.readStringUntil('\n');
  file.close();
  ssid.trim();
  password.trim();
  
  // retorna -1 se as credenciais nao estao preenchidas
  wifiConfig.ssid = ssid;
  wifiConfig.password = password;
  if (!wifiConfig.isConfigured()) {
    Serial.println("Credenciais WiFi nao configuradas");
    return -1;
  }

  Serial.println("Conectando ao WiFi...");
  Serial.print("SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid.c_str(), password.c_str());
  int tentativas = 0;

  while (WiFi.status() != WL_CONNECTED && tentativas < 50) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado ao WiFi!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    return 0;
  } else {
    Serial.println("\nFalha na conexão");
    return -1;
  }
}

/// @brief Realiza pareamento com servidor local
/// @return 0 se sucedido, -1 se falhou
int tryPairing(){
    Serial.println("=== Iniciando pareamento com servidor local ===");
    
    // Obtem o IP do gateway (assumido como servidor local)
    String serverIP = WiFi.gatewayIP().toString();
    Serial.print("Tentando conectar ao servidor em ");
    Serial.print(serverIP);
    Serial.println(":3001");
    
    // Tenta conectar ao servidor na porta 3001
    WiFiClient client;
    if (!client.connect(serverIP.c_str(), 3001)) {
      Serial.println("Falha ao conectar ao servidor local");
      return -1;
    }
    
    // Prepara a requisição GET /ping
    String mac = WiFi.macAddress();
    String httpRequest = "GET /ping?mac=" + mac + " HTTP/1.1\r\nHost: " + serverIP + ":3001\r\nConnection: close\r\n\r\n";
    
    // Envia a requisicao
    client.print(httpRequest);
    Serial.println("Requisição enviada:");
    Serial.println(httpRequest);
    
    // Aguarda a resposta
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 120000) {
        Serial.println("Timeout esperando resposta do servidor");
        client.stop();
        return -1;
      }
    }
    
    // Le a resposta
    String response;
    while (client.available()) {
      String line = client.readStringUntil('\r');
      response += line;
    }
    
    Serial.println("Resposta recebida:");
    Serial.println(response);
    
    client.stop();
    
    // Encontra o início do JSON (depois dos headers HTTP)
    int jsonStart = response.indexOf('{');
    if (jsonStart == -1) {
      Serial.println("Erro: JSON não encontrado na resposta");
      return -1;
    }
    
    String jsonResponse = response.substring(jsonStart);
    Serial.println("JSON extraido:");
    Serial.println(jsonResponse);
    
    // Faz o parse do JSON
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, jsonResponse);
    
    if (error) {
      Serial.print("Erro ao fazer parse do JSON: ");
      Serial.println(error.c_str());
      return -1;
    }
    
    // Extrai os campos do JSON
    String macRecebido = doc["mac"] | "";
    String ssid = doc["ssid"] | "";
    String password = doc["password"] | "";
    String broker = doc["broker"] | "";
    String port = doc["port"] | "";
    String id = doc["id"] | "";
    
    // Valida se todos os campos existem
    if (macRecebido.length() == 0 || ssid.length() == 0 || password.length() == 0 ||
        broker.length() == 0 || port.length() == 0 || id.length() == 0) {
      Serial.println("Erro: Campos obrigatorios faltando no JSON");
      return -1;
    }
    
    // Valida se o MAC recebido corresponde ao MAC do dispositivo (eco)
    if (macRecebido != mac) {
      Serial.println("Erro: MAC recebido não corresponde ao MAC do dispositivo");
      Serial.print("Esperado: ");
      Serial.println(mac);
      Serial.print("Recebido: ");
      Serial.println(macRecebido);
      return -1;
    }
    
    Serial.println("Validação do MAC OK - dados recebidos:");
    Serial.println("  SSID: " + ssid);
    Serial.println("  Password: " + password);
    Serial.println("  Broker: " + broker);
    Serial.println("  Port: " + port);
    Serial.println("  ID: " + id);
    
    // Salva as configuracoes WiFi
    if (saveWifiConfig(ssid.c_str(), password.c_str()) != 0) {
      Serial.println("Erro ao salvar configuracoes WiFi");
      return -1;
    }
    
    // Salva as configuracoes MQTT
    if (saveMQTTConfig(broker.c_str(), port.c_str(), id.c_str()) != 0) {
      Serial.println("Erro ao salvar configuracoes MQTT");
      return -1;
    }
    
    Serial.println("Todas as configuracoes salvas com sucesso!");
    Serial.println("Reiniciando em 2 segundos...");
    delay(2000);
    ESP.restart();
    
    return 0;
}


/// @brief Realiza busca por redes WiFi para pareamento (aberta ou com senha padrao)
/// @return 0 se sucedido, -1 se falhou
int searchWifi(){
    const char* PAIRING_PASSWORD = "12345678"; // Senha padrao para pareamento
    
    Serial.println("=== Iniciando busca por redes WiFi ===");
    
    // Escaneia redes disponiveis
    int n = WiFi.scanNetworks();
    Serial.print("Redes encontradas: ");
    Serial.println(n);
    
    if (n == 0) {
      Serial.println("Nenhuma rede encontrada");
      return -1;
    }
    
    // Primeiro: procura por redes abertas (sem senha)
    for (int i = 0; i < n; i++) {
      if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) {
        String ssid = WiFi.SSID(i);
        Serial.print("Rede aberta encontrada: ");
        Serial.println(ssid);
        
        // Tenta conectar a rede aberta
        Serial.println("Tentando conectar...");
        WiFi.begin(ssid.c_str(), "");
        
        int tentativas = 0;
        while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
          delay(500);
          Serial.print(".");
          tentativas++;
        }
  
        if (WiFi.status() == WL_CONNECTED) {
          Serial.println("\nConectado a rede aberta!");
          Serial.print("IP: ");
          Serial.println(WiFi.localIP());

          int val = tryPairing();
          if (val == 0) {
            return 0;
          }
        }
      }
    }
    
    Serial.println("Nenhuma rede aberta encontrada.");
    Serial.println("Tentando redes com senha padrao de pareamento...");
    
    // Segundo: tenta todas as redes com a senha padrao
    for (int i = 0; i < n; i++) {
      String ssid = WiFi.SSID(i);
      Serial.print("Tentando rede: ");
      Serial.println(ssid);
      
      WiFi.begin(ssid.c_str(), PAIRING_PASSWORD);
      
      int tentativas = 0;
      while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
        delay(500);
        Serial.print(".");
        tentativas++;
      }

      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConectado com senha padrao!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());

        int val = tryPairing();
        if (val == 0) {
            Serial.println(" Pareamento sucedido!");
            return 0;
        } else {
            Serial.println(" Pareamento falhou!");
        }
      }
      Serial.println(" X");
    }
    
    Serial.println("Nenhuma rede valida encontrada para pareamento");
    return -1;
}


// ==============================================================
// MQTT Handlers
void callbackMQTT(char* topic, byte* payload, unsigned int length) {
    String msg;
 
    //obtem a string do payload recebido
    for(int i = 0; i < length; i++) {
       char c = (char)payload[i];
       msg += c;
    }
    Serial.print("[MQTT] Mensagem recebida no topico: ");
    Serial.println(topic);
    Serial.print("[MQTT] Payload: ");
    Serial.println(msg);
    
    // Converte o tópico para String para facilitar comparação
    String topicStr = String(topic);
    
    // mensagens esperadas:
    String setSensor = mqttConfig.id + "/settings/sensors/set";
    String getSensor = mqttConfig.id + "/settings/sensors/get";
    String removeSensor = mqttConfig.id + "/settings/sensors/remove";
    String resetDevice = mqttConfig.id + "/settings/device/reset";

    // Caso seja para configurar um sensor ==========================================
    if (topicStr == setSensor) {
        Serial.println("[MQTT] Comando de configuracao de sensor detectado");
        
        // Cria uma copia do payload como string null-terminated
        char jsonPayload[length + 1];
        memcpy(jsonPayload, payload, length);
        jsonPayload[length] = '\0';
        
        // Parse do JSON recebido para verificar a estrutura
        StaticJsonDocument<2048> doc;
        DeserializationError error = deserializeJson(doc, jsonPayload);
        
        if (error) {
            Serial.print("Erro ao parsear JSON: ");
            Serial.println(error.c_str());
            payloadTopicSensors = mqttConfig.id + "/settings/sensors/set/response";
            payloadSensors = "ERROR: Invalid JSON";
            publishGetterMQTT = true;
            return;
        }
        
        int totalProcessados = 0;
        int totalErros = 0;
        
        // Verifica se o JSON contém um array "sensors" 
        if (doc.containsKey("sensors") && doc["sensors"].is<JsonArray>()) {
            JsonArray sensorsArray = doc["sensors"];
            Serial.print("Processando ");
            Serial.print(sensorsArray.size());
            Serial.println(" sensor(es)...");
            
            // Processa cada sensor do array
            for (JsonObject sensorObj : sensorsArray) {
                // Serializa o sensor individual em uma string JSON
                StaticJsonDocument<512> singleSensorDoc;
                singleSensorDoc.set(sensorObj);
                
                String sensorJson;
                serializeJson(singleSensorDoc, sensorJson);
                
                // Chama a funcao para adicionar ou atualizar o sensor
                int resultado = addOrUpdateSensor(sensorJson.c_str());
                
                if (resultado > 0) {
                    totalProcessados++;
                } else {
                    totalErros++;
                }
            }
        } else {
            // Caso o JSON seja um único sensor (formato antigo)
            Serial.println("JSON no formato de sensor unico");
            int resultado = addOrUpdateSensor(jsonPayload);
            
            if (resultado > 0) {
                totalProcessados = 1;
            } else {
                totalErros = 1;
            }
        }
        
        // Envia resposta ao servidor
        payloadTopicSensors = mqttConfig.id + "/settings/sensors/set/response";
        
        if (totalErros == 0 && totalProcessados > 0) {
            Serial.print("[MQTT] ");
            Serial.print(totalProcessados);
            Serial.println(" sensor(es) adicionado(s)/editado(s) com sucesso!");
            payloadSensors = "OK: " + String(totalProcessados) + " sensor(es) processado(s)";
        } else if (totalProcessados > 0 && totalErros > 0) {
            Serial.print("[MQTT] ");
            Serial.print(totalProcessados);
            Serial.print(" sensor(es) OK, ");
            Serial.print(totalErros);
            Serial.println(" com erro");
            payloadSensors = "PARTIAL: " + String(totalProcessados) + " OK, " + String(totalErros) + " errors";
        } else {
            Serial.println("[MQTT] Erro ao processar configuracao do(s) sensor(es)");
            payloadSensors = "ERROR";
        }
        
        // Aguarda um momento para garantir que os dados estejam prontos
        delay(10);
        
        // Publica a resposta
        publishGetterMQTT = true;
    }

    // Caso seja para requisitar a lista de sensores ==========================================
    else if (topicStr == getSensor) {
        Serial.println("[MQTT] Comando de requisicao de sensores detectado");
        
        // Prepara o array JSON com as configuracoes dos sensores
        StaticJsonDocument<2048> doc;
        JsonArray sensorsArray = doc.to<JsonArray>();
        
        for (const Sensor& sensor : sensores) {
            JsonObject sensorJson = sensorsArray.createNestedObject();
            sensorJson["id"] = sensor.id;
            sensorJson["tipo"] = static_cast<int>(sensor.tipo);
            sensorJson["desc"] = sensor.desc;
            sensorJson["atributo1"] = sensor.atributo1;
            sensorJson["atributo2"] = sensor.atributo2;
            sensorJson["atributo3"] = sensor.atributo3;
            sensorJson["atributo4"] = sensor.atributo4;
            
            JsonArray pinosArray = sensorJson.createNestedArray("pinos");
            for (const Pino& pino : sensor.pinos) {
                JsonObject pinoJson = pinosArray.createNestedObject();
                pinoJson["pino"] = pino.pin;
                pinoJson["tipo"] = static_cast<int>(pino.tipo);
            }
        }
        
        // Serializa o JSON para string
        payloadSensors = "";
        serializeJson(doc, payloadSensors);
        
        // Publica a resposta
        payloadTopicSensors = mqttConfig.id + "/settings/sensors/get/response";
        if (payloadSensors.length() == 0) payloadSensors = "[]"; // Garante que sempre haja algo para enviar
        
        // Aguarda um momento para garantir que os dados estejam prontos
        delay(10);
        
        publishGetterMQTT = true;
    }

    // Para remover um sensor =========================================================
    else if (topicStr == removeSensor) {
        Serial.println("[MQTT] Comando de remocao de sensor detectado");
        
        // Cria uma copia do payload como string null-terminated
        char jsonPayload[length + 1];
        memcpy(jsonPayload, payload, length);
        jsonPayload[length] = '\0';
        
        // Chama a funcao para remover o sensor
        int resultado = removeSensorById(jsonPayload);
        // Envia resposta ao servidor
        payloadTopicSensors = mqttConfig.id + "/settings/sensors/remove/response";
        if (resultado > 0) {
            Serial.println("[MQTT] Sensor removido com sucesso!");
            payloadSensors = "OK";
        } else {
            Serial.println("[MQTT] Erro ao processar remocao do sensor");
            payloadSensors = "ERROR";
        }
        
        // Aguarda um momento para garantir que os dados estejam prontos
        delay(10);
        
        // Publica a resposta
        publishGetterMQTT = true;
    }

    // Para resetar o dispositivo =========================================================
    else if (topicStr == resetDevice) {
        Serial.println("[MQTT] Comando de reset do dispositivo detectado");
        
        // Reseta o dispositivo
        eraseAllConfigurations();
        Serial.println("Reiniciando em 2 segundos...");
        delay(2000);
        ESP.restart();
    }
}


/// @brief Se inscreve nos topicos MQTT do arquivo topics.txt
/// @return 0 se sucedido, -1 se falhou
int subscribeMQTTTopics() {
    // abre arquivo com os topicos mqtt (topics.txt)
    // Le os topicos MQTT do arquivo
    File topicsMQTT = SPIFFS.open("/topics.txt", "r");
    if (!topicsMQTT) {
      Serial.println("Arquivo de topicos MQTT nao encontrado. Criando arquivo vazio.");
      if (createFileIfNotExists("/topics.txt")) return -1;
    }

    // verifica se o arquivo esta vazio
    if (topicsMQTT.size() == 0) {
        Serial.println("Arquivo de topicos MQTT vazio. Criando topicos padrao");
        topicsMQTT.close();
        topicsMQTT = SPIFFS.open("/topics.txt", "w");
        topicsMQTT.println(mqttConfig.id + "/settings/sensors/get");
        topicsMQTT.println(mqttConfig.id + "/settings/sensors/set");
        topicsMQTT.println(mqttConfig.id + "/settings/sensors/remove");
        topicsMQTT.close();
        topicsMQTT = SPIFFS.open("/topics.txt", "r");
    }

    // Le cada topico e se inscreve
    while (topicsMQTT.available()) {
        String topic = topicsMQTT.readStringUntil('\n');
        topic.trim();
        if (topic.length() > 0) {
            if (MQTT.subscribe(topic.c_str())) {
                Serial.println("> Subscrito ao topico: " + topic);
            } else {
                Serial.println("X Falha ao subscrever ao topico: " + topic);
            }
        }
    }

    topicsMQTT.close();
    return 0;
}



/// @brief Tenta reconectar ao broker MQTT com timeout
/// @param config Configuracoes do MQTT
/// @param maxAttempts Número máximo de tentativas (0 = infinito)
/// @return true se conectado, false se atingiu o limite de tentativas
bool reconnectMQTT(MQTTConfig config, int maxAttempts = 5) {
    int attempts = 0;
    
    while (!MQTT.connected()) {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.print(config.broker.c_str());
        
        Serial.print(" (tentativa ");
        Serial.print(attempts + 1);
        if (maxAttempts > 0) {
          Serial.print("/");
          Serial.print(maxAttempts);
        }
        Serial.println(")");

        if (MQTT.connect(config.id.c_str())) {
            Serial.println("> Conectado com sucesso ao broker MQTT!");
            subscribeMQTTTopics();
            return true;
        } 
        else {
            Serial.println("X Falha ao reconectar no broker.");
            attempts++;
            
            // Se atingiu o máximo de tentativas, retorna false
            if (maxAttempts > 0 && attempts >= maxAttempts) {
                Serial.println("!!! Maximo de tentativas atingido !!!");
                return false;
            }
            Serial.println("Havera nova tentatica de conexao em 2s");
            delay(2000);
        }
    }
    
    return true;
}


/// @brief Publica heartbeat MQTT com informacoes do dispositivo
void publishHeartbeatMQTT() {
    // Cria payload JSON
    StaticJsonDocument<200> doc;
    doc["mac"] = WiFi.macAddress();
    doc["ip"] = WiFi.localIP().toString();
    doc["id"] = mqttConfig.id;
    doc["timestamp"] = millis() / 1000; // timestamp em segundos

    String payload;
    serializeJson(doc, payload);

    // Publica no topico "device/{id}/heartbeat"
    String topic = "device/" + mqttConfig.id + "/heartbeat";
    if (MQTT.publish(topic.c_str(), payload.c_str())) {
        Serial.println("Ping MQTT enviado: " + payload);
    } else {
        Serial.println("Falha ao enviar ping MQTT");
    }
}


/// @brief Inicializa o cliente MQTT com configuracoes do SPIFFS
/// @return 0 se sucedido, -1 se falhou
int initMQTT(void) {
    
    // Le as configuracoes do MQTT do arquivo
    File fileMQTT = SPIFFS.open("/mqtt.txt", "r");
    if (!fileMQTT) {
      Serial.println("Erro ao abrir arquivo de configuracoes MQTT");
      return -1;
    }

    String broker = fileMQTT.readStringUntil('\n');
    String port = fileMQTT.readStringUntil('\n');
    String id = fileMQTT.readStringUntil('\n');
    fileMQTT.close();
    
    broker.trim();
    port.trim();
    id.trim();
    
    // Valida se as configuracoes não estão vazias
    if (broker.length() == 0 || port.length() == 0 || id.length() == 0) {
      Serial.println("Configuracoes MQTT vazias. Aguardando pairing via /pair");
      return -1;
    }
    
    mqttConfig.broker = broker;
    mqttConfig.port = port;
    mqttConfig.id = id;

    Serial.println("Configuracoes MQTT:");
    Serial.print("Broker: ");
    Serial.println(mqttConfig.broker);
    Serial.print(" - Porta: ");
    Serial.print(mqttConfig.port);
    Serial.print(" - ID: ");
    Serial.println(mqttConfig.id);

    MQTT.setServer(mqttConfig.broker.c_str(), port.toInt());
    MQTT.setCallback(callbackMQTT);
    MQTT.setBufferSize(2048); // Aumenta o buffer para mensagens maiores

    
    // Tenta conectar com limite de 10 tentativas
    if (!reconnectMQTT(mqttConfig, 5)) {
      Serial.println("> Nao foi possivel conectar ao MQTT apos 5 tentativas");
      return -1;
    }
    
    return 0;
}


/// @brief Apaga as configuracoes de conexao WiFi e MQTT
void eraseConnections() {
  // Apaga arquivo WiFi
  if (SPIFFS.exists("/wifi.txt")) {
    SPIFFS.remove("/wifi.txt");
    Serial.println("> Configuracoes WiFi removidas");
  }
  
  // Apaga arquivo MQTT
  if (SPIFFS.exists("/mqtt.txt")) {
    SPIFFS.remove("/mqtt.txt");
    Serial.println("> Configuracoes MQTT removidas");
  } 
}


/// @brief Apaga todas as configuracoes salvas (RESET para estado de fabrica)
void eraseAllConfigurations() {
    eraseConnections();
    // apaga topicos MQTT
    if (SPIFFS.exists("/topics.txt")) {
        SPIFFS.remove("/topics.txt");
        Serial.println("> Topicos MQTT removidos");
    }
    // apaga configuracoes de sensores e atuadores
    if (SPIFFS.exists("/devices.txt")) {
        SPIFFS.remove("/devices.txt");
        Serial.println("> Configuracoes de sensores e atuadores removidas");
    }
}


/// @brief Gerencia leitura do botao para reset de configuracoes
/// @note 3s = apaga credenciais de conexao | 8s = reset de fabrica    
void handleReset(){
  uint32_t lastmillis = 0;
  bool pressed = false, ledState = LOW;
  
  while (1) {
    if (digitalRead(0) == LOW && !pressed) {
      lastmillis = millis();
      pressed = true;
    } else if (digitalRead(0) == HIGH) {
      if (millis() - lastmillis >= 8000 && pressed) {
        // apaga tudo
        Serial.println("Botao pressionado por 8s - apagando todas as configuracoes");
        eraseAllConfigurations();
      }
      else if (millis() - lastmillis >= 3000 && pressed) {
        // apaga apenas wifi e mqtt
        Serial.println("Botao pressionado por 3s - apagando configuracoes de conexao");
        eraseConnections();
      }
      if (millis() - lastmillis >= 3000 && pressed) {
        Serial.println("Reiniciando...");
        delay(2000);
        ESP.restart();
      }

      pressed = false;
    }
    if (pressed && (millis() - lastmillis) % (millis() - lastmillis >= 8000 ? 500 : 1000) < 30) {
      ledState = !ledState;
      digitalWrite(2, ledState);
    }
  }
}

// ==============================================================
/// @brief Realiza toda a sequencia de inicializacao interna do ESP32
/// @return 0 se sucedido, -1 se falhou
int initESP() {
    pinMode(0, INPUT_PULLUP);  // Configura o pino 0 com pull-up interno
    pinMode(2, OUTPUT);
    digitalWrite(2, LOW);

    // Inicializacao do sistema (serial e arquivos)
    if (initSetup()) return -1;

    // Inicializacao do WiFi e MQTT ===============================

    // Se ocorrer alguma falha na inicializacao do WiFi -----------------------------
    if (initWiFi() != 0) {
        // Caso tenha credenciais salvas (invalidas), entra para reset
        if (wifiConfig.isConfigured()) {
            Serial.println("Falha ao conectar ao WiFi com credenciais salvas.");
            handleReset();
        }

        // caso nao tenha credenciais salvas, tenta pareamento direto
        else {
            Serial.println(
                "Credenciais WiFi nao configuradas - iniciando pareamento");

            // Caso pareamento falhe, entra em loop esperando reset fisico
            if (searchWifi())
                Serial.println(
                    "Nao foi possivel encontrar rede aberta para pareamento");
            bool ledState = LOW;

            // Loop infinito de blink indicando reset
            while (1) {
                if (millis() % 3000 < 50) {
                    ledState = !ledState;
                    digitalWrite(2, ledState);
                }
                delay(50);
            }
        }
    }

    // Se conectar corretamente no WiFi, tenta MQTT -------------------------------------------
    else {
        // Caso conexao com MQTT falhe, entra para reset
        if (initMQTT() != 0) {
            Serial.println("Falha ao conectar ao MQTT com credenciais salvas.");
            handleReset();
        }

        // Inicializa sensores e atuadores aqui
        Serial.println(" >>>>> Inicializacao concluida com sucesso! <<<<<");

        initDevices();
    }

    return 0;
}
