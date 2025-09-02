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

