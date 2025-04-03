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
  String ultimoEstadoPublicado = "";                // Guarda el último estado enviado
  unsigned long ultimoEnvioMQTT = 0;                // Tiempo del último envío
  const unsigned long RETRASO_ENTRE_ENVIOS = 1000;  // Esperar 1s entre publicaciones (ajustable)
  String ultimoComandoRecibido = "";  // Nuevo miembro para control
  unsigned long tiempoUltimoComando = 0;
  const unsigned long TIEMPO_IGNORAR_COMANDOS = 2000; // 2 segundos
  bool recibidoPorApp = false;
  unsigned long ultimoComandoApp = 0;
  const unsigned long TIEMPO_BLOQUEO_APP = 2000;

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

  void publicarAcceso(const String& metodo) {
    // Crear un JSON con los datos del acceso
    String mensaje = "{";
    mensaje += "\"fecha\":\"" + conexion.obtenerFecha() + "\",";
    mensaje += "\"hora\":\"" + conexion.obtenerHora() + "\",";
    mensaje += "\"metodo\":\"" + metodo + "\",";
    mensaje += "\"estado\":\"PERMITIDO\"";
    mensaje += "}";
    
    conexion.publicar("sentinel/accesos", mensaje.c_str());
  }

  void publicarIntentoFallido(const String& metodo) {
      // Crear un JSON con los datos del intento fallido
      String mensaje = "{";
      mensaje += "\"fecha\":\"" + conexion.obtenerFecha() + "\",";
      mensaje += "\"hora\":\"" + conexion.obtenerHora() + "\",";
      mensaje += "\"metodo\":\"" + metodo + "\",";
      mensaje += "\"estado\":\"DENEGADO\"";
      mensaje += "}";
      
      conexion.publicar("sentinel/accesos", mensaje.c_str());
  }

  void cerrarPuertaConEstado() {  // ⭐ Nuevo método privado
    chapa.cerrarPuerta();         // Asegurar que la chapa se cierra
    desbloqueado = false;         // Marcar la puerta como bloqueada
    publicarEstado("BLOQUEADO");  // Comunicación MQTT
    tiempoUltimoCambio = millis();
  }

  // Métodos privados
  void manejarEntradaTeclado(char tecla) {
    if (tecla == '*' && !claveIngresada.isEmpty()) {
      claveIngresada.remove(claveIngresada.length() - 1);
      pantalla.mostrarMensaje("                ", 0, 1);
    } else if (tecla != '*') {
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

  void publicarEstado(const String& nuevoEstado) {
    unsigned long ahora = millis();

    if (recibidoPorApp && (ahora - ultimoComandoApp < TIEMPO_BLOQUEO_APP)) {
      return;
    }

    // Resto de la lógica original...
    if (nuevoEstado != ultimoEstadoPublicado && ahora - ultimoEnvioMQTT >= RETRASO_ENTRE_ENVIOS) {
      conexion.publicar("sentinel/estadoPuerta", nuevoEstado.c_str());
      ultimoEstadoPublicado = nuevoEstado;
      ultimoEnvioMQTT = ahora;
    }
  }


  void concederAcceso(const String& metodo) {
    pantalla.limpiar();
    pantalla.mostrarMensaje("Acceso concedido", 0, 0);
    pantalla.mostrarMensaje("Por: " + metodo, 0, 1);
    desbloqueado = true;
    chapa.abrirPuerta();
    tiempoUltimoCambio = millis();
    publicarEstado("DESBLOQUEADO");
    publicarAcceso(metodo);
    reiniciarIntentos();
  }

  void iniciarPitidos(int cantidad) {
    pitidosRestantes = cantidad;
    estadoBuzzer = BUZZER_PITIDO_ON;
    tiempoBuzzer = millis();
    buzzer.activar();
  }

  void denegarAcceso(const String& metodo) {
    intentosRestantes--;
    ultimoIntentoFallido = millis();

    pantalla.limpiar();
    pantalla.mostrarMensaje("Acceso denegado", 0, 0);
    pantalla.mostrarMensaje("Intentos: " + String(intentosRestantes), 0, 1);
    publicarIntentoFallido(metodo);  

    if (intentosRestantes <= 0) {
      bloqueoPorIntentos = true;
      mostrarBloqueo();
      iniciarPitidos(3);  // 3 pitidos para bloqueo
    } else {
      iniciarPitidos(3);  // 3 pitidos para intento fallido
    }
  }

  void manejarComandosRemotos() {
      String mensaje = conexion.mensajeTopic();
      
      // Ignora comandos duplicados recientes
      if (mensaje == ultimoComandoRecibido && 
          (millis() - tiempoUltimoComando < TIEMPO_IGNORAR_COMANDOS)) {
          return;
      }

      if (mensaje == "DESBLOQUEADO") {
          if (!desbloqueado) {
              recibidoPorApp = true;
              ultimoComandoApp = millis();
              pantalla.limpiar();
              pantalla.mostrarMensaje("Remoto:", 0, 0);
              pantalla.mostrarMensaje("Acceso concedido", 0, 1);
              desbloqueado = true;
              chapa.abrirPuerta();
              tiempoUltimoCambio = millis();
              
              // Actualiza el control de comandos
              ultimoComandoRecibido = mensaje;
              tiempoUltimoComando = millis();
              
              // Publica solo si es un cambio real (no por comando remoto)
              if (ultimoEstadoPublicado != "DESBLOQUEADO") {
                  publicarEstado("DESBLOQUEADO");
              }
              publicarAcceso("APP");
          }
      } 
      else if (mensaje == "BLOQUEADO") {
          if (desbloqueado) {
              pantalla.limpiar();
              pantalla.mostrarMensaje("Remoto:", 0, 0);
              pantalla.mostrarMensaje("Puerta bloqueada", 0, 1);
              
              // Actualiza el control de comandos
              ultimoComandoRecibido = mensaje;
              tiempoUltimoComando = millis();
              
              cerrarPuertaConEstado();
          }
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
        denegarAcceso("RFID");
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
  Cerradura(byte rfidSSPin, byte rfidRSTPin)
    : rfid(rfidSSPin, rfidRSTPin) {}

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
      concederAcceso("CLAVE");
      pantalla.limpiar();
    } else {
      denegarAcceso("CLAVE");
      pantalla.limpiar();
    }
    enEspera = true;
    tiempoReferencia = millis();
  }

  void actualizar() {
    // Control del buzzer (siempre se ejecuta)
    actualizarBuzzer();
    verificarRFID();
    if (bloqueoPorIntentos) return;

    if (desbloqueado && millis() - tiempoUltimoCambio >= INTERVALO_CIERRE) {
      cerrarPuertaConEstado();
    }

    verificarRFID();

    if (!desbloqueado && millis() - ultimoIntentoFallido >= TIEMPO_ESPERA) {
      mostrarMensajeInicial();
    }

    if (enEspera && millis() - tiempoReferencia >= TIEMPO_ESPERA) {
      enEspera = false;
      if (!desbloqueado) {
        cerrarPuertaConEstado();
        publicarEstado("BLOQUEADO");
      }
    }

    if (recibidoPorApp && (millis() - ultimoComandoApp >= TIEMPO_BLOQUEO_APP)) {
      recibidoPorApp = false;
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