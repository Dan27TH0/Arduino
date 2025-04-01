#ifndef IMAN_H
#define IMAN_H

#include "Buzzer.h"

class Iman {
private:
    const int pinIman;
    Buzzer &buzzer;
    bool estadoAnterior;
    unsigned long ultimaDeteccion;

public:
    Iman(int pin, Buzzer &buz) : pinIman(pin), buzzer(buz), estadoAnterior(false) {
        pinMode(pinIman, INPUT);
    }

    void actualizar() {
        bool estadoActual = (digitalRead(pinIman) == LOW);
        
        if (estadoActual != estadoAnterior) {
            estadoAnterior = estadoActual;
            if (!estadoActual) {
                buzzer.sonarAlarma(2000);
                ultimaDeteccion = millis();
            }
        }
    }

    bool estaPresente() const { return estadoAnterior; }
};

#endif