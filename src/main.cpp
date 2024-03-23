#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h> 
#include <EEPROM.h>

#define START_ADDR 0x0
#define BTN1 D3

// read from the EEPROM. If the EEPROM contents are valid, it must be 1024.
// If it is any other value, EEPROM contents are invalid.
int checkNum; 
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

AsyncWebHandler indexHandler, wifiGetHandler, wifiPostHandler; 
unsigned long printDelay = 1000;
unsigned long lastTimePrinted; 

void writeStrToEEPROM(unsigned int addr, String str);
void readStrFromEEPROM(unsigned int addr, String *str);
void getWiFiEEPROMValid();
void checkResetButton(int btn);
void validateWiFiEEPROM();
void resetWiFiEEPROM();
void readWiFiEEPROM();
void saveWiFi(AsyncWebServerRequest *request);
void checkWiFiLoop();


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


void writeStrToEEPROM(unsigned int addr, String str) {
  int len = str.length(); 
  const char * c_str = str.c_str();
  for (int i=0;i<len;i++) {
    EEPROM.write(addr+i, c_str[i]);
  }
  EEPROM.write(addr+len, 0);
  EEPROM.commit();
}

void readStrFromEEPROM(unsigned int addr, String *str) {
  byte ch; int i = 0;
  *str = "";
  do {
    ch = EEPROM.read(addr+i);
    if (ch) *str += (char) ch;
    i++;
  } while (ch);
}

/*
  read from the EEPROM. If the EEPROM contents are valid, it must be 1024.
  If it is any other value, EEPROM contents are invalid, then the EEPROM 
  will be set to default valid string values. 
*/
void getWiFiEEPROMValid() {
  EEPROM.get(checkAddr, checkNum);
}

// reset wifi credentials if button is held LOW on power on
void checkResetButton(int btn) {
  // add 3 second delay to reset the EEPROM manually  
  delay(3000);
  if (!digitalRead(btn)) {
    checkNum = 0;
  }
}

void validateWiFiEEPROM() {
  if (checkNum != 1024) {
    Serial.println("Resetting WiFi EEPROM");
    resetWiFiEEPROM();
  }
  else {
    Serial.println("Reading WiFi EEPROM");
    readWiFiEEPROM();
  }
}

void resetWiFiEEPROM() {
  ssid = "wifi";
  pass = "password";
  EEPROM.put(checkAddr, 1024);
  writeStrToEEPROM(ssidAddr, ssid);
  writeStrToEEPROM(passAddr, pass);
  EEPROM.put(ipIndexAddr, 2);
  EEPROM.commit();
  Serial.println("EEPROM initialized!");
}

void readWiFiEEPROM() {
  readStrFromEEPROM(ssidAddr, &ssid);
  readStrFromEEPROM(passAddr, &pass);
  EEPROM.get(ipIndexAddr, ipIndex);
  Serial.print("ssid=");
  Serial.println(ssid);
  Serial.print("pass=");
  Serial.println(pass);
  Serial.print("ipIndex=");
  Serial.println(ipIndex);
  Serial.println("EEPROM read!");
}

// upon submission of wifi credentials, grab the values, turn of the WiFi 
// hotspot, then attempt to reconnect to the wifi router.
void saveWiFi(AsyncWebServerRequest *request) {
  ssid = request->arg("ssid");
  pass = request->arg("pass");
  ipIndex = request->arg("IPIndex").toInt();
  Serial.println("received parameters");
  Serial.println(ssid);
  Serial.println(pass);
  Serial.println(ipIndex);
  // save values to EEPROM 
  EEPROM.put(checkAddr, 1024);
  writeStrToEEPROM(ssidAddr, ssid);
  writeStrToEEPROM(passAddr, pass);
  EEPROM.put(ipIndexAddr, ipIndex);
  EEPROM.commit();
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
void checkWiFiLoop() {
  while (WiFi.status() != WL_CONNECTED) {
    // print wifi status
    if (millis() - lastTimePrinted > printDelay) {
      lastTimePrinted = millis(); 
      Serial.print("not connected: ");
      Serial.println(WiFi.status());
    }
    // service the background WiFi stack
    yield();
    switch (WiFi.status())
    {
      // if credentials from EEPROM are invalid, reset the WiFi credentials on the 
      // EEPROM
      case WL_IDLE_STATUS:
        resetWiFiEEPROM();
        WiFi.begin(ssid, pass);
        break;

      // if attempting to connect
      case 7:
        break;

      // if connected 
      case WL_CONNECTED:
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

        // read from EEPROM again
        readStrFromEEPROM(ssidAddr, &newSsid);
        Serial.print("newSSID=");
        Serial.println(newSsid);
        delay(100);
        // server.removeHandler(&indexHandler);
        // server.removeHandler(&wifiHandler);
        server.reset();
        indexHandler = server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
          request->send(LittleFS, "/index.html", String(), false);});
        wifiGetHandler = server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
          request->send(LittleFS, "/wifi.html", String(), false);});
        wifiPostHandler = server.on("/wifi", HTTP_POST, [](AsyncWebServerRequest *request) {
          saveWiFi(request);
        });
        server.begin();
        break;

      // if not connected due to unavailable SSID or wrong credentials
      default:
        if (apIP[0] < 1) {
          WiFi.softAP("ESP8266_wifi_config");
          Serial.println("Starting wifi SoftAP");
          apIP = WiFi.softAPIP();
          Serial.print("IP Address: ");
          Serial.println(apIP[0]);

          indexHandler = server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
          request->send(LittleFS, "/wifi.html", String(), false);});
          wifiPostHandler = server.on("/wifi", HTTP_POST, [](AsyncWebServerRequest *request) {
            saveWiFi(request);
              // turn off wifi hotspot and attempt to reconnect to the saved WiFi network
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
}

/*
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
*/