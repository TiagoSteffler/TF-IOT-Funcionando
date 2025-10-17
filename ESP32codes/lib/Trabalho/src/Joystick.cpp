#include <Trabalho.hpp>

class Joystick{
    private:
    int xpin, ypin, botpin;
    int xval, yval, botval;
    float xper, yper;

    typedef struct joystick_read{
        int x, y, bot;
    } JoyRead;

    public:
    Joystick(int x, int y, int bot);
    JoyRead getRawValues();
    float getXfloat();
    float getYfloat();
}

Joystick::Joystick(int x, int y, int bot){
    xpin = x;
    ypin = y;
    botpin = bot;

    pinMode(xpin, INPUT);
    pinMode(ypin, INPUT);
    pinMode(botpin, INPUT_PULLUP);
}

Joystick::getRawValues(){
    JoyRead read;
    read.x = analogRead(xpin);
    read.y = analogRead(ypin);
    read.bot = digitalRead(botpin);
    return read;
}

Joystick::getXfloat(){
    xval = analogRead(xpin);
    xper = ((xval - 2048) / 2048.0) * 100.0;
    return xper;
}

Joystick::getYfloat(){
    yval = analogRead(ypin);
    yper = ((yval - 2048) / 2048.0) * 100.0;
    return yper;
}