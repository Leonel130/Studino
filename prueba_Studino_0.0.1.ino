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


const unsigned long DURACION_ESTUDIO_MS   = 15000UL; // 15 seg
const unsigned long DURACION_PAUSA_MS     = 10000UL; // 10 seg
const unsigned long INTERVALO_REFRESCO_MS = 1000UL;  // 1 seg

PomodoroTimer miTimer(DURACION_ESTUDIO_MS, DURACION_PAUSA_MS, INTERVALO_REFRESCO_MS);


void setup() {
    Serial.begin(9600);
    Serial.println("--- Bienvenido a Studino! Tu compañero de estudio :) ---");

    miTimer.iniciar(); // Modificar: Llamar una vez que el usuario defina la duración de los temporizadores  
}

void loop() {
    miTimer.actualizar();
}