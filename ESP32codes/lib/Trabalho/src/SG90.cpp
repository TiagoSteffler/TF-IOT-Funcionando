#include <Trabalho.hpp>

/// @brief Servo motor SG90
class SG90 {
    private:
        // pino e variaveis de controle
        Servo servo;
        int pin;
        uint16_t id;
        uint8_t angle;

    public:
        // contrutores/destrutores  
        SG90(int pin, uint16_t id);
        ~SG90() {};

        // metodos publicos
        void setAngle(int angle);
        int getAngle();
        int isAttached() { return servo.attached(); }
        void detach() { servo.detach(); }
        uint16_t getId() { return this->id; }
};


/// @brief Construtor do servo SG90
/// @param pin Pino ao qual o servo esta conectado
/// @param id Identificador do servo
SG90::SG90(int pin, uint16_t id) {
    this->pin = pin;
    this->id = id;
    
    if (DEBUGSENS) {
        Serial.print("[SG90 ID ");
        Serial.print(id);
        Serial.print("] conectado no pino ");
        Serial.println(pin);
    }

    if (DEBUGCOMM) return;

    servo.attach(pin);
    servo.write(0);
}

/// @brief Define o angulo do servo
/// @param angle angulo (0-180)
void SG90::setAngle(int angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    this->angle = angle;
    if (!DEBUGCOMM) servo.write(this->angle);
    if (DEBUGSENS) {
        Serial.print("[SG90 ID ");
        Serial.print(id);
        Serial.print("] angulo definido para ");
        Serial.println(this->angle);
    }
}

/// @brief Retorna o angulo atual do servo
/// @return angulo (0-180)
int SG90::getAngle() {
    if (DEBUGCOMM) return this->angle;
    if (DEBUGSENS) {
        Serial.print("[SG90 ID ");
        Serial.print(id);
        Serial.print("] angulo lido: ");
        Serial.println(servo.read());
    }
    return servo.read();
}