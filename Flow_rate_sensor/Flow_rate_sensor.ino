#include <Arduino.h>

// Pin definitions
const int flowSensorPin = 26; // Replace with your ESP32 pin connected to the sensor

// Variables
volatile int pulseCount = 0;    // Counts the pulses from the sensor
float flowRate = 0.0;           // Measured flow rate (liters/minute)
float totalVolume = 0.0;        // Total volume (liters)
unsigned long oldTime = 0;

// Pulses per liter (depends on your sensor, e.g., YF-S201 = 450 pulses per liter)
const float calibrationFactor = 450.0;

// Interrupt service routine
void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

void setup() {
  Serial.begin(115200);
  pinMode(flowSensorPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(flowSensorPin), pulseCounter, RISING);

  Serial.println("Flow rate sensor initialized!");
}

void loop() {
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - oldTime;

  if (elapsedTime > 1000) { // Update every second
    oldTime = currentTime;

    // Calculate flow rate (liters/min)
    flowRate = (pulseCount / calibrationFactor) * 60.0;

    // Calculate total volume
    totalVolume += (pulseCount / calibrationFactor);

    // Print data to the serial monitor
    Serial.print("Flow Rate: ");
    Serial.print(flowRate);
    Serial.print(" L/min\t");

    Serial.print("Total Volume: ");
    Serial.print(totalVolume);
    Serial.println(" L");

    // Reset pulse count
    pulseCount = 0;
  }
}
