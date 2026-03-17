#include <TFT_eSPI.h>  // TFT library for ESP32
#include <SPI.h>
#include <WiFi.h>
#include "time.h"
#include "passwords.h" // Wi-Fi credentials (not included in public repo)

// Wi-Fi credentials
const char* ssid     = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// NTP server and timezone
const char* ntpServer = "pl.pool.ntp.org";
const long  gmtOffset_sec = 3600;       
const int   daylightOffset_sec = 3600;  

// Arrays for current time and date digits
uint8_t currentTimeDigits[4] = {0,0,0,0};
uint8_t currentDateDigits[4] = {0,0,0,0};

// Arrays for previous values (used to avoid unnecessary redraw)
uint8_t prevTimeDigits[4] = {10,10,10,10};
uint8_t prevDateDigits[4] = {32,32,13,13};

uint8_t weekday = 0;
uint8_t prevWeekday = 7; 

// Weekday names
const char* dayNames[7] = {
  "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
};

// TFT display object
TFT_eSPI tft = TFT_eSPI();
uint16_t bmoColor = tft.color565(79, 254, 160); // Custom color

// Convert number to 7-segment binary representation
uint8_t numToBinary(uint8_t num){
  switch(num){
    case 0: return 0b1111110;
    case 1: return 0b0110000;
    case 2: return 0b1101101;
    case 3: return 0b1111001;
    case 4: return 0b0110011;
    case 5: return 0b1011011;
    case 6: return 0b1011111;
    case 7: return 0b1110000;
    case 8: return 0b1111111;
    case 9: return 0b1110011;
    case 10:return 0b0000000; // blank
  }
  return 0b0000000;
}

// Draw a single 7-segment digit at position (x,y)
void drawDigit(int x, int y, uint8_t num, int size, int thickness){
  uint8_t y_coords[7] = {2*size,size,0,0,0,size,size};
  uint8_t x_coords[7] = {0,size,size,0,0,0,0};
  bool vertical[7] = {0,1,1,0,1,1,0};

  // Clear previous digit
  tft.fillRect(x, y, 2*size + thickness, size + thickness, TFT_BLACK);

  uint8_t binary = numToBinary(num);
  for (int i = 0; i < 7; i++){ 
    if (binary & (1 << (6-i))){
      if (vertical[i]){
        tft.fillRect(x + x_coords[i], y + y_coords[i], size + thickness, thickness, bmoColor);
      } else {
        tft.fillRect(x + x_coords[i], y + y_coords[i], thickness, size + thickness, bmoColor);
      }
    }
  }
}

// Display multiple digits if they changed
void showDigits(int x, int y, int size, int thickness, uint8_t digits[], uint8_t prevDigits[]){
  for (int i = 0; i < 4; i++){
    if (digits[i] != prevDigits[i]){
      int offset = i * (size + thickness + 10) + ((i>1)? 2*thickness : 0);
      drawDigit(x + offset, y, (digits[i] ? digits[i] : 10), size, thickness);
      prevDigits[i] = digits[i];
    }
  }
}

void setup(){
  // Initialize TFT display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);          
  tft.setTextColor(bmoColor);

  // Display startup message
  tft.println("                   RTC Clock");
  tft.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  // Wait until Wi-Fi is connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    tft.print(".");
  }
  tft.println();
  tft.println("WiFi connected!");

  // Configure NTP time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Clear screen for clock display
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
}

void loop(){
  struct tm timeInfo;
  if(getLocalTime(&timeInfo)){
    // Update current time and date digits
    currentTimeDigits[0] = timeInfo.tm_hour / 10;
    currentTimeDigits[1] = timeInfo.tm_hour % 10;
    currentTimeDigits[2] = timeInfo.tm_min / 10;
    currentTimeDigits[3] = timeInfo.tm_min % 10;

    currentDateDigits[0] = timeInfo.tm_mday / 10;
    currentDateDigits[1] = timeInfo.tm_mday % 10;
    currentDateDigits[2] = timeInfo.tm_mon / 10;
    currentDateDigits[3] = (timeInfo.tm_mon % 10) + 1;

    weekday = timeInfo.tm_wday;

    // Display time and date
    showDigits(12, 117, 81, 17, currentTimeDigits, prevTimeDigits);
    showDigits(315, 25, 24, 6, currentDateDigits, prevDateDigits);

    // Display weekday if changed
    if(weekday != prevWeekday){
      tft.setRotation(1);
      tft.setCursor(205, 253);
      tft.setTextColor(bmoColor);
      tft.setTextSize(6);
      tft.print(dayNames[weekday-1]);
      prevWeekday = weekday;
      tft.setRotation(0);
    }
  } else {
    tft.println("Unable to get local time");
  }

  delay(1000); // Update every second