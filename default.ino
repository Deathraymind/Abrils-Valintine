#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// --- Display Pins ---
#define SDA_PIN 25
#define SCL_PIN 33

// --- RGB Strip Pins (MOSFET Gates) ---
#define RED_PIN 18
#define GREEN_PIN 19
#define BLUE_PIN 5

// --- PWM Settings ---
const int freq = 5000;
const int resolution = 8; 

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int scrollX = SCREEN_WIDTH; 
String message = "I love you Abril from Bowyn";

// Heart properties
int heartX[6];
int heartY[6];
int heartSize[6];    
int heartDirection[6]; 

void setup() {
  // 1. Initialize I2C for the OLED
  Wire.begin(SDA_PIN, SCL_PIN); 

  // 2. Initialize RGB PWM Channels
  ledcAttach(RED_PIN, freq, resolution);
  ledcAttach(GREEN_PIN, freq, resolution);
  ledcAttach(BLUE_PIN, freq, resolution);

  // 3. Start Display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;); // Halt if screen not found
  }
  
  display.setRotation(2); // Flip screen 180 degrees
  display.setTextWrap(false);
  
  // 4. Randomize Heart Positions
  for(int i = 0; i < 6; i++) {
    heartX[i] = random(10, SCREEN_WIDTH - 10);
    heartY[i] = random(10, SCREEN_HEIGHT - 10);
    heartSize[i] = random(5, 10); 
    heartDirection[i] = (random(0, 2) == 0) ? 1 : -1;
  }
}

// Helper function to update LED strip
void setStripColor(int r, int g, int b) {
  ledcWrite(RED_PIN, r);
  ledcWrite(GREEN_PIN, g);
  ledcWrite(BLUE_PIN, b);
}

// Helper function to draw hearts
void drawPulseHeart(int x, int y, int size) {
  if (size < 3) return;
  int half = size / 2;
  display.fillCircle(x - half/2, y, half/2 + 1, WHITE); 
  display.fillCircle(x + half/2, y, half/2 + 1, WHITE); 
  display.fillTriangle(x - size/2, y, x + size/2, y, x, y + size, WHITE);
}

void loop() {
  display.clearDisplay();

  // --- SOFT BREATHING RED GLOW ---
  // Speed: Increase 3000.0 to go slower, decrease to go faster
  float breath = (exp(sin(millis() / 3000.0 * PI)) - 0.36787944) * 108.0;
  
  // Brightness: Max is 150 (not 255) to keep it "less harsh"
  int redValue = map(breath, 0, 255, 5, 150); 
  setStripColor(redValue, 0, 0); 

  // --- DRAW PULSING HEARTS ---
  for(int i = 0; i < 6; i++) {
    drawPulseHeart(heartX[i], heartY[i], heartSize[i]);
    if (millis() % 100 < 20) { 
      heartSize[i] += heartDirection[i];
      if (heartSize[i] >= 9) heartDirection[i] = -1;
      if (heartSize[i] <= 5) heartDirection[i] = 1;
    }
  }

  // --- DRAW SCROLLING TEXT ---
  display.setTextSize(4); 
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(scrollX, 16); 
  display.print(message);

  display.display();
  
  // Text scroll speed
  scrollX -= 3; 
  if (scrollX < -850) { 
    scrollX = SCREEN_WIDTH;
    // Refresh heart positions when message restarts
    for(int i = 0; i < 6; i++) {
      heartX[i] = random(10, SCREEN_WIDTH - 10);
      heartY[i] = random(10, SCREEN_HEIGHT - 10);
    }
  }
  
  delay(10); 
}
