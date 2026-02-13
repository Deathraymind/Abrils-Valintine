#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFiClientSecure.h>
#include "time.h"

// --- Config ---
const char* ssid = "OpenWrt";
const char* password = "greentara";
const char* jsonUrl = "https://api.npoint.io/72bdf243cede70fee88b";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 32400; // Tokyo JST
const int   daylightOffset_sec = 0;

#define SDA_PIN 25
#define SCL_PIN 33
#define RED_PIN 18
#define GREEN_PIN 19
#define BLUE_PIN 5

Adafruit_SSD1306 display(128, 64, &Wire, -1);

// Defaults & JSON Variables
String currentMessage = "I love you Abril from Bowyn";
String ledMode = "glow";
int targetR = 255, targetG = 0, targetB = 0;
int maxBrightness = 150;
bool showHearts = true;
bool timeSyncEnabled = true;

// Time Schedule Variables (Minutes from Midnight)
int dimStart = 1200;   // 8:00 PM
int dimEnd = 1320;     // 10:00 PM
int brightStart = 330; // 5:30 AM
int brightEnd = 450;   // 7:30 AM

// Alert Variables
unsigned long alertStartTime = 0;
bool isAlerting = false;

int scrollX = 128;
unsigned long lastCheck = 0;
int heartX[6], heartY[6], heartSize[6], heartDirection[6];

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  
  ledcAttach(RED_PIN, 5000, 8);
  ledcAttach(GREEN_PIN, 5000, 8);
  ledcAttach(BLUE_PIN, 5000, 8);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setRotation(2);
  display.setTextWrap(false);

  for(int i = 0; i < 6; i++) {
    heartX[i] = random(0, 120); heartY[i] = random(0, 60);
    heartSize[i] = random(5, 10); heartDirection[i] = 1;
  }

  WiFi.begin(ssid, password);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void drawPulseHeart(int x, int y, int size) {
  if (size < 3) return;
  int half = size / 2;
  display.fillCircle(x - half/2, y, half/2 + 1, WHITE); 
  display.fillCircle(x + half/2, y, half/2 + 1, WHITE); 
  display.fillTriangle(x - size/2, y, x + size/2, y, x, y + size, WHITE);
}

void loop() {
  // 1. SYNC LOGIC
  if (millis() - lastCheck > 10000) {
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClientSecure client;
      client.setInsecure(); 
      HTTPClient http;
      if (http.begin(client, jsonUrl)) {
        if (http.GET() == HTTP_CODE_OK) {
          StaticJsonDocument<1024> doc;
          deserializeJson(doc, http.getString());
          
          String newMessage = doc["message"] | "I love you Abril from Bowyn";
          if (currentMessage != "" && newMessage != currentMessage) {
            isAlerting = true;
            alertStartTime = millis();
          }
          currentMessage = newMessage;
          ledMode = doc["mode"] | "glow";
          targetR = doc["r"] | 255;
          targetG = doc["g"] | 0;
          targetB = doc["b"] | 0;
          maxBrightness = doc["brightness"] | 150;
          showHearts = doc["hearts"] | true;
          timeSyncEnabled = doc["time_sync_enabled"] | false;
          
          dimStart = doc["dim_start"] | 1200;
          dimEnd = doc["dim_end"] | 1320;
          brightStart = doc["bright_start"] | 330;
          brightEnd = doc["bright_end"] | 450;
        }
        http.end();
      }
    }
    lastCheck = millis();
  }

  // 2. TIME CALCULATION
  struct tm timeinfo;
  int currentMaxBrightness = maxBrightness;
  int totalMins = -1;
  bool timeSynced = (timeSyncEnabled && getLocalTime(&timeinfo));
  
  if (timeSynced) {
    totalMins = timeinfo.tm_hour * 60 + timeinfo.tm_min;

    if (totalMins >= dimStart && totalMins <= dimEnd) {
      float progress = (float)(totalMins - dimStart) / (dimEnd - dimStart);
      currentMaxBrightness = map(progress * 100, 0, 100, maxBrightness, 10);
    } 
    else if (totalMins > dimEnd || totalMins < brightStart) {
      currentMaxBrightness = 10; 
    }
    else if (totalMins >= brightStart && totalMins <= brightEnd) {
      float progress = (float)(totalMins - brightStart) / (brightEnd - brightStart);
      currentMaxBrightness = map(progress * 100, 0, 100, 10, maxBrightness);
    }
  }

  // 3. LED LOGIC (With Strobe Priority)
  int rOut, gOut, bOut;
  if (isAlerting && (millis() - alertStartTime < 15000)) {
    // STROBE PRIORITY: Use maxBrightness from JSON, ignore time-based dimming
    int strobe = (millis() % 200 < 100) ? maxBrightness : 0;
    rOut = (targetR > 0) ? strobe : 0;
    gOut = (targetG > 0) ? strobe : 0;
    bOut = (targetB > 0) ? strobe : 0;
  } else {
    isAlerting = false;
    float breath = (ledMode == "glow") ? (exp(sin(millis() / 2500.0 * PI)) - 0.36787944) * 108.0 : 255.0;
    float factor = map(breath, 0, 255, 10, currentMaxBrightness) / 255.0;
    rOut = targetR * factor;
    gOut = targetG * factor;
    bOut = targetB * factor;
  }
  ledcWrite(RED_PIN, rOut);
  ledcWrite(GREEN_PIN, gOut);
  ledcWrite(BLUE_PIN, bOut);

  // 4. DISPLAY (With Time-Based Greetings)
  display.clearDisplay();
  if (showHearts) {
    for(int i = 0; i < 6; i++) {
      drawPulseHeart(heartX[i], heartY[i], heartSize[i]);
      if (millis() % 100 < 20) {
        heartSize[i] += heartDirection[i];
        if (heartSize[i] >= 9 || heartSize[i] <= 5) heartDirection[i] *= -1;
      }
    }
  }

  String displayString = currentMessage;
  if (timeSynced) {
    if (totalMins >= brightStart && totalMins <= brightEnd) {
      displayString = "Good Morning Abril! " + currentMessage;
    } else if (totalMins >= dimStart && totalMins <= dimEnd) {
      displayString = "Goodnight Abril! " + currentMessage;
    }
  }

  display.setTextSize(4);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(scrollX, 16);
  display.print(displayString);
  display.display();

  scrollX -= 3;
  if (scrollX < (int)(displayString.length() * -24)) {
    scrollX = 128;
    for(int i = 0; i < 6; i++) { heartX[i] = random(0, 128); heartY[i] = random(0, 64); }
  }
  delay(10);
}
