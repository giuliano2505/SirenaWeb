#include <Arduino.h>

#include <ESP8266TimeAlarms.h>
#include <ESP8266WiFi.h>

#ifndef WIFI_CONFIG_H
#define YOUR_WIFI_SSID "WIFI-Arnet"
#define YOUR_WIFI_PASSWD "Topo3788"
#endif // !WIFI_CONFIG_H

#define PIN_TIMBRE 2
AlarmId id;



// Funcion para toque de sirena:
void apagadoSirena(){
  digitalWrite(PIN_TIMBRE,HIGH);
}
void toqueCorto(){
  Serial.println("Toque Corto");
  digitalWrite(PIN_TIMBRE,LOW);
  Alarm.timerOnce(5,apagadoSirena);
}
void toqueLargo(){
  Serial.println("Toque Largo");
  digitalWrite(PIN_TIMBRE,LOW);
  Alarm.timerOnce(10,apagadoSirena);
}



void digitalClockDisplay() {
  time_t tnow = time(nullptr);
  Serial.print(ctime(&tnow));

}

void setup() {
  delay(2000);
  Serial.begin(115200);
  Serial.println();

  pinMode(PIN_TIMBRE,OUTPUT);
  digitalWrite(PIN_TIMBRE,HIGH);

  WiFi.mode(WIFI_STA);
  WiFi.begin(YOUR_WIFI_SSID, YOUR_WIFI_PASSWD);

  //configTime(0, 0, "0.se.pool.ntp.org");
  //configTime(0, 0, "time.google.com");
  configTime(0, 0, "time.windows.com");
  setenv("TZ", "<-03>3", 1);
  tzset();

  Serial.print("Clock before sync: ");
  digitalClockDisplay();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("/nClock after Wifi: ");
  digitalClockDisplay();

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(YOUR_WIFI_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // create the alarms, to trigger at specific times
  Alarm.alarmRepeat(6,0,0, toqueLargo);
  Alarm.alarmRepeat(7,0,0, toqueLargo);
  Alarm.alarmRepeat(9,0,0, toqueCorto);
  Alarm.alarmRepeat(9,20,0, toqueCorto);
  Alarm.alarmRepeat(9,40,0, toqueCorto);
  Alarm.alarmRepeat(10,0,0, toqueCorto);
  Alarm.alarmRepeat(12,0,0, toqueCorto);
  Alarm.alarmRepeat(12,20,0, toqueCorto);
  Alarm.alarmRepeat(12,40,0, toqueCorto);
  Alarm.alarmRepeat(13,0,0, toqueCorto);
  Alarm.alarmRepeat(15,25,0, toqueLargo);
}

void loop() {
  digitalClockDisplay();
  //Serial.println(timeStatus());
  Alarm.delay(1000); // wait one second between clock display
}