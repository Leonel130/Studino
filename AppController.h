#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include "Arduino.h"
#include <LiquidCrystal_I2C.h>
#include "Timer.h"
#include "LedAnimator.h"
#include "Animation.h"
#include "MenuController.h"

const unsigned long INTERVALO_REFRESCO_LCD_MS = 1000;

class AppController {
public:
    enum AppEstado {
        INICIALIZANDO,
        CONFIGURACION,
        SESION_ESTUDIO,
        SESION_PAUSA
    };

private:
    AppEstado estadoActual;

    Timer* timer;
    LedAnimator* animador;
    LiquidCrystal_I2C* lcd;
    MenuController* menu;

    unsigned long duracionEstudioMS;
    unsigned long duracionPausaMS;

    const Animation *animInicio, *animConfig, *animEstudio, *animPausa;

    unsigned long ultimoRefrescoLCD;

    void transicionarA(AppEstado nuevoEstado);
    void gestionarRefrescoLCD();
    void imprimirTiempoLCD(unsigned long ms);

public:
    AppController(Timer& timer, LedAnimator& animador, 
                  LiquidCrystal_I2C& lcd, MenuController& menu,
                  const Animation& animInicio, const Animation& animConfig,
                  const Animation& animEstudio, const Animation& animPausa);
    
    void iniciar();
    void actualizar();
};

#endif