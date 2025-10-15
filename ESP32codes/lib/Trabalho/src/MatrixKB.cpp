#include "Trabalho.hpp"

#define NO_KEY '\0'
class Matrix4x4 {
private:
    char keys[4][4]{
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}
    };
    Keypad keypad;

public:
    Matrix4x4(int rowPins[4], int colPins[4]);
    ~Matrix4x4();
    char getKey();
}

Matrix4x4::Matrix4x4(int rowPins[4], int colPins[4]) {
    byte pin_rows[4];
    byte pin_cols[4];
    
    for (int i = 0; i < 4; i++) {
        pin_rows[i] = rowPins[i];
        pin_cols[i] = colPins[i];
    }
    keypad = Keypad( makeKeymap(keys), pin_rows, pin_cols, 4, 4);
}

Matrix4x4::~Matrix4x4() {

}

char Matrix4x4::getKey() {
    char key = keypad.getKey();

    if (key) {
        Serial.print("Key Pressed : ");
        Serial.println(key);
    }

    return key;
}