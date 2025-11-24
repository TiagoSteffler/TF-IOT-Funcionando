 #include "Trabalho.hpp"

/// @brief Construtor do Encoder
/// @param pin Pino de dados do encoder
/// @param id Identificador do encoder
Encoder::Encoder(int pin, int id) {
    this->dataPin = pin;
    this->id = id;
    
    if (DEBUGSENS) {
        Serial.print("[Encoder id ");
        Serial.print(this->id);
        Serial.print("] Data pin: ");
        Serial.println(this->dataPin);
    }
    
    if (!DEBUGCOMM) {
        pinMode(this->dataPin, INPUT);
    }
}

/// @brief Detecta se há obstáculo lendo o valor digital do pino
/// @return true (1) se obstáculo detectado (pino HIGH), false (0) caso contrário (pino LOW)
bool Encoder::isObstacleDetected() {
    bool detected;
    
    if (DEBUGCOMM) {
        // No modo debug, retorna valor aleatório
        detected = (rand() % 2) == 1;
    } else {
        // Leitura digital direta do pino
        detected = digitalRead(this->dataPin) == HIGH;
    }
    
    if (DEBUGSENS) {
        Serial.print("[Encoder id ");
        Serial.print(this->id);
        Serial.print("] Valor lido: ");
        Serial.print(detected ? "1 (HIGH)" : "0 (LOW)");
        Serial.print(" | Obstáculo: ");
        Serial.println(detected ? "DETECTADO" : "LIVRE");
    }
    
    return detected;
}

/// @brief Retorna a leitura do encoder
/// @return Estrutura Encoder_read com detecção de obstáculo
Encoder_read Encoder::getValues() {
    Encoder_read reading;
    
    reading.obstacle_detected = isObstacleDetected();
    
    if (DEBUGSENS && !DEBUGCOMM) {
        Serial.print("[Encoder id ");
        Serial.print(this->id);
        Serial.print("] Obstáculo: ");
        Serial.println(reading.obstacle_detected ? "DETECTADO (1)" : "LIVRE (0)");
    }
    
    return reading;
}
