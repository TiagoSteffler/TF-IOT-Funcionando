 #include "Trabalho.hpp"

/// @brief Construtor do sensor APDS9960
/// @param id identificador do sensor
/// @param interruptPin pino de interrupcao do sensor
APDS9960::APDS9960(int id, int interruptPin) {
    this->id = id;
    this->interruptPin = interruptPin;

    if (DEBUGSENS) {
        Serial.print("[APDS9960 id ");
        Serial.print(this->id);
        Serial.print("\tInt. pin: ");
        Serial.println(this->interruptPin);
    }
    if (DEBUGCOMM) return;
    
    this->apds = new Adafruit_APDS9960();
    apds->begin();
}

/// @brief Realiza a leitura de gestos
/// @return Retorna gesto detectado
uint8_t APDS9960::getGesture() {
    // Desabilitar outros modos antes de ler gestos
    apds->enableColor(false);
    apds->enableProximity(true);  // Proximidade eh necessaria para gestos
    apds->enableGesture(true);

    uint8_t gesture = apds->readGesture();
    return gesture;
}


/// @brief Realiza a leitura de proximidade
/// @return Valor de proximidade lido
uint8_t APDS9960::getProx(){
    uint8_t prox = 0;
    if (DEBUGCOMM) prox = static_cast <uint8_t> (rand()) / (static_cast <float> (RAND_MAX/255.0));
    else {
        // Desabilitar outros modos antes de ler proximidade
        apds->enableColor(false);
        apds->enableGesture(false);
        apds->enableProximity(true);
        apds->enableProximityInterrupt();
        apds->setProximityInterruptThreshold(0, 175);

        if(!digitalRead(this->interruptPin)) {
                prox = apds->readProximity();
                apds->clearInterrupt();
        }
    }

    if (DEBUGSENS) {
        Serial.print("[APDS9960 id ");
        Serial.print(this->id);
        Serial.print("] Proximity: ");
        Serial.println(prox);
    }

    return prox;
}


/// @brief Realiza leitura de cor
/// @return Retorna estrutura com valores de cor (r, g, b, c)
APDS_Color APDS9960::getColor(){
    APDS_Color color;

    if (!DEBUGCOMM) {
        // Desabilitar outros modos antes de ler cor
        apds->enableProximity(false);
        apds->enableGesture(false);
        apds->enableColor(true);

        apds->setADCGain(APDS9960_AGAIN_16X);
        apds->getColorData(&color.r, &color.g, &color.b, &color.c);
    }
    else {
        color.r = static_cast <uint16_t> (rand()) / (static_cast <float> (RAND_MAX/65535.0));
        color.g = static_cast <uint16_t> (rand()) / (static_cast <float> (RAND_MAX/65535.0));
        color.b = static_cast <uint16_t> (rand()) / (static_cast <float> (RAND_MAX/65535.0));
        color.c = static_cast <uint16_t> (rand()) / (static_cast <float> (RAND_MAX/65535.0));
    }

    if (DEBUGSENS) {
        Serial.print("[APDS9960 id ");
        Serial.print(this->id);
        Serial.print("] R: ");
        Serial.print(color.r);
        Serial.print("\tG: ");
        Serial.print(color.g);
        Serial.print("\tB: ");
        Serial.print(color.b);
        Serial.print("\tC: ");
        Serial.println(color.c);
    }

    return color;
}