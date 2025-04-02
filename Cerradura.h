#ifndef CERRADURA_H
#define CERRADURA_H

#include "Teclado.h"
#include "PantallaLCD.h"
#include "Chapa.h"
#include "ConexionIoT.h"
#include "Buzzer.h"
#include "LectorRFID.h"

class Cerradura {
private:
    // Componentes
    Teclado teclado;
    PantallaLCD pantalla;
    Chapa chapa;
    ConexionIoT conexion;
    Buzzer buzzer;
    LectorRFID rfid;
    
    // Configuración
    const String CLAVE_CORRECTA = "1587";
    const unsigned long TIEMPO_ESPERA = 3000;
    unsigned long tiempoUltimoCambio = 0;
    const unsigned long INTERVALO_CIERRE = 5000;
    String UID_PERMITIDO = "B3 67 7A 94";
    
    // Control de pitidos
    enum EstadoBuzzer {
        BUZZER_APAGADO,
        BUZZER_PITIDO_ON,
        BUZZER_PITIDO_OFF,
        BUZZER_BLOQUEO
    };
    EstadoBuzzer estadoBuzzer = BUZZER_APAGADO;
    unsigned long tiempoBuzzer = 0;
    const unsigned long duracionPitido = 300;
    int pitidosRestantes = 0;

    // Estado
    String claveIngresada;
    int intentosRestantes = 3;
    bool desbloqueado = false;
    bool bloqueoPorIntentos = false;
    unsigned long ultimoIntentoFallido = 0;
    unsigned long tiempoReferencia = 0;
    bool enEspera = false;
    bool modoRFID = false;

    // Métodos privados
    void manejarEntradaTeclado(char tecla) {
        if (tecla == '*' && !claveIngresada.isEmpty()) {
            claveIngresada.remove(claveIngresada.length() - 1);
            pantalla.mostrarMensaje("                ", 0, 1);
        } 
        else if (tecla != '*') {
            claveIngresada += tecla;
            pantalla.mostrarMensaje(claveIngresada, 0, 1);
        }
    }

    void reiniciarIntentos() {
        intentosRestantes = 3;
        bloqueoPorIntentos = false;
        estadoBuzzer = BUZZER_APAGADO;
        buzzer.desactivar();
    }

    void publicarEstado(const String& estado) {
        conexion.publicar("sentinel/estadoPuerta", estado.c_str());
    }

    void concederAcceso(const String& metodo = "CLAVE") {
        pantalla.limpiar();
        pantalla.mostrarMensaje("Acceso concedido", 0, 0);
        pantalla.mostrarMensaje("Por: " + metodo, 0, 1);
        desbloqueado = true;
        chapa.abrirPuerta();
        tiempoUltimoCambio = millis();
        publicarEstado("DESBLOQUEADO");
        reiniciarIntentos();
    }

    void iniciarPitidos(int cantidad) {
        pitidosRestantes = cantidad;
        estadoBuzzer = BUZZER_PITIDO_ON;
        tiempoBuzzer = millis();
        buzzer.activar();
    }

    void denegarAcceso() {
        intentosRestantes--;
        ultimoIntentoFallido = millis();

        pantalla.limpiar();
        pantalla.mostrarMensaje("Acceso denegado", 0, 0);
        pantalla.mostrarMensaje("Intentos: " + String(intentosRestantes), 0, 1);
        
        if (intentosRestantes <= 0) {
            bloqueoPorIntentos = true;
            mostrarBloqueo();
            iniciarPitidos(3); // 3 pitidos para bloqueo
        } else {
            iniciarPitidos(3); // 3 pitidos para intento fallido
        }
    }

    void manejarComandosRemotos() {
        String mensaje = conexion.mensajeTopic();
        
        if (mensaje == "DESBLOQUEADO" && !desbloqueado) {
            pantalla.limpiar();
            pantalla.mostrarMensaje("Remoto:", 0, 0);
            pantalla.mostrarMensaje("Acceso concedido", 0, 1);
            desbloqueado = true;
            chapa.abrirPuerta();
            tiempoReferencia = millis();
            enEspera = true;
        } 
        else if (mensaje == "BLOQUEADO" && desbloqueado) {
            pantalla.limpiar();
            pantalla.mostrarMensaje("Remoto:", 0, 0);
            pantalla.mostrarMensaje("Puerta bloqueada", 0, 1);
            desbloqueado = false;
            chapa.cerrarPuerta();
            tiempoReferencia = millis();
            enEspera = true;
        }
        else if (mensaje == "RESET") {
            reiniciarIntentos();
        }
    }

    void verificarRFID() {
        String uid = rfid.checkCard();
        if (uid != "") {
            uid.trim();
            uid.toUpperCase();
            String uidPermitido = UID_PERMITIDO;
            uidPermitido.trim();
            uidPermitido.toUpperCase();
            
            if (uid == uidPermitido) {
                concederAcceso("RFID");
            } else {
                denegarAcceso();
            }
            enEspera = true;
            tiempoReferencia = millis();
        }
    }

    void actualizarBuzzer() {
        unsigned long ahora = millis();
        
        switch (estadoBuzzer) {
            case BUZZER_PITIDO_ON:
                if (ahora - tiempoBuzzer >= duracionPitido) {
                    buzzer.desactivar();
                    tiempoBuzzer = ahora;
                    estadoBuzzer = BUZZER_PITIDO_OFF;
                }
                break;
                
            case BUZZER_PITIDO_OFF:
                if (ahora - tiempoBuzzer >= duracionPitido) {
                    pitidosRestantes--;
                    if (pitidosRestantes > 0) {
                        buzzer.activar();
                        tiempoBuzzer = ahora;
                        estadoBuzzer = BUZZER_PITIDO_ON;
                    } else {
                        estadoBuzzer = BUZZER_APAGADO;
                    }
                }
                break;
                
            case BUZZER_BLOQUEO:
                // Lógica para sonido continuo de bloqueo (si se requiere)
                break;
                
            case BUZZER_APAGADO:
            default:
                break;
        }
    }

public:
    Cerradura(byte rfidSSPin, byte rfidRSTPin) : rfid(rfidSSPin, rfidRSTPin) {}

    void iniciar() {
        pantalla.iniciar();
        rfid.init();
        conexion.conectarWiFi();
        mostrarMensajeInicial();
    }

    void mostrarMensajeInicial() {
        pantalla.mostrarMensaje("Presione # o", 0, 0);
        pantalla.mostrarMensaje("acerque tarjeta", 0, 1);
    }

    char leerTecla() { 
        return teclado.leerTecla(); 
    }

    void ingresarClave() {
        if (bloqueoPorIntentos) return;

        pantalla.limpiar();
        pantalla.mostrarMensaje("Ingrese clave:", 0, 0);
        claveIngresada = "";
        
        char tecla;
        while ((tecla = leerTecla()) != '#') {
            if (tecla) manejarEntradaTeclado(tecla);
            verificarRFID();
        }
        verificarClave();
    }

    void verificarClave() {
        if (claveIngresada == CLAVE_CORRECTA) {
            concederAcceso();
            pantalla.limpiar();
        } else {
            denegarAcceso();
            pantalla.limpiar();
        }
        enEspera = true;
        tiempoReferencia = millis();
    }

    void actualizar() {
        // Control del buzzer (siempre se ejecuta)
        actualizarBuzzer();

        if (bloqueoPorIntentos) return;

        if (desbloqueado && millis() - tiempoUltimoCambio >= INTERVALO_CIERRE) {
            chapa.cerrarPuerta();
        }

        verificarRFID();

        if (!desbloqueado && millis() - ultimoIntentoFallido >= TIEMPO_ESPERA) {
            mostrarMensajeInicial();
        }

        if (enEspera && millis() - tiempoReferencia >= TIEMPO_ESPERA) {
            enEspera = false;
            if (!desbloqueado) {
                chapa.cerrarPuerta();
                publicarEstado("BLOQUEADO");
            }
        }

        manejarComandosRemotos();
        conexion.loop();
    }

    void mostrarBloqueo() {
        pantalla.mostrarMensaje("Cerradura", 0, 0);
        pantalla.mostrarMensaje("bloqueada", 0, 1);
    }

    bool isPuertaDesbloqueada() const { 
        return desbloqueado; 
    }

    Buzzer& getBuzzer() { 
        return buzzer; 
    }

    void agregarUIDPermitido(String uid) {
        UID_PERMITIDO = uid;
    }
};
#endif