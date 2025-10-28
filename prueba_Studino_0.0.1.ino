#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "LedControl.h"

LiquidCrystal_I2C lcd(0x27, 16, 2)

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

    // Devuelve 'true' SI TERMINÓ.
    bool actualizar() {
        if (!this->activo) return false;

        unsigned long tiempoTranscurrido = millis() - this->tiempoInicial;

        if (tiempoTranscurrido >= this->duracion) {
            this->activo = false;
            return true;
        }
        
        return false;
    }
};

const unsigned long DURACION_ESTUDIO_MS   = 15000UL;
const unsigned long DURACION_PAUSA_MS     = 10000UL;
const unsigned long FRAME_DURATION_MS = 250UL;
Timer miTimer;


struct Animation {
    const byte* data;
    int numFrames;   
};


class LedAnimator {
private:
  LedControl* lc;
  
  const Animation* currentAnimation;
  int currentFrame;
  bool looping;
  
  unsigned long frameDurationMS;
  unsigned long lastFrameTime;

public:
  LedAnimator(LedControl& ledControl, unsigned long frameDuration) {
    this->lc = &ledControl;
    this->frameDurationMS = frameDuration;
    this->currentAnimation = nullptr;
    this->currentFrame = 0;
    this->lastFrameTime = 0;
    this->looping = false;
  }

  void play(const Animation& anim) {
    play(anim, true);
  }

  // Permite elegir si loopea
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

private:
  void displayCurrentFrame() {
    if (this->currentAnimation == nullptr) return;
    for (int row = 0; row < 8; row++) {
      byte rowData = this->currentAnimation->data[this->currentFrame * 8 + row];
      this->lc->setRow(0, row, rowData);
    }
  }
};

    void setDuraciones(unsigned long nuevaDuracionEstudio, unsigned long nuevaDuracionPausa) {
        this->duracionEstudioMS = nuevaDuracionEstudio;
        this->duracionPausaMS   = nuevaDuracionPausa;

        // Reinicia el timer a la nueva duración si querés:
        this->tiempoInicialFase = millis();
        this->ultimoRefresco = millis();
        this->estadoActual = ESTUDIO; // opcional: comenzar siempre en estudio
    }


byte animInicioData[6][8] = {
    {B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000},
    {B00000000,B01100000,B01000000,B01100000,B01000000,B01000000,B00000000,B00000000},
    {B00000000,B01100000,B01000000,B01100000,B01000000,B01000000,B00000000,B00000000},
    {B00000000,B01100010,B01000010,B01100010,B01000010,B01000010,B00000000,B00000000},
    {B00000000,B01100010,B01000010,B01100010,B01000010,B01000010,B00000000,B00000000},
    {B00000000,B01100010,B01000010,B01100010,B01000010,B01000010,B00000000,B00000000}
};
byte animEngranaje[4][8] = {
    {B00111100,B01100110,B11000011,B10011001,B10011001,B11000011,B01100110,B00111100},
    {B00011000,B01011010,B01100110,B10011001,B01100110,B01011010,B00011000,B00000000},
    {B00111100,B01100110,B11000011,B10011001,B10011001,B11000011,B01100110,B00111100},
    {B00011000,B01011010,B01100110,B10011001,B01100110,B01011010,B00011000,B00000000}
};
byte animEstudio[16][8] = {
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
byte animPausa[8][8] = {
    {B01111110,B01000010,B01011010,B01011010,B01011010,B01000010,B01111110,B00000000},
    {B01111110,B01000010,B01010010,B01011010,B01011010,B01000010,B01111110,B00000000},
    {B01111110,B01000010,B01000010,B01011010,B01110010,B01000010,B01111110,B00000000},
    {B01111110,B01000010,B01000010,B01010010,B01101010,B01000010,B01111110,B00000000},
    {B01111110,B01000010,B01000010,B01000010,B01111010,B01000010,B01111110,B00000000},
    {B01111110,B01000010,B01000010,B01000010,B01111110,B01000010,B01111110,B00000000},
    {B01111110,B00100100,B00011000,B00011000,B00011000,B00100100,B01111110,B00000000},
    {B01111110,B01000010,B01111110,B01111110,B01000010,B01000010,B01111110,B00000000}
};

LedControl lc = LedControl(12, 11, 10, 1); // DIN, CLK, CS, numDevices
LedAnimator miAnimador(lc, FRAME_DURATION_MS);
Animation ANIM_INICIO    = { (const byte*)animInicioData, 6 };
Animation ANIM_ENGRANAJE = { (const byte*)animEngranaje, 4 };
Animation ANIM_ESTUDIO = { (const byte*)animEstudio, 16 };
Animation ANIM_PAUSA   = { (const byte*)animPausa, 8 };


class AppController {
public:
    enum AppEstado {
        INICIALIZANDO,
        CONFIGURACION,
        ESTUDIO,
        PAUSA
    };

private:
    AppEstado estadoActual;

    Timer* timer;
    LedAnimator* animador;

    unsigned long duracionEstudioMS;
    unsigned long duracionPausaMS;

public:
    AppController(Timer& timer, LedAnimator& animador, unsigned long estudioMS, unsigned long pausaMS) {
        this->timer = &timer;
        this->animador = &animador;
        this->duracionEstudioMS = estudioMS;
        this->duracionPausaMS = pausaMS;
        
        this->estadoActual = INICIALIZANDO; // Estado inicial por defecto
    }

    void iniciar() {
        transicionarA(INICIALIZANDO);
    }
    
    void iniciarSesionEstudio() {
        if (this->estadoActual == CONFIGURACION) {
            transicionarA(ESTUDIO);
        }
    }

    void actualizar() {
        this->animador->actualizar();
        bool timerHaTerminado = this->timer->actualizar();

        switch (this->estadoActual) {
            case INICIALIZANDO:
                if (!this->animador->isRunning()) {
                    transicionarA(CONFIGURACION);
                }
                break;
            
            case CONFIGURACION:
                break;

            case ESTUDIO:
                if (timerHaTerminado) {
                    transicionarA(PAUSA);
                }
                break;

            case PAUSA:
                if (timerHaTerminado) {
                    transicionarA(ESTUDIO);
                }
                break;
        }
    }

private:
    void transicionarA(AppEstado nuevoEstado) {
        this->estadoActual = nuevoEstado;
        Serial.print("\n[AppController] Transición a: ");

        switch (nuevoEstado) {
            case INICIALIZANDO:
                Serial.println("INICIALIZANDO");
                this->timer->detener();
                this->animador->play(ANIM_INICIO, false); // false = NO loop
                break;
            
            case CONFIGURACION:
                Serial.println("CONFIGURACION");
                this->timer->detener();
                this->animador->play(ANIM_ENGRANAJE, true);
                break;

            case ESTUDIO:
                Serial.println("ESTUDIO");
                this->animador->play(ANIM_ESTUDIO, true);
                this->timer->iniciar(this->duracionEstudioMS);
                break;

            case PAUSA:
                Serial.println("PAUSA");
                this->animador->play(ANIM_PAUSA, true);
                this->timer->iniciar(this->duracionPausaMS);
                break;
        }
    }
};

unsigned long DURACION_ESTUDIO_MS   =  25 * 60000UL; // 25 min por defecto
unsigned long DURACION_PAUSA_MS     = 5 * 60000UL; // 5 min por defecto
const unsigned long INTERVALO_REFRESCO_MS = 1000UL;  // 1 seg

int minutosEstudio = DURACION_ESTUDIO_MS / 60000UL; // 0 si < 1 min
int minutosPausa   = DURACION_PAUSA_MS / 60000UL;

const unsigned int MAX_ESTUDIO_MIN = 60;
const unsigned int MAX_PAUSA_MIN   = 30;
const unsigned int STEP_MIN        = 5; // -5 / +5 min

const int PIN_VERDE = 7;  // Suma
const int PIN_ROJO  = 6;  // Resta
const int PIN_NEGRO = 5;  // Confirmar


AppController app(miTimer, miAnimador, DURACION_ESTUDIO_MS, FRAME_DURATION_MS);


enum MenuState { 
  SALUDO, 
  CONFIG_ESTUDIO, 
  CONFIG_DESCANSO, 
  CONFIRMACION, 
  FINALIZADO 
};

const unsigned long DEBOUNCE_MS = 200; // tiempo mínimo entre lecturas
unsigned long lastVerde = 0;
unsigned long lastRojo  = 0;
unsigned long lastNegro = 0;

void menuInicio() {
  MenuState estado = SALUDO;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Hola!");
  lcd.setCursor(0, 1);
  lcd.print("Soy Studino");
  unsigned long inicioSaludo = millis();
  while (millis() - inicioSaludo < 2500) {}
  lcd.clear();

  estado = CONFIG_ESTUDIO;

  while (estado != FINALIZADO) {
    unsigned long ahora = millis();

    switch (estado) {

      // --- CONFIGURAR ESTUDIO ---
      case CONFIG_ESTUDIO:
        lcd.setCursor(0, 0);
        lcd.print("Definir estudio ");
        lcd.setCursor(0, 1);
        lcd.print("   ");
        lcd.print(minutosEstudio);
        lcd.print(" min   ");
        
        if (digitalRead(PIN_VERDE) == LOW && ahora - lastVerde > DEBOUNCE_MS) {
          minutosEstudio = min(minutosEstudio + STEP_MIN, MAX_ESTUDIO_MIN);
          lastVerde = ahora;
        }
        if (digitalRead(PIN_ROJO) == LOW && ahora - lastRojo > DEBOUNCE_MS) {
          minutosEstudio = max(minutosEstudio - STEP_MIN, 5); // mínimo 5 min
          lastRojo = ahora;
        }
        if (digitalRead(PIN_NEGRO) == LOW && ahora - lastNegro > DEBOUNCE_MS) {
          lcd.clear();
          estado = CONFIG_DESCANSO;
          lastNegro = ahora;
        }
        break;

      // --- CONFIGURAR DESCANSO ---
      case CONFIG_DESCANSO:
        lcd.setCursor(0, 0);
        lcd.print("Definir descanso");
        lcd.setCursor(0, 1);
        lcd.print("   ");
        lcd.print(minutosPausa);
        lcd.print(" min   ");

        if (digitalRead(PIN_VERDE) == LOW && ahora - lastVerde > DEBOUNCE_MS) {
          minutosPausa = min(minutosPausa + STEP_MIN, 60);
          lastVerde = ahora;
        }
        if (digitalRead(PIN_ROJO) == LOW && ahora - lastRojo > DEBOUNCE_MS) {
          minutosPausa = max(minutosPausa - STEP_MIN, 5); // mínimo 5 min
          lastRojo = ahora;
        }
        if (digitalRead(PIN_NEGRO) == LOW && ahora - lastNegro > DEBOUNCE_MS) {
          lcd.clear();
          estado = CONFIRMACION;
          lastNegro = ahora;
        }
        break;

      // --- CONFIRMAR ---
      case CONFIRMACION:
        lcd.setCursor(0, 0);
        lcd.print("Iniciar?");
        lcd.setCursor(0, 1);
        lcd.print("Si    No");

        if (digitalRead(PIN_VERDE) == LOW && ahora - lastVerde > DEBOUNCE_MS) {
          estado = FINALIZADO;
          lastVerde = ahora;
        }
        if (digitalRead(PIN_ROJO) == LOW && ahora - lastRojo > DEBOUNCE_MS) {
          lcd.clear();
          estado = CONFIG_ESTUDIO; // volver atrás
          lastRojo = ahora;
        }
        break;

      default:
        break;
    }
  }

  // Cuando termina el menú, convertimos minutos → milisegundos
  DURACION_ESTUDIO_MS = (unsigned long) minutosEstudio * 60000UL;
  DURACION_PAUSA_MS   = (unsigned long) minutosPausa   * 60000UL;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Listo!");
  lcd.setCursor(0, 1);
  lcd.print("Comenzando...");
  unsigned long inicioListo = millis();
  while (millis() - inicioListo < 1500) {}
}


void setup() {
    Serial.begin(9600);
    lcd.init();
    lcd.backlight();
    
    Serial.println("--- Bienvenido a Studino! Tu compañero de estudio :) ---");

    pinMode(PIN_VERDE, INPUT_PULLUP);
    pinMode(PIN_ROJO, INPUT_PULLUP);
    pinMode(PIN_NEGRO, INPUT_PULLUP);


    // Inicializar la matriz LED
    lc.shutdown(0,false); 
    lc.setIntensity(0,8);
    lc.clearDisplay(0); 

    app.iniciar();

    menuInicio(); // Menú para ajustar los tiempos

    // Convertir minutos a ms y pasar al timer
    miTimer.setDuraciones(DURACION_ESTUDIO_MS, DURACION_PAUSA_MS);
    miTimer.iniciar();
    miAnimador.play(ANIM_INICIO);

    app.iniciarSesionEstudio();
}

void loop() {
    app.actualizar();
}