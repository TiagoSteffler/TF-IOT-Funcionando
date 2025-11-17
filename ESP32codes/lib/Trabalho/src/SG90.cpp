#include <Trabalho.hpp>


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