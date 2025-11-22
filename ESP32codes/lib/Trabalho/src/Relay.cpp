#include <Trabalho.hpp>


/// @brief Construtor do rele
/// @param pin Pino ao qual o rele esta conectado
/// @param id Identificador do rele
Relay::Relay(int pin, uint16_t id) {
    this->pin = pin;
    this->id = id;
    pinMode(this->pin, OUTPUT);
    digitalWrite(this->pin, LOW); // Inicializa o rele desligado
    if (DEBUGSENS) {
        Serial.print("[Rele ID ");
        Serial.print(id);
        Serial.print("] conectado no pino ");
        Serial.println(pin);
    }
}


/// @brief Define o estado do rele
/// @param state Novo estado do rele
void Relay::setState(int state) {
    this->state = state;
    digitalWrite(this->pin, this->state);
    if (DEBUGSENS) {
        Serial.print("[Rele ID ");
        Serial.print(this->id);
        Serial.println(this->state == 1 ? "] ligado " : "] desligado ");
    }
}


/// @brief Retorna o estado atual do rele
/// @return Estado atual do rele (ON ou OFF)
int Relay::getState() {
    if (DEBUGSENS) {
        Serial.print("[Rele ID ");
        Serial.print(this->id);
        Serial.println(this->state == HIGH ? "] estado: ON" : "] estado: OFF");
    }
    return this->state;
}