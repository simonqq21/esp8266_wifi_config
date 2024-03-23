#ifndef WIFIMODULE_H
#define WIFIMODULE_H 
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h> 
#include <EEPROM.h>

#define START_ADDR 0x0
#define BTN1 D3

void writeStrToEEPROM(unsigned int addr, String str);
void readStrFromEEPROM(unsigned int addr, String *str);
void getWiFiEEPROMValid();
void checkResetButton(int btn);
void validateWiFiEEPROM();
void resetWiFiEEPROM();
void readWiFiEEPROM();
void saveWiFi(AsyncWebServerRequest *request);
void checkWiFiLoop();

#endif