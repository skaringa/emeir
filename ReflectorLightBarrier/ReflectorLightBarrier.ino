/*
* Program to read the electrical meter using a reflective light sensor
* This is the data acquisition part running on an Arduino Nano.
* It controls the infrared light barrier, detects trigger
* and communicates with a master computer (Raspberry Pi)
* over USB serial.

* Copyright 2015 Martin Kompf
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <avr/eeprom.h>

const int analogInPin = A7;  // Analog input pin that the photo transistor is attached to
const int irOutPin = 2; // Digital output pin that the IR-LED is attached to
const int ledOutPin = 12; // Signal LED output pin

int sensorValueOff = 0;  // value read from the photo transistor when ir LED is off
int sensorValueOn = 0;  // value read from the photo transistor when ir LED is on

// command line
#define MAX_CMD_LEN 80
char command[MAX_CMD_LEN]; // command buffer
int inCount = 0; // command buffer index
boolean cmdComplete = false;

// Mode of serial line: 
// C - command, D - data output
char mode = 'D';

// Data output mode:
// R - raw data
// T - trigger events
// C - counter
char dataOutput = 'T';

// trigger state and level
int triggerLevelLow;
int triggerLevelHigh;
boolean triggerState = false;

// Address of trigger levels in EEPROM
uint16_t triggerLevelLowAddr = 0;
uint16_t triggerLevelHighAddr = 4;

/**
 * Set trigger levels (low high) from the command line
 * and store them into EEPROM
 */
void setTriggerLevels() {
  char *p = &command[1];
  while (*p != 0 && *p == ' ') {
    ++p;
  }
  char *q = p + 1;
  while (*q != 0 && *q != ' ') {
    ++q;
  }
  *q = 0;
  eeprom_write_word(&triggerLevelLowAddr, atoi(p));
  
  p = q + 1;
  while (*p != 0 && *p == ' ') {
    ++p;
  }
  q = p + 1;
  while (*q != 0 && *q != ' ') {
    ++q;
  }
  *q = 0;
  eeprom_write_word(&triggerLevelHighAddr, atoi(p));
}

/**
 * Read trigger levels from EEPROM
 */
void readTriggerLevels() {
  triggerLevelLow = (int)eeprom_read_word(&triggerLevelLowAddr);
  triggerLevelHigh = (int)eeprom_read_word(&triggerLevelHighAddr);
  Serial.print("Trigger levels: ");
  Serial.print(triggerLevelLow);
  Serial.print(" ");
  Serial.println(triggerLevelHigh);
}

/**
 * Detect and print a trigger event
 */
void detectTrigger(int val) {
  boolean nextState = triggerState;
  if (val > triggerLevelHigh) {
    nextState = true;
  } else if (val < triggerLevelLow) {
    nextState = false;
  }
  if (nextState != triggerState) {
    triggerState = nextState;
    Serial.println(triggerState? 1 : 0);
    // control internal LED
    digitalWrite(ledOutPin, triggerState);
  }
}

/**
 * Read one line from serial connection and interpret it as a command
 */
void doCommand() {
  // print prompt 
  Serial.print(">");
  while (! cmdComplete) {
    // read input
    while (Serial.available()) {
      // get the new byte:
      char inChar = (char)Serial.read();
      if (inChar == '\n' || inChar == '\r') {
        command[inCount] = 0;
        Serial.println();
        cmdComplete = true;
        break; // End of line
      } else if (inCount < MAX_CMD_LEN - 1) {
        command[inCount] = inChar;
        inCount++;
        // echo
        Serial.print(inChar);
      }
    }
  }
  
  // interprete command
  switch (command[0]) {
    case 'D':
      // start raw data mode
      mode = 'D';
      dataOutput = 'R';
      break;
    case 'T':
      // start trigger data mode
      mode = 'D';
      dataOutput = 'T';
      break;
    case 'S':
      // set trigger levels
      setTriggerLevels();
      readTriggerLevels();
      break;
  }
  
  // clear command buffer
  command[0] = 0;
  inCount = 0;
  cmdComplete = false;
}

/**
 * Setup.
 */
void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);
  // initialize the digital pins as an output.
  pinMode(irOutPin, OUTPUT);
  pinMode(ledOutPin, OUTPUT);
  // read config from EEPROM
  readTriggerLevels();
}

/**
 * Main loop.
 */
void loop() {
  if (mode == 'C') {
    doCommand();
  } else if (mode == 'D') {
    if (Serial.available()) {
      char inChar = (char)Serial.read();
      if (inChar == 'C') {
        // exit data mode
        mode = 'C';
      }
    }
  }
  if (mode == 'D') {
    // perform measurement
    // turn IR LED off
    digitalWrite(irOutPin, LOW);
    // wait 10 milliseconds
    delay(10);
    // read the analog in value:
    sensorValueOff = analogRead(analogInPin);           
    // turn IR LED on
    digitalWrite(irOutPin, HIGH);
    delay(10);
    // read the analog in value:
    sensorValueOn = analogRead(analogInPin);           
    
    switch (dataOutput) {
      case 'R':
        // print the raw data to the serial monitor         
        Serial.println(sensorValueOn - sensorValueOff);
        break;
      case 'T':
        // detect and output trigger
        detectTrigger(sensorValueOn - sensorValueOff);
        break;
    }
  }
  delay(10);                     
}
