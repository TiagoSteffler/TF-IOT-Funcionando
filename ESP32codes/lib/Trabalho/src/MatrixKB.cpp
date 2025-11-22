#include <Trabalho.hpp>
#define NO_KEY '\0'

/// @brief Construtor do keypad 4x4
/// @param rowPins vetor com os pinos das linhas
/// @param colPins vetor com os pinos das colunas
/// @param id identificador do keypad
KeyPad::KeyPad(int rowPins[4], int colPins[4], int id) {
    this->id = id;
    this->keypad = nullptr;

    for (int i = 0; i < 4; i++) {
        this->pin_rows[i] = rowPins[i];
        this->pin_cols[i] = colPins[i];
    }
    
    if (DEBUGSENS) {
        Serial.print("[KeyPad id ");
        Serial.print(this->id);
        Serial.println("] conectado com sucesso");
    }
    
    if (DEBUGCOMM) return;
    
    this->keypad = new Keypad(makeKeymap(keys), this->pin_rows, this->pin_cols, 4, 4);
    
    if (this->keypad == nullptr) {
        Serial.println("[KeyPad] ERRO: Falha ao criar objeto Keypad");
    }
}


/// @brief Realiza leitura por tecla pressionada
/// @return Tecla pressionada ou NO_KEY se nenhuma tecla for pressionada
char KeyPad::getKey() {
    if (DEBUGCOMM) {
        // Retorna tecla aleatória para modo dummy (simula teclas '0'-'9', 'A'-'D', '*', '#')
        static const char dummyKeys[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','*','#',NO_KEY};
        char key = dummyKeys[rand() % 17];
        
        if (key != NO_KEY && DEBUGSENS) {
            Serial.print("[KeyPad id ");
            Serial.print(this->id);
            Serial.print("] dummy key: ");
            Serial.println(key);
        }
        
        return key;
    }
    
    // Verifica se keypad foi inicializado
    if (this->keypad == nullptr) {
        return NO_KEY;
    }
    
    char key = keypad->getKey();

    if (key && DEBUGSENS) {
        Serial.print("[KeyPad id ");
        Serial.print(this->id);
        Serial.print("] key pressed : ");
        if (key == NO_KEY) Serial.println("NO_KEY");
        else Serial.println(key);
    }

    return key;
}