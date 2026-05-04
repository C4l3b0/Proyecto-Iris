#include <esp_now.h>
#include <WiFi.h>
#include <nvs_flash.h>

// --- CONFIGURACIÓN DE LIMPIEZA ---
void limpiezaProfunda() {
  Serial.println("Iniciando limpieza profunda...");
  WiFi.disconnect(true, true);
  nvs_flash_erase();
  nvs_flash_init();
  Serial.println("NVS y WiFi reseteados.");
}

// Estructura idéntica a la de los sensores
struct ModuloData {
  float temp;
  float hum;
  int luz;
  int co2;
  int bat;
  int id; // ID del módulo (1 o 2)
};

ModuloData incomingData;

// Usaremos Serial2 para enviar datos al ESP32 #1 (RX: 16, TX: 17)
#define RXD2 16
#define TXD2 17

// Callback cuando se recibe data
void OnDataRecv(const uint8_t * mac, const uint8_t *incoming, int len) {
  memcpy(&incomingData, incoming, sizeof(incomingData));
  
  // Formatear mensaje para el ESP32 #1
  // Ejemplo: "M1,24.5,60,800,400,100"
  String msg = "M" + String(incomingData.id) + "," +
               String(incomingData.temp, 1) + "," +
               String(incomingData.hum, 0) + "," +
               String(incomingData.luz) + "," +
               String(incomingData.co2) + "," +
               String(incomingData.bat);
               
  // Enviar por Serial2 al otro ESP32
  Serial2.println(msg);
  
  // Debug por Serial principal
  Serial.println("Recibido de ESP-NOW y enviado a Gateway: " + msg);
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  
  // IMPORTANTE: Si tienes problemas, descomenta estas líneas una vez para limpiar el chip
  // limpiezaProfunda();
  // delay(1000);

  WiFi.mode(WIFI_OFF);
  delay(100);
  
  WiFi.mode(WIFI_STA);
  Serial.println("Modo Station activo para ESP-NOW");

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("Receptor ESP-NOW listo. Esperando datos...");
}

void loop() {
  // El receptor no necesita hacer nada en el loop, todo ocurre en el callback
  delay(1000);
}
