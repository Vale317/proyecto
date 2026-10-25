#include <DHT.h>
#include <LiquidCrystal.h>

// Definición de pines
#define TRIG_PIN 7        // PD7
#define ECHO_PIN 6        // PD6
#define DHT_PIN 11        // PB3
// LDR está en A0 y Motor en A1

#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);

// LCD: RS=8, RW=9, E=10, D4=5, D5=4, D6=3, D7=2
LiquidCrystal lcd(8, 9, 10, 5, 4, 3, 2);

// Funciones en Ensamblador (extern "C")
extern "C" {
  void init_ports_asm(void);
  uint16_t read_adc_ldr_asm(void);
  uint16_t read_adc_wind_asm(void);
  uint16_t medir_distancia(void);
}

void setup() {
  // 1. Iniciar LCD
  delay(100);
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print(" Estacion Meteo ");
  delay(1500);
  lcd.clear();

  // 2. Iniciar Puertos y Sensores
  init_ports_asm();
  dht.begin();
  Serial.begin(9600);
  
  Serial.println("Sistema Iniciado.");
}

void loop() {
  // --- LECTURAS ---
  uint16_t ldr_raw = read_adc_ldr_asm();       // Luz (0-1023)
  uint16_t viento_raw = read_adc_wind_asm();   // Viento (0-1023)
  uint16_t distancia = medir_distancia();      // Distancia en cm
  
  float humedad = dht.readHumidity();
  float temperatura = dht.readTemperature();

  // MOSTRAR EN LCD
  if (!isnan(humedad) && !isnan(temperatura)) {
    
    // Fila 0: Temperatura y Humedad
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print((int)temperatura);
    lcd.print("C H:");
    lcd.print((int)humedad);
    lcd.print("%    "); // Espacios para limpiar

    // Fila 1: Luz, Viento(W) y Distancia(D)
    lcd.setCursor(0, 1);
    lcd.print("L:"); 
    lcd.print(ldr_raw); 
    
    lcd.print(" W:"); // Usamos W (Wind) para que quepa
    lcd.print(viento_raw); 
    
    lcd.print(" D:");
    lcd.print(distancia);
    lcd.print("  "); 
    
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Error Sensor DHT");
  }

  //MONITOR SERIE (Para pruebas)
  Serial.print("LDR: "); Serial.print(ldr_raw);
  Serial.print(" | Viento: "); Serial.print(viento_raw);
  Serial.print(" | Distancia: "); Serial.print(distancia); Serial.print("cm");
  Serial.print(" | Temp: "); Serial.print(temperatura); Serial.println("C");

  delay(1000); // Actualizar cada 1 segundo
}
