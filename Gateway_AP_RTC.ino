#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Wire.h>
#include "RTClib.h"
#include <nvs_flash.h>

// --- CONFIGURACIÓN DE LIMPIEZA ---
void limpiezaProfunda() {
  Serial.println("Iniciando limpieza profunda...");
  WiFi.disconnect(true, true);
  nvs_flash_erase();
  nvs_flash_init();
  Serial.println("NVS y WiFi reseteados.");
}

RTC_DS3231 rtc;
const char* ssid = "Invernadero Iris Gateway";
const char* password = "iris_password";

const byte DNS_PORT = 53;
DNSServer dnsServer;
WebServer server(80);
IPAddress apIP(192, 168, 4, 1);

struct ModuloData { float temp; float hum; int luz; int co2; int bat; };
ModuloData m1 = {0, 0, 0, 0, 0};
ModuloData m2 = {0, 0, 0, 0, 0};
int batMaestro = 100;

// Usaremos Serial2 para recibir datos del otro ESP32 (RX: 16, TX: 17)
#define RXD2 16
#define TXD2 17

String getFormattedTime() {
  DateTime now = rtc.now();
  char buff[30];
  sprintf(buff, "%02d/%02d/%04d %02d:%02d:%02d", now.day(), now.month(), now.year(), now.hour(), now.minute(), now.second());
  return String(buff);
}

void handleData() {
  String json = "{";
  json += "\"time\":\"" + getFormattedTime() + "\",";
  json += "\"m1_temp\":" + String(m1.temp, 1) + ",";
  json += "\"m1_hum\":" + String(m1.hum, 0) + ",";
  json += "\"m2_temp\":" + String(m2.temp, 1) + ",";
  json += "\"m2_hum\":" + String(m2.hum, 0) + ",";
  json += "\"bat_maestro\":" + String(batMaestro);
  json += "}";
  server.send(200, "application/json", json);
}

void handleRoot() {
  // Reutilizamos el HTML de tu firmware.ino
  String html = "<!DOCTYPE html><html lang='es'><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<meta charset='UTF-8'><title>Iris Gateway</title>";
  html += "<style>";
  html += "body{font-family:'Segoe UI',sans-serif; background-color:#f0f7f4; color:#2d3436; margin:0; padding:15px;}";
  html += "header{background:linear-gradient(135deg, #27ae60, #2ecc71); color:white; padding:20px; border-radius:15px; text-align:center; box-shadow:0 4px 15px rgba(0,0,0,0.1); margin-bottom:20px; position:relative;}";
  html += ".bat-maestro{position:absolute; top:10px; right:15px; font-size:12px; display:flex; align-items:center; background:rgba(255,255,255,0.2); padding:4px 8px; border-radius:10px;}";
  html += ".time-box{text-align:center; color:#27ae60; font-weight:bold; font-size:16px; margin-bottom:20px;}";
  html += ".grid{display:grid; grid-template-columns: 1fr; gap:20px;} @media(min-width:600px){ .grid{grid-template-columns: 1fr 1fr;} }";
  html += ".card{background:white; border-radius:20px; padding:20px; box-shadow:0 8px 16px rgba(0,0,0,0.05); border-top:6px solid #27ae60; position:relative;}";
  html += ".card h2{margin:0 0 15px 0; color:#27ae60; font-size:20px;}";
  html += ".sensor-item{display:flex; justify-content:space-between; align-items:center; padding:12px 0; border-bottom:1px solid #f1f2f6;}";
  html += ".value{font-weight:800; color:#2d3436; font-size:16px;}";
  html += ".battery-icon{width:22px; height:11px; border:1.5px solid currentColor; border-radius:2px; position:relative; margin-left:6px;}";
  html += ".battery-level{position:absolute; left:1px; top:1px; bottom:1px; background:#2ecc71;}";
  html += "</style></head><body>";
  
  html += "<header><h1>GATEWAY IRIS</h1><div class='bat-maestro'>" + String(batMaestro) + "% <div class='battery-icon'><div class='battery-level' style='width:" + String(batMaestro) + "%'></div></div></div></header>";
  html += "<div class='time-box'>📅 " + getFormattedTime() + "</div>";
  html += "<div class='grid'>";
  
  // Módulo 1
  html += "<div class='card'><h2>Módulo 1</h2>";
  html += "<div class='sensor-item'><span class='label'>🌡️ Temp</span><span class='value'>" + String(m1.temp, 1) + "°C</span></div>";
  html += "<div class='sensor-item'><span class='label'>💧 Hum</span><span class='value'>" + String(m1.hum, 0) + "%</span></div>";
  html += "</div>";

  // Módulo 2
  html += "<div class='card'><h2>Módulo 2</h2>";
  html += "<div class='sensor-item'><span class='label'>🌡️ Temp</span><span class='value'>" + String(m2.temp, 1) + "°C</span></div>";
  html += "<div class='sensor-item'><span class='label'>💧 Hum</span><span class='value'>" + String(m2.hum, 0) + "%</span></div>";
  html += "</div></div></body></html>";

  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  
  // OPCIONAL: Descomentar las siguientes dos líneas si se quiere forzar limpieza en cada booteo
  // limpiezaProfunda(); 
  // delay(1000);

  WiFi.mode(WIFI_OFF); // Reset stack
  delay(100);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid, password);
  
  Wire.begin();
  if (!rtc.begin()) Serial.println("No RTC");
  
  dnsServer.start(DNS_PORT, "*", apIP);
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("Gateway AP listo.");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  // Escuchar datos del ESP32 #2
  if (Serial2.available()) {
    String input = Serial2.readStringUntil('\n');
    // Formato esperado: M1,temp,hum,luz,co2,bat o M2...
    if (input.startsWith("M1")) {
      sscanf(input.c_str(), "M1,%f,%f,%d,%d,%d", &m1.temp, &m1.hum, &m1.luz, &m1.co2, &m1.bat);
    } else if (input.startsWith("M2")) {
      sscanf(input.c_str(), "M2,%f,%f,%d,%d,%d", &m2.temp, &m2.hum, &m2.luz, &m2.co2, &m2.bat);
    }
  }
}
