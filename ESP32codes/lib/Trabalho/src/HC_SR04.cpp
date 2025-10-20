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

HCSR04::getDistance() {
    digitalWrite(trigPin, LOW);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    double duration = pulseIn(echoPin, HIGH);
    distance = duration * 0.034 / 2;  // cm

    return distance;
}