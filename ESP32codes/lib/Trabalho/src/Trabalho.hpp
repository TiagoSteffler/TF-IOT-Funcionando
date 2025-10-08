#ifndef TRABALHO_HPP
#define TRABALHO_HPP

// ----------------------------- INCLUDES ----------------------------
#include <WiFi.h>          
#include <PubSubClient.h>  
#include <Arduino.h>        
#include <ArduinoJson.h>    

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <Vector>


// ---------------------------- CONFIGS MQTT --------------------------
#define ID_MQTT  "ESP32_005" 
const char* BROKER_MQTT = "192.168.2.100"; // Alterar para IP do servidor
int BROKER_PORT = 1883;
#define TOPICO_SUBSCRIBE "/iot2025/led/70"
#define TOPICO_PUBLISH   "/iot2025/sensors/70" 
const char* SSID = "iot2022"; // Alterar para sua rede Wi-Fi
const char* PASSWORD = "S3nhab0@"; // Alterar para sua senha Wi-Fi

// --------------------------- CONFIG INIT ----------------------------
/*VARS*/
//char* json_config; //JSON recebido por protocolo
/*FUNÇÕES*/

void init_sensor_config(char* json_config);
/*
*ENUM JSON
*O padrão descrito será utilizado pelo json de configuração inicial
*Cada número pode configurar como deve ser feita a leitura do pino
*/
typedef enum estado_pino_t{
    DESATIVADO,
    DIGITAL_INPUT,
    DIGITAL_OUTPUT,
    ANALOG,
    SCL,
    SDA,
    ONE_WIRE
} Pino_tipo;

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

typedef struct Pino
{
    int pin;
    Pino_tipo tipo;
}Pino;


typedef struct Sensor
{
    int id;
    Sensor_tipo tipo;
    
    int atributo1;
    int atributo2;
    int atributo3;
    int atributo4;

}Sensor;




typedef struct apds_read_t{
    int r, g, b, c, proximity;
} APDS_read;

// ----------------------------- PINOS --------------------------------
#define PIN_SCL 17
#define PIN_SDA 18

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