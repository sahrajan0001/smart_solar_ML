#include <WiFi.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include <Adafruit_GFX.h> 
#include <Adafruit_ST7735.h> 
#include <Firebase_ESP_Client.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "time.h"

#define TFT_CS         5   // Case select connected to pin 5
#define TFT_RST        15  // Reset connected to pin 15
#define TFT_DC         32  // AO connected to pin 32
#define TFT_MOSI       23  // Data = SDA connected to pin 23
#define TFT_SCLK       18  // Clock = SCK connected to pin 18

HardwareSerial MPPTSerial(1); 

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

float batteryVoltage;           // Battery voltage (Volts)
float panelVoltage;             // PV Panel voltage (Volts)
float chargeCurrent;            // Charging current (Amps)
float panelPower;               // Panel power (Watts)
float Yieldtoday;               // Yield today
float Yieldtotal;               // Yield total
int chargeStatus;               // Charging status
String mpptMode;                // MPPT Mode
String operationStatus;         // Operational status or flags
IPAddress localIP;
String getIPAddress(IPAddress ip) {
  return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
}

char buf[45];                   // Buffer for reading serial data

struct tm timeinfo;

const char *ssid = "WAVLINK-N";
const char *password = "Sunshine60";

const char *api_key = "AIzaSyAGsuhAQbsAsW_21grJx9DtuqHIzEOX19s";
const char *db_url = "https://esp-weather-client-default-rtdb.europe-west1.firebasedatabase.app";

const char *errorMsg = "";

unsigned int ttfDisplayTimer = 0;
unsigned int dataUpdateTimer = 0;

FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;
unsigned int count = 0;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

// Create a server on port 80
WiFiServer server(80);

void MPPTParser();

void setupFirebase();
void saveDataToFirebase();

void setupTTFScreen();
void updateTTFScreen();

void sendClientData();

void getLocalTime();

void setup() {
  Serial.begin(19200);

  MPPTSerial.begin(19200, SERIAL_8N1, 16, 17);
  Serial.println("Starting MPPT data read...");

  setupTTFScreen();
  
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  localIP = WiFi.localIP();
  Serial.println(WiFi.localIP());

  setupFirebase();

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  getLocalTime();

  server.begin();
}

void loop() {
  MPPTParser();
  saveDataToFirebase();
  updateTTFScreen();
  sendClientData();
}

void setupTTFScreen(){
  tft.initR(INITR_BLACKTAB);      
  Serial.println("Initialized");
  // Rotate the display by 90 degrees
  tft.setRotation(1);  
  // Clear the screen
  tft.fillScreen(ST77XX_BLACK);
}

void getLocalTime()
{
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void updateTTFScreen(){
  if (millis() - ttfDisplayTimer >= 1000) {
    ttfDisplayTimer = millis();
    Serial.println("Updating TFT screen with new data.");

    // Clear the screen before displaying new data
    tft.fillScreen(ST77XX_BLACK);
    
    // Display Battery Voltage
    tft.setTextWrap(false);
    tft.setCursor(0, 0);
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(1.5);
    tft.print("Battery Voltage: ");
    tft.print(batteryVoltage, 1);
    tft.println("V");

    // Display Charge Current
    tft.setCursor(0, 15);  
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(1.5);
    tft.print("Charge Current: ");
    tft.print(chargeCurrent, 1);
    tft.println("A");

    // Display Panel Voltage
    tft.setCursor(0, 30);
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(1.5);
    tft.print("Panel Voltage: ");
    tft.print(panelVoltage, 1);
    tft.println("V");

    // Display Panel Power
    tft.setCursor(0, 45);
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(1.5);
    tft.print("Power: ");
    tft.print(panelPower, 1);
    tft.println("W");

    // Display Yield Today
    tft.setCursor(0, 60);
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(1.5);
    tft.print("Yield Today: ");
    tft.print(Yieldtoday, 1);
    tft.println("kWh");

    // Display Yield Total
    tft.setCursor(0, 75);
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(1.5);
    tft.print("Yield Total: ");
    tft.print(Yieldtotal, 1);
    tft.println("kWh");

    // Display IP Address
    tft.setCursor(0, 90);
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(1.5);
    tft.print("IP Address: ");
    tft.println(getIPAddress(localIP));
  }
}


void setupFirebase(){
  /* Assign the api key (required) */
  config.api_key = api_key;

  /* Assign the RTDB URL (required) */
  config.database_url = db_url;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void saveDataToFirebase(){
  if(millis() - dataUpdateTimer > 5000){
    dataUpdateTimer = millis();
    if (Firebase.ready() && signupOK){
      getLocalTime();
      String timeStamp = String(millis()); // You can replace this with a real timestamp if you have one

      // Create a JSON object to hold your data
      FirebaseJson json;

      json.set("date", String(1900 + timeinfo.tm_year) + "-" + String(1 + timeinfo.tm_mon) + "-" + String(timeinfo.tm_mday));
      json.set("time", String(timeinfo.tm_hour) + ":" + String(timeinfo.tm_min) + ":" + String(timeinfo.tm_sec));

      json.set("datas/[0]/data","batteryVoltage");
      json.set("datas/[0]/value",batteryVoltage);

      json.set("datas/[1]/data","panelVoltage");
      json.set("datas/[1]/value",panelVoltage);

      json.set("datas/[2]/data","chargeCurrent");
      json.set("datas/[2]/value",chargeCurrent);

      json.set("datas/[3]/data","panelPower");
      json.set("datas/[3]/value",panelPower);

      json.set("datas/[4]/data","yieldToday");
      json.set("datas/[4]/value",yieldToday);
      
      // Construct the path for storing data with the timestamp as the key
      String path = "mppt_data/" + timeStamp;

      // Send the JSON object to Firebase
      if (Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json)) {
        Serial.println("PASSED");
        Serial.println("PATH: " + fbdo.dataPath());
        Serial.println("TYPE: " + fbdo.dataType());
      } else {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo.errorReason());
      }
    }
  }
}

void MPPTParser() {
  if (MPPTSerial.available()) {
    // Read the raw data from MPPT and print it to Serial Monitor for debugging
    String rawData = MPPTSerial.readStringUntil('\n');
    Serial.print("Raw Data: "); Serial.println(rawData);

    // Attempt to parse label and value
    int tabPos = rawData.indexOf('\t'); // Find the position of the tab character
    if (tabPos > 0) {
      String label = rawData.substring(0, tabPos);   // Extract the label
      String val = rawData.substring(tabPos + 1);    // Extract the value after the tab

      // Convert the value based on the label
      val.toCharArray(buf, sizeof(buf));  // Convert the value to a char array


      // Only process data if it matches one of the specified labels
      if (label == "V") {   // Battery Voltage
        batteryVoltage = atof(buf) / 1000;  // Convert to float and scale to volts
      } 
      else if (label == "I") {   // Charging Current
        chargeCurrent = atof(buf) / 1000;
      } 
      else if (label == "VPV") {   // PV Panel Voltage
        panelVoltage = atof(buf) / 1000;
      } 
      else if (label == "PPV") {   // PV Panel Power
        panelPower = atof(buf);
      } 
      else if (label == "H19") {   // Yield Total
        Yieldtotal = atof(buf) / 100;
      } 
      else if (label == "H20") {   // Yield Today
        Yieldtoday = atof(buf) / 100;
      } 
      else {
        Serial.print("Ignored: Unknown data label received - "); Serial.println(label);
      }
      
    } else {
      // Error if data couldn't be parsed properly
      Serial.println("Error parsing data from MPPT: No tab character found.");
    }
  } else {
    // No data available
    Serial.println("No new data from MPPT.");
  }
}


void sendClientData(){
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New client connected");
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        if (c == '\n') {
          if (currentLine.length() == 0) {
            
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/json");
            client.println("Access-Control-Allow-Origin: *");
            client.println("Access-Control-Allow-Headers: Content-Type");
            client.println("Connection: close");
            client.println();

            StaticJsonDocument<200> doc;

            doc["data"][0]["data"] = "temperature";
            doc["data"][0]["value"] = 20; // Read sensor data here
            doc["data"][0]["unit"] = "Celsius";
            doc["data"][0]["error"] = errorMsg;

            doc["data"][1]["data"] = "chargeCurrent";
            doc["data"][1]["value"] = chargeCurrent; // Read sensor data here
            doc["data"][1]["unit"] = "Ampere";
            doc["data"][1]["error"] = errorMsg;

            doc["data"][2]["data"] = "panelVoltage";
            doc["data"][2]["value"] = panelVoltage; // Read sensor data here
            doc["data"][2]["unit"] = "volt";
            doc["data"][2]["error"] = errorMsg;

            doc["data"][3]["data"] = "batteryVoltage";
            doc["data"][3]["value"] = batteryVoltage; // Read sensor data here
            doc["data"][3]["unit"] = "volt";
            doc["data"][3]["error"] = errorMsg;

            doc["data"][4]["data"] = "panelPower";
            doc["data"][4]["value"] = panelPower; // Read sensor data here
            doc["data"][4]["unit"] = "Watt";
            doc["data"][4]["error"] = errorMsg;

            doc["data"][5]["data"] = "yieldToday";
            doc["data"][5]["value"] = Yieldtoday; // Read sensor data here
            doc["data"][5]["unit"] = "Watt";
            doc["data"][5]["error"] = errorMsg;

            doc["data"][6]["data"] = "yieldTotal";
            doc["data"][6]["value"] = Yieldtotal; // Read sensor data here
            doc["data"][6]["unit"] = "Watt";
            doc["data"][6]["error"] = errorMsg;

            serializeJson(doc, client);
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    // Close the connection
    client.stop();
    Serial.println("Client disconnected");
  }
}
