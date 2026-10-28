// =====================================================================
// proyecto_2.ino - Estacion meteorologica
// Carpeta del sketch: proyecto_2/proyecto_2.ino  +  proyecto_2/AVR_2.S
// Necesita la biblioteca "DHT sensor library" (Adafruit) y LiquidCrystal.
// =====================================================================
#include <DHT.h>
#include <LiquidCrystal.h>
#include <stdio.h>

// ---- Pines ------------------------------------------------------------
#define DHT_PIN   11          // PB3
#define DHTTYPE   DHT11
DHT dht(DHT_PIN, DHTTYPE);

// LCD: RS=8, RW=9, E=10, D4=5, D5=4, D6=3, D7=2
LiquidCrystal lcd(8, 9, 10, 5, 4, 3, 2);

// ---- Rutinas en ensamblador (AVR_2.S) ----------------------------------
extern "C" {
  void     init_ports_asm(void);
  uint16_t leer_adc(uint8_t canal);
  uint16_t escalar_adc(uint16_t raw, uint8_t k);
  uint16_t medir_distancia(void);
  void     led_alerta(uint8_t encendido);
}

// ---- Canales y calibracion -----------------------------------------------
#define CANAL_LDR     0       // A0  (si mueven el motor a A0: LDR=1, MOTOR=0)
#define CANAL_MOTOR   1       // A1
#define K_LUX         250     // lux  = (raw*K)>>8   -> ajustar con luxometro
#define K_VIENTO      20      // dm/s = (raw*K)>>8   -> ajustar en calibracion

#define DIST_ERROR    0xFFFF
#define DIST_MIN      2
#define DIST_MAX      400
#define ADC_MIN       2       // 0 o casi 0  => LDR desconectado / en corto
#define ADC_MAX       1021    // saturado

static uint16_t contador = 0;

// Escribe una linea completa de 16 caracteres (borra restos anteriores)
static void lcd_linea(uint8_t fila, const char *txt) {
  char b[17];
  snprintf(b, sizeof(b), "%-16s", txt);
  lcd.setCursor(0, fila);
  lcd.print(b);
}

void setup() {
  delay(100);
  lcd.begin(16, 2);
  lcd.clear();
  lcd_linea(0, " Estacion Meteo");
  delay(1500);
  lcd.clear();

  init_ports_asm();
  dht.begin();
  Serial.begin(9600);
  Serial.println("Sistema Iniciado.");
}

void loop() {
  contador++;

  // --- LECTURAS (ensamblador) ---------------------------------------------
  uint16_t ldr_raw    = leer_adc(CANAL_LDR);
  uint16_t viento_raw = leer_adc(CANAL_MOTOR);
  uint16_t distancia  = medir_distancia();
  uint16_t lux        = escalar_adc(ldr_raw, K_LUX);
  uint16_t viento     = escalar_adc(viento_raw, K_VIENTO);   // decimas de m/s

  // --- DHT11 (biblioteca) ---------------------------------------------------
  float humedad     = dht.readHumidity();
  float temperatura = dht.readTemperature();

  // --- Validacion ---------------------------------------------------------------
  bool ok_th = !isnan(humedad) && !isnan(temperatura);
  bool ok_l  = (ldr_raw >= ADC_MIN) && (ldr_raw <= ADC_MAX);
  bool ok_v  = (viento_raw <= ADC_MAX);
  bool ok_d  = (distancia != DIST_ERROR) && (distancia >= DIST_MIN) && (distancia <= DIST_MAX);

  led_alerta(!(ok_th && ok_l && ok_v && ok_d));

  // --- LCD: dos pantallas que alternan cada 3 s --------------------------------
  char a[17], b[17];
  if (((contador / 3) & 1) == 0) {
    // Pantalla 1: temperatura, humedad y luz
    if (ok_th) snprintf(a, sizeof(a), "T:%dC H:%d%%", (int)temperatura, (int)humedad);
    else       snprintf(a, sizeof(a), "T:ERR H:ERR");
    if (ok_l)  snprintf(b, sizeof(b), "Luz:%u lux", lux);
    else       snprintf(b, sizeof(b), "Luz: ERR");
  } else {
    // Pantalla 2: viento y distancia
    if (ok_v)  snprintf(a, sizeof(a), "Viento:%u.%u m/s", viento / 10, viento % 10);
    else       snprintf(a, sizeof(a), "Viento: ERR");
    if (ok_d)  snprintf(b, sizeof(b), "Dist:%u cm", distancia);
    else       snprintf(b, sizeof(b), "Dist: ERR");
  }
  lcd_linea(0, a);
  lcd_linea(1, b);

  // --- Monitor serie (pruebas) ------------------------------------------------------
  Serial.print("LDR: ");        Serial.print(ldr_raw);
  Serial.print(" | Motor: ");   Serial.print(viento_raw);
  Serial.print(" | Dist: ");    Serial.print(ok_d ? (int)distancia : -1);
  Serial.print(" cm | Temp: "); Serial.print(temperatura);
  Serial.print(" C | Hum: ");   Serial.println(humedad);

  delay(1000);
}
