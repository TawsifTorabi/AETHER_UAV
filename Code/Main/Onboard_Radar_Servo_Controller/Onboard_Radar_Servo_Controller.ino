#include <WiFi.h>
#include <WiFiMulti.h>
#include <WebSocketsClient.h>
#include <ESP32Servo.h>

WiFiMulti WiFiMulti;

const char* websocket_host = "192.168.137.1";
const uint16_t websocket_port = 8767;
const char* websocket_path = "/";

#define SERVO_PIN 13
#define RX2_PIN 16  // Sensor TX → ESP32 RX2
#define TX2_PIN 17  // Optional

WebSocketsClient webSocket;
Servo myServo;
HardwareSerial SensorSerial(2);

bool servoState = false;

void toggleServo() {
  servoState = !servoState;
  int angle = servoState ? 130 : 50;
  myServo.write(angle);
  Serial.printf("Servo moved to: %d°\n", angle);
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.println("Connected to WebSocket server");
      break;
    case WStype_DISCONNECTED:
      Serial.println("Disconnected from WebSocket server");
      break;
    case WStype_TEXT:
      Serial.printf("Received: %s\n", payload);
      if (strcmp((char*)payload, "drop") == 0) {
        toggleServo();
      }
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  WiFiMulti.addAP("lenovo", "123456789");
  Serial.print("Connecting to WiFi");
  while (WiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Try lower baud rate if 115200 fails
  SensorSerial.begin(9600, SERIAL_8N1, RX2_PIN, TX2_PIN);  // try 9600 or 115200

  myServo.setPeriodHertz(50);
  myServo.attach(SERVO_PIN, 500, 2400);
  myServo.write(servoState ? 130 : 50);

  webSocket.begin(websocket_host, websocket_port, websocket_path);
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
}

String sensorBuffer = "";

void loop() {
  webSocket.loop();

  // Read all bytes from SensorSerial (raw mode)
  while (SensorSerial.available()) {
    char c = SensorSerial.read();
    Serial.print(c);  // Debug output to Serial Monitor
    if (c == '\n' || c == '\r') {
      sensorBuffer.trim();
      if (sensorBuffer.length() > 0 && webSocket.isConnected()) {
        webSocket.sendTXT(sensorBuffer);
        Serial.println("[Sent to WebSocket] " + sensorBuffer);
      }
      sensorBuffer = "";  // Clear buffer after sending
    } else {
      sensorBuffer += c;
    }
  }
}
