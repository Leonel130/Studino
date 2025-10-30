#ifndef MENU_CONTROLLER_H
#define MENU_CONTROLLER_H

#include "Arduino.h"
#include <LiquidCrystal_I2C.h>

const unsigned int MAX_ESTUDIO_MIN = 60;
const unsigned int MAX_PAUSA_MIN   = 30;
const unsigned int STEP_MIN        = 5;
const unsigned long DEBOUNCE_MS    = 200;

class MenuController {
public:
    enum MenuEstado {
        CONFIG_ESTUDIO,
        CONFIG_DESCANSO,
        CONFIRMACION,
        FINALIZADO
    };

private:
    MenuEstado estadoActual;
    LiquidCrystal_I2C* lcd;
    
    int minutosEstudio;
    int minutosPausa;

    int pinSuma, pinResta, pinConfirma;
    unsigned long lastSuma, lastResta, lastConfirma;

    void imprimirPantallaMenu();

public:
    MenuController(LiquidCrystal_I2C& lcd, 
                   int pinSuma, int pinResta, int pinConfirma,
                   int minEstudioDefecto, int minPausaDefecto);

    void iniciar();
    void actualizar();
    
    MenuEstado getEstado() const;
    int getMinutosEstudio() const;
    int getMinutosPausa() const;
};

#endif