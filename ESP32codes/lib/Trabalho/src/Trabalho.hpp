#ifndef TRABALHO_HPP
#define TRABALHO_HPP
using namespace std;
// ----------------------------- INCLUDES ----------------------------
#include <Arduino.h>   
#include <Wire.h>   

#include <vector>
#include <string>  
// Wifi + MQTT + JSON
#include <WiFi.h>          
#include <PubSubClient.h>  
#include <ArduinoJson.h>    
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



// ---------------------------- CONFIGS MQTT --------------------------
#define ID_MQTT  "ESP32_005" 
const char* BROKER_MQTT = "192.168.2.100"; // Alterar para IP do servidor
int BROKER_PORT = 1883;
#define TOPICO_SUBSCRIBE "/iot2025/led/70"
#define TOPICO_PUBLISH   "/iot2025/sensors/70" 
const char* SSID = "iot2022"; // Alterar para sua rede Wi-Fi
const char* PASSWORD = "S3nhab0@"; // Alterar para sua senha Wi-Fi

// -------------------------- CONFIG DEBUG -------------------------
#define DEBUGCOMM true      // valores dummy de sensores para comunicacao
#define DEBUGSENS true      // imprime valores lidos dos sensores no monitor serial

// --------------------------- CONFIG INIT ----------------------------
/*TIPOS*/
typedef int atributo_1_t;
typedef int atributo_2_t;
typedef int atributo_3_t;
typedef int atributo_4_t;

/*VARS*/
//char* json_config; //JSON recebido por protocolo
/*FUNÇÕES*/

void init_sensor_config(char* json_config);
/*
*ENUM JSON
*O padrão descrito será utilizado pelo json de configuração inicial
*Cada número pode configurar como deve ser feita a leitura do pino
*/

// Gestos APDS9960
enum Gestures {UP = APDS9960_UP, DOWN = APDS9960_DOWN, LEFT = APDS9960_LEFT, RIGHT = APDS9960_RIGHT};

// Estados dos pinos
typedef enum estado_pino_t {
    DESATIVADO,
    DIGITAL_INPUT,
    DIGITAL_OUTPUT,
    ANALOGIC,
    SCL_,
    SDA_,
    ONE_WIRE
} Pino_tipo;

// Tipos de sensores
typedef enum tipo_sensor_t{
    MPU6050,
    DS18B20,
    HC_SR04,
    APDS_9960,
    SG_90,
    RELE,
    JOYSTICK,
    TECLADO_4X4,
    IR,
    ENCODER
} Sensor_tipo;

// Structs de pinos
typedef struct Pino {
    int pin;
    Pino_tipo tipo;
} Pino;

// Structs de sensores
typedef struct dado_sensor_t {
    int id;

    Sensor_tipo tipo;
    string desc;
    vector<Pino> pinos;
    atributo_1_t atributo1;
    atributo_2_t atributo2;
    atributo_3_t atributo3;
    atributo_4_t atributo4;

} Sensor;

// Structs de leitura do MPU6050
typedef struct mpu_read_t{
    float x, y, z, gx, gy, gz, temp;
} MPU_read;


// Struct para leitura de cor APDS9960
typedef struct apds_color_t{
    uint16_t r, g, b, c;
} APDS_Color;

// ----------------------------- PINOS --------------------------------

/// @brief Gerenciamento do I2C
class I2C_Manager {
private:
    // Pinos utilizados
    int sda_pin, scl_pin;
    
public:
    /// @brief Inicializa o barramento I2C com pinos default
    /// @param sda Pino SDA (default 17)
    /// @param scl Pino SCL (default 18)
    /// @return Se foi possivel inicializar o I2C
    bool begin(int sda = 17, int scl = 18) {
        sda_pin = sda;
        scl_pin = scl;
        return Wire.begin(sda, scl);
    }
    
    /// @brief Inicializa o barramento I2C com pinos customizados
    /// @param sda Pino SDA
    /// @param scl Pino SCL
    /// @return Se foi possivel reinicializar o I2C
    bool setI2C(int sda, int scl) {
        sda_pin = sda;
        scl_pin = scl;
        return Wire.begin(sda_pin, scl_pin);
    }
    
    /// @brief Getter pino SDA
    /// @return Retorna o pino SDA utilizado
    int getSDA() { return sda_pin; }

    /// @brief Getter pino SCL
    /// @return Retorna o pino SCL utilizado
    int getSCL() { return scl_pin; }
};



// Sensores a serem utilizados:
/*
 - Sensor de temperatura DS18B20 - 1-Wire
 - Sensor Acelerômetro MPU6050 - I2C
 - Sensor de gestos e cor APDS-9960 - I2C (ou sensor de cor TCS34725 - I2C)
 - Sensor de velocidade (encoder rotativo) - digital
 - Sensor ultrassônico HC-SR04 - digital (2 pinos)
 - Módulo relé 1 canal - digital (1 pino)
 - Micro servo SG90 - PWM (1 pino)
 - Joystick analógico - 2 pinos analógicos + 1 pino digital (botao)
 - Teclado matricial 4x4 - digital (8 pinos)    
 - Receptor IR - digital (1 pino)
*/


#endif