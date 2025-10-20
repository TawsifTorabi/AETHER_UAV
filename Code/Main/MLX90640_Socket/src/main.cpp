// === MLX90640_Socket Main Code ===
/*
Code for ESP32 to read MLX90640 thermal sensor data, control a servo via WebSocket, and toggle servo position with a button. 
Author: AETHER Team
Date: June 2024
*/

#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_MLX90640.h>
#include <WebSocketsClient.h>
#include <ESP32Servo.h>  // Include Servo library

// === WiFi credentials ===
const char *ssid = "SSID_OF_YOUR_HOTSPOT";
const char *password = "YOUR_PASSWORD_HERE";

// === WebSocket ===
WebSocketsClient webSocket;
uint16_t websocket_port = 8765;
const char *websocket_path = "/";
IPAddress gatewayIP;

// === MLX90640 Thermal Sensor ===
Adafruit_MLX90640 mlx;
float frame[32 * 24];

// === Servo ===
Servo myServo;
const int SERVO_PIN = 18;     // Servo control pin
int servoPosition = 0;        // 0° or 90°
bool manualState = false;     // Manual toggle state

// === Button ===
const int BUTTON_PIN = 14;     // Push button pin
bool lastButtonState = HIGH;  // For debouncing
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// === WebSocket Event Handler ===
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_CONNECTED:
    Serial.println("✅ Connected to WebSocket server");
    break;

  case WStype_DISCONNECTED:
    Serial.println("❌ Disconnected from WebSocket server");
    break;

  case WStype_TEXT:
  {
    String message = String((char *)payload);
    Serial.printf("💬 Server says: %s\n", message.c_str());

    if (message == "drop")
    {
      servoPosition = 180;
      myServo.write(servoPosition);
      Serial.println("🟢 Servo moved to 90° (DROP)");
    }
    else if (message == "close")
    {
      servoPosition = 0;
      myServo.write(servoPosition);
      Serial.println("🔴 Servo moved to 0° (CLOSE)");
    }
    break;
  }

  default:
    break;
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  // === Connect WiFi ===
  Serial.printf("Connecting to WiFi SSID: %s\n", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Connected to WiFi");
  Serial.print("📶 ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  // === Get gateway IP (PC Hotspot) ===
  gatewayIP = WiFi.gatewayIP();
  Serial.print("🌐 Detected Gateway (PC) IP: ");
  Serial.println(gatewayIP);

  // === Setup Servo ===
  myServo.attach(SERVO_PIN);
  myServo.write(0);
  Serial.println("⚙️ Servo initialized at 0°");

  // === Setup Button ===
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // === Initialize MLX90640 ===
  Wire.begin();
  Wire.setClock(400000);
  if (!mlx.begin(0x33, &Wire))
  {
    Serial.println("❌ MLX90640 not found!");
    while (1)
      ;
  }
  Serial.println("✅ MLX90640 online");
  mlx.setMode(MLX90640_CHESS);
  mlx.setResolution(MLX90640_ADC_18BIT);
  mlx.setRefreshRate(MLX90640_8_HZ);

  // === Setup WebSocket ===
  webSocket.begin(gatewayIP.toString().c_str(), websocket_port, websocket_path);
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
}

void loop()
{
  webSocket.loop();

  // === Manual Button Toggle ===
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonState)
  {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    if (reading == LOW && lastButtonState == HIGH)
    {
      manualState = !manualState;
      servoPosition = manualState ? 180 : 0;
      myServo.write(servoPosition);
      Serial.printf("🔘 Manual toggle → Servo %d°\n", servoPosition);
    }
  }
  lastButtonState = reading;

  // === Send MLX90640 Frame ===
  static unsigned long lastSent = 0;
  if (millis() - lastSent >= 250 && webSocket.isConnected())
  {
    lastSent = millis();

    if (mlx.getFrame(frame) != 0)
    {
      Serial.println("⚠️ Failed to read MLX90640 frame");
      return;
    }

    String data;
    for (int i = 0; i < 768; i++)
    {
      data += String(frame[i], 2);
      if (i < 767)
        data += ",";
    }

    webSocket.sendTXT(data);
    Serial.println("📤 Frame sent");
  }
}
// === End of Code ===