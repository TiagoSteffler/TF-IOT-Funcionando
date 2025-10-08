#ifndef TRABALHO_HPP
#define TRABALHO_HPP

// ----------------------------- INCLUDES ----------------------------
#include <WiFi.h>          
#include <PubSubClient.h>  
#include <Arduino.h>        
#include <ArduinoJson.h>    

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