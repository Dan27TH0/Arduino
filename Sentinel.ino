#include "Cerradura.h"
#include "Iman.h"

Cerradura cerradura(21, 5);
Iman iman(34, cerradura.getBuzzer());

void setup() {
    Serial.begin(115200);
    cerradura.iniciar();
    Serial.println("Sistema iniciado");
}

void loop() {
    char tecla = cerradura.leerTecla();

    iman.actualizar();
    cerradura.actualizar();

    if (tecla == '#') {
        cerradura.ingresarClave();
    }

   // Procesar tecla para desactivar alarma
    if (cerradura.getBuzzer().alarmaActivada()) {
        cerradura.getBuzzer().procesarTecla(tecla);
    }
}