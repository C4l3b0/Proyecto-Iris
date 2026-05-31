#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <BH1750.h>
#include <Adafruit_CCS811.h>
#include <DHT.h>

// --- CONFIGURACIÓN ---
#define DHTPIN 4           
#define DHTTYPE DHT22
#define SOIL_PIN 32        
#define NODE_ID 1          

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

struct ModuloData {
  float temp; float hum; int luz; int co2; int suelo; int bat; int id;
};

ModuloData data;
DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter;
Adafruit_CCS811 ccs;

bool bh1750_found = false;
bool ccs811_found = false;

// Función para verificar si un dispositivo I2C responde
bool checkI2C(uint8_t address) {
  Wire.beginTransmission(address);
  return (Wire.endTransmission() == 0);
}

void setup() {
  // 1. ESPERA DE ESTABILIZACIÓN ELÉCTRICA
  delay(3000); 
  Serial.begin(115200);
  Serial.println("\n\n--- PROYECTO IRIS: INICIO SEGURO ---");

  // 2. INICIAR SENSORES ANTES QUE WIFI (Ahorro de energía inicial)
  dht.begin();
  Serial.println("[OK] DHT22 listo");

  Wire.begin(21, 22);
  Wire.setClock(100000);
  delay(500);

  // Verificar BH1750 (Dirección 0x23)
  if (checkI2C(0x23)) {
    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
      bh1750_found = true;
      Serial.println("[OK] BH1750 encontrado");
    }
  } else {
    Serial.println("[!] BH1750 no responde en I2C");
  }

  // Verificar CCS811 (Dirección 0x5A)
  if (checkI2C(0x5A)) {
    if (ccs.begin()) {
      ccs811_found = true;
      Serial.println("[OK] CCS811 encontrado");
    }
  } else {
    Serial.println("[!] CCS811 no responde (¿WAK a GND?)");
  }

  // 3. INICIAR WIFI/ESP-NOW AL FINAL
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); // Asegurar que no intente conectarse a un router
  
  if (esp_now_init() == ESP_OK) {
    Serial.println("[OK] ESP-NOW iniciado");
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
  }

  data.id = NODE_ID;
  data.bat = 100;
  Serial.println("--- Sistema Listo ---\n");
}

void loop() {
  Serial.println("--- Capturando Datos ---");

  // Lectura DHT22
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) {
    data.temp = -99; data.hum = -99;
    Serial.println("DHT22: SIN DATOS");
  } else {
    data.temp = t; data.hum = h;
    Serial.printf("Temp: %.1f C | Hum: %.1f %%\n", t, h);
  }

  // Lectura BH1750
  if (bh1750_found) {
    float lux = lightMeter.readLightLevel();
    data.luz = (lux < 0) ? -1 : (int)lux;
    Serial.printf("Luz: %d lux\n", data.luz);
  } else {
    data.luz = -1;
    Serial.println("Luz: SIN SENSOR");
  }

  // Lectura CCS811
  if (ccs811_found && ccs.available() && !ccs.readData()) {
    data.co2 = ccs.geteCO2();
    Serial.printf("CO2: %d ppm\n", data.co2);
  } else {
    data.co2 = -1;
    Serial.println("CO2: SIN DATOS");
  }

  // Lectura Suelo
  data.suelo = analogRead(SOIL_PIN);
  Serial.printf("Suelo: %d\n", data.suelo);

  // Enviar por ESP-NOW
  esp_now_send(broadcastAddress, (uint8_t *) &data, sizeof(data));

  Serial.println("------------------------");
  delay(3000); // Muestreo cada 3 segundos
}
