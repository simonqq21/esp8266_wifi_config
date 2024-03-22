#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h> 
#include <EEPROM.h>

#define START_ADDR 0x0

// read from the EEPROM. If the EEPROM contents are valid, it must be 1024.
// If it is any other value, EEPROM contents are invalid.
int checkNum; 
bool wifiSubmitted = false; 
String ssid, pass;
byte ipIndex;
String newSsid, newPass;
unsigned long checkAddr, ssidAddr, passAddr, ipIndexAddr;

AsyncWebServer server(80); 
AsyncWebSocket ws("/ws"); 
File indexPage;  

IPAddress localIP; 
IPAddress apIP;
IPAddress gateway;
IPAddress subnet(255,255,255,0);
IPAddress dns(8,8,8,8);

AsyncWebHandler indexHandler, wifiHandler; 
unsigned long printDelay = 1000;
unsigned long lastTimePrinted; 

void setup() {
  Serial.begin(115200);

  // littleFS 
  if (!LittleFS.begin()) {
    Serial.println("An error occured while mounting LittleFS.");
  }
  
  // initialize EEPROM
  EEPROM.begin(sizeof(int) + sizeof(char) * 160 + sizeof(byte));
  checkAddr = START_ADDR;
  ssidAddr = checkAddr + sizeof(int);
  passAddr = ssidAddr + sizeof(char) * 64;
  ipIndexAddr = passAddr + sizeof(char) * 64;
  /*
  read from the EEPROM. If the EEPROM contents are valid, it must be 1024.
  If it is any other value, EEPROM contents are invalid, then the EEPROM 
  will be set to default valid string values. 
  */
  // EEPROM.put(checkAddr, 0);
  // EEPROM.commit();
  EEPROM.get(checkAddr, checkNum);
  Serial.print(checkNum); 
  checkNum = 0;
  if (checkNum != 1024) {
    ssid = "wifi";
    pass = "password";
    EEPROM.put(checkAddr, 1024);
    EEPROM.put(ssidAddr, ssid);
    EEPROM.put(passAddr, pass);
    EEPROM.put(ipIndexAddr, 2);
    EEPROM.commit();

    Serial.print("ssid=");
    Serial.println(newSsid);
    Serial.print("pass=");
    Serial.println(newPass);

    EEPROM.get(ssidAddr, newSsid);
    EEPROM.get(passAddr, newPass);
    EEPROM.get(ssidAddr, ssid);
    EEPROM.get(passAddr, pass); 
    
    Serial.print("ssid=");
    Serial.println(newSsid);
    Serial.print("pass=");
    Serial.println(newPass);
    Serial.println("EEPROM initialized!");
  }
  else {
    EEPROM.get(ssidAddr, ssid);
    EEPROM.get(passAddr, pass); 
    EEPROM.get(ipIndexAddr, ipIndex);

    Serial.print("ssid=");
    Serial.println(ssid);
    Serial.print("pass=");
    Serial.println(pass);
    Serial.print("ipIndex=");
    Serial.println(ipIndex);
    Serial.println("EEPROM read!");
  }

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
    if (millis() - lastTimePrinted > printDelay) {
      lastTimePrinted = millis(); 
      Serial.print("not connected: ");
      Serial.println(WiFi.status());
      Serial.print("ip=");
      Serial.println(apIP[0]);
    }
    yield();
    switch (WiFi.status())
    {
    case WL_IDLE_STATUS:
    case WL_CONNECTED:
    case 7:
      break;
    
    default:
      if (apIP[0] < 1) {
        WiFi.softAP("ESP8266_wifi_config");
        Serial.println("Starting wifi SoftAP");
        apIP = WiFi.softAPIP();
        Serial.print("IP Address: ");
        Serial.println(apIP[0]);

        indexHandler = server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/wifi.html", String(), false);});
        wifiHandler = server.on("/wifi", HTTP_POST, [](AsyncWebServerRequest *request) {
          // upon submission of wifi credentials, grab the values, turn of the WiFi 
          // hotspot, then attempt to reconnect to the wifi router.
          ssid = request->arg("ssid");
          pass = request->arg("pass");
          ipIndex = request->arg("IPIndex").toInt();
          Serial.println("received parameters");
          Serial.println(ssid);
          Serial.println(pass);
          Serial.println(ipIndex);
          // save values to EEPROM 
          EEPROM.put(ssidAddr, ssid);
          EEPROM.put(passAddr, pass);
          EEPROM.put(ipIndexAddr, ipIndex);
          EEPROM.commit();
          // turn off wifi hotspot
          WiFi.softAPdisconnect(true);
          apIP[0] = 0;
          Serial.println("stopped softAP");
          WiFi.begin(ssid, pass);
          Serial.println("started WiFi");
        });

        server.begin();
      }
    }
  }

  Serial.print("Connected: ");
  Serial.println(WiFi.status());
  localIP = WiFi.localIP();
  Serial.println(localIP);
  localIP[3] = ipIndex;
  gateway = localIP;
  gateway[3] = 1;
  Serial.print("gateway=");
  Serial.println(gateway);
  WiFi.config(localIP, gateway, subnet);
  Serial.println(localIP);

  // server.removeHandler(&indexHandler);
  // server.removeHandler(&wifiHandler);
  server.reset();
  indexHandler = server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", String(), false);});
  server.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
}


/*

void writeStrToEEPROM(unsigned int addr, String str) {
  int len = str.length(); 
  Serial.print("len=");
  Serial.println(len);
  for (int i=0;i<len;i++) {
    EEPROM.write(addr+i, str[i]);
  }
  EEPROM.write(addr+len, 0);
}

void readStrFromEEPROM(unsigned int addr, String str) {
  byte ch;
  int i = 0;
  do
  {
    ch = EEPROM.read(addr+i);
    Serial.println(ch);
    str.setCharAt(addr+i, ch);
    i++;
  } while (ch);
  Serial.print("str in funcion = ");
  Serial.println(str);
}


void f1(String *str) {
  // *str = "username2";
  // modifying an Arduino C++ string through pass by reference
  str->setCharAt(3, 'z');
}

  ssid = "username1";
  ssid[2] = 'z';
  f1(&ssid);
  Serial.print("ssid=");
  Serial.println(ssid);


  ssid = "username1";
  pass = "password1";
  EEPROM.put(ssidAddr, ssid);
  Serial.print("ssid=");
  Serial.println(newSsid);
  EEPROM.get(ssidAddr, newSsid);
  Serial.print("ssid=");
  Serial.println(newSsid);


  WiFi.softAP("ESP8266_wifi_config");
  Serial.println("Starting wifi SoftAP");

  delay(10000);
  Serial.println("Stopping wifi SoftAP and starting STA");
  WiFi.softAPdisconnect(true);
  ssid = String("QUE-STARLINK");
  pass = String("Quefamily01259");
  delay(500);
  WiFi.begin(ssid, pass);

  Serial.print("IP address: "); 
  Serial.println(WiFi.localIP()); 
  delay(5000);
  Serial.print("IP address: "); 
  Serial.println(WiFi.localIP()); 
*/