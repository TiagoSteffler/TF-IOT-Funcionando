#include <Trabalho.hpp>

/// @brief Construtor do joystick
/// @param x Pino do eixo X
/// @param y Pino do eixo Y
/// @param bot Pino do botao
/// @param id Identificador do joystick
Joystick::Joystick(int x, int y, int bot, uint16_t id) {
    this->xpin = x;
    this->ypin = y;
    this->botpin = bot;
    this->id = id;
    this->xval = 0;
    this->yval = 0;
    this->xper = 0.0;
    this->yper = 0.0;
    
    if (DEBUGSENS) {
        Serial.print("[Joystick id ");
        Serial.print(this->id);
        Serial.println("] Inicializado");
    }
    
    if (DEBUGCOMM) return;
    
    pinMode(this->xpin, INPUT);
    pinMode(this->ypin, INPUT);
    pinMode(this->botpin, INPUT_PULLUP);
}

/// @brief Realiza a leitura bruta do joystick
/// @return Estrutura com os valores crus lidos
JoyRead Joystick::getRawValues(){
    JoyRead read;
    
    if (DEBUGCOMM) {
        // Valores dummy (0-4095 para ESP32, centro em ~2048)
        read.x = rand() % 4096;
        read.y = rand() % 4096;
        read.bot = rand() % 2; // 0 ou 1
    } else {
        read.x = analogRead(xpin);
        read.y = analogRead(ypin);
        read.bot = !digitalRead(botpin);
    }
    
    return read;
}

/// @brief Leitura do eixo X em porcentagem
/// @return Porcentagem do eixo X (-100% - 0% - 100%)
float Joystick::getXfloat(){
    if (DEBUGCOMM) {
        // Valor dummy entre -100 e 100
        xper = (static_cast<float>(rand()) / RAND_MAX) * 200.0 - 100.0;
    } else {
        xval = analogRead(xpin);
        xper = ((xval - 2048) / 2048.0) * 100.0;
    }
    return xper;
}

/// @brief Leitura do eixo Y em porcentagem
/// @return Porcentagem do eixo Y (-100% - 0% - 100%)
float Joystick::getYfloat(){
    if (DEBUGCOMM) {
        // Valor dummy entre -100 e 100
        yper = (static_cast<float>(rand()) / RAND_MAX) * 200.0 - 100.0;
    } else {
        yval = analogRead(ypin);
        yper = ((yval - 2048) / 2048.0) * 100.0;
    }
    return yper;
}