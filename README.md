# M5-StamPLC-StationBat-ESPNOW



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
******************************************************************************
## To-Do
- [ ] Download Instructions
- [ ] Repository Folder Structure
- [x] IDE Version & Configuration Settings
- [x] SLS Version & Configuration Settings
- [ ] Required Libraries & Boards Manager Versions
- [ ] LVGL Conf file
- [ ] Images
- [ ] AOB
