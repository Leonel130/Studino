#include "LedControl.h"
class Timer {
public:
    enum Estado {
        ESTUDIO,
        PAUSA
    };

    Timer(unsigned long duracionEstudio, unsigned long duracionPausa, unsigned long intervaloRefresco) {
        this->duracionEstudioMS = duracionEstudio;
        this->duracionPausaMS = duracionPausa;
        this->intervaloRefrescoMS = intervaloRefresco;

        this->estadoActual = ESTUDIO; // Siempre empezamos con estudio
        this->activo = false;
        this->tiempoInicialFase = 0;
        this->ultimoRefresco = 0;
    }

    void iniciar() { 
        this->activo = true;
        this->tiempoInicialFase = millis();
        this->ultimoRefresco = this->tiempoInicialFase;
        
        Serial.println("\n*** ¡COMIENZA EL ESTUDIO! ***");
        imprimirTiempoRestante(duracionEstudioMS); // Es redundante, pero es para que el timer se muestre inmediatamente
    }

    void actualizar() {
        if (!this->activo) return;

        unsigned long duracionActualMS = (this->estadoActual == ESTUDIO) ? this->duracionEstudioMS : this->duracionPausaMS;
        unsigned long ahora = millis();
        unsigned long tiempoTranscurrido = ahora - this->tiempoInicialFase;

        if (tiempoTranscurrido >= duracionActualMS) {
            Serial.println("--- ¡TEMPORIZADOR FINALIZADO! ---");
            this->cambiarFase();
            return; 
        }

        if (ahora - this->ultimoRefresco >= this->intervaloRefrescoMS) {
            unsigned long tiempoRestante = duracionActualMS - tiempoTranscurrido;
            imprimirTiempoRestante(tiempoRestante);
            this->ultimoRefresco = ahora;
            // Llamar a la actualización de la pantalla LED acá
        }
    }

    void detener(){
      this->activo = false;
    }

    Estado getEstado() {
      return this->estadoActual;
    }

private:
    Estado estadoActual;
    bool activo;
    unsigned long duracionEstudioMS;
    unsigned long duracionPausaMS;
    unsigned long intervaloRefrescoMS;

    unsigned long tiempoInicialFase;
    unsigned long ultimoRefresco;

    void cambiarFase() {
        if (this->estadoActual == ESTUDIO) {
            this->estadoActual = PAUSA;
            Serial.println("\n*** Transición: ¡COMIENZA LA PAUSA! ***");
        } else {
            this->estadoActual = ESTUDIO;
            Serial.println("\n*** Transición: ¡COMIENZA EL ESTUDIO! ***");
        }

        this->tiempoInicialFase = millis();
        this->ultimoRefresco = this->tiempoInicialFase;

        unsigned long duracionNuevaFase = (this->estadoActual == ESTUDIO) ? this->duracionEstudioMS : this->duracionPausaMS;
        imprimirTiempoRestante(duracionNuevaFase); // Es redundante, pero es para que el timer se muestre inmediatamente
    }

    void imprimirTiempoRestante(unsigned long ms) {
        long totalSegundos = ms / 1000;
        int horas = totalSegundos / 3600;
        totalSegundos %= 3600;
        int minutos = totalSegundos / 60;
        int segundos = totalSegundos % 60;

        Serial.print("Tiempo restante (");
        Serial.print((this->estadoActual == ESTUDIO) ? "ESTUDIO" : "PAUSA");
        Serial.print("): ");

        if (horas > 0) {
            Serial.print(horas);
            Serial.print(":");
            if (minutos < 10) Serial.print("0");
        }

        Serial.print(minutos);
        Serial.print(":");

        if (segundos < 10) Serial.print("0");
        Serial.println(segundos);
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
  unsigned long frameDurationMS;
  unsigned long lastFrameTime;

public:
  LedAnimator(LedControl& ledControl, unsigned long frameDuration) {
    this->lc = &ledControl;
    this->frameDurationMS = frameDuration;
    this->currentAnimation = nullptr;
    this->currentFrame = 0;
    this->lastFrameTime = 0;
  }

  void play(const Animation& anim) {
    // Evita reiniciar la animación si ya se está reproduciendo
    if (this->currentAnimation == &anim) return; 
    
    this->currentAnimation = &anim;
    this->currentFrame = 0;
    this->lastFrameTime = millis();
    displayCurrentFrame();
  }

  void stop() {
    this->currentAnimation = nullptr;
    this->lc->clearDisplay(0);
  }

  void actualizar() {
    if (this->currentAnimation == nullptr) return;
    unsigned long ahora = millis();
    if (ahora - this->lastFrameTime >= this->frameDurationMS) {
      this->currentFrame++;
      if (this->currentFrame >= this->currentAnimation->numFrames) {
        this->currentFrame = 0;
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


const unsigned long DURACION_ESTUDIO_MS   = 15000UL;
const unsigned long DURACION_PAUSA_MS     = 10000UL;
const unsigned long INTERVALO_REFRESCO_MS = 1000UL;
Timer miTimer(DURACION_ESTUDIO_MS, DURACION_PAUSA_MS, INTERVALO_REFRESCO_MS);

//Pines                   DIN, CLK, CS, numDevices
LedControl lc = LedControl(12, 11, 10, 1);
const unsigned long FRAME_DURATION_MS = 500UL;
LedAnimator miAnimador(lc, FRAME_DURATION_MS);
byte animInicio[6][8] = {
    {B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000},
    {B00000000,B01100000,B01000000,B01100000,B01000000,B01000000,B00000000,B00000000},
    {B00000000,B01100000,B01000000,B01100000,B01000000,B01000000,B00000000,B00000000},
    {B00000000,B01100010,B01000010,B01100010,B01000010,B01000010,B00000000,B00000000},
    {B00000000,B01100010,B01000010,B01100010,B01000010,B01000010,B00000000,B00000000},
    {B00000000,B01100010,B01000010,B01100010,B01000010,B01000010,B00000000,B00000000}
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

Animation ANIM_INICIO = { (const byte*)animInicio, 6 };
Animation ANIM_ESTUDIO = { (const byte*)animEstudio, 16 };
Animation ANIM_PAUSA   = { (const byte*)animPausa, 7 };


void setup() {
    Serial.begin(9600);
    Serial.println("--- Bienvenido a Studino! Tu compañero de estudio :) ---");

    // Inicializar la matriz LED
    lc.shutdown(0,false); 
    lc.setIntensity(0,8);
    lc.clearDisplay(0); 

    miAnimador.play(ANIM_INICIO);
    miTimer.iniciar(); 
}

void loop() {
    miTimer.actualizar();

    miAnimador.actualizar();

    if (miTimer.getEstado() == Timer::ESTUDIO) {
        miAnimador.play(ANIM_ESTUDIO);
    } else {
        miAnimador.play(ANIM_PAUSA);
    }
}