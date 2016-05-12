#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include "FS.h"

//Link config http://192.168.4.1/config?ssid=Icosmetic.vn&pass=00000000&host=192.168.1.69 
/* Set these to your desired credentials. */
const char *myssid = "ESPKeNaNa";
const char *mypassword = "kennguyen";
//IPAddress myip(192, 168, 1, 96); 

char* ssid = NULL;
char* password = NULL;
char* host = NULL;


ESP8266WebServer server(80);
int beginTimeout;
boolean conWifiStt = false;


bool loadConfig() {
  Serial.println("Loading config");
  File configFile = SPIFFS.open("/config.txt", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }

  String t1 = configFile.readStringUntil('\n');
  String t2 = configFile.readStringUntil('\n');
  String t3 = configFile.readStringUntil('\n');
  configFile.close();

  Serial.println(t1 + " " + t2 + " " + t3);

  clearWifiInfo();
  clearHost();

  ssid = new char[t1.length()]; 
  t1.toCharArray(ssid, t1.length());
  password = new char[t2.length()]; 
  t2.toCharArray(password, t2.length());
  host = new char[t3.length()];
  t3.toCharArray(host, t3.length());
  
  return true;
}

void handleRoot() {
  String mes = "Link config http://192.168.4.1/config?ssid=TenWifi&pass=MatKhau&host=192.168.1.69";
  server.send ( 200, "text/html", mes );

}

void handleNotFound() {
  String message = server.uri() + " Not Found\n\n";
  server.send ( 404, "text/plain", message );
}

void handleConfig(){
  String host, ssid, pass;
  for ( uint8_t i = 0; i < server.args(); i++ ) {
    if (server.argName(i).equals("ssid")){
      ssid = server.arg(i);
    } else if (server.argName(i).equals("host")){
      host = server.arg(i);
    } else if (server.argName(i).equals("pass")){
      pass = server.arg(i);
    }
  }

  Serial.println(ssid);
  Serial.println(pass);
  Serial.println(host);

  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    server.send(200, "text/plain", "Mount failed");
    return;
  } else {
    File configFile = SPIFFS.open("/config.txt", "w");
    if (!configFile) {
      Serial.println("Failed to open config file for writing");
      server.send(200, "text/plain", "Open config file failed");
      return;
    }
    configFile.println(ssid);
    configFile.println(pass);
    configFile.println(host);
    configFile.close();
  }
  server.send(200, "text/plain", "Config OKAY");
//  ESP.restart();
  setupAPSTA();
}

boolean connectWifi(){
  WiFi.disconnect();
  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(false);
  if (!SPIFFS.begin()) {
      Serial.println("Failed to mount file system");
      return false;
  } else {
      if (!loadConfig()) {
          Serial.println("Failed to load config");
          return false;
      } else {
          Serial.println("Config loaded");
          WiFi.begin(ssid, password);
          beginTimeout = 30;
          while (WiFi.status() != WL_CONNECTED) {
            delay(1000);
            beginTimeout --;
            if (beginTimeout == 0){
              clearWifiInfo();
              clearHost();
              return false;
            }
            Serial.println(beginTimeout);
          }
          WiFi.setAutoReconnect(true);
          Serial.println("WiFi connected");
          Serial.println(WiFi.localIP());
      }
  }
  return true;
}

void clearWifiInfo(){
  if (ssid && *ssid != 0x00){
    delete[] ssid;
  }
  if (password && *password != 0x00){
    delete[] password;
  }
}

void clearHost(){
  if (host && *host != 0x00){
    delete[] host;
  }
}

void setupAPSTA(){
  WiFi.mode(WIFI_AP_STA);
  WiFi.setPhyMode(WIFI_PHY_MODE_11N);
  
  Serial.print("Setup access point");
  
  conWifiStt = connectWifi();

  WiFi.softAP(myssid, mypassword, WiFi.channel());
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/config", handleConfig);
  server.onNotFound ( handleNotFound );
  server.begin();
}

void setup() {
  Serial.begin(115200);
  delay(3000);

  setupAPSTA();
  
  pinMode(5, INPUT);
}


int pin = 0;
const int httpPort = 6690;
WiFiClient sendClient;
boolean connected = false;

void testPIR(){
  int pin = 0;
  long t = 0;
  while(true){
    if (pin != digitalRead(5)){
      pin = 1 - pin;
      Serial.print(pin);
      Serial.print(" : ");
      t = millis() - t;
      Serial.println(t);
      t = millis();  
    }
    delay(1);
  }
}

void loop() {
  delay(300);
  server.handleClient();

  if (conWifiStt){
    Serial.println("Start check send client");
    if (!sendClient.connected() && host && sendClient.connect(host, httpPort)) {
      connected = true;
      sendClient.print("Start Read data\n");  
      Serial.println("Connected to " + String(host));
    }
    Serial.println("Start check GPIO5");
//    if (sendClient.connected()){
//      if(pin != digitalRead(5)) {
//        pin = 1 - pin;
//        String a = String(pin);
//        Serial.println(a);
//        sendClient.print(a + "\n");
//      }
//    }
    long t = millis();
    while(sendClient.connected()){
      if (pin != digitalRead(5)){
        pin = 1 - pin;
        Serial.print(pin);
        Serial.print(" : ");
        t = millis() - t;
        Serial.println(t);
        t = millis();  
        sendClient.print(String(pin) + "\n");
      }
      delay(1);
    }
  }
}

