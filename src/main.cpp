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

AsyncWebServer server(80); 
AsyncWebSocket ws("/ws"); 
File indexPage;  

void setup() {
  Serial.begin(115200);

  // littleFS 
  if (!LittleFS.begin()) {
    Serial.println("An error occured while mounting LittleFS.");
  }
  
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

ESP starts up, loading wifi credentials from the EEPROM. 
If the ESP connects successfully, 
  proceed with the rest of the application.
Else, 
  Open an open WiFi AP 
  serve the WiFi configuration webpage
  User loads the webpage on a device, enters the WiFi credentials, then submits it.
  Upon credentials submission, 
    save the credentials to the EEPROM
    shut off the WiFi AP 
    turn on the WiFi STA 
    attempt to reconnect to the WiFi 
    If the ESP connects successfully, 
      proceed with the rest of the application.
*/

  Serial.println("Attempting to connect to WiFi:");
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("not connected: ");
    Serial.println(WiFi.status());
    delay(500);
    switch (WiFi.status())
    {
    case WL_CONNECTED:
    case WL_IDLE_STATUS:
    case 7:
      break;
    
    default:
      WiFi.softAP("ESP8266_wifi_config");
      Serial.println("Starting wifi SoftAP");
      Serial.print("IP Address: ");
      Serial.println(WiFi.softAPIP());
      server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/wifi.html", String(), false);});
      server.begin();
    }
  }
  Serial.print("Connected: ");
  Serial.println(WiFi.status());

  // testing
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
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", String(), false);});
  server.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
}
