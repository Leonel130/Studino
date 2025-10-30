#include "Timer.h"

unsigned long Timer::getTiempoRestanteMS() const { 
    if (!this->activo) {
        return 0;
    }
    unsigned long tiempoTranscurrido = millis() - this->tiempoInicial;
    if (tiempoTranscurrido >= this->duracion) {
        return 0;
    }
    return this->duracion - tiempoTranscurrido;
}