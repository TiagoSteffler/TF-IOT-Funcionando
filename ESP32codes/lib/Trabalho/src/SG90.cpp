#include <Servo.h>

class SG90 {
    public:
        SG90(int pin);
        void setAngle(int angle);
        int getAngle();
        int isAttached() { return servo.attached(); }
        void detach() { servo.detach(); }
    private:
        Servo servo;
        int pin;
}

SG90::SG90(int pin) : pin(pin) {
    servo.attach(pin);
    servo.write(0); // Inicializa o servo na posição 0
}

SG90::setAngle(int angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    servo.write(angle);
}

SG90::getAngle() {
    return servo.read();
}