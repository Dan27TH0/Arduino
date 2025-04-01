#ifndef CHAPA_H
#define CHAPA_H

class Chapa {
private:
    const int pinChapa;
    bool estado;
    unsigned long tiempoUltimoCambio = 0;
    const unsigned long intervalo = 7000;  // 7 segundos
    bool modoAutomatico = false;  // Nuevo: controla si la chapa debe alternar automáticamente

public:
    Chapa(int pin = 4) : pinChapa(pin), estado(false) {
        pinMode(pinChapa, OUTPUT);
        digitalWrite(pinChapa, LOW);  // Asegura que inicia cerrada
    }

    // Abre la chapa y desactiva el modo automático
    void abrirPuerta() {
        digitalWrite(pinChapa, HIGH);
        estado = true;
        modoAutomatico = false;  // Evita que se cierre automáticamente
    }

    // Cierra la chapa y desactiva el modo automático
    void cerrarPuerta() {
        digitalWrite(pinChapa, LOW);
        estado = false;
        modoAutomatico = false;  // Evita que se abra automáticamente
    }

    // Alterna el estado solo si está en modo automático
    void actualizar() {
        if (modoAutomatico && millis() - tiempoUltimoCambio >= intervalo) {
            tiempoUltimoCambio = millis();
            estado = !estado;
            digitalWrite(pinChapa, estado);
        }
    }

    // Activa/desactiva el modo automático (ej: para temporizadores)
    void setModoAutomatico(bool activar) {
        modoAutomatico = activar;
        if (activar) {
            tiempoUltimoCambio = millis();  // Reinicia el temporizador
        }
    }

    bool estaAbierta() const { return estado; }
};

#endif