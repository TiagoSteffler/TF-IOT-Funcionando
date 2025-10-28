#include <Trabalho.hpp>

/// @brief Rele solenoide
class Relay {
    private:
        // pino e variaveis de controle
        int pin;
        uint16_t id;
        enum State { OFF = LOW, ON = HIGH } state = OFF;

    public:
        // contrutores/destrutores
        Relay(int pin, uint16_t id);
        ~Relay() {};

        // metodos publicos
        void setState(State state);
        State getState();
        uint16_t getId() { return this->id; }
};


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
void Relay::setState(State state) {
    this->state = state;
    digitalWrite(this->pin, this->state);
    if (DEBUGSENS) {
        Serial.print("[Rele ID ");
        Serial.print(this->id);
        Serial.println(this->state == Relay::ON ? "] ligado " : "] desligado ");
    }
}


/// @brief Retorna o estado atual do rele
/// @return Estado atual do rele (ON ou OFF)
Relay::State Relay::getState() {
    if (DEBUGSENS) {
        Serial.print("[Rele ID ");
        Serial.print(this->id);
        Serial.println(this->state == HIGH ? "] estado: ON" : "] estado: OFF");
    }
    return this->state;
}