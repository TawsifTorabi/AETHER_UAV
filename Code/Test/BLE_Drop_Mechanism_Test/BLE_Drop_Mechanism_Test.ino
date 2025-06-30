#include <ESP32Servo.h>
#include "BluetoothSerial.h"

#define SERVO_PIN 13

Servo myServo;
BluetoothSerial SerialBT;  // Built-in Bluetooth Serial

void setup() {
  Serial.begin(115200);        // USB Serial for debugging
  SerialBT.begin("ESP32-SERVO");  // Bluetooth device name

  myServo.setPeriodHertz(50);       // Standard 50Hz servo
  myServo.attach(SERVO_PIN, 500, 2400); // Calibrated pulse range

  myServo.write(90); // Start at center
  Serial.println("ESP32 Bluetooth Servo Ready");
  Serial.println("Waiting for 'F' or 'R' over Bluetooth...");
}

void loop() {
  if (SerialBT.available()) {
    char cmd = SerialBT.read();
    Serial.print("Received: ");
    Serial.println(cmd);

    if (cmd == 'F') {
      myServo.write(50); // Move to 0°
      Serial.println("Servo: Forward (0°)");
    } else if (cmd == 'B') {
      myServo.write(130); // Move to 180°
      Serial.println("Servo: Reverse (180°)");
    }
  }
}
