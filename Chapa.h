#ifndef CHAPA_H
#define CHAPA_H

class Chapa {
private:
    const int pinChapa;
    bool estado;

public:
    Chapa(int pin = 4) : pinChapa(pin), estado(false) {
        pinMode(pinChapa, OUTPUT);
        digitalWrite(pinChapa, LOW);  // Asegura que inicia cerrada
    }

    // Abre la chapa y desactiva el modo automático
    void abrirPuerta() {
        digitalWrite(pinChapa, HIGH);
        estado = true;
    }

    // Cierra la chapa y desactiva el modo automático
    void cerrarPuerta() {
        digitalWrite(pinChapa, LOW);
        estado = false;
    }

    bool estaAbierta() const { return estado; }
};

#endif