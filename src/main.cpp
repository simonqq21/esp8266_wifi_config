#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h> 
#include <EEPROM.h>

#define START_ADDR 0x0

String ssid, pass;
String newSsid, newPass;
unsigned long ssidAddr, passAddr;

void setup() {
  Serial.begin(115200);

  EEPROM.begin(sizeof(char) * 160);
  ssidAddr = START_ADDR;
  passAddr = ssidAddr + sizeof(char) * 64;

  ssid = String("wifiSSID");
  pass = String("password1234");

  ssid = String("QUE-STARLINK");
  pass = String("Quefamily01259");

/*
ssid not available - 1
wrong password, disconnected - 6
connected - 3
initializing - 7
*/

  Serial.println("Attempting to connect to WiFi:");
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("not connected: ");
    Serial.println(WiFi.status());
    delay(500);
  }
  Serial.print("Connected: ");
  Serial.println(WiFi.status());
  // EEPROM.put(ssidAddr, ssid);
  // EEPROM.put(passAddr, pass);
  // Serial.print("ssid=");
  // Serial.print(ssid);
  // Serial.print(" pass=");
  // Serial.println(pass);
  // Serial.println("saved strings to EEPROM");
  // delay(1000);
  // EEPROM.get(ssidAddr, newSsid);
  // EEPROM.get(passAddr, newPass);
  // Serial.print("ssid=");
  // Serial.print(newSsid);
  // Serial.print(" pass=");
  // Serial.println(newPass);
  // Serial.println("loaded strings from EEPROM");
}

void loop() {
  // put your main code here, to run repeatedly:
}
