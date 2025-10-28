#include <Trabalho.hpp>


class Joystick{
    private:
    int xpin, ypin, botpin;
    int xval, yval, botval;
    float xper, yper;
    uint16_t id;

    public:
    Joystick(int x, int y, int bot, uint16_t id);
    JoyRead getRawValues();
    float getXfloat();
    float getYfloat();
};

/// @brief Construtor do joystick
/// @param x Pino do eixo X
/// @param y Pino do eixo Y
/// @param bot Pino do botao
/// @param id Identificador do joystick
Joystick::Joystick(int x, int y, int bot, uint16_t id) {
    xpin = x;
    ypin = y;
    botpin = bot;
    this->id = id;

    pinMode(xpin, INPUT);
    pinMode(ypin, INPUT);
    pinMode(botpin, INPUT_PULLUP);
}

JoyRead Joystick::getRawValues(){
    JoyRead read;
    read.x = analogRead(xpin);
    read.y = analogRead(ypin);
    read.bot = digitalRead(botpin);
    return read;
}

float Joystick::getXfloat(){
    xval = analogRead(xpin);
    xper = ((xval - 2048) / 2048.0) * 100.0;
    return xper;
}

float Joystick::getYfloat(){
    yval = analogRead(ypin);
    yper = ((yval - 2048) / 2048.0) * 100.0;
    return yper;
}