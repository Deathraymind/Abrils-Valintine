#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFiClientSecure.h>

// --- Config ---
const char* ssid = "OpenWrt";
const char* password = "greentara";
const char* jsonUrl = "https://raw.githubusercontent.com/Deathraymind/Abrils-Valintine/refs/heads/main/message.json";

#define SDA_PIN 25
#define SCL_PIN 33
#define RED_PIN 18
#define GREEN_PIN 19
#define BLUE_PIN 5

Adafruit_SSD1306 display(128, 64, &Wire, -1);

// Defaults
String currentMessage = "I love you Abril from Bowyn";
String ledMode = "glow";
int targetR = 255, targetG = 0, targetB = 0;
int maxBrightness = 150;
bool showHearts = true;

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
    heartX[i] = random(0, 120);
    heartY[i] = random(0, 60);
    heartSize[i] = random(5, 10);
    heartDirection[i] = 1;
  }

  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
}

void drawPulseHeart(int x, int y, int size) {
  if (size < 3) return;
  int half = size / 2;
  display.fillCircle(x - half/2, y, half/2 + 1, WHITE); 
  display.fillCircle(x + half/2, y, half/2 + 1, WHITE); 
  display.fillTriangle(x - size/2, y, x + size/2, y, x, y + size, WHITE);
}

void loop() {
  // 1. SYNC LOGIC with Debugging
  if (millis() - lastCheck > 10000) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Checking GitHub...");
      WiFiClientSecure client;
      client.setInsecure(); // Bypass SSL thumbprint check
      
      HTTPClient http;
      if (http.begin(client, jsonUrl)) {
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
          Serial.println("Received payload: " + payload);
          
          StaticJsonDocument<1024> doc; // Increased size slightly
          DeserializationError error = deserializeJson(doc, payload);
          
          if (!error) {
            currentMessage = doc["message"] | "I love you Abril from Bowyn";
            ledMode = doc["mode"] | "glow";
            targetR = doc["r"] | 255;
            targetG = doc["g"] | 0;
            targetB = doc["b"] | 0;
            maxBrightness = doc["brightness"] | 150;
            showHearts = doc["hearts"] | true;
            Serial.println("Sync Successful!");
          } else {
            Serial.print("JSON Parse Error: ");
            Serial.println(error.c_str());
          }
        } else {
          Serial.printf("HTTP Error Code: %d\n", httpCode);
        }
        http.end();
      }
    } else {
      Serial.println("WiFi Disconnected. Reconnecting...");
      WiFi.begin(ssid, password);
    }
    lastCheck = millis();
  }

  // 2. LED LOGIC
  int rOut, gOut, bOut;
  if (ledMode == "glow") {
    float breath = (exp(sin(millis() / 2500.0 * PI)) - 0.36787944) * 108.0;
    float factor = map(breath, 0, 255, 10, maxBrightness) / 255.0;
    rOut = targetR * factor;
    gOut = targetG * factor;
    bOut = targetB * factor;
  } else {
    float factor = maxBrightness / 255.0;
    rOut = targetR * factor;
    gOut = targetG * factor;
    bOut = targetB * factor;
  }
  ledcWrite(RED_PIN, rOut);
  ledcWrite(GREEN_PIN, gOut);
  ledcWrite(BLUE_PIN, bOut);

  // 3. DISPLAY LOGIC (Hearts Background, Text Foreground)
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

  display.setTextSize(4);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(scrollX, 16);
  display.print(currentMessage);
  display.display();

  scrollX -= 3;
  if (scrollX < (int)(currentMessage.length() * -24)) {
    scrollX = 128;
    for(int i = 0; i < 6; i++) {
      heartX[i] = random(0, 128);
      heartY[i] = random(0, 64);
    }
  }
  delay(10);
}
