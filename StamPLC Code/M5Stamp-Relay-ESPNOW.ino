// M5Stamp-Relay-ESPNOW.ino
// StamPLC device - Relay controller with ESP-NOW communication

#include <Arduino.h>
#include <M5StamPLC.h>
#include <esp_now.h>
#include <WiFi.h>
#include "smallFont.h"
#include "midFont.h"
#include "newFont.h"

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

// Display sprite for custom UI
LGFX_Sprite* sprite = new LGFX_Sprite(&M5StamPLC.Display);

// Color definitions
unsigned short blue = 0x0250;
unsigned short lightblue = 0x0BD0;
unsigned short darkblue = 0x0063;
unsigned short green = 0x0C61;
unsigned short red = 0xD061;

// Relay display parameters
int relayBoxWidth = 50;
int relayBoxHeight = 45;
int relayMargin = 10;
int relayY = 135 - relayBoxHeight - 5;  // Position at bottom of screen
unsigned short relayColor[2] = {red, green};  // Colors for status indicator

// Relay state management
int relayState = 0;  // 0: all off, 1-4: respective relay on
bool relayStates[4] = {false, false, false, false};

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
    // State update received - update relays
    relayState = incomingMessage.relayState;
    updateRelays();
    drawRelayBoxes();
  } else if (incomingMessage.command == 1) {
    // State request received - send current state
    outgoingMessage.command = 0;
    outgoingMessage.relayState = relayState;
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)&outgoingMessage, sizeof(outgoingMessage));
  }
}

// Update physical relays based on current state
void updateRelays() {
  // Turn off all relays first
  for (int i = 0; i < 4; i++) {
    M5StamPLC.writePlcRelay(i, false);
    relayStates[i] = false;
  }

  // Turn on specific relay based on state
  if (relayState > 0 && relayState <= 4) {
    M5StamPLC.writePlcRelay(relayState - 1, true);
    relayStates[relayState - 1] = true;
  }

  // Print current state to serial
  Serial.print("Relay state: ");
  if (relayState == 0) {
    Serial.println("All off");
  } else {
    Serial.print("RL");
    Serial.print(relayState);
    Serial.println(" on");
  }

  // Send state update via ESPNOW
  outgoingMessage.command = 0;
  outgoingMessage.relayState = relayState;
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)&outgoingMessage, sizeof(outgoingMessage));
}

// Draw relay status boxes on display
void drawRelayBoxes() {
  sprite->fillSprite(BLACK);  // Clear screen

  for (int i = 0; i < 4; i++) {
    int x = relayMargin + i * (relayBoxWidth + relayMargin);

    // Draw relay box background
    sprite->fillSmoothRoundRect(x, relayY, relayBoxWidth, relayBoxHeight, 2, darkblue);

    // Draw status indicator line
    sprite->fillRect(x + 4, relayY + 6, relayBoxWidth - 8, 3, relayColor[relayStates[i]]);

    // Draw relay label
    sprite->setTextColor(0xCE59, darkblue);  // Light gray text on darkblue
    sprite->loadFont(midFont);
    sprite->setTextDatum(MC_DATUM);
    sprite->drawString("RL" + String(i + 1), x + relayBoxWidth / 2, relayY + 18);

    // Draw state text
    sprite->setTextColor(0xAD55, darkblue);  // Lighter gray text
    sprite->loadFont(smallFont);
    sprite->drawString(relayStates[i] ? "ON" : "OFF", x + relayBoxWidth / 2, relayY + 32);
  }

  sprite->pushSprite(0, 0);
  sprite->unloadFont();
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
  M5StamPLC.begin();
  M5StamPLC.Display.setBrightness(1);
  sprite->createSprite(240, 135);
  Serial.begin(115200);

  // Initialize ESP-NOW
  setupESPNOW();

  // Initialize all relays to off
  relayState = 0;
  updateRelays();
  drawRelayBoxes();
}

void loop() {
  M5StamPLC.update();

  // Check for button press to cycle relay states
  if (M5StamPLC.BtnA.wasClicked()) {
    M5StamPLC.tone(1080, 40);  // Audible feedback

    // Cycle to next state (0-4)
    relayState = (relayState + 1) % 5;

    // Update relays and display
    updateRelays();
    drawRelayBoxes();
  }

  delay(50);  // Small delay for debouncing
}