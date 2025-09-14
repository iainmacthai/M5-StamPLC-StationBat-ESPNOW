// M5StationBat-Relay-ESPNOW.ino
// Station Bat device - Controller with magnetometer and ESP-NOW communication

#include <M5Unified.h>
#include <esp_now.h>
#include <WiFi.h>
#include "smallFont.h"
#include "midFont.h"
#include "newFont.h"
#include <Adafruit_QMC5883P.h>
#include <Wire.h>
#include <FastLED.h>

// FastLED Configuration
#define NEO_PIXEL_PIN 4
#define NEO_PIXEL_NUM 7
CRGB NeoPixel[NEO_PIXEL_NUM];

// Magnetometer Configuration
qmc5883p_range_t ranges[] = {QMC5883P_RANGE_2G, QMC5883P_RANGE_8G, QMC5883P_RANGE_12G, QMC5883P_RANGE_30G};
const char* rangeNames[] = {"2G", "8G", "12G", "30G"};
int currentRangeIndex = 1;  // Start with 8G

// ESPNOW Configuration
esp_now_peer_info_t peerInfo;
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Data structure for ESP-NOW communication
typedef struct struct_message {
  uint8_t command;     // 0 = state update, 1 = request state
  uint8_t relayState;  // Current relay state (0-4)
} struct_message;

struct_message outgoingMessage;
struct_message incomingMessage;

// Display sprite for UI
LGFX_Sprite* sprite;

// Color definitions
unsigned short blue = 0x0250;
unsigned short lightblue = 0x0BD0;
unsigned short darkblue = 0x0063;
unsigned short green = 0x0C61;
unsigned short red = 0xD061;

// UI Layout Parameters
int relayBoxWidth = 50;
int relayBoxHeight = 30;
int relayMargin = 10;
int relayStartX = 5;
int relayY = 135 - relayBoxHeight - 25;

int inputBoxWidth = 50;
int inputBoxHeight = 30;
int inputMargin = 10;
int inputStartX = 5;
int inputRow1Y = 40;
int inputRow2Y = 75;

int batteryWidth = 30;
int batteryHeight = 12;
int batteryX = 200;
int batteryY = 5;
int batteryTipWidth = 2;
int batteryTipHeight = 6;

int titleBarHeight = 34;
int titleBarWidth = 140;
int titleBarColor = lightblue;
int titleTextColor = WHITE;

int footerHeight = 20;
int footerColor = darkblue;
int footerTextColor = WHITE;

unsigned short relayColor[2] = {red, green};
unsigned short inputColor[2] = {red, green};

// Page Definitions
enum Page {
  PAGE_STAMPLC_RELAYS = 0,
  PAGE_MAGNETOMETER,
  PAGE_MOTION_ALARMS,
  PAGE_STAMPLC_INPUTS,
  PAGE_STATION_PORT_A1,
  PAGE_STATION_PORT_B1,
  PAGE_STATION_PORT_C1,
  PAGE_STATION_PORT_A2,
  PAGE_STATION_PORT_B2,
  PAGE_STATION_PORT_C2,
  PAGE_SETTINGS,
  PAGE_COUNT
};

const char* pageTitles[PAGE_COUNT] = {
  "StamPLC Relays", "Magnetometer",   "Motion Alarms",   "StamPLC Inputs", "Station Port A1",
  "Station Port B1", "Station Port C1", "Station Port A2", "Station Port B2", "Station Port C2",
  "Settings"};

// Button labels for each page
const char* buttonLabels[PAGE_COUNT][3] = {
  {"Page", "Refresh", "Off"},  // StamPLC Relays
  {"Page", "Mode", "Range"},   // Magnetometer
  {"Page", "", ""},            // Motion Alarms
  {"Page", "", ""},            // StamPLC Inputs
  {"Page", "", ""},            // Station Port A1
  {"Page", "", ""},            // Station Port B1
  {"Page", "", ""},            // Station Port C1
  {"Page", "", ""},            // Station Port A2
  {"Page", "", ""},            // Station Port B2
  {"Page", "", ""},            // Station Port C2
  {"Page", "", ""}             // Settings
};

int currentPage = PAGE_STAMPLC_RELAYS;

// Magnetometer variables
Adafruit_QMC5883P qmc;
bool magnetometerFound = false;
bool showRawValues = true;  // Toggle between raw and Gauss values

// Relay state management
int relayState = 0;  // 0: all off, 1-4: respective relay on
bool relayStates[4] = {false, false, false, false};

// Input state management (placeholder)
bool inputStates[8] = {false, false, false, false, false, false, false, false};

// Battery status variables
bool isCharging = false;
int batteryLevel = 0;
unsigned long lastBatteryUpdate = 0;
const unsigned long BATTERY_UPDATE_INTERVAL = 2000;  // Update every 2 seconds

// ESPNOW callback when data is sent
void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// ESPNOW callback when data is received
void OnDataRecv(const esp_now_recv_info_t* info, const uint8_t* incomingData, int len) {
  memcpy(&incomingMessage, incomingData, sizeof(incomingMessage));
  Serial.print("Bytes received: ");
  Serial.println(len);

  if (incomingMessage.command == 0) {
    // State update received
    relayState = incomingMessage.relayState;
    // Update display if on relays page
    if (currentPage == PAGE_STAMPLC_RELAYS) {
      updateDisplay();
    }
  } else if (incomingMessage.command == 1) {
    // State request received - send current state
    outgoingMessage.command = 0;
    outgoingMessage.relayState = relayState;
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)&outgoingMessage, sizeof(outgoingMessage));
  }
}

// Update battery status from M5Stack power management
void updateBatteryStatus() {
  isCharging = M5.Power.isCharging();
  batteryLevel = M5.Power.getBatteryLevel();
  lastBatteryUpdate = millis();
}

// Update RGB LEDs based on current page and status
void updateLEDs() {
  // Set all port LEDs to orange by default
  for (int i = 0; i < 7; i++) {
    if (i != 3) {  // Skip power LED
      NeoPixel[i] = CRGB(255, 165, 0);  // Orange
    }
  }

  // Set power LED based on charging status
  NeoPixel[3] = isCharging ? CRGB(0, 255, 0) : CRGB(0, 0, 255);  // Green/Blue

  // Set LED for current page to blue
  switch (currentPage) {
    case PAGE_STATION_PORT_A1: NeoPixel[0] = CRGB(0, 0, 255); break;  // RGB1
    case PAGE_STATION_PORT_B1: NeoPixel[1] = CRGB(0, 0, 255); break;  // RGB2
    case PAGE_STATION_PORT_C1: NeoPixel[2] = CRGB(0, 0, 255); break;  // RGB3
    case PAGE_STATION_PORT_A2: NeoPixel[6] = CRGB(0, 0, 255); break;  // RGB7
    case PAGE_STATION_PORT_B2: NeoPixel[5] = CRGB(0, 0, 255); break;  // RGB6
    case PAGE_STATION_PORT_C2: NeoPixel[4] = CRGB(0, 0, 255); break;  // RGB5
    default: break;  // Keep default orange for other pages
  }

  FastLED.show();
}

// Draw battery indicator on display
void drawBatteryIndicator() {
  // Draw battery outline
  sprite->drawRect(batteryX, batteryY, batteryWidth, batteryHeight, WHITE);

  // Draw battery tip
  sprite->fillRect(batteryX + batteryWidth, batteryY + (batteryHeight - batteryTipHeight) / 2, 
                  batteryTipWidth, batteryTipHeight, WHITE);

  // Determine battery color
  unsigned short batteryColor;
  if (isCharging) {
    batteryColor = green;
  } else if (batteryLevel < 20) {
    batteryColor = red;
  } else {
    batteryColor = blue;
  }

  // Calculate fill width based on battery level
  int fillWidth = (batteryLevel * (batteryWidth - 2)) / 100;

  // Draw battery fill
  if (fillWidth > 0) {
    sprite->fillRect(batteryX + 1, batteryY + 1, fillWidth, batteryHeight - 2, batteryColor);
  }

  // Draw battery percentage
  sprite->setTextColor(WHITE, BLACK);
  sprite->loadFont(midFont);
  sprite->setTextDatum(MC_DATUM);
  sprite->drawString(String(batteryLevel) + "%", batteryX + batteryWidth / 2, batteryY + batteryHeight + 10);
  sprite->unloadFont();
}

// Draw title bar with page title
void drawTitleBar() {
  // Draw title bar background
  sprite->fillRect(0, 0, titleBarWidth, titleBarHeight, titleBarColor);

  // Draw page title
  sprite->setTextColor(titleTextColor, titleBarColor);
  sprite->loadFont(newFont);
  sprite->setTextDatum(ML_DATUM);

  // Check if text fits, fall back to smaller font if needed
  int textWidth = sprite->textWidth(pageTitles[currentPage]);
  if (textWidth > titleBarWidth - 10) {
    sprite->unloadFont();
    sprite->loadFont(midFont);
  }

  // Center text vertically and draw
  int textY = titleBarHeight / 2;
  sprite->drawString(pageTitles[currentPage], 5, textY);
  sprite->unloadFont();
}

// Draw footer with button labels
void drawFooter() {
  // Draw footer background
  sprite->fillRect(0, 135 - footerHeight, 240, footerHeight, footerColor);

  // Draw button labels
  sprite->setTextColor(footerTextColor, footerColor);
  sprite->loadFont(midFont);
  sprite->setTextDatum(MC_DATUM);

  // Position labels under each button
  sprite->drawString(buttonLabels[currentPage][0], 40, 135 - footerHeight / 2);   // Button A
  sprite->drawString(buttonLabels[currentPage][1], 120, 135 - footerHeight / 2);  // Button B
  sprite->drawString(buttonLabels[currentPage][2], 200, 135 - footerHeight / 2);  // Button C

  sprite->unloadFont();
}

// Draw StamPLC relays page
void drawStamPLCRelaysPage() {
  for (int i = 0; i < 4; i++) {
    int x = relayStartX + i * (relayBoxWidth + relayMargin);

    // Draw relay box
    sprite->fillSmoothRoundRect(x, relayY, relayBoxWidth, relayBoxHeight, 2, darkblue);

    // Draw status indicator
    sprite->fillRect(x + 4, relayY + 3, relayBoxWidth - 8, 5, relayColor[relayStates[i]]);

    // Draw relay label
    sprite->setTextColor(0xCE59, darkblue);
    sprite->loadFont(midFont);
    sprite->setTextDatum(MC_DATUM);
    sprite->drawString("RL" + String(i + 1), x + relayBoxWidth / 2, relayY + relayBoxHeight / 2 + 5);
  }
}

// Draw StamPLC inputs page (placeholder)
void drawStamPLCInputsPage() {
  for (int i = 0; i < 8; i++) {
    int row = i < 4 ? 0 : 1;
    int col = i % 4;
    int x = inputStartX + col * (inputBoxWidth + inputMargin);
    int y = row == 0 ? inputRow1Y : inputRow2Y;

    // Draw input box
    sprite->fillSmoothRoundRect(x, y, inputBoxWidth, inputBoxHeight, 2, darkblue);

    // Draw status indicator
    sprite->fillRect(x + 4, y + 3, inputBoxWidth - 8, 5, inputColor[inputStates[i]]);

    // Draw input label
    sprite->setTextColor(0xCE59, darkblue);
    sprite->loadFont(midFont);
    sprite->setTextDatum(MC_DATUM);
    sprite->drawString("IP-" + String(i + 1), x + inputBoxWidth / 2, y + inputBoxHeight / 2 + 5);
  }
}

// Draw magnetometer data page
void drawMagnetometerPage() {
  if (!magnetometerFound) {
    sprite->setTextColor(WHITE, BLACK);
    sprite->loadFont(newFont);
    sprite->setTextDatum(MC_DATUM);
    sprite->drawString("Magnetometer Not Found", 120, 70);
    sprite->unloadFont();
    return;
  }

  // Read magnetometer data
  int16_t x, y, z;
  float gx, gy, gz;

  if (qmc.getRawMagnetic(&x, &y, &z) && qmc.getGaussField(&gx, &gy, &gz)) {
    sprite->setTextColor(WHITE, BLACK);
    sprite->loadFont(newFont);
    sprite->setTextDatum(ML_DATUM);

    // Display mode and range
    sprite->drawString(showRawValues ? "Raw Values" : "Gauss Values", 135, 75);
    sprite->drawString("Range: " + String(rangeNames[currentRangeIndex]), 145, 55);

    // Display values
    if (showRawValues) {
      sprite->drawString("X: " + String(x), 10, 50);
      sprite->drawString("Y: " + String(y), 10, 75);
      sprite->drawString("Z: " + String(z), 10, 100);
    } else {
      sprite->drawString("X: " + String(gx, 3), 10, 50);
      sprite->drawString("Y: " + String(gy, 3), 10, 75);
      sprite->drawString("Z: " + String(gz, 3), 10, 100);
    }

    sprite->unloadFont();
  } else {
    sprite->setTextColor(WHITE, BLACK);
    sprite->loadFont(newFont);
    sprite->setTextDatum(MC_DATUM);
    sprite->drawString("Failed to read data", 120, 70);
    sprite->unloadFont();
  }
}

// Placeholder functions for other pages
void drawMotionAlarmsPage() {
  sprite->setTextColor(WHITE, BLACK);
  sprite->loadFont(midFont);
  sprite->setTextDatum(MC_DATUM);
  sprite->drawString("Motion Alarms", 120, 70);
  sprite->unloadFont();
}

void drawStationPortA1Page() {
  sprite->setTextColor(WHITE, BLACK);
  sprite->loadFont(midFont);
  sprite->setTextDatum(MC_DATUM);
  sprite->drawString("Port A1 Data", 120, 70);
  sprite->unloadFont();
}

// Similar placeholder functions for other port pages...
void drawStationPortB1Page() { /* Implementation */ }
void drawStationPortC1Page() { /* Implementation */ }
void drawStationPortA2Page() { /* Implementation */ }
void drawStationPortB2Page() { /* Implementation */ }
void drawStationPortC2Page() { /* Implementation */ }
void drawSettingsPage() { /* Implementation */ }

// Update display based on current page
void updateDisplay() {
  // Update relay states array
  for (int i = 0; i < 4; i++) {
    relayStates[i] = (relayState == i + 1);
  }

  // Draw the display
  drawCurrentPage();
}

// Draw current page content
void drawCurrentPage() {
  sprite->fillSprite(BLACK);  // Clear screen

  // Draw common elements
  drawTitleBar();
  drawBatteryIndicator();

  // Draw page-specific content
  switch (currentPage) {
    case PAGE_STAMPLC_RELAYS: drawStamPLCRelaysPage(); break;
    case PAGE_MAGNETOMETER: drawMagnetometerPage(); break;
    case PAGE_MOTION_ALARMS: drawMotionAlarmsPage(); break;
    case PAGE_STAMPLC_INPUTS: drawStamPLCInputsPage(); break;
    case PAGE_STATION_PORT_A1: drawStationPortA1Page(); break;
    case PAGE_STATION_PORT_B1: drawStationPortB1Page(); break;
    case PAGE_STATION_PORT_C1: drawStationPortC1Page(); break;
    case PAGE_STATION_PORT_A2: drawStationPortA2Page(); break;
    case PAGE_STATION_PORT_B2: drawStationPortB2Page(); break;
    case PAGE_STATION_PORT_C2: drawStationPortC2Page(); break;
    case PAGE_SETTINGS: drawSettingsPage(); break;
  }

  // Draw footer
  drawFooter();

  sprite->pushSprite(0, 0);
  sprite->unloadFont();
}

// Send state update via ESP-NOW
void sendStateUpdate() {
  outgoingMessage.command = 0;
  outgoingMessage.relayState = relayState;
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)&outgoingMessage, sizeof(outgoingMessage));
}

// Request state from StamPLC
void requestState() {
  outgoingMessage.command = 1;
  outgoingMessage.relayState = 0;
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)&outgoingMessage, sizeof(outgoingMessage));
}

// Initialize ESP-NOW communication
void setupESPNOW() {
  // Set device as Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register callbacks
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  // Initialize I2C for Port A1 (SDA=32, SCL=33)
  Wire.begin(32, 33);

  // Initialize NeoPixel LEDs
  FastLED.addLeds<SK6812, NEO_PIXEL_PIN, GRB>(NeoPixel, NEO_PIXEL_NUM);
  FastLED.setBrightness(60);

  // Initialize magnetometer
  if (qmc.begin()) {
    magnetometerFound = true;
    Serial.println("QMC5883P Found!");

    // Configure magnetometer
    qmc.setMode(QMC5883P_MODE_NORMAL);
    qmc.setODR(QMC5883P_ODR_50HZ);
    qmc.setOSR(QMC5883P_OSR_4);
    qmc.setDSR(QMC5883P_DSR_2);
    qmc.setRange(ranges[currentRangeIndex]);
    qmc.setSetResetMode(QMC5883P_SETRESET_ON);
  } else {
    Serial.println("Failed to find QMC5883P chip");
  }

  // Initialize display
  M5.Display.init();
  M5.Display.setRotation(1);
  M5.Display.setBrightness(255);

  // Create display sprite
  sprite = new LGFX_Sprite(&M5.Display);
  sprite->createSprite(240, 135);

  Serial.begin(115200);

  // Initialize ESP-NOW
  setupESPNOW();

  // Get initial battery status
  updateBatteryStatus();

  // Initialize display and LEDs
  updateDisplay();
  updateLEDs();

  // Request current state from StamPLC
  requestState();
}

void loop() {
  M5.update();

  // Update battery status periodically
  if (millis() - lastBatteryUpdate >= BATTERY_UPDATE_INTERVAL) {
    updateBatteryStatus();
    updateDisplay();
    updateLEDs();
  }

  // Update magnetometer display if on that page
  if (currentPage == PAGE_MAGNETOMETER) {
    updateDisplay();
  }

  // Button A: Cycle through pages
  if (M5.BtnA.wasPressed()) {
    currentPage = (currentPage + 1) % PAGE_COUNT;
    updateDisplay();
    updateLEDs();
  }

  // Button B: Toggle between raw/Gauss values on magnetometer page
  if (M5.BtnB.wasPressed()) {
    if (currentPage == PAGE_MAGNETOMETER) {
      showRawValues = !showRawValues;
      updateDisplay();
    }
  }

  // Button C: Cycle magnetometer ranges or toggle relays off
  if (M5.BtnC.wasPressed()) {
    if (currentPage == PAGE_MAGNETOMETER) {
      // Cycle to next range
      currentRangeIndex = (currentRangeIndex + 1) % 4;
      qmc.setRange(ranges[currentRangeIndex]);
      updateDisplay();
    } else if (currentPage == PAGE_STAMPLC_RELAYS) {
      // Turn all relays off
      relayState = 0;
      updateDisplay();
      sendStateUpdate();
    }
  }

  delay(50);
}