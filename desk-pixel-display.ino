#include <PxMatrix.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "time.h"
#include "chars.h"
#include "wifi.h"

// ------- Wifi ----------------
// ssid + password are stored in wifi.h (as const char*)
const char* ntpServer = "pool.ntp.org";
const long gmtOffsetSec = 3600;
const int daylightOffsetSec = 3600;

// ------- Sensor --------------
// I2C (SCL -> 22, SDA -> 21)
Adafruit_BME680 bme;

// ------- Pixel Display -------
#define PxMATRIX_MAX_HEIGHT 32
#define PxMATRIX_MAX_WIDTH 64
#define P_LAT 2
#define P_A 19
#define P_B 23
#define P_C 18
#define P_D 5
#define P_E 15
#define P_OE 16
// Display R1 > 13
// Display CLK > 14

// On the Display, connect input to output
// G1	-> R2, B1 -> G2, R2 -> R1, G2 -> G1, B2 -> B1

PxMATRIX display(PxMATRIX_MAX_WIDTH, PxMATRIX_MAX_HEIGHT, P_LAT, P_OE, P_A, P_B, P_C, P_D, P_E);

uint16_t RED = display.color565(255, 0, 0);
uint16_t GREEN = display.color565(0, 255, 0);
uint16_t BLUE = display.color565(0, 0, 255);
uint16_t WHITE = display.color565(255, 255, 255);
uint16_t YELLOW = display.color565(255, 255, 0);
uint16_t CYAN = display.color565(0, 255, 255);
uint16_t MAGENTA = display.color565(255, 0, 255);

unsigned long whenRefresh = 0;
char* errMsg = "";

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("Desk Pixel Display");

  // Init Wifi + and get time
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(" connected");
  configTime(gmtOffsetSec, daylightOffsetSec, ntpServer);
  getTime();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  // Init Display
  display.begin(16);
  display.setBrightness(50);
  display.clearDisplay();
  display.setTextSize(1);

  // Init Temperature sensor
  if (!bme.begin()) {
    Serial.println("Could not find BME680 sensor!");
    errMsg = "BME680 error";
  } else {
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
  }
  Serial.println("Set-up done");
}

void loop() {
  errMsg = "";
  struct tm timeinfo = getTime();
  updateTemperatureValues();

  display.clearDisplay();

  // Display date + time
  int offsetX = 3;
  int offsetY = 1;
  offsetX = displayChar(offsetX, offsetY, (timeinfo.tm_mday / 10), GREEN);
  offsetX = displayChar(offsetX, offsetY, (timeinfo.tm_mday % 10), GREEN);
  offsetX = displayChar(offsetX, offsetY, POINT, GREEN);
  offsetX = displayChar(offsetX, offsetY, ((timeinfo.tm_mon + 1) / 10), GREEN);
  offsetX = displayChar(offsetX, offsetY, ((timeinfo.tm_mon + 1) % 10), GREEN);
  offsetX = displayChar(offsetX, offsetY, POINT, GREEN);
  offsetX = displayChar(offsetX, offsetY, ((timeinfo.tm_year / 10) % 10), GREEN);
  offsetX = displayChar(offsetX, offsetY, (timeinfo.tm_year % 10), GREEN);

  offsetX = 42;
  offsetX = displayChar(offsetX, offsetY, (timeinfo.tm_hour / 10), CYAN);
  offsetX = displayChar(offsetX, offsetY, (timeinfo.tm_hour % 10), CYAN);
  offsetX = displayChar(offsetX, offsetY, COLON, CYAN);
  offsetX = displayChar(offsetX, offsetY, (timeinfo.tm_min / 10), CYAN);
  offsetX = displayChar(offsetX, offsetY, (timeinfo.tm_min % 10), CYAN);

  // Display Sensor data
  offsetX = 3;
  offsetY = 26;

  uint16_t color = BLUE;
  if (bme.temperature > 22 && bme.temperature < 25) {
    color = YELLOW;
  } else if (bme.temperature >= 25) {
    color = RED;
  }
  int temparatureAsInt = (int)(bme.temperature * 10);  // x 10 to have one digit after comma
  offsetX = displayChar(offsetX, offsetY, (temparatureAsInt / 100), color);
  offsetX = displayChar(offsetX, offsetY, ((temparatureAsInt / 10) % 10), color);
  offsetX = displayChar(offsetX, offsetY, POINT, color);
  offsetX = displayChar(offsetX, offsetY, (temparatureAsInt % 10), color);
  offsetX = displayChar(offsetX, offsetY, CELSIUS, color);
  offsetX = displayChar(offsetX, offsetY, SPACE, color);

  int humidityAsInt = (int)(bme.humidity);
  offsetX = 50;
  offsetX = displayChar(offsetX, offsetY, (humidityAsInt / 10), BLUE);
  offsetX = displayChar(offsetX, offsetY, (humidityAsInt % 10), BLUE);
  offsetX = displayChar(offsetX, offsetY, PERCENT, BLUE);
  offsetX = displayChar(offsetX, offsetY, SPACE, BLUE);

  // Display text
  display.setTextColor(WHITE);
  display.setCursor(3, 8);
  display.print("Hello");

  display.setCursor(3, 17);
  if (strlen(errMsg) == 0) {
    display.setTextColor(MAGENTA);
    display.print(&timeinfo, "%A");
  } else {
    display.setTextColor(RED);
    display.print(errMsg);
  }

  // Refresh every minute
  whenRefresh = millis() + (60 - timeinfo.tm_sec) * 1000;
  while (millis() < whenRefresh) {
    display.display(20);
  }
}

void updateTemperatureValues() {
  if (!bme.beginReading()) {
    Serial.println("BME680: Failed to begin reading");
    errMsg = "BME error";
    return;
  }

  if (!bme.endReading()) {
    Serial.println("BME680: Failed to complete reading");
    errMsg = "BME error";
    return;
  }
  Serial.print("Temperature: ");
  Serial.println(bme.temperature);
}

tm getTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    errMsg = "Getting time error";
  }
  Serial.println(&timeinfo, "%A, %d %B %Y %H:%M:%S");
  return timeinfo;
}

int displayChar(int offsetX, int offsetY, int charPos, uint16_t color) {
  display.setCursor(offsetX, offsetY);
  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 3; x++) {
      if (chars[charPos][y][x] == 1) {
        display.drawPixel(offsetX + x, offsetY + y, color);
      }
    }
  }
  return offsetX + 4;
}