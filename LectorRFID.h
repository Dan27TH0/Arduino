#ifndef RFID_RC522_H
#define RFID_RC522_H

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

class LectorRFID {
  private:
    MFRC522 mfrc522;
    
  public:
    // Constructor que recibe los pines SS y RST
    LectorRFID(byte ssPin, byte rstPin) : mfrc522(ssPin, rstPin) {
      // No se necesita hacer nada más en el constructor
    }
    
    // Método de inicialización
    void init() {
      SPI.begin();         // Inicia el bus SPI
      mfrc522.PCD_Init();  // Inicia el lector MFRC522
    }
    
    // Método para revisar si hay una tarjeta y obtener su UID
    String checkCard() {
      // Revisa si hay una tarjeta presente
      if (!mfrc522.PICC_IsNewCardPresent()) {
        return "";
      }
      
      // Intenta leer el UID de la tarjeta
      if (!mfrc522.PICC_ReadCardSerial()) {
        return "";
      }
      
      // Genera una cadena con el UID
      String uidString = "";
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        if (mfrc522.uid.uidByte[i] < 0x10) {
          uidString += " 0";
        } else {
          uidString += " ";
        }
        uidString += String(mfrc522.uid.uidByte[i], HEX);
      }
      
      // Detiene la comunicación con la tarjeta
      mfrc522.PICC_HaltA();
      
      return uidString;
    }
    
    // Método para imprimir el UID por Serial
    void printUID() {
      String uid = checkCard();
      if (uid != "") {
        Serial.print("UID:");
        Serial.println(uid);
      }
    }
    
    // Método para obtener acceso directo al objeto MFRC522 si se necesita
    MFRC522* getRFIDInstance() {
      return &mfrc522;
    }
};

#endif // RFID_RC522_H