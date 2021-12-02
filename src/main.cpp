#define BELL_PIN 0
#define AP_SSID "SirenaWeb"
#define AP_PASS "Sirena1234"

#include <Arduino.h>
#include <ESP8266TimeAlarms.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <FS.h> // Include the SPIFFS library

ESP8266WebServer server(80); // Create a webserver object that listens for HTTP request on port 80

File fsUploadFile; // a File object to temporarily store the received file
File fsConfigFile;
File fsAlarmsFile;

String getContentType(String filename); // Convert the file extension to the MIME type
bool handleFileRead(String path);       // Send the right file to the client (if it exists)
void handleFileUpload();                // Upload a new file to the SPIFFS
void timedCall();                       //
void bellOff();                         //
void digitalClockDisplay();
int setupWiFi(const char *filename);
void startAPmode();

int alarmDuration[30];

void setup()
{
  Serial.begin(115200); // Start the Serial communication to send messages to the computer
  delay(2000);
  Serial.println('\n');

  pinMode(BELL_PIN, OUTPUT);
  digitalWrite(BELL_PIN, LOW);

  if (!SPIFFS.begin()) // Init the SPIFFS
  {
    Serial.println("Error initialization SPIFSS");
  }

  delay(10);
  if (setupWiFi("/wifi_config.json"))
  {
    Serial.println("Error while starting Wi-Fi");
    startAPmode();
  }

  if (WiFi.getMode() == WIFI_STA)
  {
    Serial.println("Connecting ...");
    int i = 0;
    while (WiFi.status() != WL_CONNECTED)
    { // Wait for the Wi-Fi to connect
      delay(1000);
      Serial.print(++i);
      Serial.print(' ');
      if (i > 60)
      {
        startAPmode();
        break;
      }
    }
    Serial.println('\n');
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID()); // Tell us what network we're connected to
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP()); // Send the IP address of the ESP8266 to the computer
  }
  if (WiFi.getMode() == WIFI_AP)
  {
    Serial.println('\n');
    Serial.println("AP mode started");
    Serial.println("Please Connect to : ");
    Serial.print("SSID:\t");
    Serial.println(WiFi.softAPSSID()); // Tell us what network we need to connect
    Serial.print("PASS:\t");
    Serial.println(WiFi.softAPPSK()); // Password
    Serial.print("IP:\t");
    Serial.println(WiFi.softAPIP()); // IP
  }

  //Setup NTP server address
  configTime(0, 0, "time.windows.com");
  //configTime(0, 0, "0.se.pool.ntp.org");
  //configTime(0, 0, "time.google.com");
  //And timezone
  setenv("TZ", "<-03>3", 1);
  tzset();

  //Display clock time after wifi conection
  digitalClockDisplay();

  if (!MDNS.begin("esp8266"))
  { // Start the mDNS responder for esp8266.local
    Serial.println("Error setting up MDNS responder!");
  }
  MDNS.addService("http", "tcp", 80);
  Serial.println("mDNS responder started");

  server.on("/upload", HTTP_GET, []() {                 // if the client requests the upload page
    if (!handleFileRead("/upload.html"))                // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  });

  server.on(
      "/upload", HTTP_POST, // if the client posts to the upload page
      []()
      {
        server.send(200);
      },               // Send status 200 (OK) to tell the client we are ready to receive
      handleFileUpload // Receive and save the file
  );

  server.on("/restart", HTTP_GET, // if client requests the restart page
            []()
            {
              ESP.restart(); // Call Restart function
            });

  server.onNotFound([]() {                              // If the client requests any URI
    if (!handleFileRead(server.uri()))                  // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  });

  server.begin(); // Actually start the server
  Serial.println("HTTP server started");

  // Settings Alarms
  if (WiFi.getMode() == WIFI_STA)
  {
    if (SPIFFS.exists("/alarms.csv")) //If exists an file with the alarms
    {
      Serial.println("Alarms File found");
      int HH, MM, SS, TiempoOn;
      fsAlarmsFile = SPIFFS.open("/alarms.csv", "r"); //Open it
      fsAlarmsFile.readStringUntil('\n');             //Read first line
      while (fsAlarmsFile.available())                //While there is info on the file
      {
        HH = fsAlarmsFile.readStringUntil(';').toDouble();
        MM = fsAlarmsFile.readStringUntil(';').toDouble();
        SS = fsAlarmsFile.readStringUntil(';').toDouble();
        TiempoOn = fsAlarmsFile.readStringUntil('\n').toDouble();
        //Set the alarm and save
        int alarmID = Alarm.alarmRepeat(HH, MM, SS, timedCall);
        if (alarmID != 255)
        {
          alarmDuration[alarmID] = TiempoOn;
        }
      }
      fsAlarmsFile.close();
    }
    else
    {
      Serial.println("Alarms File not found");
    }
    Alarm.timerRepeat(10, digitalClockDisplay);
    Serial.println("Periodic Time Alarm Created");
  }
}

void loop()
{
  MDNS.update();
  server.handleClient();
  Alarm.delay(1);
}

String getContentType(String filename)
{ // convert the file extension to the MIME type
  if (filename.endsWith(".html"))
    return "text/html";
  else if (filename.endsWith(".csv"))
    return "text/plain";
  else if (filename.endsWith(".json"))
    return "application/octet-stream";
    //return "application/json";
  return "text/plain";
}

bool handleFileRead(String path)
{ // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/"))
    path += "index.html";                    // If a folder is requested, send the index file
  String contentType = getContentType(path); // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path))
  {                                       // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))        // If there's a compressed version available
      path += ".gz";                      // Use the compressed verion
    File file = SPIFFS.open(path, "r");   // Open the file
    server.streamFile(file, contentType); // Send it to the client
    file.close();                         // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path); // If the file doesn't exist, return false
  return false;
}

void handleFileUpload()
{ // upload a new file to the SPIFFS
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START)
  {
    String filename = upload.filename;
    if (!filename.startsWith("/"))
      filename = "/" + filename;
    Serial.print("handleFileUpload Name: ");
    Serial.println(filename);
    if (SPIFFS.exists(filename))
    {
      Serial.println("\tFile Exists, deleting It");
      SPIFFS.remove(filename);
    }
    fsUploadFile = SPIFFS.open(filename, "w"); // Open the file for writing in SPIFFS (create if it doesn't exist)
    filename = String();
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    if (fsUploadFile)
    {                       // If the file was successfully created
      fsUploadFile.close(); // Close the file again
      Serial.print("handleFileUpload Size: ");
      Serial.println(upload.totalSize);
      server.sendHeader("Location", "/success.html"); // Redirect the client to the success page
      server.send(303);
    }
    else
    {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

// Functions for bell ringing:
void bellOff()
{
  digitalWrite(BELL_PIN, LOW);
}

void timedCall()
{
  digitalWrite(BELL_PIN, HIGH);
  Alarm.timerOnce(alarmDuration[Alarm.getTriggeredAlarmId()], bellOff);
}

void digitalClockDisplay()
{
  time_t tnow = time(nullptr);
  Serial.print(ctime(&tnow));
}

int setupWiFi(const char *filename)
{
  if (!SPIFFS.exists(filename))
  {
    Serial.println("Wi-FI Configuration file not found!");
    return 1;
  }
  Serial.println("Wi-Fi configuration file found!");
  StaticJsonDocument<512> doc;
  fsConfigFile = SPIFFS.open(filename, "r");
  DeserializationError error = deserializeJson(doc, fsConfigFile);
  fsConfigFile.close();
  if (error)
  {
    Serial.println("Deserialization Error: ");
    Serial.println(error.c_str());
    return 1;
  }
  if (!(doc.containsKey("SSID") && doc.containsKey("PASS")))
  {
    Serial.println("Wrong Wi-Fi configuration file!");
    return 1;
  }

  char jsonSSID[32];
  char jsonPASS[32];
  WiFi.mode(WIFI_STA);
  strcpy(jsonSSID, doc["SSID"]);
  strcpy(jsonPASS, doc["PASS"]);
  WiFi.begin(jsonSSID, jsonPASS);
  return 0;
}

void startAPmode()
{
  Serial.println("Starting AP");
  WiFi.mode(WIFI_AP);
  while (!WiFi.softAP(AP_SSID, AP_PASS))
  {
    Serial.println(".");
    delay(100);
  }
}