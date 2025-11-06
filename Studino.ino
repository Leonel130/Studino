#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "LedControl.h"

LiquidCrystal_I2C lcd(0x27, 16, 2);
LedControl lc = LedControl(12, 11, 10, 1); // DIN, CLK, CS, numDevices

const int PIN_VERDE = 7; // Suma / "Si"
const int PIN_ROJO  = 6; // Resta / "No"
const int PIN_NEGRO = 5; // Confirmar / Cancelar
const int BUZZER_PIN = 3;

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
        if (!this->activo) {
            return 0;
        }
        unsigned long tiempoTranscurrido = millis() - this->tiempoInicial;
        if (tiempoTranscurrido >= this->duracion) {
            return 0;
        }
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
      byte rowData = this->currentAnimation->data[this->currentFrame * 8 + row];
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
    unsigned long lastSuma, lastResta, lastConfirma;

    const Animation *animInicio, *animConfig, *animEstudio, *animPausa;

    unsigned long ultimoRefrescoLCD;

    unsigned long pitidoStartTime;
    unsigned long pitidoDuration;

public:
    AppController(Timer& timer, LedAnimator& animador, LiquidCrystal_I2C& lcd,
                  int pinSuma, int pinResta, int pinConfirma, int pinBuzzer,
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
    }

    void iniciar() {
        pinMode(pinSuma, INPUT_PULLUP);
        pinMode(pinResta, INPUT_PULLUP);
        pinMode(pinConfirma, INPUT_PULLUP);
        pinMode(pinBuzzer, OUTPUT); 
        digitalWrite(pinBuzzer, LOW);
        
        this->lcd->init();
        this->lcd->backlight();

        transicionarA(INICIALIZANDO);
    }

    void actualizar() {
        this->animador->actualizar();
        gestionarPitido();
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

private:
    void transicionarA(AppEstado nuevoEstado) {
        this->estadoActual = nuevoEstado;
        Serial.print("\n[AppController] Transición a: ");
        this->lcd->clear();

        this->ultimoRefrescoLCD = millis(); // Reiniciar reloj

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
                
                iniciarPitido(300);

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
            case CONFIRMACION:
                this->lcd->print("Iniciar?");
                this->lcd->setCursor(0, 1);
                this->lcd->print("Si (Ver) No (Roj)");

                Serial.println("LCD: Iniciar?");
                Serial.println("LCD: Si    No");
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
};


byte animInicioData[5][8] = {
    {B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000},
    {B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B11111111,B11111111},
    {B00000000,B00000000,B00000000,B00000000,B11111111,B11111111,B11111111,B11111111},
    {B00000000,B00000000,B00000000,B11111111,B11111111,B11111111,B11111111,B11111111},
    {B11111111,B11111111,B11111111,B11111111,B11111111,B11111111,B11111111,B11111111}
};
byte animEngranajeData[4][8] = {
    {B00000000,B01011010,B00100100,B01000010,B01000010,B00100100,B01011010,B00000000},
    {B00001000,B00111100,B01100110,B11000010,B01000011,B01100110,B00111100,B00010000},
    {B0010000,B00111100,B01100110,B01000011,B11000010,B01100110,B00111100,B00001000},
    {B00100000,B00111100,B01100111,B01000010,B01000010,B11100110,B00111100,B00000100}
};
byte animEstudioData[16][8] = {
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
byte animPausaData[8][8] = {
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
