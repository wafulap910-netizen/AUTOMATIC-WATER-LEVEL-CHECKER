#define BLYNK_TEMPLATE_ID "TMPL20YFT0C6G"
#define BLYNK_TEMPLATE_NAME "Water level checker "
#define BLYNK_AUTH_TOKEN "irkcfRGj-W9oZikVQ0OZ6YiPgTubL68j"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <WebServer.h>

// --- PIN DEFINITIONS ---
#define I2C_SDA 21
#define I2C_SCL 22
#define LED_RED     5   
#define LED_YELLOW 19  
#define LED_GREEN  18  
const int trigPin = 17; 
const int echoPin = 16; 

// --- CALIBRATION ---
const int totalDepth = 23;      // Distance from sensor to tank bottom (cm)
const int maxWaterLevel = 20;   // Your 100% mark (cm)

LiquidCrystal_I2C lcd(0x27, 16, 2); 
WebServer server(80);
BlynkTimer timer;

const char* ssid     = "Dankash";
const char* password = "kashD092";

float currentLevel = 0;
int currentPercentage = 0;

// --- WEB DASHBOARD ---
void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'><meta http-equiv='refresh' content='2'>";
  html += "<style>body{font-family:sans-serif; background:#0e1621; color:white; text-align:center; padding:20px;}";
  html += ".box{background:#17212b; padding:25px; border-radius:20px; border-top:5px solid #00BFFF; display:inline-block; box-shadow: 0 10px 20px rgba(0,0,0,0.5);}";
  html += ".lvl{font-size:50px; color:#00BFFF; font-weight:bold; margin:10px 0;}";
  html += ".info{color:#FFD700; font-size:18px;}</style></head><body>";
  html += "<div class='box'><h2>LIVE TANK MONITOR</h2><div class='lvl'>" + String(currentPercentage) + "%</div>";
  html += "<div class='info'>" + String(currentLevel, 1) + " CM</div>";
  html += "<hr style='border:0.5px solid #333'><p style='font-size:12px; color:#555;'>IP ADDRESS: " + WiFi.localIP().toString() + "</p></div></body></html>";
  server.send(200, "text/html", html);
}

// --- SENSOR PROCESSING ---
void readSensor() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 30000); 
  float distance = (duration > 0) ? (duration * 0.034 / 2) : totalDepth;

  // Level Calculation
  currentLevel = (float)totalDepth - distance; 
  currentLevel = constrain(currentLevel, 0, maxWaterLevel); 
  currentPercentage = (currentLevel / (float)maxWaterLevel) * 100;

  // LED Control
  digitalWrite(LED_RED, (currentPercentage <= 20));
  digitalWrite(LED_YELLOW, (currentPercentage > 20 && currentPercentage <= 80));
  digitalWrite(LED_GREEN, (currentPercentage > 80));

  // Sync to Blynk
  if(Blynk.connected()) {
    Blynk.virtualWrite(V0, currentPercentage); 
    Blynk.virtualWrite(V1, currentLevel);
  }
  
  // Update LCD
  lcd.setCursor(0, 0);
  lcd.print("IP:"); lcd.print(WiFi.localIP().toString());
  lcd.print("    "); // Clear artifacts
  lcd.setCursor(0, 1);
  lcd.print("L:"); lcd.print(currentLevel, 1); lcd.print("cm ");
  lcd.setCursor(10, 1);
  lcd.print(currentPercentage); lcd.print("%  ");
}

void setup() {
  Serial.begin(115200);
  
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  Wire.begin(I2C_SDA, I2C_SCL);
  lcd.init();
  lcd.backlight();
  lcd.print("CONNECTING WIFI");

  WiFi.begin(ssid, password);
  
  // Quick attempt to connect
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED && counter < 15) {
    delay(500);
    Serial.print(".");
    counter++;
  }

  Blynk.config(BLYNK_AUTH_TOKEN);
  server.on("/", handleRoot);
  server.begin();
  
  lcd.clear();
  timer.setInterval(1500L, readSensor); // Update every 1.5 seconds
}

void loop() {
  // Only run Blynk/Server if WiFi is active to prevent crashes
  if (WiFi.status() == WL_CONNECTED) {
    Blynk.run();
    server.handleClient();
  }
  timer.run();
}