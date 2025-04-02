#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

class Buzzer {
private:
    const int pinBuzzer;

public:
    Buzzer(int pin = 15) : pinBuzzer(pin) {
        pinMode(pinBuzzer, OUTPUT);
        digitalWrite(pinBuzzer, LOW);
    }

    // Enciende el buzzer (HIGH)
    void activar() { 
        digitalWrite(pinBuzzer, HIGH); 
    }

    // Apaga el buzzer (LOW)
    void desactivar() { 
        digitalWrite(pinBuzzer, LOW); 
    }
};

#endif