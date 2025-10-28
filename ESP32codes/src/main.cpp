//Programa: Sistema IoT ESP32 com MQTT - Sensores e Automação
//Adaptado para trabalho final IoT 2025
#include <Trabalho.hpp>
vector<Sensor> sensores;
  
//Prototypes

I2C_Manager i2c; // Objeto gerenciador do I2C

void init_serial();
void init_sensors();

void setup() {
    init_serial();
    init_sensors();
    Serial.println("====================================");
    Serial.println("Sistema IoT ESP32 Iniciado!");
    Serial.println("====================================");
    i2c.begin();
}

void loop() {   
    delay(50);
}


  
/// @brief Inicializacao da comunicacao serial
void init_serial() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nIniciando sistema IoT...");
}

void init_sensors() {
    // joystick
    Serial.println("Inicializando sensores...");
    Joystick joy1(34, 35, 32, 1);
   
}