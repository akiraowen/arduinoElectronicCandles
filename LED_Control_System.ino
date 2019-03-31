//
// Description: LED Control System
// - Responds to a single switch input by randomly selecting and activating a LED that is currently off.
// - Automatically turns off the LED after the timeout limit.
//
// Author: Anthony Owen
// Copyright: (c) 2015
// Version: 1.0
// Licence: GPLv3
//

// Constants
const int bufferSize = 7;
const int numberOfOutputs = 56;
const unsigned long timeoutValue = 1800000; // 30 minutes

// I/O Pins
int dataPin = 2;
int latchPin = 3;
int clockPin = 4;
int testButtonPin = 5;

int detectorPin1 = 6;
int detectorPin2 = 7;
bool detectionEvent = false;
bool prevDetectionEvent = false;

// Data buffers and counters
byte outputDataBuffer[bufferSize] = { 0, 0, 0, 0, 0, 0, 0 };
unsigned long outputTimerBuffer[numberOfOutputs];
unsigned int outputsActiveCount = 0;

void setup() {
    Serial.begin(9600);
  
    pinMode(dataPin, OUTPUT);
    pinMode(latchPin, OUTPUT);
    pinMode(clockPin, OUTPUT);

    pinMode(detectorPin1, INPUT);
    pinMode(detectorPin2, INPUT);    
    pinMode(testButtonPin, INPUT);
    
    // Set random seed from floating analog input 0
    randomSeed(analogRead(0));
    
    // Clear all outputs
    writeSerialByteData(latchPin, dataPin, clockPin, outputDataBuffer, bufferSize);

}

void loop() {

  // Respond to sensor detect, wait for sensor release, ignore if all outputs active
  if(digitalRead(testButtonPin) == HIGH && digitalRead(detectorPin1) == HIGH && digitalRead(detectorPin2) == HIGH) {
    detectionEvent = false;
    prevDetectionEvent = false;
  } else {
    detectionEvent = true;
  }
  if(detectionEvent == true && prevDetectionEvent == false && outputsActiveCount < numberOfOutputs) {
    prevDetectionEvent = true;
    // Generate random position to activate, limited to those available
    long randPosition = random(0, numberOfOutputs - outputsActiveCount);
    outputsActiveCount++;
    
    Serial.print("Random number: ");
    Serial.print(randPosition);
    Serial.print(", ");

    // Avoid selecting active outputs
    int finalpos = 0, cleared = 0;
    for(int x = 0; x < numberOfOutputs; x++) {
      if(readSerialByteData(outputDataBuffer, x) == 0) {
        cleared += 1;
      }
      if(cleared >= randPosition+1) {
        finalpos = x;
        break;
      }
    }
    
    // Activate output and record activation time
    outputTimerBuffer[finalpos] = millis();
    setSerialByteData(outputDataBuffer, finalpos);
    writeSerialByteData(latchPin, dataPin, clockPin, outputDataBuffer, bufferSize);

    Serial.print("Time: ");
    Serial.print(outputTimerBuffer[finalpos]);
    Serial.print(", ");
    Serial.print("Finalpos: ");
    Serial.print(finalpos);
    Serial.print(", ");
    Serial.print("Remaining: ");
    Serial.println(numberOfOutputs - outputsActiveCount);
  }
  
  // Process expired timers
  for(int x = 0; x < numberOfOutputs; x++) {
    // Check if the timer is active and expired: deactivate timer, deactivate output and decrement global output count
    if(outputTimerBuffer[x] != 0) {
      unsigned long elapsedValue = millis() - outputTimerBuffer[x];
      if(elapsedValue >= timeoutValue) {
        outputTimerBuffer[x] = 0;
        outputsActiveCount--;
        clearSerialByteData(outputDataBuffer, x);
        writeSerialByteData(latchPin, dataPin, clockPin, outputDataBuffer, bufferSize);
        
        Serial.print("Timer expired: ");
        Serial.print(x);
        Serial.print(", Elapsed time: ");
        Serial.println(elapsedValue);
      }
    }
  }
  
  // Slow detection down
  delay(100);
}

int readSerialByteData(byte dataBytes[], int pos) {
  return bitRead(dataBytes[pos/8], pos-((pos/8)*8));
}

void setSerialByteData(byte dataBytes[], int pos) {
  bitSet(dataBytes[pos/8], pos-((pos/8)*8));
}

void clearSerialByteData(byte dataBytes[], int pos) {
  bitClear(dataBytes[pos/8], pos-((pos/8)*8));
}

void writeSerialByteData(int latchPin, int dataPin, int clockPin, byte dataBytes[], int numberOfBytes) {
  digitalWrite(latchPin, LOW);            //Pull latch LOW to start sending data
  for(int x = 0; x < numberOfBytes; x++) {
    shiftOut(dataPin, clockPin, MSBFIRST, dataBytes[x]);  //Send the data byte
  }
  digitalWrite(latchPin, HIGH);           //Pull latch HIGH to stop sending data
}

