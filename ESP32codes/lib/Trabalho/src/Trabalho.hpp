#ifndef TRABALHO_HPP
#define TRABALHO_HPP
using namespace std;
// ----------------------------- INCLUDES ----------------------------
#include <Arduino.h>   
#include <Wire.h>
#include <vector>
#include <string>  

// Wifi + MQTT + JSON + FS
#include <WiFi.h>
#include <WiFiManager.h>      
#include <PubSubClient.h>  
#include <ArduinoJson.h>    
#include <SPIFFS.h>

#include <Preferences.h> // Para armazenamento NVS das configurações MQTT
#include "Sensors.hpp" // Inclui bibliotecas dos sensores

extern Preferences preferences; // Objeto para armazenamento NVS

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
    MPU_6050,
    DS18_B20,
    HC_SR04,
    APDS_9960,
    SG_90,
    RELE,
    JOYSTICK,
    TECLADO_4X4,
    ENCODER,
    DHT_11
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
    int atributo1;
    int atributo2;
    int atributo3;
    int atributo4;
    void *objeto; // Ponteiro para o objeto do sensor

}Sensor;

/*JSON*/
extern char SENSOR_CONFIG[]; // JSON de configuração dos sensores
void loadJSONSensorConfig();
void saveJSONSensorConfig(const char* json_config);


vector<Sensor> init_sensor_config(const char* json_config);
extern vector<Sensor> sensores; // Vetor global de sensores

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


// ---------------------------- CONFIGS MQTT --------------------------

/* Configuracoes para rede */
extern WiFiClient espClient;
extern PubSubClient MQTT;

typedef struct mqttconfig {
  String broker;
  String port;
  String id;
  bool isConfigured() {
    return (broker.length() > 0 && port.length() > 0 && id.length() > 0);
  }
} MQTTConfig;

typedef struct wificonfig {
  String ssid = "";
  String password = "";
  bool isConfigured() {
    return (ssid.length() > 0 && password.length() > 0);
  }
} WiFiConfig;

extern MQTTConfig mqttConfig;
extern WiFiConfig wifiConfig;

// Controle de heartbeat
extern unsigned long lastPing;
extern const unsigned long PING_INTERVAL;

extern I2C_Manager i2c;

extern bool publishGetterMQTT;
extern String payloadTopicSensors;
extern String payloadSensors;
// ------------------------------------------------------------

// ------------------------ INITIALIZING ---------------------------
int initESP();
int initSetup();
void initSerial();
int initSPIFFS();
int initMQTT(void);
int initWiFi();
int tryPairing();
void handleReset();
int searchWifi();

// ------------------------- FILE MANAGEMENT ---------------------------------
int createFileIfNotExists(const char* path);
int saveWifiConfig(const char* ssid, const char* password);
int saveMQTTConfig(const char* broker, const char* port, const char* id);
void eraseConnections();
void eraseAllConfigurations();

// ----------------------------- MQTT RELATED -------------------------------------
bool reconnectMQTT(MQTTConfig config, int maxAttempts);
int subscribeMQTTTopics();
void callbackMQTT(char* topic, byte* payload, unsigned int length);
void publishHeartbeatMQTT();

// --------------------------------- SENSOR CONFIG --------------------------------
int initDevices();
int addOrUpdateSensor(const char* jsonPayload);
bool saveDevicesToFile();
bool createSensorObject(Sensor &sensor);
int removeSensorById(const char* jsonPayload);
String buildSensorPayload(Sensor *sensor);
// Sensores a serem utilizados:
/*
 - [x] Sensor de temperatura DS18B20 - 1-Wire
 - [x] Sensor Acelerômetro MPU6050 - I2C
 - [x] Sensor de gestos e cor APDS-9960 - I2C (ou sensor de cor TCS34725 - I2C)
 - [ ] Sensor de velocidade (encoder rotativo) - digital
 - [x] Sensor ultrassônico HC-SR04 - digital (2 pinos)
 - [x] Módulo relé 1 canal - digital (1 pino)
 - [x] Micro servo SG90 - PWM (1 pino)
 - [x] Joystick analógico - 2 pinos analógicos + 1 pino digital (botao)
 - [x] Teclado matricial 4x4 - digital (8 pinos)    
 - [ ] Receptor IR - digital (1 pino)
*/


#endif