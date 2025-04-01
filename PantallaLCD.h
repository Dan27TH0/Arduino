#ifndef PANTALLA_LCD_H
#define PANTALLA_LCD_H

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

class PantallaLCD {
private:
    LiquidCrystal_I2C lcd;
    const int pinSDA = 17;
    const int pinSCL = 16;

public:
    PantallaLCD() : lcd(0x27, 16, 2) {}

    void iniciar() {  // Asegúrate que se llama iniciar() no iniciarLCD()
        Wire.begin(pinSDA, pinSCL);
        lcd.init();
        lcd.backlight();
        limpiar();
    }

    void mostrarMensaje(const String &mensaje, int col = 0, int fila = 0) {
        lcd.setCursor(col, fila);
        lcd.print(mensaje);
    }

    void limpiar() { 
      lcd.setCursor(0, 0);
      lcd.print("                "); 
      lcd.setCursor(0, 1);
      lcd.print("                "); 
    }
};

#endif