#include <Trabalho.hpp>

enum { LOW, HIGH };

class Rele {
    private:
        int pin;
        int state;
    public:
        Rele(int pin);
        void on();
        void off();
        int getState();
}

Rele::Rele(int pin) {
    pin = pin;
    state = LOW;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, state);
}

Rele::on() {
    state = HIGH;
    digitalWrite(pin, state);
}

Rele::off() {
    state = LOW;
    digitalWrite(pin, state);
}

int Rele::getState() {
    return state;
}