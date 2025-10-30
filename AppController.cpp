#include "AppController.h"

AppController::AppController(Timer& timer, LedAnimator& animador, LiquidCrystal_I2C& lcd,
                             int pinSuma, int pinResta, int pinConfirma,
                             unsigned long estudioDefectoMS, unsigned long pausaDefectoMS,
                             const Animation& animInicio, const Animation& animConfig,
                             const Animation& animEstudio, const Animation& animPausa) {
    
    this->timer = &timer;
    this->animador = &animador;
    this->lcd = &lcd;
    
    this->pinSuma = pinSuma;
    this->pinResta = pinResta;
    this->pinConfirma = pinConfirma;

    this->duracionEstudioMS = estudioDefectoMS;
    this->duracionPausaMS = pausaDefectoMS;
    
    this->minutosEstudio = estudioDefectoMS / 60000UL;
    this->minutosPausa = pausaDefectoMS / 60000UL;

    this->animInicio = &animInicio;
    this->animConfig = &animConfig;
    this->animEstudio = &animEstudio;
    this->animPausa = &animPausa;

    this->estadoActual = INICIALIZANDO;
    this->menuEstadoActual = CONFIG_ESTUDIO;
    
    this->lastSuma = 0; this->lastResta = 0; this->lastConfirma = 0;
    this->ultimoRefrescoLCD = 0;
}

void AppController::iniciar() {
    pinMode(pinSuma, INPUT_PULLUP);
    pinMode(pinResta, INPUT_PULLUP);
    pinMode(pinConfirma, INPUT_PULLUP);
    
    this->lcd->init();
    this->lcd->backlight();

    transicionarA(INICIALIZANDO);
}

void AppController::actualizar() {
    this->animador->actualizar();
    bool timerHaTerminado = this->timer->actualizar();

    unsigned long ahora = millis();
    bool btnSuma = (digitalRead(pinSuma) == LOW && ahora - lastSuma > DEBOUNCE_MS_H);
    bool btnResta = (digitalRead(pinResta) == LOW && ahora - lastResta > DEBOUNCE_MS_H);
    bool btnConfirma = (digitalRead(pinConfirma) == LOW && ahora - lastConfirma > DEBOUNCE_MS_H);

    if (btnSuma) lastSuma = ahora;
    if (btnResta) lastResta = ahora;
    if (btnConfirma) lastConfirma = ahora;

    switch (this->estadoActual) {
        case INICIALIZANDO:
            if (!this->animador->isRunning()) {
                transicionarA(CONFIGURACION);
            }
            break;
        case CONFIGURACION:
            gestionarMenuConfiguracion(btnSuma, btnResta, btnConfirma);
            break;
        
        case SESION_ESTUDIO:
            if (timerHaTerminado) {
                transicionarA(SESION_PAUSA);
            } else if (btnConfirma) {
                Serial.println("\n[AppController] Sesión cancelada. Volviendo a Configuración.");
                transicionarA(CONFIGURACION);
            } else {
                gestionarRefrescoLCD();
            }
            break;
        
        case SESION_PAUSA:
            if (timerHaTerminado) {
                transicionarA(SESION_ESTUDIO);
            } else if (btnConfirma) {
                Serial.println("\n[AppController] Sesión cancelada. Volviendo a Configuración.");
                transicionarA(CONFIGURACION);
            } else {
                gestionarRefrescoLCD();
            }
            break;
    }
}

void AppController::transicionarA(AppEstado nuevoEstado) {
    this->estadoActual = nuevoEstado;
    Serial.print("\n[AppController] Transición a: ");
    this->lcd->clear();

    this->ultimoRefrescoLCD = millis();

    switch (nuevoEstado) {
        case INICIALIZANDO:
            Serial.println("INICIALIZANDO");
            this->timer->detener();
            this->animador->play(*this->animInicio, false);
            
            this->lcd->setCursor(0, 0); this->lcd->print("Hola!");
            this->lcd->setCursor(0, 1); this->lcd->print("Soy Studino");
            Serial.println("LCD: Hola!");
            Serial.println("LCD: Soy Studino");
            break;

        case CONFIGURACION:
            menu->actualizar(); 
            if (menu->getEstado() == MenuController::FINALIZADO) {
                this->duracionEstudioMS = (unsigned long)menu->getMinutosEstudio() * 60000UL;
                this->duracionPausaMS = (unsigned long)menu->getMinutosPausa() * 60000UL;
                
                transicionarA(SESION_ESTUDIO);
            }
            break;

        case SESION_ESTUDIO:
            Serial.println("ESTUDIO");
            this->duracionEstudioMS = (unsigned long)this->minutosEstudio * 60000UL;
            
            this->animador->play(*this->animEstudio, true);
            this->timer->iniciar(this->duracionEstudioMS);
            
            imprimirTiempoLCD(this->duracionEstudioMS);
            break;

        case SESION_PAUSA:
            Serial.println("PAUSA");
            this->duracionPausaMS = (unsigned long)this->minutosPausa * 60000UL;

            this->animador->play(*this->animPausa, true);
            this->timer->iniciar(this->duracionPausaMS);
            
            imprimirTiempoLCD(this->duracionPausaMS);
            break;
    }
}

void AppController::gestionarMenuConfiguracion(bool btnSuma, bool btnResta, bool btnConfirma) {
    bool cambioDePantalla = false;

    switch (this->menuEstadoActual) {
        case CONFIG_ESTUDIO:
            if (btnSuma) {
                minutosEstudio = min(minutosEstudio + STEP_MIN_H, MAX_ESTUDIO_MIN_H);
                cambioDePantalla = true;
            }
            if (btnResta) {
                minutosEstudio = max(minutosEstudio - STEP_MIN_H, 5);
                cambioDePantalla = true;
            }
            if (btnConfirma) {
                this->menuEstadoActual = CONFIG_DESCANSO;
                cambioDePantalla = true;
            }
            break;
        case CONFIG_DESCANSO:
            if (btnSuma) {
                minutosPausa = min(minutosPausa + STEP_MIN_H, MAX_PAUSA_MIN_H);
                cambioDePantalla = true;
            }
            if (btnResta) {
                minutosPausa = max(minutosPausa - STEP_MIN_H, 5);
                cambioDePantalla = true;
            }
            if (btnConfirma) {
                this->menuEstadoActual = CONFIRMACION;
                cambioDePantalla = true;
            }
            break;
        case CONFIRMACION:
            if (btnSuma) { // "Si"
                transicionarA(SESION_ESTUDIO);
            }
            if (btnResta) { // "No"
                this->menuEstadoActual = CONFIG_ESTUDIO;
                cambioDePantalla = true;
            }
            break;
    }
    if (cambioDePantalla) {
        imprimirPantallaMenu();
    }
}

void AppController::imprimirPantallaMenu() {
    this->lcd->clear();
    this->lcd->setCursor(0, 0);
    
    switch (this->menuEstadoActual) {
        case CONFIG_ESTUDIO:
            this->lcd->print("Definir estudio ");
            this->lcd->setCursor(0, 1);
            this->lcd->print("   ");
            this->lcd->print(minutosEstudio);
            this->lcd->print(" min");
            
            Serial.println("LCD: Definir estudio ");
            Serial.print("LCD:    "); Serial.print(minutosEstudio); Serial.println(" min");
            break;
        case CONFIG_DESCANSO:
            this->lcd->print("Definir descanso");
            this->lcd->setCursor(0, 1);
            this->lcd->print("   ");
            this->lcd->print(minutosPausa);
            this->lcd->print(" min");
            
            Serial.println("LCD: Definir descanso");
            Serial.print("LCD:    "); Serial.print(minutosPausa); Serial.println(" min");
            break;
        case CONFIRMACION:
            this->lcd->print("Iniciar?");
            this->lcd->setCursor(0, 1);
            this->lcd->print("Si (Ver) No (Roj)");

            Serial.println("LCD: Iniciar?");
            Serial.println("LCD: Si (Ver) No (Roj)");
            break;
    }
}

void AppController::gestionarRefrescoLCD() {
    unsigned long ahora = millis();
    if (ahora - this->ultimoRefrescoLCD >= INTERVALO_REFRESCO_LCD_MS_H) {
        unsigned long msRestantes = this->timer->getTiempoRestanteMS();
        imprimirTiempoLCD(msRestantes);
        this->ultimoRefrescoLCD = ahora;
    }
}
        
void AppController::imprimirTiempoLCD(unsigned long ms) {
    long totalSegundos = (ms + 999) / 1000;
    int minutos = totalSegundos / 60;
    int segundos = totalSegundos % 60;
    
    this->lcd->clear();
    this->lcd->setCursor(0, 0);
    
    if (this->estadoActual == SESION_ESTUDIO) {
        this->lcd->print("Estudiando...");
        Serial.println("LCD: Estudiando...");
    } else {
        this->lcd->print("Descansando...");
        Serial.println("LCD: Descansando...");
    }

    this->lcd->setCursor(0, 1);
    this->lcd->print("Tiempo: ");
    
    if (minutos < 10) this->lcd->print("0");
    this->lcd->print(minutos);
    this->lcd->print(":");
    if (segundos < 10) this->lcd->print("0");
    this->lcd->print(segundos);

    Serial.print("LCD: Tiempo: ");
    if (minutos < 10) Serial.print("0");
    Serial.print(minutos);
    Serial.print(":");
    if (segundos < 10) Serial.print("0");
    Serial.println(segundos);
}