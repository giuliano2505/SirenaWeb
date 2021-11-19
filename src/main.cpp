#include <Arduino.h>
#include <ESP8266TimeAlarms.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>



//WiFi settings of AP
#ifndef WIFI_CONFIG_H
#define WIFI_SSID "WIFI-Arnet"
#define WIFI_PASSWD "Topo3788"
#endif // !WIFI_CONFIG_H

//Pin for bell ringing (on ESP-01s relay shield is GPIO0)
#define BELL_PIN 2

//WebServer for OTA updates and webinfo
AsyncWebServer server(80);


// Functions for bell ringing:
void bellOff(){
  digitalWrite(BELL_PIN,HIGH);
}
void shortCall(){
  Serial.println("Toque Corto");
  digitalWrite(BELL_PIN,LOW);
  Alarm.timerOnce(3,bellOff);
}
void longCall(){
  Serial.println("Toque Largo");
  digitalWrite(BELL_PIN,LOW);
  Alarm.timerOnce(10,bellOff);
}

void digitalClockDisplay() {
  time_t tnow = time(nullptr);
  Serial.print(ctime(&tnow));
}

void setup() {
  //Little delay for safety reasons
  delay(1000);
  Serial.begin(115200);
  Serial.println();

  //Initilization and shut down relay (inverse logic)
  pinMode(BELL_PIN,OUTPUT);
  digitalWrite(BELL_PIN,HIGH);

  //Set up WiFi as Station
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID,WIFI_PASSWD);


  //Setup NTP server address
  configTime(0, 0, "time.windows.com");
  //configTime(0, 0, "0.se.pool.ntp.org");
  //configTime(0, 0, "time.google.com");
  
  //And timezone
  setenv("TZ", "<-03>3", 1);
  tzset();

  //Display clock time beafore and after wifi conection
  Serial.print("Clock before sync: ");
  digitalClockDisplay();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nClock after Wifi: ");
  digitalClockDisplay();

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());


  //Start http server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hola Juan Carlos, como estas chupapijas?");
  });
  
  AsyncElegantOTA.begin(&server);    // Start ElegantOTA binded to server
  server.begin();
  Serial.println("HTTP server started");


  // create the alarms, to trigger at specific times
  Alarm.timerRepeat(10,digitalClockDisplay);
  Alarm.alarmRepeat(6,0,0, longCall);
  Alarm.alarmRepeat(7,0,0, longCall);
  Alarm.alarmRepeat(9,0,0, shortCall);
  Alarm.alarmRepeat(9,20,0, shortCall);
  Alarm.alarmRepeat(9,40,0, shortCall);
  Alarm.alarmRepeat(10,0,0, shortCall);
  Alarm.alarmRepeat(12,0,0, shortCall);
  Alarm.alarmRepeat(12,20,0, shortCall);
  Alarm.alarmRepeat(12,40,0, shortCall);
  Alarm.alarmRepeat(13,0,0, shortCall);
  Alarm.alarmRepeat(15,25,0, longCall);
}

void loop() {
  //digitalClockDisplay();
  Alarm.delay(1); // wait one second between clock display
}