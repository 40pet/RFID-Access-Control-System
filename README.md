# Arduino Nano RFID Access Control System

This project implements an RFID-based access control system using an Arduino Nano, MFRC522 RFID reader, relay module, and LEDs to indicate the system status. The system is designed to control access by verifying RFID tags, managing an EEPROM-based database of authorized tags, and utilizing power-saving sleep modes for efficiency.

## Table of Contents

- [Components](#components)
- [Pinout](#pinout)
- [How It Works](#how-it-works)
  - [Initialization](#initialization)
  - [Normal Mode](#normal-mode)
  - [Manage Mode](#manage-mode)
  - [Sleep Mode](#sleep-mode)
- [Setup](#setup)
- [Usage](#usage)
  - [Add/Remove Cards](#addremove-cards)
  - [Normal Operation](#normal-operation)
- [Code](#code)
- [License](#license)

## Components

- Arduino Nano
- MFRC522 RFID Reader
- Relay Module
- Green LED
- Red LED
- Yellow LED
- Connecting Wires

## Pinout

| Component             | Function       | Arduino Nano Pin | Description                          |
|-----------------------|----------------|------------------|--------------------------------------|
| MFRC522 RFID Reader   | RST            | 9                | Reset pin                            |
| MFRC522 RFID Reader   | SS (SDA)       | 10               | Slave Select (Chip Select) pin       |
| MFRC522 RFID Reader   | MOSI           | 11               | Master Out Slave In (SPI)            |
| MFRC522 RFID Reader   | MISO           | 12               | Master In Slave Out (SPI)            |
| MFRC522 RFID Reader   | SCK            | 13               | Serial Clock (SPI)                   |
| Relay Module          | Control        | 6                | Control pin for the relay            |
| Green LED             | Control        | 4                | Pin controlling the Green LED        |
| Red LED               | Control        | 5                | Pin controlling the Red LED          |
| Yellow LED            | Control        | 7                | Pin controlling the Yellow LED       |
| GND                   | Ground         | GND              | Ground connection for all components |
| VCC (MFRC522)         | Power          | 3.3V or 5V       | Power supply for the RFID reader     |
| VCC (Relay Module)    | Power          | 5V               | Power supply for the relay module    |
| VCC (LEDs)            | Power          | 5V               | Power supply for the LEDs            |

## How It Works

### Initialization

When powered on, the Arduino Nano initializes the RFID reader and sets up the control pins for the LEDs and relay. The system is ready to scan RFID tags and respond accordingly.

### Normal Mode

In normal operation mode:
1. The system continuously scans for RFID cards.
2. When a card is detected, its UID is read and processed.
3. The UID is checked against an EEPROM-stored list of authorized UIDs.
4. If the UID is found, access is granted:
   - The relay is activated.
   - The green LED is turned on.
   - A message is printed to the serial monitor.
   - After a delay, the relay and green LED are turned off.
5. If the UID is not found, access is denied:
   - The red LED is turned on.
   - A message is printed to the serial monitor.
   - After a delay, the red LED is turned off.

### Manage Mode

Manage mode allows adding or removing RFID cards from the system:
1. Scan a master card to enter manage mode (indicated by the red LED).
2. In manage mode, scan a card to add or remove it from the EEPROM:
   - If the card is not in the EEPROM, it is added.
   - If the card is already in the EEPROM, it is removed.
3. The yellow LED blinks to indicate the add/remove action.
4. Scan the master card again to exit manage mode.

### Sleep Mode

To conserve power, the Arduino enters sleep mode after 30 minutes of inactivity:
1. The system tracks the last activity timestamp.
2. If no card is scanned within 30 minutes, the Arduino enters power-down sleep mode.
3. The system wakes up when an RFID card is scanned.

## Setup

1. Connect the components according to the pinout table above.
2. Upload the provided Arduino code to your Arduino Nano.
3. Open the Serial Monitor to view status messages and interact with the system.

## Usage

### Add/Remove Cards

1. Scan a master card to enter manage mode (red LED on).
2. Scan a card to add it to the EEPROM or remove it if it already exists (yellow LED blinks).
3. Scan the master card again to exit manage mode (red LED off).

### Normal Operation

1. Scan recognized cards to activate the relay and grant access (green LED on).
2. Unrecognized cards will be denied access (red LED on).

## Code

```cpp
#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#define RST_PIN 9         // Configurable, set to -1 if unused
#define SS_PIN 10         // Configurable
#define MASTER_UID1 43    // First master UID
#define MASTER_UID2 98    // Second master UID
#define MASTER_UID3 123   // Third master UID
#define MASTER_UID4 456   // Fourth master UID
#define RELAY_PIN 6       // Define the pin connected to the relay module
#define GREEN_LED_PIN 4   // Green LED pin
#define RED_LED_PIN 5     // Red LED pin
#define YELLOW_LED_PIN 7  // Yellow LED pin

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

volatile bool manageMode = false;  // Flag to indicate manage mode
volatile bool activityFlag = false; // Flag to indicate activity
unsigned long lastActivityTime = 0; // Timestamp of the last activity

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();

  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);

  attachInterrupt(digitalPinToInterrupt(SS_PIN), wakeUp, LOW); // Attach interrupt to wake up on RFID scan

  Serial.println("Scan RFID tag...");
}

void loop() {
  if (millis() - lastActivityTime >= 1800000) { // 30 minutes inactivity
    goToSleep();
  }

  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    activityFlag = true; // Activity detected
    lastActivityTime = millis(); // Reset inactivity timer

    Serial.print("Card UID: ");
    int uidSum = 0;
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      uidSum += mfrc522.uid.uidByte[i];
    }
    int shrunkUID = uidSum % 100;
    Serial.println(shrunkUID);

    // Check if scanned UID matches any of the master UIDs
    if (shrunkUID == MASTER_UID1 || shrunkUID == MASTER_UID2 || shrunkUID == MASTER_UID3 || shrunkUID == MASTER_UID4) {
      if (manageMode) {
        Serial.println("Master card detected again. Exiting manage mode...");
        manageMode = false;
        digitalWrite(RED_LED_PIN, LOW);  // Turn off red LED when exiting manage mode
      } else {
        Serial.println("Master card detected. Entering manage mode...");
        manageMode = true;
        digitalWrite(RED_LED_PIN, HIGH);  // Turn on red LED when entering manage mode
        digitalWrite(GREEN_LED_PIN, HIGH);  
        digitalWrite(RELAY_PIN, HIGH); 
        delay(3000);
        digitalWrite(RELAY_PIN, LOW); 
        digitalWrite(GREEN_LED_PIN, LOW);   
      }
    } else {
      if (manageMode) {
        // Check if scanned UID is already stored in EEPROM
        int existingAddress = findEEPROMAddress(shrunkUID);
        if (existingAddress != -1) {
          // Remove the scanned UID
          EEPROM.write(existingAddress, 0);
          Serial.println("Scanned UID removed from EEPROM.");
          digitalWrite(YELLOW_LED_PIN, HIGH); 
          delay(100);
          digitalWrite(YELLOW_LED_PIN, LOW); 
        } else {
          // Store the scanned UID in EEPROM
          int emptyAddress = findEmptyEEPROMAddress();
          if (emptyAddress != -1) {
            EEPROM.write(emptyAddress, shrunkUID);
            Serial.print("New card added to EEPROM at address ");
            Serial.println(emptyAddress);
            digitalWrite(YELLOW_LED_PIN, HIGH); 
            delay(1000);
            digitalWrite(YELLOW_LED_PIN, LOW); 
          } else {
            Serial.println("EEPROM is full. Cannot add more cards.");
          }
        }
        manageMode = false; // Exit manage mode after processing the card
        digitalWrite(RED_LED_PIN, LOW);  // Turn off red LED when exiting manage mode
      } else {
        // Check if scanned UID is present in EEPROM
        if (findEEPROMAddress(shrunkUID) != -1) {
          Serial.println("Access granted.");
          digitalWrite(GREEN_LED_PIN, HIGH);  
          digitalWrite(RELAY_PIN, HIGH); 
          delay(3000);
          digitalWrite(RELAY_PIN, LOW); 
          digitalWrite(GREEN_LED_PIN, LOW);   
        } else {
          Serial.println("Access denied.");
          digitalWrite(RED_LED_PIN, HIGH);    
          delay(2000); 
          digitalWrite(RED_LED_PIN, LOW);  
        }
      }
    }

    // Halt PICC
    mfrc522.PICC_HaltA();
    // Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();
  }
}

int findEmptyEEPROMAddress() {
  for (int i = 0; i < EEPROM.length(); i++) {
    if (EEPROM.read(i) == 0) { // 0 indicates an empty EEPROM cell
      return i;
    }
  }
  return -1; // Return -1 if no empty address found
}

int findEEPROMAddress(int uid) {
  for (int i = 0; i < EEPROM.length(); i++) {
    if (EEPROM.read(i) == uid) {
      return i;
    }
  }
  return -1; // Return -1 if UID not found in EEPROM
}

void wakeUp() {
  activityFlag = true;
}

void goToSleep() {
  Serial.println("Going to sleep...");
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  attachInterrupt(digitalPinToInterrupt(SS_PIN), wakeUp, LOW); // Enable interrupt on RFID scan
  sleep_cpu();
  sleep_disable();
  detachInterrupt(digitalPinToInterrupt(SS_PIN)); // Disable interrupt after waking up
  Serial.println("Waking up...");
}
