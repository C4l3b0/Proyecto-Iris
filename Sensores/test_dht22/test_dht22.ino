#include "DHT.h"

#define DHTPIN 2     // Pin donde conectamos el sensor (GPIO 4)
#define DHTTYPE DHT22   // Definimos el tipo de sensor

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  Serial.println(F("Iniciando prueba de DHT22..."));
  dht.begin();
}

void loop() {
  // Esperamos 2 segundos entre mediciones (el DHT22 es lento)
  delay(2000);

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Comprobamos si la lectura falló
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Error al leer el sensor DHT22. Revisa las conexiones."));
    return;
  }

  Serial.print(F("Humedad: "));
  Serial.print(h);
  Serial.print(F("%  Temperatura: "));
  Serial.print(t);
  Serial.println(F("°C"));
}
