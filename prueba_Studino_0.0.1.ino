#include "LedControl.h"


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

const unsigned long DURACION_ESTUDIO_MS   = 15000UL;
const unsigned long DURACION_PAUSA_MS     = 10000UL;
const unsigned long FRAME_DURATION_MS = 250UL;

LedControl lc = LedControl(12, 11, 10, 1); // DIN, CLK, CS, numDevices
Timer miTimer;
LedAnimator miAnimador(lc, FRAME_DURATION_MS);
AppController app(miTimer, miAnimador, DURACION_ESTUDIO_MS, FRAME_DURATION_MS);

Animation ANIM_INICIO    = { (const byte*)animInicioData, 6 };
Animation ANIM_ENGRANAJE = { (const byte*)animEngranaje, 4 };
Animation ANIM_ESTUDIO = { (const byte*)animEstudio, 16 };
Animation ANIM_PAUSA   = { (const byte*)animPausa, 8 };


void setup() {
    Serial.begin(9600);
    Serial.println("--- Bienvenido a Studino! Tu compañero de estudio :) ---");

    // Inicializar la matriz LED
    lc.shutdown(0,false); 
    lc.setIntensity(0,8);
    lc.clearDisplay(0); 

    app.iniciar();

    delay(3000); //Poner lógica para la configuración

    miAnimador.play(ANIM_INICIO);

    app.iniciarSesionEstudio();
}

void loop() {
    app.actualizar();
}