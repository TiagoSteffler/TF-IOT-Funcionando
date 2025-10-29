//Programa: Sistema IoT ESP32 com MQTT - Sensores e Automação
//Adaptado para trabalho final IoT 2025
#include <Trabalho.hpp>
vector<Sensor *> sensores;
  
//Prototypes

I2C_Manager i2c; // Objeto gerenciador do I2C

void init_serial();
void init_sensors();
void read_sensors();

void setup() {
    init_serial();
    init_sensors();
    Serial.println("====================================");
    Serial.println("Sistema IoT ESP32 Iniciado!");
    Serial.println("====================================");
    i2c.begin();
}

void loop() {   
    read_sensors();
    delay(50);
}


  
/// @brief Inicializacao da comunicacao serial
void init_serial() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nIniciando sistema IoT...");
}


/// @brief Inicializacao dos sensores
void init_sensors() {
    Serial.println("Inicializando sensores...");
    
    // Joystick
    Joystick* joy1 = new Joystick(4, 5, 6, 1);
    
    Sensor *s1 = new Sensor();
    s1->id = 1;
    s1->tipo = JOYSTICK;
    s1->desc = "Joystick Analogico 1";
    s1->objeto = (void*)joy1;
    sensores.push_back(s1);
    
    Serial.println("Sensores inicializados!");
}


/// @brief Leitura dos sensores
void read_sensors() {
    for (auto& sensor : sensores) {
        if (sensor->tipo == JOYSTICK) {
            Joystick* joy = static_cast<Joystick*>(sensor->objeto);
            JoyRead values = joy->getRawValues();
            Serial.print("Joystick ID ");
            Serial.print(sensor->id);
            Serial.print(" - X: ");
            Serial.print(values.x);
            Serial.print(" Y: ");
            Serial.print(values.y);
            Serial.print(" Button: ");
            Serial.println(values.bot);
        }
    }
}