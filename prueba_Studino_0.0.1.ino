#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <LedControl.h>

unsigned long ultimoRefrescoLCD = 0;


// --- CONFIGURACIÓN LCD I2C ---
LiquidCrystal_I2C lcd(0x27, 16, 2);  // cambia 0x27 si tu módulo usa otra dirección

// --- CONFIGURACIÓN LED MATRIX (MAX7219) ---
LedControl lc = LedControl(9, 13, 10, 1); // DIN, CLK, CS, #displays

// --- PINES ---
#define BTN_START 7
#define BTN_RESET 8
#define SENSOR 6

// --- VARIABLES DE TIEMPO ---
unsigned long anterior = 0;
int tiempoEstudio = 25 * 60; // 25 minutos
int tiempoDescanso = 5 * 60; // 5 minutos
int tiempoActual = tiempoEstudio;
bool enEstudio = true;
bool activo = false;

// --- CONTROL SENSOR ---
unsigned long tiempoUltimaDeteccion = 0;

// --- MATRIZ (ejemplo simple) ---
byte carita[8] = {
  B00000000,
  B01000010,
  B00000000,
  B00000000,
  B01000010,
  B00111100,
  B00000000,
  B00000000
};

void setup() {
  // LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Studino listo!");
  delay(1000);
  lcd.clear();

  // MATRIZ LED
  lc.shutdown(0, false);
  lc.setIntensity(0, 5);
  lc.clearDisplay(0);
  mostrarCarita();

  // Pines
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_RESET, INPUT_PULLUP);
  pinMode(SENSOR, INPUT);
}

void loop() {
  unsigned long ahora = millis();

  leerBotones();

  actualizarTimer(ahora);
  mostrarLCD();
}

void leerBotones() {
  if (digitalRead(BTN_START) == LOW) {
    activo = !activo;
    delay(200);
  }

  if (digitalRead(BTN_RESET) == LOW) {
    tiempoActual = enEstudio ? tiempoEstudio : tiempoDescanso;
    activo = false;
    delay(200);
  }
}


void actualizarTimer(unsigned long ahora) {
  if (activo && ahora - anterior >= 1000) {
    anterior = ahora;
    tiempoActual--;

    if (tiempoActual <= 0) {
      cambiarModo();
    }
  }
}

void cambiarModo() {
  enEstudio = !enEstudio;
  tiempoActual = enEstudio ? tiempoEstudio : tiempoDescanso;

  lcd.clear();
  if (enEstudio) lcd.print("Estudia 💪");
  else lcd.print("Descansa 😌");

  lc.clearDisplay(0);
  mostrarCarita();
}

void mostrarLCD() {
  unsigned long ahora = millis();
  if (ahora - ultimoRefrescoLCD < 1000) return;  // solo actualiza cada 1 seg
  ultimoRefrescoLCD = ahora;

  lcd.setCursor(0, 0);
  lcd.print(enEstudio ? "Estudio:  " : "Descanso: ");
  lcd.setCursor(0, 1);

  int minutos = tiempoActual / 60;
  int segundos = tiempoActual % 60;

  // Borramos la línea antes de escribir
  lcd.print("                ");  
  lcd.setCursor(0, 1);

  if (minutos < 10) lcd.print('0');
  lcd.print(minutos);
  lcd.print(':');
  if (segundos < 10) lcd.print('0');
  lcd.print(segundos);
}



void mostrarCarita() {
  for (int i = 0; i < 8; i++) {
    lc.setRow(0, i, carita[i]);
  }
}
