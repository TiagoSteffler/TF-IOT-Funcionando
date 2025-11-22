 #include "Trabalho.hpp"

/// @brief Construtor do sensor DHT11
/// @param pin pino de dados do sensor
/// @param id identificador do sensor
DHT11_Sensor::DHT11_Sensor(int pin, int id) {
    this->dataPin = pin;
    this->id = id;
    if (DEBUGSENS) {
        Serial.print("[DHT11 id ");
        Serial.print(this->id);
        Serial.print("] data pin: ");
        Serial.println(this->dataPin);
    }
    if (DEBUGCOMM) return;
    this->dht = new DHT_Unified(this->dataPin, DHT11);
    this->dht->begin();
}

/// @brief Realiza a leitura completa do sensor DHT11
/// @return Estrutura DHT_read com temperatura e umidade
DHT_read DHT11_Sensor::getValues() {
    DHT_read reading;
    
    if (DEBUGCOMM) {
        // Valores dummy para debug
        reading.temperature = static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 40.0)); // 0-40°C
        reading.humidity = static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 100.0));    // 0-100%
    } else {
        // Leitura real usando a biblioteca Adafruit
        sensors_event_t event;
        
        // Lê temperatura
        dht->temperature().getEvent(&event);
        if (isnan(event.temperature)) {
            Serial.println("[DHT11] Erro ao ler temperatura");
            reading.temperature = 0.0;
        } else {
            reading.temperature = event.temperature;
        }
        
        // Lê umidade
        dht->humidity().getEvent(&event);
        if (isnan(event.relative_humidity)) {
            Serial.println("[DHT11] Erro ao ler umidade");
            reading.humidity = 0.0;
        } else {
            reading.humidity = event.relative_humidity;
        }
    }
    
    if (DEBUGSENS) {
        Serial.print("[DHT11 id ");
        Serial.print(this->id);
        Serial.print("] Temperature: ");
        Serial.print(reading.temperature);
        Serial.print(" °C\tHumidity: ");
        Serial.print(reading.humidity);
        Serial.println(" %");
    }
    
    return reading;
}

/// @brief Retorna apenas a temperatura
/// @return Temperatura em Celsius
float DHT11_Sensor::getTemperature() {
    if (DEBUGCOMM) {
        return static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 40.0));
    }
    
    sensors_event_t event;
    dht->temperature().getEvent(&event);
    
    if (isnan(event.temperature)) {
        if (DEBUGSENS) Serial.println("[DHT11] Erro ao ler temperatura");
        return 0.0;
    }
    
    if (DEBUGSENS) {
        Serial.print("[DHT11 id ");
        Serial.print(this->id);
        Serial.print("] Temperature: ");
        Serial.print(event.temperature);
        Serial.println(" °C");
    }
    
    return event.temperature;
}

/// @brief Retorna apenas a umidade
/// @return Umidade relativa em porcentagem
float DHT11_Sensor::getHumidity() {
    if (DEBUGCOMM) {
        return static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 100.0));
    }
    
    sensors_event_t event;
    dht->humidity().getEvent(&event);
    
    if (isnan(event.relative_humidity)) {
        if (DEBUGSENS) Serial.println("[DHT11] Erro ao ler umidade");
        return 0.0;
    }
    
    if (DEBUGSENS) {
        Serial.print("[DHT11 id ");
        Serial.print(this->id);
        Serial.print("] Humidity: ");
        Serial.print(event.relative_humidity);
        Serial.println(" %");
    }
    
    return event.relative_humidity;
}
