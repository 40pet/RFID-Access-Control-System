#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>

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

volatile bool manageMode = false;   // Flag to indicate manage mode

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

  Serial.println("Scan RFID tag...");
}

void loop() {
  // Look for new cards
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    // Show some details of the PICC (that is: the tag/card)
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
      return i
    
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
