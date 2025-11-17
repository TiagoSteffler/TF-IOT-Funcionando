#include <Trabalho.hpp>
#include <Sensors.hpp>

/*
=======================================================================
BIBLIOTECA DE INICIALIZACAO DE SENSORES E ATUADORES

Rotinas para inicializacao e leitura de sensores e atuadores
=======================================================================
*/

// Vetor global de sensores
vector<Sensor> sensores;

int initDevices(){
    // Le as configuracoes de sensores e atuadores do SPIFFS
    File fileDevices = SPIFFS.open("/devices.txt", "r");
    if (!fileDevices) {
      Serial.println("Erro ao abrir arquivo de configuracoes dos dispositivos");
      return -1;
    }

    // Le o conteúdo do arquivo
    String jsonContent = fileDevices.readString();
    fileDevices.close();
    if (jsonContent.length() == 0) {
        Serial.println("Arquivo de configuracao vazio");
        return 0;
    }

    Serial.println("JSON lido do arquivo:");
    Serial.println(jsonContent);

    // Parse do JSON
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, jsonContent);

    if (error) {
        Serial.print("Erro ao parsear JSON: ");
        Serial.println(error.c_str());
        return -1;
    }
    if (!doc.is<JsonArray>()) {
        Serial.println("JSON nao e um array");
        return -1;
    }

    // Limpa o vetor de sensores antes de adicionar novos
    sensores.clear();

    // Itera sobre o array de sensores no JSON
    JsonArray sensoresArray = doc.as<JsonArray>();
    for (JsonObject sensorJson : sensoresArray) {
        Sensor sensor;
        
        // Le os campos básicos
        sensor.id = sensorJson["id"] | -1;
        sensor.tipo = static_cast<Sensor_tipo>(sensorJson["tipo"].as<int>());
        sensor.desc = sensorJson["desc"].as<String>().c_str();
        sensor.atributo1 = sensorJson["atributo1"] | 0;
        sensor.atributo2 = sensorJson["atributo2"] | 0;
        sensor.atributo3 = sensorJson["atributo3"] | 0;
        sensor.atributo4 = sensorJson["atributo4"] | 0;

        Serial.print("Configurando sensor ID ");
        Serial.println(sensor.id);
        Serial.print("Tipo do sensor: ");
        Serial.println(static_cast<int>(sensor.tipo));
        Serial.print("Atributo1 recebido: ");
        Serial.println(sensor.atributo1);

        // Le os pinos
        JsonArray pinosArray = sensorJson["pinos"];
        for (JsonObject pinoJson : pinosArray) {
            Pino pino;
            pino.pin = pinoJson["pino"];
            pino.tipo = static_cast<Pino_tipo>(pinoJson["tipo"].as<int>());
            sensor.pinos.push_back(pino);
        }

        // Cria o objeto do sensor baseado no tipo
        sensor.objeto = nullptr;
        
        // Usa a função auxiliar para criar o objeto do sensor
        if (createSensorObject(sensor)) {
            sensores.push_back(sensor);
            Serial.print("Sensor adicionado ao vetor. Total de sensores: ");
            Serial.println(sensores.size());
        } else {
            Serial.print("Falha ao criar sensor ID ");
            Serial.println(sensor.id);
        }
    }

    Serial.print("Inicializacao concluida. Total de sensores carregados: ");
    Serial.println(sensores.size());
    
    return sensores.size();
}


/// @brief Cria o objeto do sensor baseado no tipo e configurações
/// @param sensor Referencia para a estrutura do sensor
/// @return true se o objeto foi criado com sucesso, false caso contrário
bool createSensorObject(Sensor &sensor) {
    // Libera o objeto anterior se existir
    if (sensor.objeto != nullptr) {
        // Aqui seria ideal ter destrutores apropriados, mas por simplicidade vamos apenas sobrescrever
        delete sensor.objeto;
        sensor.objeto = nullptr;
    }
    
    switch (sensor.tipo) {
        case MPU_6050: {
            Serial.print("Criando MPU6050 com ID ");
            Serial.println(sensor.id);
            sensor.objeto = new MPU6050(sensor.id);
            break;
        }
        
        case DS18_B20: {
            Serial.print("Criando DS18B20 com ID ");
            Serial.println(sensor.id);
            if (sensor.pinos.size() > 0) {
                sensor.objeto = new DS18B20(sensor.pinos[0].pin, sensor.id);
            } else {
                Serial.println("Erro: DS18B20 precisa de pelo menos 1 pino");
                return false;
            }
            break;
        }
        
        case DHT_11: {
            Serial.print("Criando DHT11 com ID ");
            Serial.println(sensor.id);
            if (sensor.pinos.size() > 0) {
                sensor.objeto = new DHT11_Sensor(sensor.pinos[0].pin, sensor.id);
            } else {
                Serial.println("Erro: DHT11 precisa de pelo menos 1 pino");
                return false;
            }
            break;
        }
        
        case HC_SR04: {
            Serial.print("Criando HC-SR04 com ID ");
            Serial.println(sensor.id);
            if (sensor.pinos.size() >= 2) {
                sensor.objeto = new HCSR04(sensor.pinos[0].pin, sensor.pinos[1].pin, sensor.id);
            } else {
                Serial.println("Erro: HC-SR04 precisa de 2 pinos (trigger e echo)");
                return false;
            }
            break;
        }
        
        case APDS_9960: {
            Serial.print("Criando APDS9960 com ID ");
            Serial.println(sensor.id);
            int interruptPin = (sensor.pinos.size() > 0) ? sensor.pinos[0].pin : -1;
            sensor.objeto = new APDS9960(sensor.id, interruptPin);
            break;
        }
        
        case SG_90: {
            Serial.print("Criando SG90 com ID ");
            Serial.println(sensor.id);
            if (sensor.pinos.size() > 0) {
                sensor.objeto = new SG90(sensor.pinos[0].pin, sensor.id);
                // Aplica o angulo inicial se definido
                if (sensor.atributo1 >= 0 && sensor.atributo1 <= 180) {
                    SG90* servo = static_cast<SG90*>(sensor.objeto);
                    servo->setAngle(sensor.atributo1);
                    Serial.print("SG90 angulo inicial definido para: ");
                    Serial.println(sensor.atributo1);
                }
            } else {
                Serial.println("Erro: SG90 precisa de 1 pino");
                return false;
            }
            break;
        }
        
        case RELE: {
            Serial.print("Criando Relay com ID ");
            Serial.println(sensor.id);
            if (sensor.pinos.size() > 0) {
                sensor.objeto = new Relay(sensor.pinos[0].pin, sensor.id);
                // Aplica o estado inicial do rele
                Relay* rele = static_cast<Relay*>(sensor.objeto);
                rele->setState(sensor.atributo1);
                Serial.print("Relay estado inicial definido para: ");
                Serial.println(sensor.atributo1);
            } else {
                Serial.println("Erro: Relay precisa de 1 pino");
                return false;
            }
            break;
        }
        
        case JOYSTICK: {
            Serial.print("Criando Joystick com ID ");
            Serial.println(sensor.id);
            if (sensor.pinos.size() >= 3) {
                sensor.objeto = new Joystick(sensor.pinos[0].pin, sensor.pinos[1].pin, 
                                            sensor.pinos[2].pin, sensor.id);
            } else {
                Serial.println("Erro: Joystick precisa de 3 pinos (X, Y, botao)");
                return false;
            }
            break;
        }
        
        case TECLADO_4X4: {
            Serial.print("Criando KeyPad 4x4 com ID ");
            Serial.println(sensor.id);
            if (sensor.pinos.size() >= 8) {
                int rowPins[4] = {sensor.pinos[0].pin, sensor.pinos[1].pin, 
                                 sensor.pinos[2].pin, sensor.pinos[3].pin};
                int colPins[4] = {sensor.pinos[4].pin, sensor.pinos[5].pin, 
                                 sensor.pinos[6].pin, sensor.pinos[7].pin};
                sensor.objeto = new KeyPad(rowPins, colPins, sensor.id);
            } else {
                Serial.println("Erro: Teclado 4x4 precisa de 8 pinos (4 linhas + 4 colunas)");
                return false;
            }
            break;
        }
        
        default:
            Serial.print("Tipo de sensor desconhecido: ");
            Serial.println(sensor.tipo);
            return false;
    }
    
    return (sensor.objeto != nullptr);
}

/// @brief Salva a configuracao atual de sensores no arquivo devices.txt
/// @return true se salvou com sucesso, false caso contrario
bool saveDevicesToFile() {
    // Cria o documento JSON
    StaticJsonDocument<2048> doc;
    JsonArray sensoresArray = doc.to<JsonArray>();
    
    // Adiciona cada sensor ao array JSON
    for (const Sensor &sensor : sensores) {
        JsonObject sensorJson = sensoresArray.createNestedObject();
        
        sensorJson["id"] = sensor.id;
        sensorJson["tipo"] = static_cast<int>(sensor.tipo);
        sensorJson["desc"] = sensor.desc.c_str();
        sensorJson["atributo1"] = sensor.atributo1;
        sensorJson["atributo2"] = sensor.atributo2;
        sensorJson["atributo3"] = sensor.atributo3;
        sensorJson["atributo4"] = sensor.atributo4;
        
        // Adiciona os pinos
        JsonArray pinosArray = sensorJson.createNestedArray("pinos");
        for (const Pino &pino : sensor.pinos) {
            JsonObject pinoJson = pinosArray.createNestedObject();
            pinoJson["pino"] = pino.pin;
            pinoJson["tipo"] = static_cast<int>(pino.tipo);
        }
    }
    
    // Abre o arquivo para escrita
    File fileDevices = SPIFFS.open("/devices.txt", "w");
    if (!fileDevices) {
        Serial.println("Erro ao abrir arquivo para escrita - criando novo arquivo");
        if (createFileIfNotExists("/devices.txt")) return false;
    }
    
    // Serializa o JSON no arquivo
    if (serializeJson(doc, fileDevices) == 0) {
        Serial.println("Erro ao escrever JSON no arquivo");
        fileDevices.close();
        return false;
    }
    
    fileDevices.close();
    Serial.println("Configuracao de sensores salva com sucesso");
    return true;
}

/// @brief Adiciona ou atualiza um sensor a partir de um payload JSON MQTT
/// @param jsonPayload String JSON com a configuração do sensor
/// @return 1 se adicionou, 2 se atualizou, -1 em caso de erro, 0 se não fez nada
int addOrUpdateSensor(const char* jsonPayload) {
    Serial.println("Recebido payload para adicionar/atualizar sensor:");
    Serial.println(jsonPayload);
    
    // Parse do JSON recebido
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, jsonPayload);
    
    if (error) {
        Serial.print("Erro ao parsear JSON: ");
        Serial.println(error.c_str());
        return -1;
    }
    
    // Valida se o JSON tem os campos necessários
    if (!doc.containsKey("id") || !doc.containsKey("tipo")) {
        Serial.println("JSON invalido: faltam campos 'id' ou 'tipo'");
        return -1;
    }
    
    // Cria estrutura temporaria do sensor
    Sensor novoSensor;
    novoSensor.id = doc["id"];
    novoSensor.tipo = static_cast<Sensor_tipo>(doc["tipo"].as<int>());
    novoSensor.desc = doc["desc"].as<String>().c_str();
    novoSensor.atributo1 = doc["atributo1"] | 0;
    novoSensor.atributo2 = doc["atributo2"] | 0;
    novoSensor.atributo3 = doc["atributo3"] | 0;
    novoSensor.atributo4 = doc["atributo4"] | 0;
    novoSensor.objeto = nullptr;
    
    Serial.print("Processando sensor ID ");
    Serial.println(novoSensor.id);
    Serial.print("Tipo do sensor: ");
    Serial.println(static_cast<int>(novoSensor.tipo));
    Serial.print("Atributo1 recebido: ");
    Serial.println(novoSensor.atributo1);


    // Le os pinos
    if (doc.containsKey("pinos")) {
        JsonArray pinosArray = doc["pinos"];
        for (JsonObject pinoJson : pinosArray) {
            Pino pino;
            pino.pin = pinoJson["pino"];
            pino.tipo = static_cast<Pino_tipo>(pinoJson["tipo"].as<int>());
            novoSensor.pinos.push_back(pino);
        }
    }
    
    // Procura se ja existe um sensor com o mesmo ID
    int sensorIndex = -1;
    for (size_t i = 0; i < sensores.size(); i++) {
        if (sensores[i].id == novoSensor.id) {
            sensorIndex = i;
            break;
        }
    }
    
    int resultado = 0;
    
    if (sensorIndex != -1) {
        // Sensor ja existe - atualizar
        Serial.print("Atualizando sensor existente com ID ");
        Serial.println(novoSensor.id);
        
        // Verifica se o tipo do sensor mudou
        bool tipoMudou = (sensores[sensorIndex].tipo != novoSensor.tipo);
        
        if (tipoMudou) {
            // Se o tipo mudou, sempre recria o objeto completamente
            Serial.println("Tipo de sensor alterado - recriando objeto");
            
            if (sensores[sensorIndex].objeto != nullptr) {
                delete sensores[sensorIndex].objeto;
            }
            
            // Atualiza os dados do sensor
            sensores[sensorIndex] = novoSensor;
            
            // Cria o novo objeto do sensor
            if (createSensorObject(sensores[sensorIndex])) {
                Serial.println("Sensor recriado com sucesso com novo tipo");
                resultado = 2; // Atualizado
            } else {
                Serial.println("Erro ao criar objeto do sensor com novo tipo");
                return -1;
            }
        }
        // Para atuadores (Relay e Servo) do mesmo tipo, apenas atualiza os atributos sem recriar o objeto
        else if (sensores[sensorIndex].tipo == RELE || sensores[sensorIndex].tipo == SG_90) {
            // Atualiza apenas os atributos
            sensores[sensorIndex].atributo1 = novoSensor.atributo1;
            sensores[sensorIndex].atributo2 = novoSensor.atributo2;
            sensores[sensorIndex].atributo3 = novoSensor.atributo3;
            sensores[sensorIndex].atributo4 = novoSensor.atributo4;
            sensores[sensorIndex].desc = novoSensor.desc;
            
            // Aplica o novo estado ao atuador
            if (sensores[sensorIndex].tipo == RELE && sensores[sensorIndex].objeto != nullptr) {
                Relay* rele = static_cast<Relay*>(sensores[sensorIndex].objeto);
                rele->setState(sensores[sensorIndex].atributo1);
                Serial.print("Relay estado atualizado para: ");
                Serial.println(sensores[sensorIndex].atributo1);
            } else if (sensores[sensorIndex].tipo == SG_90 && sensores[sensorIndex].objeto != nullptr) {
                SG90* servo = static_cast<SG90*>(sensores[sensorIndex].objeto);
                if (sensores[sensorIndex].atributo1 >= 0 && sensores[sensorIndex].atributo1 <= 180) {
                    servo->setAngle(sensores[sensorIndex].atributo1);
                    Serial.print("Servo angulo atualizado para: ");
                    Serial.println(sensores[sensorIndex].atributo1);
                }
            }
            
            Serial.println("Atuador atualizado sem recriar objeto");
            resultado = 2; // Atualizado
        }
        // Para HC-SR04, atualiza atributos e executa calibração se solicitado
        else if (sensores[sensorIndex].tipo == HC_SR04) {
            // Atualiza os atributos
            sensores[sensorIndex].atributo1 = novoSensor.atributo1;
            sensores[sensorIndex].atributo2 = novoSensor.atributo2;
            sensores[sensorIndex].atributo3 = novoSensor.atributo3;
            sensores[sensorIndex].atributo4 = novoSensor.atributo4;
            sensores[sensorIndex].desc = novoSensor.desc;
            
            Serial.println("HC-SR04 atualizado sem recriar objeto");
            resultado = 2; // Atualizado
        } else {
            // Para sensores do mesmo tipo, recria o objeto normalmente
            if (sensores[sensorIndex].objeto != nullptr) {
                delete sensores[sensorIndex].objeto;
            }
            
            // Atualiza os dados do sensor
            sensores[sensorIndex] = novoSensor;
            
            // Cria o novo objeto do sensor
            if (createSensorObject(sensores[sensorIndex])) {
                Serial.println("Sensor atualizado com sucesso");
                resultado = 2; // Atualizado
            } else {
                Serial.println("Erro ao criar objeto do sensor atualizado");
                return -1;
            }
        }
    } else {
        // Sensor novo - adicionar
        Serial.print("Adicionando novo sensor com ID ");
        Serial.println(novoSensor.id);
        
        // Cria o objeto do sensor
        if (createSensorObject(novoSensor)) {
            sensores.push_back(novoSensor);
            Serial.print("Sensor adicionado com sucesso. Total: ");
            Serial.println(sensores.size());
            resultado = 1; // Adicionado
        } else {
            Serial.println("Erro ao criar objeto do novo sensor");
            return -1;
        }
    }
    
    // Salva as alterações no arquivo
    if (saveDevicesToFile()) {
        Serial.println("Alteracoes salvas no arquivo devices.txt");
        return resultado;
    } else {
        Serial.println("Erro ao salvar alteracoes no arquivo");
        return -1;
    }
}

/// @brief Remove um ou mais sensores especificados por ID no payload JSON
/// @param jsonPayload String JSON com o(s) ID(s) do(s) sensor(es) a remover
/// @return Número de sensores removidos, -1 em caso de erro
int removeSensorById(const char* jsonPayload) {
    Serial.println("Recebido payload para remover sensor(es):");
    Serial.println(jsonPayload);
    
    // Parse do JSON recebido
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, jsonPayload);
    
    if (error) {
        Serial.print("Erro ao parsear JSON: ");
        Serial.println(error.c_str());
        return -1;
    }
    
    int sensoresRemovidos = 0;
    
    // Verifica se o JSON é um único objeto ou um array
    if (doc.is<JsonObject>()) {
        // JSON é um único objeto, espera um campo "id"
        if (!doc.containsKey("id")) {
            Serial.println("JSON invalido: falta campo 'id'");
            return -1;
        }
        
        int idToRemove = doc["id"];
        Serial.print("Removendo sensor com ID: ");
        Serial.println(idToRemove);
        
        // Procura e remove o sensor
        for (size_t i = 0; i < sensores.size(); i++) {
            if (sensores[i].id == idToRemove) {
                // Libera o objeto do sensor
                if (sensores[i].objeto != nullptr) {
                    delete sensores[i].objeto;
                    sensores[i].objeto = nullptr;
                }
                
                // Remove o sensor do vetor
                sensores.erase(sensores.begin() + i);
                sensoresRemovidos++;
                Serial.print("Sensor ID ");
                Serial.print(idToRemove);
                Serial.println(" removido com sucesso");
                break;
            }
        }
        
        if (sensoresRemovidos == 0) {
            Serial.print("Sensor com ID ");
            Serial.print(idToRemove);
            Serial.println(" nao encontrado");
        }
        
    } else if (doc.is<JsonArray>()) {
        // JSON é um array de objetos
        JsonArray sensoresArray = doc.as<JsonArray>();
        
        for (JsonObject sensorJson : sensoresArray) {
            if (!sensorJson.containsKey("id")) {
                Serial.println("JSON invalido: objeto sem campo 'id'");
                continue;
            }
            
            int idToRemove = sensorJson["id"];
            Serial.print("Removendo sensor com ID: ");
            Serial.println(idToRemove);
            
            // Procura e remove o sensor
            for (size_t i = 0; i < sensores.size(); i++) {
                if (sensores[i].id == idToRemove) {
                    // Libera o objeto do sensor
                    if (sensores[i].objeto != nullptr) {
                        delete sensores[i].objeto;
                        sensores[i].objeto = nullptr;
                    }
                    
                    // Remove o sensor do vetor
                    sensores.erase(sensores.begin() + i);
                    sensoresRemovidos++;
                    Serial.print("Sensor ID ");
                    Serial.print(idToRemove);
                    Serial.println(" removido com sucesso");
                    break;
                }
            }
        }
    } else {
        Serial.println("Formato JSON invalido: esperado objeto ou array");
        return -1;
    }
    
    // Se removeu algum sensor, salva as alterações
    if (sensoresRemovidos > 0) {
        if (saveDevicesToFile()) {
            Serial.print("Total de sensores removidos: ");
            Serial.println(sensoresRemovidos);
            Serial.print("Sensores restantes: ");
            Serial.println(sensores.size());
            return sensoresRemovidos;
        } else {
            Serial.println("Erro ao salvar alteracoes no arquivo");
            return -1;
        }
    }
    
    return 0;
}




/// @brief Constrói o payload JSON para publicação MQTT de um sensor
/// @param sensor Ponteiro para o sensor
/// @return String JSON com formato {"device_id":X, "sensor_id":Y, "type":Z, "values":{...}}
String buildSensorPayload(Sensor *sensor) {
    if (sensor == nullptr || sensor->objeto == nullptr) {
        Serial.println("Erro: sensor ou objeto nulo");
        return "";
    }
    
    StaticJsonDocument<512> doc;
    doc["device_id"] = mqttConfig.id;
    doc["sensor_id"] = sensor->id;
    doc["type"] = static_cast<int>(sensor->tipo);
    JsonObject values = doc.createNestedObject("values");
    
    switch (sensor->tipo) {
        case MPU_6050: {
            MPU6050* mpu = static_cast<MPU6050*>(sensor->objeto);
            MPU_read read = mpu->getValues();
            values["accel_x"] = read.x;
            values["accel_y"] = read.y;
            values["accel_z"] = read.z;
            values["gyro_x"] = read.gx;
            values["gyro_y"] = read.gy;
            values["gyro_z"] = read.gz;
            values["temp"] = read.temp;
            break;
        }
        
        case DS18_B20: {
            DS18B20* ds = static_cast<DS18B20*>(sensor->objeto);
            float temp = ds->readTemperature();
            values["temperature"] = temp;
            break;
        }
        
        case DHT_11: {
            DHT11_Sensor* dht = static_cast<DHT11_Sensor*>(sensor->objeto);
            DHT_read read = dht->getValues();
            values["temperature"] = read.temperature;
            values["humidity"] = read.humidity;
            break;
        }
        
        case HC_SR04: {
            HCSR04* hc = static_cast<HCSR04*>(sensor->objeto);
            double dist = hc->getDistance();
            values["distance"] = dist;
            break;
        }
        
        case APDS_9960: {
            APDS9960* apds = static_cast<APDS9960*>(sensor->objeto);
            APDS_Color color = apds->getColor();
            uint8_t prox = apds->getProx();
            uint8_t gesture = apds->getGesture();
            values["red"] = color.r;
            values["green"] = color.g;
            values["blue"] = color.b;
            values["clear"] = color.c;
            values["proximity"] = prox;
            values["gesture"] = gesture;
            break;
        }
        
        case SG_90: {
            SG90* servo = static_cast<SG90*>(sensor->objeto);
            int angle = servo->getAngle();
            values["angle"] = angle;
            break;
        }
        
        case RELE: {
            Relay* relay = static_cast<Relay*>(sensor->objeto);
            int state = static_cast<int>(relay->getState());
            values["state"] = state;
            break;
        }
        
        case JOYSTICK: {
            Joystick* joy = static_cast<Joystick*>(sensor->objeto);
            JoyRead read = joy->getRawValues();
            values["x"] = read.x;
            values["y"] = read.y;
            values["button"] = read.bot;
            break;
        }
        
        case TECLADO_4X4: {
            KeyPad* keypad = static_cast<KeyPad*>(sensor->objeto);
            char key = keypad->getKey();
            if (key != '\0') {
                char keyStr[2] = {key, '\0'};
                values["key"] = keyStr;
            } else {
                values["key"] = (const char*)nullptr;
            }
            break;
        }
        
        case ENCODER: {
            Encoder* encoder = static_cast<Encoder*>(sensor->objeto);
            Encoder_read read = encoder->getValues();
            values["obstacle_detected"] = read.obstacle_detected ? 1 : 0;
            break;
        }
        
        default:
            Serial.print("Tipo de sensor desconhecido: ");
            Serial.println(sensor->tipo);
            return "";
    }
    
    String payload;
    serializeJson(doc, payload);
    
    if (DEBUGSENS) {
        Serial.print("Payload gerado para sensor ID ");
        Serial.print(sensor->id);
        Serial.print(": ");
        Serial.println(payload);
    }
    
    return payload;
}