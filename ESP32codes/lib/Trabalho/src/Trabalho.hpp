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
#include <WiFiManager.h>      
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
// #include <Servo.h>
// DS18B20
#include <OneWire.h>  
#include <DallasTemperature.h>



#include <Preferences.h> // Para armazenamento NVS das configurações MQTT

extern Preferences preferences; // Objeto para armazenamento NVS

// ---------------------------- CONFIGS MQTT --------------------------
// #define ID_MQTT  "ESP32_005" // Removido, usando ID_DEVICE em vez disso

#define TOPICO_SUBSCRIBE_PREFIX "config"
#define TOPICO_PUBLISH_PREFIX "sensor" 

extern char BROKER_MQTT[];
extern int BROKER_PORT;
extern char SSID[];
extern char PASSWORD[];
extern char ID_DEVICE[];

/* Objetos compartilhados (definidos em MQTTsettings.cpp) */

extern WiFiClient espClient;
extern PubSubClient MQTT;

/* Funções do módulo MQTT/MANAGEMENT usadas pela main */
void init_mqtt(void);
void reconnect_mqtt(void);
void reconnect_wifi(void);
void verifica_conexoes_wifi_mqtt(void);
void loadMQTTSettings(void); // Declaração da função para carregar configurações MQTT
void saveMQTTSettings(void); // Declaração da função para salvar configurações MQTT
void readAndPublishSensors(void);

// --------------------------- CONFIG INIT ----------------------------
/*TIPOS*/
typedef int atributo_1_t;
typedef int atributo_2_t;
typedef int atributo_3_t;
typedef int atributo_4_t;
/*
*ENUM JSON
*O padrão descrito será utilizado pelo json de configuração inicial
*Cada número pode configurar como deve ser feita a leitura do pino
*/
typedef enum estado_pino_t {
    DESATIVADO,
    DIGITAL_INPUT,
    DIGITAL_OUTPUT,
    ANALOGIC,
    SCL_,
    SDA_,
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


typedef struct dado_sensor_t
{
    int id;

    Sensor_tipo tipo;
    string desc;
    vector<Pino> pinos;
    atributo_1_t atributo1;
    atributo_2_t atributo2;
    atributo_3_t atributo3;
    atributo_4_t atributo4;

}Sensor;

/*JSON*/
extern char SENSOR_CONFIG[]; // JSON de configuração dos sensores
void loadJSONSensorConfig();
void saveJSONSensorConfig(const char* json_config);

/*VARS*/
//char* json_config; //JSON recebido por protocolo
/*FUNÇÕES*/

vector<Sensor> init_sensor_config(const char* json_config);


//--------------------------------------------------------------------


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