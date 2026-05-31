#include <esp_now.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Wire.h>
#include "RTClib.h"
#include <Preferences.h>
#include <LittleFS.h> 

// --- CONFIGURACIÓN ---
#define PIN_BOTON 13      
#define PIN_GND_BOTON 14  
#define NOMBRE_ARCHIVO "/datos_iris.csv"

struct ModuloData {
  float temp; float hum; int luz; int co2; int suelo; int bat; int id;
};

struct RegistroData {
  ModuloData info;
  int dia, mes, anio, hora, minuto, segundo;
};

// Variables globales
RegistroData reg1, reg2;
int modoActual = 0; 
unsigned long tiempoPulsado = 0;
unsigned long tiempoInicioAP = 0;

WebServer server(80);
DNSServer dnsServer;
RTC_DS3231 rtc;
Preferences memoria;

// --- FUNCIONES AUXILIARES ---

void actualizarTimestamp(RegistroData &reg) {
  if (!rtc.begin()) return; // Si el RTC falla, no intentamos leer
  DateTime now = rtc.now();
  reg.dia = now.day(); reg.mes = now.month(); reg.anio = now.year();
  reg.hora = now.hour(); reg.minuto = now.minute(); reg.segundo = now.second();
}

String formatearFecha(RegistroData reg) {
  if (reg.anio < 2000) return "Sin datos previos";
  char buffer[30];
  sprintf(buffer, "%02d/%02d/%04d %02d:%02d", reg.dia, reg.mes, reg.anio, reg.hora, reg.minuto);
  return String(buffer);
}

// --- FUNCIONES DE ARCHIVOS ---

void guardarEnCSV(RegistroData reg) {
  File file = LittleFS.open(NOMBRE_ARCHIVO, FILE_APPEND);
  if (!file) {
    file = LittleFS.open(NOMBRE_ARCHIVO, FILE_WRITE);
    file.println("Fecha;Hora;Modulo;Temp;Hum;Luz;CO2;Suelo;Bat");
  }
  if (file) {
    char buffer[100];
    sprintf(buffer, "%02d/%02d/%04d;%02d:%02d:%02d;%d;%.1f;%.1f;%d;%d;%d;%d",
            reg.dia, reg.mes, reg.anio, reg.hora, reg.minuto, reg.segundo,
            reg.info.id, reg.info.temp, reg.info.hum, reg.info.luz, 
            reg.info.co2, reg.info.suelo, reg.info.bat);
    file.println(buffer);
    file.close();
  }
}

void manejarDescarga() {
  if (LittleFS.exists(NOMBRE_ARCHIVO)) {
    File file = LittleFS.open(NOMBRE_ARCHIVO, FILE_READ);
    server.streamFile(file, "text/csv");
    file.close();
  } else {
    server.send(404, "text/plain", "No hay datos para descargar");
  }
}

// --- INTERFAZ WEB ---

void enviarPaginaWeb() {
  float totalBytes = LittleFS.totalBytes();
  float usedBytes = LittleFS.usedBytes();
  float porcentajeUso = (totalBytes > 0) ? (usedBytes / totalBytes) * 100.0 : 0;
  float totalMB = totalBytes / (1024.0 * 1024.0);
  float usedMB = usedBytes / (1024.0 * 1024.0);

  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-family:sans-serif; background:#f4f7f6; padding:20px; text-align:center;}";
  html += ".card{background:white; border-radius:15px; padding:20px; margin:15px auto; max-width:400px; box-shadow:0 4px 10px rgba(0,0,0,0.1); text-align:left; border-top:8px solid #3498db;}";
  html += ".item{display:flex; justify-content:space-between; padding:8px 0; border-bottom:1px solid #eee;}";
  html += ".btn{display:block; background:#2ecc71; color:white; padding:15px; text-decoration:none; border-radius:10px; margin:20px auto; font-weight:bold;}";
  html += ".storage-box{background:#fff; border-radius:10px; padding:15px; margin-top:20px;}";
  html += ".bar-bg{background:#eee; height:15px; border-radius:10px; overflow:hidden; margin:10px 0;}";
  html += ".bar-fill{background:#3498db; height:100%; width:" + String(porcentajeUso) + "%;}";
  html += "</style></head><body>";
  
  html += "<h1>📊 Sistema de Monitoreo IRIS</h1>";
  
  RegistroData regs[2] = {reg1, reg2};
  for(int i=0; i<2; i++) {
    html += "<div class='card'><h2>Módulo " + String(i+1) + "</h2>";
    String campos[] = {"Temperatura", "Humedad", "CO2", "Luz", "Suelo", "Batería"};
    String valores[] = {String(regs[i].info.temp,1)+" °C", String(regs[i].info.hum,0)+" %", String(regs[i].info.co2)+" ppm", String(regs[i].info.luz)+" lux", String(regs[i].info.suelo), String(regs[i].info.bat)+" %"};
    
    for(int j=0; j<6; j++) {
      html += "<div class='item'><b>"+campos[j]+":</b> ";
      if(regs[i].anio >= 2000) html += "<span>"+valores[j]+"</span>";
      else html += "<span style='color:red;'>Sin datos</span>";
      html += "</div>";
    }
    html += "<small style='color:gray;'>Última vez: " + formatearFecha(regs[i]) + "</small></div>";
  }

  html += "<a href='/descargar' class='btn'>📥 DESCARGAR DATOS (.CSV)</a>";

  html += "<div class='storage-box'><b>Memoria del Controlador</b>";
  html += "<div class='bar-bg'><div class='bar-fill'></div></div>";
  html += "<small>" + String(usedMB, 3) + " MB / " + String(totalMB, 2) + " MB usados</small></div>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}

// --- COMUNICACIÓN ESP-NOW ---

void alRecibir(const esp_now_recv_info *info, const uint8_t *data, int len) {
  ModuloData tmp;
  memcpy(&tmp, data, sizeof(tmp));
  if (tmp.id == 1) { 
    reg1.info = tmp; actualizarTimestamp(reg1); guardarEnCSV(reg1);
  } else if (tmp.id == 2) { 
    reg2.info = tmp; actualizarTimestamp(reg2); guardarEnCSV(reg2);
  }
}

// --- SETUP ---

void setup() {
  Serial.begin(115200);
  delay(1000); // Pequeña espera para estabilizar el puerto serie
  Serial.println("\n\n--- INICIANDO SISTEMA IRIS ---");

  // 1. Inicializar LittleFS (Archivos)
  Serial.print("Paso 1: LittleFS... ");
  if(LittleFS.begin(true)) {
    Serial.println("OK");
  } else {
    Serial.println("FALLÓ (Verifica el esquema de particiones)");
  }

  // 2. Cargar preferencias
  Serial.print("Paso 2: Preferencias... ");
  memoria.begin("sistema", false);
  modoActual = memoria.getInt("modo", 0); 
  memoria.putInt("modo", 0); // Siempre volver a ESP-NOW tras un reinicio inesperado
  memoria.getBytes("reg1", &reg1, sizeof(reg1));
  memoria.getBytes("reg2", &reg2, sizeof(reg2));
  Serial.println("OK");

  // 3. Inicializar RTC e I2C
  Serial.print("Paso 3: I2C y RTC... ");
  Wire.begin();
  if (rtc.begin()) {
    Serial.println("OK");
  } else {
    Serial.println("RTC No detectado");
  }

  // 4. Configurar Pines
  pinMode(PIN_GND_BOTON, OUTPUT); digitalWrite(PIN_GND_BOTON, LOW);
  pinMode(PIN_BOTON, INPUT_PULLUP);

  // 5. Iniciar Modo de Red
  if (modoActual == 1) {
    Serial.println("Paso 4: Iniciando Modo Punto de Acceso (WiFi)...");
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
    
    if(WiFi.softAP("IRIS_CENTRAL_GESTION", "12345678")) {
      Serial.println("Red WiFi Creada con éxito.");
    } else {
      Serial.println("ERROR al crear WiFi.");
    }

    dnsServer.start(53, "*", IPAddress(192,168,4,1));
    server.on("/", enviarPaginaWeb);
    server.on("/descargar", manejarDescarga);
    server.onNotFound([](){ 
      server.sendHeader("Location", "/", true);
      server.send(302, "text/plain", ""); 
    });
    server.begin();
    tiempoInicioAP = millis();
    Serial.println("Servidor Web Listo.");
  } else {
    Serial.println("Paso 4: Iniciando Modo Base (ESP-NOW)...");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    if (esp_now_init() == ESP_OK) {
      esp_now_register_recv_cb(alRecibir);
      Serial.println("ESP-NOW Listo para recibir.");
    } else {
      Serial.println("Error al iniciar ESP-NOW.");
    }
  }
}

void loop() {
  unsigned long ahora = millis();

  // Botón para cambiar de modo
  if (digitalRead(PIN_BOTON) == LOW) {
    if (tiempoPulsado == 0) tiempoPulsado = ahora;
    if (ahora - tiempoPulsado > 3000) {
      Serial.println("Cambio de modo solicitado...");
      memoria.putInt("modo", (modoActual == 0) ? 1 : 0);
      memoria.putBytes("reg1", &reg1, sizeof(reg1));
      memoria.putBytes("reg2", &reg2, sizeof(reg2));
      memoria.end();
      delay(500);
      ESP.restart();
    }
  } else {
    tiempoPulsado = 0;
  }

  // Tareas del servidor si está en modo AP
  if (modoActual == 1) {
    dnsServer.processNextRequest();
    server.handleClient();
    
    // Auto-reinicio tras 3 minutos para volver a recibir datos
    if (ahora - tiempoInicioAP > 180000) {
      Serial.println("Tiempo de gestión terminado. Reiniciando a Modo Base...");
      ESP.restart();
    }
  }
}
