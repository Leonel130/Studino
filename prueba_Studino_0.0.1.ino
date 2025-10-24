#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

class PomodoroTimer {

  public:
    enum Estado {
        ESTUDIO,
        PAUSA
    };

    PomodoroTimer(unsigned long duracionEstudio, unsigned long duracionPausa, unsigned long intervaloRefresco) {
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

    void setDuraciones(unsigned long nuevaDuracionEstudio, unsigned long nuevaDuracionPausa) {
        this->duracionEstudioMS = nuevaDuracionEstudio;
        this->duracionPausaMS   = nuevaDuracionPausa;

        // Reinicia el timer a la nueva duración si querés:
        this->tiempoInicialFase = millis();
        this->ultimoRefresco = millis();
        this->estadoActual = ESTUDIO; // opcional: comenzar siempre en estudio
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


PomodoroTimer miTimer(DURACION_ESTUDIO_MS, DURACION_PAUSA_MS, INTERVALO_REFRESCO_MS);

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
          minutosPausa = min(minutosPausa + STEP_MIN, MAX_PAUSA_MIN);
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

    menuInicio(); // Menú para ajustar los tiempos

    // Convertir minutos a ms y pasar al timer
    miTimer.setDuraciones(DURACION_ESTUDIO_MS, DURACION_PAUSA_MS);
    miTimer.iniciar();
}

void loop() {
    miTimer.actualizar();
}