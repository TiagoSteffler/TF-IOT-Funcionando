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
    
    pinMode(this->xpin, INPUT);
    pinMode(this->ypin, INPUT);
    pinMode(this->botpin, INPUT_PULLUP);
    
    if (DEBUGSENS) {
        Serial.print("[Joystick id ");
        Serial.print(this->id);
        Serial.println("] Inicializado");
    }
}

/// @brief Realiza a leitura bruta do joystick
/// @return Estrutura com os valores crus lidos
JoyRead Joystick::getRawValues(){
    JoyRead read;
    read.x = analogRead(xpin);
    read.y = analogRead(ypin);
    read.bot = !digitalRead(botpin);
    return read;
}

/// @brief Leitura do eixo X em porcentagem
/// @return Porcentagem do eixo X (-100% - 0% - 100%)
float Joystick::getXfloat(){
    xval = analogRead(xpin);
    xper = ((xval - 2048) / 2048.0) * 100.0;
    return xper;
}

/// @brief Leitura do eixo Y em porcentagem
/// @return Porcentagem do eixo Y (-100% - 0% - 100%)
float Joystick::getYfloat(){
    yval = analogRead(ypin);
    yper = ((yval - 2048) / 2048.0) * 100.0;
    return yper;
}