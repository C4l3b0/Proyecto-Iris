#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Wire.h>
#include "RTClib.h"
#include <time.h>

RTC_DS3231 rtc;

const char* ssid = "Invernadero Iris";
const char* password = "iris_password";

const byte DNS_PORT = 53;
DNSServer dnsServer;
WebServer server(80);
IPAddress apIP(192, 168, 4, 1);

struct ModuloData { float temp; float hum; int luz; int co2; int bat; };
ModuloData m1 = {24.5, 60.0, 800, 415, 100};
ModuloData m2 = {23.8, 62.5, 750, 420, 100};
int batMaestro = 100;

String getFormattedTime() {
  DateTime now = rtc.now();
  char buff[30];
  // Formato: DD/MM/YYYY HH:mm:ss
  sprintf(buff, "%02d/%02d/%04d %02d:%02d:%02d", now.day(), now.month(), now.year(), now.hour(), now.minute(), now.second());
  return String(buff);
}

void handleData() {
  DateTime now = rtc.now();
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
  String html = "<!DOCTYPE html><html lang='es'><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<meta charset='UTF-8'><title>Iris Agriculture</title>";
  html += "<style>";
  html += "body{font-family:'Segoe UI',sans-serif; background-color:#f0f7f4; color:#2d3436; margin:0; padding:15px;}";
  html += "header{background:linear-gradient(135deg, #27ae60, #2ecc71); color:white; padding:20px; border-radius:15px; text-align:center; box-shadow:0 4px 15px rgba(0,0,0,0.1); margin-bottom:20px; position:relative;}";
  html += ".bat-maestro{position:absolute; top:10px; right:15px; font-size:12px; display:flex; align-items:center; background:rgba(255,255,255,0.2); padding:4px 8px; border-radius:10px;}";
  html += ".time-box{text-align:center; color:#27ae60; font-weight:bold; font-size:16px; margin-bottom:20px;}";
  html += ".grid{display:grid; grid-template-columns: 1fr; gap:20px;} @media(min-width:600px){ .grid{grid-template-columns: 1fr 1fr;} }";
  html += ".card{background:white; border-radius:20px; padding:20px; box-shadow:0 8px 16px rgba(0,0,0,0.05); border-top:6px solid #27ae60; position:relative;}";
  html += ".card h2{margin:0 0 15px 0; color:#27ae60; font-size:20px;}";
  html += ".bat-sensor{position:absolute; top:20px; right:20px; font-size:13px; font-weight:bold; color:#2ecc71; display:flex; align-items:center;}";
  html += ".sensor-item{display:flex; justify-content:space-between; align-items:center; padding:12px 0; border-bottom:1px solid #f1f2f6;}";
  html += ".sensor-item:last-child{border-bottom:none;}";
  html += ".label{color:#636e72; font-size:14px;}";
  html += ".value{font-weight:800; color:#2d3436; font-size:16px;}";
  html += ".unit{font-size:12px; color:#b2bec3; margin-left:3px;}";
  html += ".download-btn{background:#2d3436; color:white; text-align:center; padding:18px; border-radius:15px; margin-top:30px; cursor:pointer; font-weight:bold; border:none; width:100%; font-size:16px; box-shadow:0 4px 10px rgba(0,0,0,0.2);}";
  html += ".battery-icon{width:22px; height:11px; border:1.5px solid currentColor; border-radius:2px; position:relative; margin-left:6px;}";
  html += ".battery-icon::after{content:''; position:absolute; right:-4px; top:2px; width:3px; height:4px; background:currentColor; border-radius:0 1px 1px 0;}";
  html += ".battery-level{position:absolute; left:1px; top:1px; bottom:1px; background:#fff; width:90%;}";
  html += ".card .battery-level{background:#2ecc71;}";
  html += "</style>";
  html += "<script>window.onload = () => { fetch('/sync?time=' + Math.floor(Date.now()/1000)); };</script>";
  html += "</head><body>";
  
  // HEADER CON BATERÍA MAESTRO
  html += "<header>";
  html += "<h1>PROYECTO IRIS</h1><p style='margin:0; opacity:0.9;'>Monitoreo de Hortalizas</p>";
  html += "<div class='bat-maestro'>" + String(batMaestro) + "% <div class='battery-icon'><div class='battery-level' style='width:95%'></div></div></div>";
  html += "</header>";
  
  html += "<div class='time-box'>📅 " + getFormattedTime() + "</div>";

  html += "<div class='grid'>";
  
  // MÓDULO 1
  html += "<div class='card'>";
  html += "<h2>Módulo 1</h2>";
  html += "<div class='bat-sensor'>" + String(m1.bat) + "% <div class='battery-icon'><div class='battery-level' style='width:95%'></div></div></div>";
  html += "<div class='sensor-item'><span class='label'>🌡️ Temperatura</span><span class='value'>" + String(m1.temp, 1) + "<span class='unit'>°C</span></span></div>";
  html += "<div class='sensor-item'><span class='label'>💧 Humedad Relativa</span><span class='value'>" + String(m1.hum, 0) + "<span class='unit'>%</span></span></div>";
  html += "<div class='sensor-item'><span class='label'>☀️ Intensidad de Luz</span><span class='value'>" + String(m1.luz) + "<span class='unit'>Lux</span></span></div>";
  html += "<div class='sensor-item'><span class='label'>☁️ Dióxido de Carbono</span><span class='value'>" + String(m1.co2) + "<span class='unit'>ppm</span></span></div>";
  html += "</div>";

  // MÓDULO 2
  html += "<div class='card'>";
  html += "<h2>Módulo 2</h2>";
  html += "<div class='bat-sensor'>" + String(m2.bat) + "% <div class='battery-icon'><div class='battery-level' style='width:95%'></div></div></div>";
  html += "<div class='sensor-item'><span class='label'>🌡️ Temperatura</span><span class='value'>" + String(m2.temp, 1) + "<span class='unit'>°C</span></span></div>";
  html += "<div class='sensor-item'><span class='label'>💧 Humedad Relativa</span><span class='value'>" + String(m2.hum, 0) + "<span class='unit'>%</span></span></div>";
  html += "<div class='sensor-item'><span class='label'>☀️ Intensidad de Luz</span><span class='value'>" + String(m2.luz) + "<span class='unit'>Lux</span></span></div>";
  html += "<div class='sensor-item'><span class='label'>☁️ Dióxido de Carbono</span><span class='value'>" + String(m2.co2) + "<span class='unit'>ppm</span></span></div>";
  html += "</div>";

  html += "</div>";

  html += "<button class='download-btn' onclick='location.href=\"/download\"'>📊 DESCARGAR REPORTE HISTÓRICO (.CSV)</button>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleDownload() {
  String csv = "HORA,M1_TEMPERATURA(C),M1_HUMEDAD(%),M1_LUZ(LX),M1_CO2(PPM),M2_TEMPERATURA(C),M2_HUMEDAD(%),M2_LUZ(LX),M2_CO2(PPM)\n";
  int hora = 0; int min = 0;
  for (int i = 0; i < 72; i++) {
    char timeStr[10]; sprintf(timeStr, "%02d:%02d", hora, min);
    csv += String(timeStr) + "," + String(22.0 + (random(0, 50)/10.0), 1) + "," + String(random(55, 65)) + "," + String(random(700, 900)) + "," + String(random(400, 450)) + ",";
    csv += String(21.5 + (random(0, 50)/10.0), 1) + "," + String(random(58, 68)) + "," + String(random(650, 850)) + "," + String(random(410, 460)) + "\n";
    min += 20; if (min >= 60) { min = 0; hora++; }
  }
  server.sendHeader("Content-Type", "text/csv");
  server.sendHeader("Content-Disposition", "attachment; filename=reporte_invernadero_iris.csv");
  server.send(200, "text/csv", csv);
}

void handleSync() {
  server.send(200, "text/plain", "Sincronizacion manual deshabilitada. Usando hora de compilacion.");
}

void setup() {
  Serial.begin(115200);
  
  Wire.begin(); // Asegurar inicio de I2C
  if (!rtc.begin()) {
    Serial.println("No se pudo encontrar el RTC");
  } else {
    // Sincroniza el RTC con la fecha y hora en que se compilo el codigo
    Serial.println("Sincronizando RTC con hora de compilacion...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid, password);
  dnsServer.start(DNS_PORT, "*", apIP);
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/sync", handleSync);
  server.on("/download", handleDownload);
  server.onNotFound([]() { server.sendHeader("Location", "/", true); server.send(302, "text/plain", ""); });
  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  static unsigned long last_ms = 0;
  if (millis() - last_ms > 4000) {
    last_ms = millis();
    m1.temp += (random(-2, 3) / 10.0);
    m2.temp += (random(-2, 3) / 10.0);
  }
}
