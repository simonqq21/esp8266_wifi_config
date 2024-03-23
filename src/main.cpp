#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h> 
#include <EEPROM.h>
#include "wifiModule.h"

extern unsigned long checkAddr, ssidAddr, passAddr, ipIndexAddr;
extern int checkNum; 
extern String ssid, pass;

void setup() {
  Serial.begin(115200);
  pinMode(BTN1, INPUT_PULLUP);

  // littleFS 
  if (!LittleFS.begin()) {
    Serial.println("An error occured while mounting LittleFS.");
  }
  
  // initialize EEPROM and addresses
  EEPROM.begin(sizeof(int) + sizeof(char) * 80 + sizeof(byte));
  checkAddr = START_ADDR;
  ssidAddr = checkAddr + sizeof(int);
  passAddr = ssidAddr + sizeof(char) * 32;
  ipIndexAddr = passAddr + sizeof(char) * 32;

  getWiFiEEPROMValid();
  checkResetButton(BTN1);
  Serial.print(checkNum); 
  validateWiFiEEPROM();

  Serial.println("Attempting to connect to WiFi:");
  WiFi.begin(ssid, pass);
}

void loop() {
  checkWiFiLoop();
}




