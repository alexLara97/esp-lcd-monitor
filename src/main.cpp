#include <Arduino.h>

#if defined(ARDUINO_ARCH_ESP8266)
  #include <ESP8266WiFi.h>
#elif defineed(ARDUINO_ARCH_ESP32)
  #include <WiFi.h>
#endif

#include <WiFiUdp.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include "secrets.h"

LiquidCrystal_I2C lcd(LCD_I2C_ADDR, 16, 2);
WiFiUDP udp;

// Ajuste de Payload esperado. 512 suele ser suficiente para un JSON simple
static const size_t UDP_BUFFER_SIZE = 512;
char udpBuffer[UDP_BUFFER_SIZE];

void connectToWifi() {
  Serial.print("Wifi connecting...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - start > 20000) { // 20s timeout
      Serial.println("\nWifi timeout. Restart...");
      ESP.restart;
    }
  }
  Serial.print("\nConecting... IP: ");
  Serial.println(WiFi.localIP());
}

void setupLCD() { 
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ESP UDP READY");
}

void showStatusLine2(const String& msg) {
  lcd.setCursor(0, 1);
  String line = msg;
  if (line.length() > 16) line = line.substring(0, 16);
  // Rellena con espacios si es mas corta para limpiar restos
  while (line.length() < 16) line += ' ';
  lcd.print(line);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  setupLCD();
  connectToWifi();

  if (udp.begin(UDP_PORT)) {
    lcd.setCursor(0,0);
    lcd.print("UDP on port: ");
    lcd.print(UDP_PORT);
  } else {
    Serial.println("Error to start UDP");
    lcd.setCursor(0, 0);
    lcd.print("Error init UDP");
  }
}

void loop() {
  int packetSize = udp.parsePacket();
  if (packetSize > 0) {
    if (packetSize >= (int)UDP_BUFFER_SIZE) {
      Serial.println("Package too large. Download....");
      // Vacía el paquete para no bloquear el buffer
      while (udp.available()) udp.read();
      showStatusLine2("Pkt too large");
      return;
    }
    int len = udp.read(udpBuffer, UDP_BUFFER_SIZE - 1);
    if (len < 0) return;
    udpBuffer[len] = '\0';

    Serial.print("UDP recevied");
    Serial.print(udpBuffer);

    // JSON --> {"cpu": 45.2, "mem": 62.7, "temp": 55.3}
    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, udpBuffer);
    if (err) {
      Serial.print("Error JSON: ");
      Serial.println(err.c_str());
      showStatusLine2("JSON error");
      return;
    }

    // data extraction with default values
    float cpu = doc["cpu"] | -1.0;
    float mem = doc["mem"] | -1.0;
    float temp = doc["temp"] | -1.0;

    // LCD Show
    lcd.setCursor(0,0);
    // Line 1: CPU: x MEM: x
    char line1[17];
    snprintf(line1, sizeof(line1), "CPU:%4.1f MEM:%2.0f", cpu, mem);
    lcd.print(line1);

    int fill1 = 16 - strlen(line1);
    while (fill1-- > 0) lcd.print(' ');

    // Line 2: TMP: XC IP final
    lcd.setCursor(0, 1);
    char line2[17];
    snprintf(line2, sizeof(line2), "TMP:%4.1fC", temp);
    lcd.print(line2);

    int fill2 = 16 - strlen(line2);
    while (fill2-- > 0) lcd.print(' ');
    
    // Optional: respond or doing ack (UDP is connectionless, but you can respond)
    // udp.beginPacket(udp.remoteIP(), udp.remotePort());
    // udp.print("OK");
    // udp.endPacket();
  }

  delay(5);
}