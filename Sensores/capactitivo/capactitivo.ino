// Usamos el GPIO 34 (uno de los ADC más estables del ESP32)
const int sensorPin = 34;

void setup() {
  Serial.begin(115200); // El ESP32 suele usar 115200 baudios
  Serial.println("--- Prueba ESP32 + Sensor Capacitivo ---");
}

void loop() {
  // Leemos el valor (rango 0-4095 en ESP32)
  int lectura = analogRead(sensorPin);

  // Imprimimos la lectura cruda
  Serial.print("Lectura ADC: ");
  Serial.print(lectura);

  // Interpretación básica (ajusta según tus pruebas)
  if (lectura > 2500) {
    Serial.println(" -> Estado: Seco");
  } else if (lectura > 1500) {
    Serial.println(" -> Estado: Humedad óptima");
  } else {
    Serial.println(" -> Estado: Muy mojado / En agua");
  }

  delay(1000);
}