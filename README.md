# M5-StamPLC-StationBat-ESPNOW

![Station Bat Image](https://github.com/user-attachments/assets/4bc530bc-d080-48c3-801e-8b34924fa8a9)
![StamPLC Image](https://github.com/user-attachments/assets/5a1db47c-f010-470b-84b4-60df12cc6ff4)


## Hardware Summary:

### Device 1 - M5Stack StamPLC:

Connected to industrial tower light with 3 LEDs (green, orange, red) and buzzer

Relay 1 controls green "Operational" light

Relay 2 controls orange "Caution" light

Relay 3 controls red "Alarm" light

Relay 4 controls alarm buzzer

Tower light powered by 12V connected to VIN pin

### Device 2 - M5Stack Station Bat:

Connected to Adafruit QMC5883P magnetometer via I2C (Port A1)

Features battery monitoring and RGB LED indicators

Serves as control interface with display

### Program Summary:

M5Stamp-Relay-ESPNOW.ino (StamPLC):

Acts as ESP-NOW receiver for relay control commands

Controls 4 relays based on received commands

Provides local control via Button A to cycle through relay states

Displays relay status on screen with custom UI

Sends state updates back to controller

M5StationBat-Relay-ESPNOW.ino (Station Bat):

Acts as ESP-NOW controller and sensor hub

Features multi-page interface with 11 different screens

Reads and displays magnetometer data (raw and Gauss values)

Monitors battery status and charging state

Controls RGB LEDs to indicate current page

Sends relay control commands to StamPLC

Provides user interface with three buttons


******************************************************************************
## IDE Version
#### Arduino IDE Setup (IDE Version : 2.3.6)
## Boards Version
#### ESP32 Boards Manager Version By Espressif Systems : 3.3.0
#### M5Stack By M5 Stack Official : 3.2.2
## Library Version
#### M5StamPLC By M5Stack : 1.1.0
#### M5Unified By M5Stack : 0.2.7
#### Adafruit QMC5883P Library By Adafruit : 1.0.1
#### FastLED By Daniel Garcia : 3.10.2
******************************************************************************
## To-Do
- [ ] Blah Blah Blah
- [ ] Blah Blah Blah
- [ ] Blah Blah Blah
- [ ] Blah Blah Blah
- [ ] Blah Blah Blah
- [x] Blah Blah Blah
- [x] Blah Blah Blah
- [ ] Images
- [ ] AOB
