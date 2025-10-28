// MPU6050
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
// APDS9960
#include <Adafruit_APDS9960.h>
// Keypad 4x4
#include <Keypad.h>
// Servo SG90
#include <ESP32Servo.h>
// DS18B20
#include <OneWire.h>  
#include <DallasTemperature.h>

// -------------------------- CONFIG DEBUG -------------------------
#define DEBUGCOMM true      // valores dummy de sensores para comunicacao
#define DEBUGSENS true      // imprime valores lidos dos sensores no monitor serial

// ------------- ESTRUTURAS DE DADOS ---------------
// Structs de leitura do MPU6050
typedef struct mpu_read_t{
    float x, y, z, gx, gy, gz, temp;
} MPU_read;


// Struct para leitura de cor APDS9960
typedef struct apds_color_t{
    uint16_t r, g, b, c;
} APDS_Color;

// Struct para leitura do joystick
typedef struct joystick_read{
    int x, y, bot;
} JoyRead;
// ------------------------------------------------------

/// @brief Sensor de cor e gestos APDS9960
class APDS9960{
    private:
        // variaveis de controle
        Adafruit_APDS9960 *apds;
        int id;
        int interruptPin;

    public:
        // contrutores/destrutores
        APDS9960::APDS9960(int id, int interruptPin);
        ~APDS9960() {};

        // metodos publicos
        APDS_Color getColor();
        uint8_t getProx();
        uint8_t getGesture();
};


/// @brief Sensor de temperatura DS18B20
class DS18B20{
private:
    // variaveis de controle
    int dataPin;
    int id;
    OneWire* oneWire;
    DallasTemperature* sensors;
    enum Unit {C, F};

    public:
    // contrutores/destrutores
    DS18B20(int pin, int id);
    ~DS18B20() {};

    // metodos publicos
    float readTemperature(Unit unit = C);
};


/// @brief Classe do joystick analogico
class Joystick{
    private:
    int xpin, ypin, botpin;
    int xval, yval, botval;
    float xper, yper;
    uint16_t id;

    public:
    Joystick(int x, int y, int bot, uint16_t id);
    JoyRead getRawValues();
    float getXfloat();
    float getYfloat();
};


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


/// @brief Classe do keypad 4x4
class KeyPad {
    private:
        /// @brief Mapa de teclas do keypad e controle
        char keys[4][4]{
            {'1', '2', '3', 'A'},
            {'4', '5', '6', 'B'},
            {'7', '8', '9', 'C'},
            {'*', '0', '#', 'D'}
        };
        Keypad* keypad;
        byte pin_rows[4], pin_cols[4];
        int id;

    public:
        // contrutores/destrutores
        KeyPad(int rowPins[4], int colPins[4], int id);
        ~KeyPad() { delete keypad; };

        // metodos publicos
        char getKey();
};


/// @brief Classe do acelerometro MPU6050
class MPU6050 {
    private:
        // pino e variaveis de controle
        Adafruit_MPU6050 mpu;
        int id, addr_pin, addr;
        MPU_read read;
        enum AccRange {A2G = MPU6050_RANGE_2_G, A4G = MPU6050_RANGE_4_G, A8G = MPU6050_RANGE_8_G, A16G = MPU6050_RANGE_16_G} acc_range = A8G;
        enum GyroRange {G250DPS = MPU6050_RANGE_250_DEG, G500DPS = MPU6050_RANGE_500_DEG, G1000DPS = MPU6050_RANGE_1000_DEG, G2000DPS = MPU6050_RANGE_2000_DEG} gyro_range = G500DPS;
        enum FilterBandwidth {B260HZ = MPU6050_BAND_260_HZ, B184HZ = MPU6050_BAND_184_HZ, B94HZ = MPU6050_BAND_94_HZ, B44HZ = MPU6050_BAND_44_HZ, B21HZ = MPU6050_BAND_21_HZ, B10HZ = MPU6050_BAND_10_HZ, B5HZ = MPU6050_BAND_5_HZ} filter_bandwidth = B5HZ;

    public:
        // contrutores/destrutores
        MPU6050(int id, int addr_pin = -1);
        ~MPU6050() {};

        // metodos publicos
        void setParamsMPU(AccRange acc_range, GyroRange gyro_range, FilterBandwidth filter_bandwidth);
        MPU_read getValues();
};


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

