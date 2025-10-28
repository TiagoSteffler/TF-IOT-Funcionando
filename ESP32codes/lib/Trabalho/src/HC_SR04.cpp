#include <Trabalho.hpp>

/// @brief Sensor ultrassonico HC-SR04
class HCSR04{  
    private:
        // pinos e variaveis de controle
        int triggerPin, echoPin;
        float distanceCm, distanceInch;
        double soundSpeed = 0.034;
        double cmtoInch = 0.39370078;
        enum Unit {CM, INCH};
        
        // dados para calibracao
        vector<pair<double, double>> calibration;
        double calSlope = 1.0;
        double calIntercept = 0.0;
        
        // metodos privados
        double getrawcm();

        // identificador
        int id;
        
    public:
        // contrutores/destrutores
        HCSR04(int trigPin, int echoPin, int id);
        ~HCSR04() {};

        // metodos publicos
        double getDistance(Unit unit = CM);
        void calibrate(float dist);
        void resetcalibration();
        int getId() { return this->id; }
};


/// @brief Contrutor do sensor HC-SR04
/// @param trigPin pino de trigger
/// @param echoPin pino de echo
/// @param id identificador do sensor
HCSR04::HCSR04(int trigPin, int echoPin, int id) {
    this->triggerPin = trigPin;
    this->echoPin = echoPin;
    this->id = id;
    pinMode(this->triggerPin, OUTPUT);
    pinMode(this->echoPin, INPUT);

    if (DEBUGSENS) {
        Serial.print("[HC-SR04 id ");
        Serial.print(this->id);
        Serial.print("] echo pin: ");
        Serial.print(this->echoPin);
        Serial.print("\ttrigger pin: ");
        Serial.println(this->triggerPin);
    }
}


/// @brief Retorna a distancia bruta em cm
/// @return Distancia em cm
double HCSR04::getrawcm() {

    digitalWrite(this->triggerPin, LOW);
    delayMicroseconds(2);
    digitalWrite(this->triggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(this->triggerPin, LOW);

    unsigned long duration = pulseIn(this->echoPin, HIGH);
    double distance = duration * this->soundSpeed / 2.0;  // cm
    
    if (DEBUGCOMM) distance = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    if (DEBUGSENS) {
        Serial.print("[HC-SR04 id ");
        Serial.print(this->id);
        Serial.print("] raw: ");
        Serial.println(distance);
    }

    return distance;
}


/// @brief Retorna a distancia calibrada
/// @param unit Unidade de medida (CM ou INCH)
/// @return Distancia calibrada em CM ou INCH
double HCSR04::getDistance(Unit unit) {
    double rawDist = this->getrawcm();

    double corrected = calSlope * rawDist + calIntercept;

    if (DEBUGSENS) {
        Serial.print("[HC-SR04 id ");
        Serial.print(this->id);
        Serial.print("] corrected cm: ");
        Serial.print(corrected);
        Serial.print("\t inch: ");
        Serial.println(corrected * this->cmtoInch);
    }

    if (unit == CM) return corrected;
    else return corrected * this->cmtoInch;
}



/// @brief Calibra o sensor com uma distancia conhecida
/// @param dist Distancia em CM ou INCH
void HCSR04::calibrate(float dist) {
    double raw = getrawcm();
    this->calibration.emplace_back(raw, (double)dist);

    // ajuste por minimos quadrados
    size_t n = this->calibration.size();
    if (n == 0) {
        this->calSlope = 1.0;
        this->calIntercept = 0.0;
        return;
    } else if (n == 1) {
        double x = this->calibration[0].first;
        double y = this->calibration[0].second;
        this->calSlope = 1.0;
        this->calIntercept = y - this->calSlope * x;
        return;
    }

    double sumX = 0, sumY = 0, sumXX = 0, sumXY = 0;
    for (auto &p : this->calibration) {
        double x = p.first;
        double y = p.second;
        sumX += x;
        sumY += y;
        sumXX += x * x;
        sumXY += x * y;
    }

    double denom = (double)n * sumXX - sumX * sumX;
    if (fabs(denom) < 1e-9) {
        this->calSlope = 1.0;
        this->calIntercept = sumY / (double)n - this->calSlope * (sumX / (double)n);
    } else {
        this->calSlope = ((double)n * sumXY - sumX * sumY) / denom;
        this->calIntercept = (sumY - this->calSlope * sumX) / (double)n;
    }

    if (DEBUGSENS) {
        Serial.print("[HC-SR04 id ");
        Serial.print(this->id);
        Serial.print("] dist input: ");
        Serial.print(dist);
        Serial.print("\t>\tcalibration updated: slope = ");
        Serial.print(this->calSlope);
        Serial.print(" intercept = ");
        Serial.println(this->calIntercept);
    }
}


/// @brief Reinicia a calibracao do sensor para a original
void HCSR04::resetcalibration() {
    this->calibration.clear();
    this->calSlope = 1.0;
    this->calIntercept = 0.0;
    if (DEBUGSENS) {
        Serial.print("[HC-SR04 id ");
        Serial.print(this->id);
        Serial.println("] calibration reset to default");
    }
}
