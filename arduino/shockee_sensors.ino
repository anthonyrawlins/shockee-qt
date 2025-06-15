#include "HX711.h"

// Pin definitions
const int POTENTIOMETER_PIN = A0;
const int ENCODER_PIN_A = 2;
const int ENCODER_PIN_B = 3;
const int HX711_DOUT_PIN = 4;
const int HX711_SCK_PIN = 5;

// HX711 load cell amplifier
HX711 scale;

// Encoder variables
volatile long encoderPosition = 0;
volatile bool encoderA_prev = false;

// Calibration values
float loadCellCalibration = 1.0; // Adjust based on your load cell
float potentiometerScale = 75.0 / 1023.0; // 75mm / 1023 ADC steps

// Timing
unsigned long lastSampleTime = 0;
const unsigned long SAMPLE_INTERVAL = 10; // 100Hz sampling (10ms)

void setup() {
  Serial.begin(9600);
  
  // Initialize load cell
  scale.begin(HX711_DOUT_PIN, HX711_SCK_PIN);
  scale.set_scale(loadCellCalibration);
  scale.tare(); // Reset to zero
  
  // Initialize encoder pins
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);
  
  // Attach interrupts for encoder
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), encoderISR, CHANGE);
  
  // Initialize potentiometer pin
  pinMode(POTENTIOMETER_PIN, INPUT);
  
  Serial.println("# Shockee Sensor Data");
  Serial.println("# Format: timestamp,position_mm,force_kg,encoder_pulses");
  
  delay(1000); // Allow sensors to stabilize
}

void loop() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastSampleTime >= SAMPLE_INTERVAL) {
    // Read potentiometer (position)
    int potValue = analogRead(POTENTIOMETER_PIN);
    float position = potValue * potentiometerScale;
    
    // Read load cell (force)
    float force = 0.0;
    if (scale.is_ready()) {
      force = scale.get_units(1); // Average of 1 reading for speed
    }
    
    // Read encoder position
    long currentEncoderPos = encoderPosition;
    
    // Send data in CSV format
    Serial.print(currentTime);
    Serial.print(",");
    Serial.print(position, 2);
    Serial.print(",");
    Serial.print(force, 2);
    Serial.print(",");
    Serial.println(currentEncoderPos);
    
    lastSampleTime = currentTime;
  }
  
  // Check for calibration commands
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "TARE") {
      scale.tare();
      Serial.println("# Load cell tared");
    } else if (command == "RESET_ENCODER") {
      encoderPosition = 0;
      Serial.println("# Encoder reset");
    } else if (command.startsWith("CAL_LOAD:")) {
      float calValue = command.substring(9).toFloat();
      scale.set_scale(calValue);
      Serial.println("# Load cell calibration set");
    }
  }
}

void encoderISR() {
  bool encoderA = digitalRead(ENCODER_PIN_A);
  bool encoderB = digitalRead(ENCODER_PIN_B);
  
  if (encoderA != encoderA_prev) {
    if (encoderA == encoderB) {
      encoderPosition--;
    } else {
      encoderPosition++;
    }
  }
  encoderA_prev = encoderA;
}