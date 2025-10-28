#include "Trabalho.hpp"

/// @brief Construtor do sensor DS18B20
/// @param pin pino de dados do sensor
/// @param id identificador do sensor
/// @note Apenas um sensor deve ser conectado por pino de dados
DS18B20::DS18B20(int pin, int id) {
    this->dataPin = pin;
    this->id = id;
    if (DEBUGSENS) {
        Serial.print("[DS18B20 id");
        Serial.print(this->id);
        Serial.print("] data pin: ");
        Serial.println(this->dataPin);
    }
    if (DEBUGCOMM) return;
    this->oneWire = new OneWire(this->dataPin);
    sensors = new DallasTemperature(this->oneWire);
    sensors->begin();
    
}

/// @brief Realiza a leitura da temperatura do sensor
/// @param unit unidade de medida (C ou F)
/// @return temperatura lida
float DS18B20::readTemperature(Unit unit) { 
    float temp;
    
    if (!DEBUGCOMM) {
        sensors->requestTemperatures();
        if (unit == C) temp = sensors->getTempCByIndex(0);
        else if (unit == F) temp = sensors->getTempFByIndex(0);
    } else temp = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/100.0));

    if (DEBUGSENS) {
        Serial.print("[DS18B20 id ");
        Serial.print(this->id);
        Serial.print("] temperature: ");
        Serial.print(temp);
        Serial.println((unit == C) ? " C" : " F");
    }
    return temp;
}
