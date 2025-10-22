#include <Trabalho.hpp>

class HCSR04{  
private:
    int triggerPin, echoPin;
public:
    HCSR04(int trigPin, int echoPin);
    ~HCSR04();

    double getDistance();
};

HCSR04::HCSR04(int trigPin, int echoPin) {
    this->triggerPin = trigPin;
    this->echoPin = echoPin;    

    pinMode(this->triggerPin, OUTPUT);
    pinMode(this->echoPin, INPUT);
}

HCSR04::~HCSR04() {}

double HCSR04::getDistance() {
    // Ensure trigger is low, then send a 10µs pulse
    digitalWrite(this->triggerPin, LOW);
    delayMicroseconds(2);
    digitalWrite(this->triggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(this->triggerPin, LOW);

    unsigned long duration = pulseIn(this->echoPin, HIGH);
    double distance = duration * 0.034 / 2.0;  // cm

    return distance;
}