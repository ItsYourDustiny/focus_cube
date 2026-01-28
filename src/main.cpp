#include <Wire.h>
#include <FastIMU.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SDA_PIN 21
#define SCL_PIN 22
#define BUTTON_PIN 18      // Button connected to D18
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1      // no reset pin
#define OLED_ADDR 0x3C     // most common I2C address

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

MPU6500 IMU(Wire);
AccelData accel;
GyroData gyro;
calData calibration;

// ---- STATE VARIABLES ----
String prevFaceUp = "";
String setFaceUp = "";
int count = 0;
unsigned long timerStartTime = 0;
unsigned long totalElapsedTime = 0;
bool buttonPressed = false;
bool timerRunning = false;
String currentMode = "";

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize button pin
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED init failed");
    while (1);
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Focus Cube");
  display.setTextSize(1);
  display.setCursor(0, 32);
  display.println("Initializing...");
  display.display();

  calibration.valid = false;

  int err = IMU.init(calibration);
  if (err != 0) {
    Serial.print("IMU init failed: ");
    Serial.println(err);
    while (1);
  }

  Serial.println("Focus Cube Ready");
  delay(2000);
}

void drawTimer() {
  display.clearDisplay();
  
  // Title at top
  display.setTextSize(1);
  display.setCursor(0, 0);
  if (currentMode == "top") {
    display.println("WORK FOCUS");
  } else if (currentMode == "bottom") {
    display.println("PERSONAL FOCUS");
  } else {
    display.println("PLACE CUBE");
  }
  
  // Draw line under title
  display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
  
  if (currentMode == "top" || currentMode == "bottom") {
    // Calculate elapsed time (accumulated + current session if button pressed)
    unsigned long elapsed;
    if (buttonPressed && timerRunning) {
      unsigned long currentSessionTime = millis() - timerStartTime;
      elapsed = (totalElapsedTime + currentSessionTime) / 1000;
    } else {
      elapsed = totalElapsedTime / 1000;
    }
    
    // Format time as MM:SS
    int minutes = elapsed / 60;
    int seconds = elapsed % 60;
    
    // Large timer display
    display.setTextSize(3);
    display.setCursor(10, 20);
    
    if (minutes < 10) display.print("0");
    display.print(minutes);
    display.print(":");
    if (seconds < 10) display.print("0");
    display.print(seconds);
    
    // Small seconds counter below for more precision
    display.setTextSize(1);
    display.setCursor(45, 50);
    display.print("(");
    display.print(elapsed);
    display.print("s)");
    
  } else {
    // Show waiting message
    display.setTextSize(2);
    display.setCursor(15, 25);
    display.println("00:00");
    
    display.setTextSize(1);
    display.setCursor(20, 50);
    display.println("Position cube");
  }
  
  display.display();
}

String getFaceUp() {
  float x = accel.accelX;
  float y = accel.accelY;
  float z = accel.accelZ;

  // Only care about top and bottom for focus modes
  if (z > 0.8) return "top";      // Work focus
  if (z < -0.8) return "bottom";  // Personal focus

  return "unknown";
}

void loop() {
  IMU.update();
  IMU.getAccel(&accel);

  // Read button state (LOW when pressed due to INPUT_PULLUP)
  bool currentButtonState = digitalRead(BUTTON_PIN) == LOW;
  
  String currentFaceUp = getFaceUp();

  // Debounce orientation detection
  if (currentFaceUp == prevFaceUp) {
    count += 1;
    if (count >= 5) {  // Stable for ~1.5 seconds
      if (setFaceUp != currentFaceUp) {
        setFaceUp = currentFaceUp;
        
        // Handle mode changes
        if (currentFaceUp == "top" || currentFaceUp == "bottom") {
          if (currentMode != currentFaceUp) {
            // New mode detected - reset timer
            currentMode = currentFaceUp;
            totalElapsedTime = 0;
            timerRunning = true;
            
            Serial.print("FOCUS MODE: ");
            Serial.println(currentMode == "top" ? "WORK" : "PERSONAL");
          }
        } else {
          // Cube not in valid position - stop timer
          currentMode = "";
          timerRunning = false;
          totalElapsedTime = 0;
          Serial.println("TIMER STOPPED - Invalid position");
        }
      }
      count = 0;
    }
  } else {
    count = 1;
  }

  prevFaceUp = currentFaceUp;
  
  // Handle button press timing
  if (currentButtonState != buttonPressed) {
    buttonPressed = currentButtonState;
    
    if (buttonPressed && timerRunning) {
      // Button just pressed - start timing
      timerStartTime = millis();
      Serial.println("Timer started - button pressed");
    } else if (!buttonPressed && timerRunning) {
      // Button just released - add to total time
      totalElapsedTime += (millis() - timerStartTime);
      Serial.println("Timer paused - button released");
    }
  }
  
  // Update timer while button is held
  if (buttonPressed && timerRunning) {
    // Currently timing
  }
  
  // Update display
  drawTimer();
  
  // Debug output
  Serial.print("Face: ");
  Serial.print(currentFaceUp);
  Serial.print("  Mode: ");
  Serial.print(currentMode);
  Serial.print("  Button: ");
  Serial.print(buttonPressed ? "PRESSED" : "RELEASED");
  Serial.print("  Total: ");
  Serial.print(totalElapsedTime / 1000);
  Serial.println("s");

  delay(300);
}