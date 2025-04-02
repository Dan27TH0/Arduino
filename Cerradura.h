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
    const String CLAVE_CORRECTA = "1587";          // Cambia esto por tu clave
    const unsigned long TIEMPO_ESPERA = 3000;      // 3 segundos entre intentos
    unsigned long tiempoUltimoCambio = 0;
    const unsigned long INTERVALO_CIERRE = 5000;  // 7 segundos
    String UID_PERMITIDO = "B3 67 7A 94";   // Ejemplo de UID permitido
    
    // Estado
    String claveIngresada;
    int intentosRestantes = 3;
    bool desbloqueado = false;
    bool bloqueoPorIntentos = false;               // Bloqueo total por intentos
    unsigned long ultimoIntentoFallido = 0;        // Tiempo del último fallo
    unsigned long tiempoReferencia = 0;            // Para acciones temporales
    bool enEspera = false;                         // Control de esperas
    bool modoRFID = false;                         // Control de modo de entrada

    // --- Métodos privados ---
    void manejarEntradaTeclado(char tecla) {
        if (tecla == '*' && !claveIngresada.isEmpty()) {
            claveIngresada.remove(claveIngresada.length() - 1);
            pantalla.mostrarMensaje("                ", 0, 1);  // Borra el último carácter
        } 
        else if (tecla != '*') {
            claveIngresada += tecla;
            pantalla.mostrarMensaje(claveIngresada, 0, 1);     // Muestra la clave
        }
    }

    void reiniciarIntentos() {
        intentosRestantes = 3;
        bloqueoPorIntentos = false;  // Resetea el bloqueo
    }

    void publicarEstado(const String& estado) {
        conexion.publicar("sentinel/estadoPuerta", estado.c_str());  // IoT
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
        buzzer.detenerAlarma();
    }

    void denegarAcceso() {
        intentosRestantes--;
        ultimoIntentoFallido = millis();  // Registra el momento del fallo

        pantalla.limpiar();
        pantalla.mostrarMensaje("Acceso denegado", 0, 0);
        pantalla.mostrarMensaje("Intentos: " + String(intentosRestantes), 0, 1);
        buzzer.sonarAlarma(intentosRestantes >= 0 ? 600 : 1500);  // Tonos distintos
        
        if (intentosRestantes <= 0) {
            bloqueoPorIntentos = true;  // Bloquea la cerradura
            mostrarBloqueo();
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
            reiniciarIntentos();  // Reset remoto
        }
    }

    void verificarRFID() {
        String uid = rfid.checkCard();
        if (uid != "") {
            // Normaliza ambos UIDs (elimina espacios al inicio/fin y convierte a mayúsculas)
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

public:
    // Constructor que incluye los pines para el RFID
    Cerradura(byte rfidSSPin, byte rfidRSTPin) : rfid(rfidSSPin, rfidRSTPin) {
        // Inicialización de otros componentes se hace en iniciar()
    }

    // --- Métodos públicos ---
    void iniciar() {
        pantalla.iniciar();
        rfid.init();  // Inicializa el lector RFID
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
        if (bloqueoPorIntentos) return;  // Ignora si está bloqueada

        pantalla.limpiar();
        pantalla.mostrarMensaje("Ingrese clave:", 0, 0);
        claveIngresada = "";
        
        char tecla;
        while ((tecla = leerTecla()) != '#') {
            if (tecla) manejarEntradaTeclado(tecla);
            verificarRFID();  // Verifica RFID mientras se ingresa clave
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
        if (bloqueoPorIntentos) return;  // No hace nada si está bloqueada

        if (desbloqueado && millis() - tiempoUltimoCambio >= INTERVALO_CIERRE) {
            chapa.cerrarPuerta();
        }

        // 1. Verificación constante de tarjetas RFID
        verificarRFID();

        // 2. Si pasó el TIEMPO_ESPERA tras un fallo, vuelve al inicio
        if (!desbloqueado && millis() - ultimoIntentoFallido >= TIEMPO_ESPERA) {
            mostrarMensajeInicial();
        }

        // 3. Control de acciones temporales (ej: mensajes)
        if (enEspera && millis() - tiempoReferencia >= TIEMPO_ESPERA) {
            enEspera = false;
            if (!desbloqueado) {
                chapa.cerrarPuerta();
                publicarEstado("BLOQUEADO");
            }
        }

        // 4. Comandos remotos y actualización de componentes
        manejarComandosRemotos();
        conexion.loop();
        buzzer.actualizar();
    }

    void mostrarBloqueo() {
        pantalla.limpiar();
        pantalla.mostrarMensaje("Cerradura", 0, 0);
        pantalla.mostrarMensaje("bloqueada", 0, 1);
        buzzer.sonarAlarma(intentosRestantes >= 0 ? 600 : 1500);
    }

    bool isPuertaDesbloqueada() const { 
        return desbloqueado; 
    }

    Buzzer& getBuzzer() { 
        return buzzer; 
    }

    // Método para agregar UID permitidos dinámicamente
    void agregarUIDPermitido(String uid) {
        UID_PERMITIDO = uid;
    }
};
#endif