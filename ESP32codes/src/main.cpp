#include <Trabalho.hpp>


// Tasks de configuracao, sensoriamento e afins -----------------------
TaskHandle_t connectTask;
TaskHandle_t sensorsTask;
TaskHandle_t actuatorsTask;
void connectionVoid(void * pvParameters);
void sensorsVoid(void * pvParameters);


// ==============================================================
void setup() {
  if (initESP()) {
    Serial.println("Erro na inicializacao do ESP. Reiniciando...");
    ESP.restart();
  }

  xTaskCreatePinnedToCore(
                    connectionVoid,   /* Task function. */
                    "Connecting",     /* name of task. */
                    10000,            /* Stack size of task */
                    NULL,             /* parameter of the task */
                    1,                /* priority of the task */
                    &connectTask,     /* Task handle to keep track of created task */
                    0);               /* pin task to core 0 */                  
  delay(500); 

  xTaskCreatePinnedToCore(
                    sensorsVoid,    /* Task function. */
                    "Sensing",      /* name of task. */
                    10000,          /* Stack size of task */
                    NULL,           /* parameter of the task */
                    1,              /* priority of the task */
                    &sensorsTask,   /* Task handle to keep track of created task */
                    1);             /* pin task to core 1 */
    delay(500);
}


void connectionVoid( void * pvParameters ){
  for (;;) {
    // Verifica conexão MQTT e reconecta se necessario (sem limite de tentativas no loop)
    
    if (!MQTT.connected()) {
      Serial.println("MQTT desconectado. Tentando reconectar...");  
      bool attempt = reconnectMQTT(mqttConfig, 0); // 0 = tentativas infinitas no loop
        if (!attempt) {
            Serial.println("Erro desconhecido ao reconectar ao MQTT no loop");
            continue;
        }
    }

    // Envia ping MQTT apenas a cada PING_INTERVAL
    unsigned long now = millis();
    if (now - lastPing >= PING_INTERVAL) {
        lastPing = now;
        publishHeartbeatMQTT();
    }

    if (publishGetterMQTT) {
        Serial.println("[MQTT] Preparando para publicar resposta...");
        Serial.print("[MQTT] Topico: ");
        Serial.println(payloadTopicSensors);
        Serial.print("[MQTT] Payload: ");
        Serial.println(payloadSensors);
        
        // Processa eventos pendentes antes de publicar
        MQTT.loop();
        delay(50);
        
        // Tenta publicar com retry
        bool published = false;
        for (int retry = 0; retry < 3 && !published; retry++) {
            if (retry > 0) {
                Serial.print("[MQTT] Tentativa ");
                Serial.print(retry + 1);
                Serial.println("/3...");
                delay(100);
            }
            
            published = MQTT.publish(payloadTopicSensors.c_str(), payloadSensors.c_str(), false);
            
            if (published) {
                Serial.println("[MQTT] Resposta publicada com sucesso!");
            } else {
                Serial.println("[MQTT] Falha ao publicar. Verificando conexao...");
                if (!MQTT.connected()) {
                    Serial.println("[MQTT] Conexao perdida durante publicacao");
                    break;
                }
            }
            MQTT.loop();
        }
        
        if (!published) {
            Serial.println("[MQTT] ERRO: Nao foi possivel publicar resposta apos 3 tentativas");
        }
        
        publishGetterMQTT = false;
    }

    MQTT.loop();
    delay(200);
  }
}


void sensorsVoid( void * pvParameters ){
  Serial.print("sensorsTask running on core ");
  Serial.println(xPortGetCoreID());

  unsigned long lastPublish = 0;
  const unsigned long PUBLISH_INTERVAL = 5000; // Publica a cada 5 segundos
  
  // Variáveis para gerenciar o teclado matricial
  String keyboardBuffer = "";

  for(;;){
    // Busca dinâmica do teclado a cada iteração
    KeyPad* teclado = nullptr;
    int tecladoId = -1;
    
    for (size_t i = 0; i < sensores.size(); i++) {
      if (sensores[i].tipo == TECLADO_4X4 && sensores[i].objeto != nullptr) {
        teclado = static_cast<KeyPad*>(sensores[i].objeto);
        tecladoId = sensores[i].id;
        break;
      }
    }
    
    // Leitura contínua do teclado (se existir)
    if (teclado != nullptr) {
      char key = teclado->getKey();
      
      if (key != '\0') {
        if (key == '*') {
          // Asterisco limpa o buffer
          keyboardBuffer = "";
          Serial.println("[TECLADO] Buffer limpo");
        } else if (key == '#') {
          // Hashtag envia a string se não estiver vazia
          if (keyboardBuffer.length() > 0) {
            // Find the keyboard sensor struct to build proper payload
            Sensor* keypadSensor = nullptr;
            for (size_t i = 0; i < sensores.size(); i++) {
              if (sensores[i].id == tecladoId && sensores[i].tipo == TECLADO_4X4) {
                keypadSensor = &sensores[i];
                break;
              }
            }
            
            if (keypadSensor != nullptr && keypadSensor->objeto != nullptr) {
              // Store the buffer in the keypad object temporarily
              KeyPad* kp = static_cast<KeyPad*>(keypadSensor->objeto);
              // Use a temporary variable to store the string
              // Since KeyPad stores last key, we need to build custom payload
              
              StaticJsonDocument<256> doc;
              doc["device_id"] = mqttConfig.id;
              doc["sensor_id"] = tecladoId;
              doc["type"] = static_cast<int>(TECLADO_4X4);
              JsonObject valores = doc.createNestedObject("values");
              valores["input"] = keyboardBuffer;
              
              String payload;
              serializeJson(doc, payload);
              
              String topic = mqttConfig.id + "/sensors/" + String(tecladoId) + "/data";
              
              if (MQTT.connected()) {
                bool published = MQTT.publish(topic.c_str(), payload.c_str());
                
                if (published) {
                  Serial.print("[TECLADO] String publicada: ");
                  Serial.println(keyboardBuffer);
                } else {
                  Serial.println("[TECLADO] Falha ao publicar");
                }
              }
              
              // Limpa o buffer após enviar
              keyboardBuffer = "";
            } else {
              Serial.println("[TECLADO] Erro: sensor nao encontrado");
            }
          } else {
            Serial.println("[TECLADO] Buffer vazio, nada para enviar");
          }
        } else {
          // Qualquer outra tecla é adicionada ao buffer
          keyboardBuffer += key;
          Serial.print("[TECLADO] Buffer: ");
          Serial.println(keyboardBuffer);
        }
      }
    }
    
    // Verifica se já passou o intervalo de publicação para os outros sensores
    unsigned long now = millis();
    if (now - lastPublish >= PUBLISH_INTERVAL) {
      lastPublish = now;
      
      Serial.println("[SENSOR] Lendo e publicando sensores no MQTT...");
      
      // Lê e publica os valores de todos os sensores (exceto teclado)
      for (size_t i = 0; i < sensores.size(); i++) {
        Sensor &sensor = sensores[i];
        if (sensor.objeto == nullptr || sensor.tipo == TECLADO_4X4) {
          continue;
        }
        // Gera o payload JSON com os valores lidos do sensor
        String payload = buildSensorPayload(&sensor);
        
        if (payload.length() == 0) {
          Serial.print("[SENSOR] Erro ao gerar payload para sensor ID ");
          Serial.println(sensor.id);
          continue;
        }
        
        // Monta o tópico individual do sensor: <esp_id>/sensors/<id_sensor>/data
        String topic = mqttConfig.id + "/sensors/" + String(sensor.id) + "/data";
        
        // Publica no tópico individual
        if (MQTT.connected()) {
          bool published = MQTT.publish(topic.c_str(), payload.c_str());
          
          if (published) {
            Serial.print("[SENSOR] Publicado sensor ID ");
            Serial.print(sensor.id);
            Serial.print(" (");
            Serial.print(sensor.desc.c_str());
            Serial.print(") -> ");
            Serial.println(payload);
          } else {
            Serial.print("[SENSOR] Falha ao publicar sensor ID ");
            Serial.println(sensor.id);
          }
        } else {
          Serial.println("[SENSOR] MQTT desconectado, aguardando conexao...");
          break;
        }
        
        // Pequeno delay entre publicações de sensores
        delay(50);
      }
    }
    
    // Delay curto para leitura responsiva do teclado
    delay(100);
  }

}

void loop() {  
}

