#include "Cerradura.h"
#include "Iman.h"
#include "ConexionIoT.h"

Cerradura cerradura(21, 5);
Iman iman(34, cerradura.getBuzzer());  // Iman simple sin callback
ConexionIoT iot;

bool ultimoEstadoIman = false;

void setup() {
    Serial.begin(115200);
    iot.conectarWiFi();
    cerradura.iniciar();
    
    // Inicializar el estado del imán al arrancar
    ultimoEstadoIman = iman.estaPresente();
    // Publicar estado inicial
    iot.publicar("sentinel/estadoPuertaAbierta", ultimoEstadoIman ? "ABIERTO" : "CERRADO");
    
    Serial.println("Sistema iniciado. Estado inicial de la puerta: " + String(ultimoEstadoIman ? "ABIERTO" : "CERRADO"));
}

void loop() {
    // Actualizar componentes
    iman.actualizar();
    cerradura.actualizar();
    // iot.loop();

    // Control del imán y publicación MQTT
    bool estadoActual = iman.estaPresente();
    
        Serial.println("Cambio detectado en el imán: " + String(estadoActual ? "ABIERTO" : "CERRADO"));
        ultimoEstadoIman = estadoActual;
        iot.publicar("sentinel/estadoPuertaAbierta", estadoActual ? "ABIERTO" : "CERRADO");
    

    // Control del teclado
    if (cerradura.leerTecla() == '#') {
        cerradura.ingresarClave();
    }
}

