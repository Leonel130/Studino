#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "LedControl.h"
#include <avr/pgmspace.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
LedControl lc = LedControl(12, 11, 10, 1); // DIN, CLK, CS, numDevices

const int PIN_VERDE = 7; // Suma / "Si"
const int PIN_ROJO  = 6; // Resta / "No"
const int PIN_NEGRO = 5; // Confirmar / Cancelar
const int BUZZER_PIN = 3;
const int PIN_TRIG = 8; // Pin Trigger del sensor
const int PIN_ECHO = 9; // Pin Echo del sensor

unsigned long DURACION_ESTUDIO_MS_DEFECTO = 1 * 60000UL;
unsigned long DURACION_PAUSA_MS_DEFECTO   = 1 * 60000UL;
const unsigned long FRAME_DURATION_MS     = 800UL;

const unsigned int MAX_ESTUDIO_MIN = 60;
const unsigned int MAX_PAUSA_MIN   = 30;
const unsigned int STEP_MIN        = 5;
const unsigned long DEBOUNCE_MS    = 200;

const unsigned long INTERVALO_REFRESCO_LCD_MS = 1000;


struct Animation {
  const byte* data;
  int numFrames;
};


class Timer {
private:
    unsigned long tiempoInicial;
    unsigned long duracion;
    bool activo;
public:
    Timer() {
        this->activo = false;
        this->tiempoInicial = 0;
        this->duracion = 0;
    }
    void iniciar(unsigned long duracionMS) {
        this->duracion = duracionMS;
        this->tiempoInicial = millis();
        this->activo = true;
    }
    void detener() {
        this->activo = false;
    }
    bool actualizar() {
        if (!this->activo) return false;
        unsigned long tiempoTranscurrido = millis() - this->tiempoInicial;
        if (tiempoTranscurrido >= this->duracion) {
            this->activo = false;
            return true;
        }
        return false;
    }
    unsigned long getTiempoRestanteMS() {
        if (!this->activo) { return 0; }
        unsigned long tiempoTranscurrido = millis() - this->tiempoInicial;
        if (tiempoTranscurrido >= this->duracion) { return 0; }
        return this->duracion - tiempoTranscurrido;
    }
};


class LedAnimator {
private:
  LedControl* lc;
  const Animation* currentAnimation;
  int currentFrame;
  bool looping;
  unsigned long frameDurationMS;
  unsigned long lastFrameTime;
  void displayCurrentFrame() {
    if (this->currentAnimation == nullptr) return;
    for (int row = 0; row < 8; row++) {
      byte rowData = pgm_read_byte(&(this->currentAnimation->data[this->currentFrame * 8 + row]));
      this->lc->setRow(0, row, rowData);
    }
  }
public:
  LedAnimator(LedControl& ledControl, unsigned long frameDuration) {
    this->lc = &ledControl;
    this->frameDurationMS = frameDuration;
    this->currentAnimation = nullptr;
    this->currentFrame = 0;
    this->lastFrameTime = 0;
    this->looping = false;
  }
  void play(const Animation& anim) { play(anim, true); }
  void play(const Animation& anim, bool loop) {
    if (this->currentAnimation == &anim) return;
    this->currentAnimation = &anim;
    this->currentFrame = 0;
    this->looping = loop;
    this->lastFrameTime = millis();
    displayCurrentFrame();
  }
  void stop() {
    this->currentAnimation = nullptr;
    this->lc->clearDisplay(0);
  }
  bool isRunning() {
    return (this->currentAnimation != nullptr);
  }
  void actualizar() {
    if (this->currentAnimation == nullptr) return;
    unsigned long ahora = millis();
    if (ahora - this->lastFrameTime >= this->frameDurationMS) {
      this->currentFrame++;
      if (this->currentFrame >= this->currentAnimation->numFrames) {
        if (this->looping) {
          this->currentFrame = 0;
        } else {
          this->currentAnimation = nullptr;
          return;
        }
      }
      displayCurrentFrame();
      this->lastFrameTime = ahora;
    }
  }
};


class AppController {
public:
    enum AppEstado {
        INICIALIZANDO,
        CONFIGURACION,
        SESION_ESTUDIO,
        SESION_PAUSA
    };
    enum MenuEstado {
        CONFIG_ESTUDIO,
        CONFIG_DESCANSO,
        CONFIG_SENSOR,
        CONFIG_DISTANCIA, 
        CONFIRMACION
    };

private:
    AppEstado estadoActual;
    MenuEstado menuEstadoActual;

    Timer* timer;
    LedAnimator* animador;
    LiquidCrystal_I2C* lcd;

    unsigned long duracionEstudioMS;
    unsigned long duracionPausaMS;
    int minutosEstudio;
    int minutosPausa;

    int pinSuma, pinResta, pinConfirma, pinBuzzer;
    int pinTrig, pinEcho;
    unsigned long lastSuma, lastResta, lastConfirma;

    const Animation *animInicio, *animConfig, *animEstudio, *animPausa;

    unsigned long ultimoRefrescoLCD;

    unsigned long pitidoStartTime;
    unsigned long pitidoDuration;

    enum SensorEstado {
        S_IDLE,
        S_TRIG_LOW,
        S_TRIG_HIGH_START,
        S_TRIG_HIGH_END,
        S_WAIT_ECHO_START,
        S_WAIT_ECHO_END
    };
    SensorEstado sensorEstado;
    unsigned long sensorUltimoTiempoMicros;
    unsigned long sensorEchoStartTimeMicros;
    long distanciaActualCm;
    unsigned long sensorUltimaLecturaMs;
    bool sensorActivo;
    unsigned long tiempoAusenteInicioMs;
    int beepsRestantes;
    unsigned long proximoBeepMs;
    bool beepEstaSonando;

    int umbralDistanciaCm; 
    const unsigned long TIEMPO_LIMITE_AUSENTE_MS = 5000;
    const int MIN_DISTANCIA_CM = 30;
    const int MAX_DISTANCIA_CM = 120;
    const int STEP_DISTANCIA_CM = 10;

public:
    AppController(Timer& timer, LedAnimator& animador, LiquidCrystal_I2C& lcd,
                  int pinSuma, int pinResta, int pinConfirma, int pinBuzzer,
                  int pinTrig, int pinEcho,
                  unsigned long estudioDefectoMS, unsigned long pausaDefectoMS,
                  const Animation& animInicio, const Animation& animConfig,
                  const Animation& animEstudio, const Animation& animPausa) {
        
        this->timer = &timer;
        this->animador = &animador;
        this->lcd = &lcd;
        
        this->pinSuma = pinSuma;
        this->pinResta = pinResta;
        this->pinConfirma = pinConfirma;
        this->pinBuzzer = pinBuzzer;
        this->pinTrig = pinTrig;
        this->pinEcho = pinEcho;

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
        this->pitidoStartTime = 0;
        this->pitidoDuration = 0;
        this->sensorEstado = S_IDLE;
        this->distanciaActualCm = -1;
        this->sensorUltimaLecturaMs = 0;
        this->sensorActivo = false;
        this->tiempoAusenteInicioMs = 0;
        this->beepsRestantes = 0;
        this->proximoBeepMs = 0;
        this->beepEstaSonando = false;
        this->umbralDistanciaCm = 60; 
    }

    void iniciar() {
        pinMode(pinSuma, INPUT_PULLUP);
        pinMode(pinResta, INPUT_PULLUP);
        pinMode(pinConfirma, INPUT_PULLUP);
        pinMode(pinBuzzer, OUTPUT); 
        digitalWrite(pinBuzzer, LOW);

        pinMode(this->pinTrig, OUTPUT);
        pinMode(this->pinEcho, INPUT);
        digitalWrite(this->pinTrig, LOW);
        
        this->lcd->init();
        this->lcd->backlight();

        transicionarA(INICIALIZANDO);
    }

    void actualizar() {
        this->animador->actualizar();
        gestionarPitido();       
        gestionarAlarmaBeep();   
        if (this->sensorActivo) { 
          actualizarSensor();
        }
        
        bool timerHaTerminado = this->timer->actualizar();

        unsigned long ahora = millis();
        bool btnSuma = (digitalRead(pinSuma) == LOW && ahora - lastSuma > DEBOUNCE_MS);
        bool btnResta = (digitalRead(pinResta) == LOW && ahora - lastResta > DEBOUNCE_MS);
        bool btnConfirma = (digitalRead(pinConfirma) == LOW && ahora - lastConfirma > DEBOUNCE_MS);

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
                    if (this->sensorActivo) {
                        long dist = this->distanciaActualCm;
                        if (dist > this->umbralDistanciaCm && dist > 0) { 
                            if (this->tiempoAusenteInicioMs == 0) {
                                this->tiempoAusenteInicioMs = millis();
                                Serial.println("[Sensor] Usuario ausente, iniciando conteo...");
                            } else if (millis() - this->tiempoAusenteInicioMs > TIEMPO_LIMITE_AUSENTE_MS) {
                                if (this->beepsRestantes <= 0) {
                                    Serial.println("[Sensor] ¡Alarma disparada!");
                                    this->beepsRestantes = 3; 
                                    this->proximoBeepMs = millis();
                                    this->beepEstaSonando = false;
                                }
                            }
                        } else if (dist <= this->umbralDistanciaCm && dist > 0) { 
                            this->tiempoAusenteInicioMs = 0; 
                        }
                    }
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

    long getDistanciaActualCm() {
        return this->distanciaActualCm;
    }

private:
    void transicionarA(AppEstado nuevoEstado) {
        AppEstado estadoAnterior = this->estadoActual; 
        this->estadoActual = nuevoEstado;
        Serial.print("\n[AppController] Transición a: ");
        this->lcd->clear();

        this->ultimoRefrescoLCD = millis(); 

        if (estadoAnterior == SESION_ESTUDIO) {
            this->beepsRestantes = 0;
            digitalWrite(this->pinBuzzer, LOW);
            this->beepEstaSonando = false;
            this->tiempoAusenteInicioMs = 0; 
        }

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
                Serial.println("CONFIGURACION");
                this->timer->detener();
                this->animador->play(*this->animConfig, true);
                
                this->menuEstadoActual = CONFIG_ESTUDIO;
                imprimirPantallaMenu();
                break;

            case SESION_ESTUDIO:
                Serial.println("ESTUDIO");
                this->duracionEstudioMS = (unsigned long)this->minutosEstudio * 60000UL;
                
                if (estadoAnterior == SESION_PAUSA) {
                    Serial.println("[Buzzer] Fin de descanso (Beep Beep)");
                    this->beepsRestantes = 2; // Disparar 2 beeps
                    this->proximoBeepMs = millis();
                    this->beepEstaSonando = false;
                } else {
                    iniciarPitido(300);
                }

                this->animador->play(*this->animEstudio, true);
                this->timer->iniciar(this->duracionEstudioMS);
                
                imprimirTiempoLCD(this->duracionEstudioMS);
                break;

            case SESION_PAUSA:
                Serial.println("PAUSA");
                this->duracionPausaMS = (unsigned long)this->minutosPausa * 60000UL;

                iniciarPitido(600); 

                this->animador->play(*this->animPausa, true);
                this->timer->iniciar(this->duracionPausaMS);
                
                imprimirTiempoLCD(this->duracionPausaMS);
                break;
        }
    }

    void iniciarPitido(unsigned long duration) {
        this->pitidoDuration = duration;
        this->pitidoStartTime = millis();
        digitalWrite(this->pinBuzzer, HIGH);
    }

    void gestionarPitido() {
        if (this->pitidoStartTime != 0) {
            if (millis() - this->pitidoStartTime >= this->pitidoDuration) {
                digitalWrite(this->pinBuzzer, LOW);
                this->pitidoStartTime = 0; 
            }
        }
    }
    
    void gestionarAlarmaBeep() {
        if (this->beepsRestantes <= 0) return; 

        unsigned long ahora = millis();
        if (ahora >= this->proximoBeepMs) {
            if (this->beepEstaSonando) {
                digitalWrite(this->pinBuzzer, LOW);
                this->beepEstaSonando = false;
                this->beepsRestantes--; 
                if (this->beepsRestantes > 0) {
                    this->proximoBeepMs = ahora + 200; // Pausa
                }
            } else {
                digitalWrite(this->pinBuzzer, HIGH);
                this->beepEstaSonando = true;
                this->proximoBeepMs = ahora + 250; // Duración
            }
        }
    }

    void gestionarMenuConfiguracion(bool btnSuma, bool btnResta, bool btnConfirma) {
        bool cambioDePantalla = false;

        switch (this->menuEstadoActual) {
            case CONFIG_ESTUDIO:
                if (btnSuma) {
                    minutosEstudio = min(minutosEstudio + STEP_MIN, MAX_ESTUDIO_MIN);
                    cambioDePantalla = true;
                }
                if (btnResta) {
                    minutosEstudio = max(minutosEstudio - STEP_MIN, 5);
                    cambioDePantalla = true;
                }
                if (btnConfirma) {
                    this->menuEstadoActual = CONFIG_DESCANSO; 
                    cambioDePantalla = true;
                }
                break;
            case CONFIG_DESCANSO:
                if (btnSuma) {
                    minutosPausa = min(minutosPausa + STEP_MIN, MAX_PAUSA_MIN);
                    cambioDePantalla = true;
                }
                if (btnResta) {
                    minutosPausa = max(minutosPausa - STEP_MIN, 5);
                    cambioDePantalla = true;
                }
                if (btnConfirma) {
                    this->menuEstadoActual = CONFIG_SENSOR; 
                    cambioDePantalla = true;
                }
                break;

            case CONFIG_SENSOR:
                if (btnSuma) { // "Si"
                    this->sensorActivo = true;
                    this->menuEstadoActual = CONFIG_DISTANCIA; 
                    cambioDePantalla = true;
                }
                if (btnResta) { // "No"
                    this->sensorActivo = false;
                    this->umbralDistanciaCm = 60; 
                    this->menuEstadoActual = CONFIRMACION; 
                    cambioDePantalla = true;
                }
                break;
            
            case CONFIG_DISTANCIA:
                if (btnSuma) { 
                    this->umbralDistanciaCm = min(this->umbralDistanciaCm + STEP_DISTANCIA_CM, MAX_DISTANCIA_CM);
                    cambioDePantalla = true;
                }
                if (btnResta) { 
                    this->umbralDistanciaCm = max(this->umbralDistanciaCm - STEP_DISTANCIA_CM, MIN_DISTANCIA_CM);
                    cambioDePantalla = true;
                }
                if (btnConfirma) { // OK
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

    void imprimirPantallaMenu() {
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

            case CONFIG_SENSOR:
                this->lcd->print("Usar sensor?");
                this->lcd->setCursor(0, 1);
                this->lcd->print(" Si    No"); 
                Serial.println("LCD: Usar sensor?");
                Serial.println("LCD: Si (Ver) No (Roj)");
                break;
            
            case CONFIG_DISTANCIA:
                this->lcd->print("Dist. Alarma:");
                this->lcd->setCursor(0, 1);
                this->lcd->print("   ");
                this->lcd->print(this->umbralDistanciaCm);
                this->lcd->print(" cm");
                Serial.println("LCD: Dist. Alarma:");
                Serial.print("LCD:    "); Serial.print(this->umbralDistanciaCm); Serial.println(" cm");
                break;
            
            case CONFIRMACION:
                this->lcd->print("Iniciar?");
                this->lcd->setCursor(0, 1);
                this->lcd->print(" Si    No"); 
                Serial.println("LCD: Iniciar?");
                Serial.println("LCD: Si (Ver) No (Roj)");
                break;
        }
    }

    void gestionarRefrescoLCD() {
        unsigned long ahora = millis();
        if (ahora - this->ultimoRefrescoLCD >= INTERVALO_REFRESCO_LCD_MS) {
            unsigned long msRestantes = this->timer->getTiempoRestanteMS();
            imprimirTiempoLCD(msRestantes);
            this->ultimoRefrescoLCD = ahora;
        }
    }
    void imprimirTiempoLCD(unsigned long ms) {
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

    void actualizarSensor() {
        unsigned long ahoraMicros = micros();
        unsigned long ahoraMs = millis();

        if (this->sensorEstado == S_IDLE && (ahoraMs - this->sensorUltimaLecturaMs > 500)) {
            this->sensorEstado = S_TRIG_LOW;
            this->sensorUltimoTiempoMicros = ahoraMicros;
            digitalWrite(this->pinTrig, LOW);
        }

        switch (this->sensorEstado) {
            case S_TRIG_LOW:
                if (ahoraMicros - this->sensorUltimoTiempoMicros >= 2) {
                    this->sensorEstado = S_TRIG_HIGH_START;
                    this->sensorUltimoTiempoMicros = ahoraMicros;
                    digitalWrite(this->pinTrig, HIGH);
                }
                break;
            case S_TRIG_HIGH_START:
                if (ahoraMicros - this->sensorUltimoTiempoMicros >= 10) {
                    this->sensorEstado = S_TRIG_HIGH_END;
                    digitalWrite(this->pinTrig, LOW);
                }
                break;
            case S_TRIG_HIGH_END:
                this->sensorEstado = S_WAIT_ECHO_START;
                this->sensorUltimoTiempoMicros = ahoraMicros; 
                break;
            case S_WAIT_ECHO_START:
                if (digitalRead(this->pinEcho) == HIGH) {
                    this->sensorEstado = S_WAIT_ECHO_END;
                    this->sensorEchoStartTimeMicros = ahoraMicros;
                }
                else if (ahoraMicros - this->sensorUltimoTiempoMicros > 30000) { 
                    this->sensorEstado = S_IDLE;
                    this->distanciaActualCm = -1; // Timeout
                    this->sensorUltimaLecturaMs = ahoraMs;
                }
                break;
            case S_WAIT_ECHO_END:
                if (digitalRead(this->pinEcho) == LOW) {
                    unsigned long duracionEcoMicros = ahoraMicros - this->sensorEchoStartTimeMicros;
                    this->distanciaActualCm = duracionEcoMicros / 58;
                    this->sensorEstado = S_IDLE;
                    this->sensorUltimaLecturaMs = ahoraMs;
                }
                else if (ahoraMicros - this->sensorEchoStartTimeMicros > 25000) {
                    this->sensorEstado = S_IDLE;
                    this->distanciaActualCm = -1; // Timeout
                    this->sensorUltimaLecturaMs = ahoraMs;
                }
                break;
            case S_IDLE:
                break;
        }
    }
};


const byte animInicioData[5][8] PROGMEM = {
    {B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000},
    {B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B11111111,B11111111},
    {B00000000,B00000000,B00000000,B00000000,B11111111,B11111111,B11111111,B11111111},
    {B00000000,B00000000,B00000000,B11111111,B11111111,B11111111,B11111111,B11111111},
    {B11111111,B11111111,B11111111,B11111111,B11111111,B11111111,B11111111,B11111111}
};
const byte animEngranajeData[4][8] PROGMEM = {
    {B00000000,B01011010,B00100100,B01000010,B01000010,B00100100,B01011010,B00000000},
    {B00001000,B00111100,B01100110,B11000010,B01000011,B01100110,B00111100,B00010000},
    {B0010000,B00111100,B01100110,B01000011,B11000010,B01100110,B00111100,B00001000},
    {B00100000,B00111100,B01100111,B01000010,B01000010,B11100110,B00111100,B00000100}
};
const byte animEstudioData[16][8] PROGMEM = {
    {B00000000,B00000000,B01000010,B00000000,B01000010,B01111110,B00001100,B00000000},
    {B00000000,B00000000,B01000010,B00000000,B01000010,B01111110,B00000000,B00000000},
    {B00000000,B00000000,B01000010,B00000000,B01000010,B01111110,B00000000,B00000000},
    {B00000000,B00000000,B01000010,B00000000,B00000000,B01111110,B00000000,B00000000},
    {B00000000,B00000000,B01000100,B00000000,B00000010,B00111110,B00000000,B00000000},
    {B00000000,B00000000,B01000100,B00000000,B00000000,B00011100,B00000000,B00000000},
    {B00000000,B00000000,B01000100,B00000000,B00000000,B00011100,B00000000,B00000000},
    {B00000000,B00000000,B01000100,B00000000,B00000000,B00011100,B00000000,B00000000},
    {B00000000,B00000000,B01000100,B00000000,B00000000,B00011100,B00000100,B00000000},

    {B00000000,B00000000,B01000100,B00000000,B00000000,B00011100,B00000000,B00000000},
    {B00000000,B00000000,B01000100,B00000000,B00000000,B00011100,B00000000,B00000000},
    {B00000000,B00000000,B01000100,B00000000,B00000000,B00111110,B00000000,B00000000},
    {B00000000,B00000000,B01000010,B00000000,B00000000,B01111110,B00000000,B00000000},
    {B00000000,B00000000,B01000010,B00000000,B01000010,B01111110,B00000000,B00000000},
    {B00000000,B00000000,B01000010,B00000000,B01000010,B01111110,B00000000,B00000000},
    {B00000000,B00000000,B01000010,B00000000,B01000010,B01111110,B00000000,B00000000}
};
const byte animPausaData[8][8] PROGMEM = {
    {B00000000,B01111110,B01111110,B00111100,B00011000,B00100100,B01000010,B01111110},
    {B00000000,B01111110,B01111010,B00111100,B00011000,B00100100,B01000110,B01111110},
    {B00000000,B01111110,B01110010,B00111100,B00011000,B00100100,B01010110,B01111110},
    {B00000000,B01111110,B01100010,B00101100,B00011000,B00100100,B01111110,B01111110},
    {B00000000,B01111110,B01000010,B00101100,B00011000,B00101100,B01111110,B01111110},
    {B00000000,B01111110,B01000010,B00100100,B00011000,B00111100,B01111110,B01111110},
    {B00000000,B01100011,B01010101,B01001001,B01001001,B01010101,B01100011,B00000000},
    {B00000000,B01111110,B01111110,B00111100,B00011000,B00100100,B01000010,B01111110}
};


Timer miTimer;
LedAnimator miAnimador(lc, FRAME_DURATION_MS);

Animation ANIM_INICIO    = { (const byte*)animInicioData, 5 };
Animation ANIM_ENGRANAJE = { (const byte*)animEngranajeData, 4 };
Animation ANIM_ESTUDIO   = { (const byte*)animEstudioData, 16 };
Animation ANIM_PAUSA     = { (const byte*)animPausaData, 8 };

AppController app(miTimer, miAnimador, lcd,
                  PIN_VERDE, PIN_ROJO, PIN_NEGRO, BUZZER_PIN,
                  PIN_TRIG, PIN_ECHO,
                  DURACION_ESTUDIO_MS_DEFECTO, DURACION_PAUSA_MS_DEFECTO,
                  ANIM_INICIO, ANIM_ENGRANAJE, ANIM_ESTUDIO, ANIM_PAUSA);


void setup() {
    Serial.begin(9600);
    Serial.println("--- Buenvenido a Studino! ---");

    lc.shutdown(0,false); 
    lc.setIntensity(0,8);
    lc.clearDisplay(0); 

    app.iniciar();
}

void loop() {
    app.actualizar();
}