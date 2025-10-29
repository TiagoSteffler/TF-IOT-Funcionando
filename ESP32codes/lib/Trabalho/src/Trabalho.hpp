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

#include <Preferences.h> // Para armazenamento NVS das configurações MQTT
#include "Sensors.hpp" // Inclui bibliotecas dos sensores

extern Preferences preferences; // Objeto para armazenamento NVS

// ---------------------------- CONFIGS MQTT --------------------------
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

// Gestos APDS9960
enum Gestures {UP = APDS9960_UP, DOWN = APDS9960_DOWN, LEFT = APDS9960_LEFT, RIGHT = APDS9960_RIGHT};

// Estados dos pinos
typedef enum estado_pino_t {
    DESATIVADO,
    DIGITAL_INPUT,
    DIGITAL_OUTPUT,
    ANALOGICO,
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
    void *objeto; // Ponteiro para o objeto do sensor

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


// ----------------------------- CLASSES DE SENSORES --------------------------------

// ---------------------------------------------------------------------------------


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