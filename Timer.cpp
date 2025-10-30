#include "Timer.h"

Timer::Timer() {
    this->activo = false;
    this->tiempoInicial = 0;
    this->duracion = 0;
}

void Timer::iniciar(unsigned long duracionMS) {
    this->duracion = duracionMS;
    this->tiempoInicial = millis();
    this->activo = true;
}

void Timer::detener() {
    this->activo = false;
}

bool Timer::actualizar() {
    if (!this->activo) return false;
    unsigned long tiempoTranscurrido = millis() - this->tiempoInicial;
    if (tiempoTranscurrido >= this->duracion) {
        this->activo = false;
        return true;
    }
    return false;
}

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