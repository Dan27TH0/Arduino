#ifndef TECLADO_H
#define TECLADO_H

#include <Keypad.h>

class Teclado {
private:
    static const byte ROWS = 4;
    static const byte COLS = 4;
    const char keys[ROWS][COLS] = {
        {'1','2','3','A'},
        {'4','5','6','B'},
        {'7','8','9','C'},
        {'*','0','#','D'}
    };
    byte rowPins[ROWS] = {13, 12, 14, 27};
    byte colPins[COLS] = {26, 25, 33, 32};
    Keypad keypad;

public:
    Teclado() : keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS) {}
    
    char leerTecla() { return keypad.getKey(); }
};

#endif