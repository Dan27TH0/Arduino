#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

class Buzzer {
private:
    const int pinBuzzer;
    bool estadoAlarma; // Cambiado de alarmaActiva a estadoAlarma
    unsigned long tiempoAlarmaInicio;
    const unsigned long intervaloPitido = 500;
    const String codigoDesactivacion = "9999";
    String codigoIngresado;

public:
    Buzzer(int pin = 15) : pinBuzzer(pin), estadoAlarma(false) {
        pinMode(pinBuzzer, OUTPUT);
    }

    void activar() { digitalWrite(pinBuzzer, HIGH); }
    void desactivar() { digitalWrite(pinBuzzer, LOW); }

    void sonarAlarma(unsigned long duracion) {
        if (!estadoAlarma) {
            estadoAlarma = true;
            tiempoAlarmaInicio = millis();
        }
    }

    void detenerAlarma() {
        estadoAlarma = false;
        desactivar();
    }

    void actualizar() {
        if (estadoAlarma) {
            unsigned long tiempoActual = millis() - tiempoAlarmaInicio;
            digitalWrite(pinBuzzer, (tiempoActual % 1000) < 500 ? HIGH : LOW);
        }
    }

    bool procesarTecla(char tecla) {
        if (!estadoAlarma) return false;
        
        codigoIngresado += tecla;
        if (codigoIngresado.length() >= codigoDesactivacion.length()) {
            if (codigoIngresado == codigoDesactivacion) {
                detenerAlarma();
                codigoIngresado = "";
                return true;
            }
            codigoIngresado = "";
        }
        return false;
    }

    bool alarmaActivada() const { return estadoAlarma; } // Cambiado el nombre del getter
};

#endif