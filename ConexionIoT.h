#ifndef CONEXION_IOT_H
#define CONEXION_IOT_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

class ConexionIoT {
private:
    const char* ssid = "Redmi Note 12";
    const char* password = "1q2w3e4r5t6y";
    const char* mqtt_server = "kfa70af4.ala.eu-central-1.emqxsl.com";
    const int mqtt_port = 8883;
    const char* mqtt_user = "danohx";
    const char* mqtt_pass = "270105Dt";

    WiFiClientSecure espClient;
    PubSubClient client;
    String ultimoMensaje;

    void reconnect() {
        while (!client.connected()) {
            Serial.print("Conectando MQTT...");
            if (client.connect("ESP32Client", mqtt_user, mqtt_pass)) {
                client.subscribe("sentinel/estadoPuerta");
                Serial.println("conectado");
            } else {
                Serial.print("falló, rc=");
                Serial.print(client.state());
                delay(5000);
            }
        }
    }

public:
    ConexionIoT() : client(espClient) {
        espClient.setInsecure();
        client.setServer(mqtt_server, mqtt_port);
        client.setCallback([this](char* topic, byte* payload, unsigned int length) {
            ultimoMensaje = "";
            for (unsigned int i = 0; i < length; i++) {
                ultimoMensaje += (char)payload[i];
            }
            Serial.printf("Mensaje [%s]: %s\n", topic, ultimoMensaje.c_str());
        });
    }

    void conectarWiFi() {
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
        Serial.printf("\nConectado. IP: %s\n", WiFi.localIP().toString().c_str());
    }

    String mensajeTopic() const { return ultimoMensaje; }

    void publicar(const char* topic, const char* mensaje) {
        if (!client.connected()) reconnect();
        client.publish(topic, mensaje);
    }

    void loop() {
        if (!client.connected()) reconnect();
        client.loop();
    }
    String obtenerFecha() {
        // Implementación para obtener fecha actual (YYYY-MM-DD)
        time_t now = time(nullptr);
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        char buffer[11];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d", &timeinfo);
        return String(buffer);
    }

    String obtenerHora() {
        // Implementación para obtener hora actual (HH:MM:SS)
        time_t now = time(nullptr);
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        char buffer[9];
        strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
        return String(buffer);
    }

};

#endif