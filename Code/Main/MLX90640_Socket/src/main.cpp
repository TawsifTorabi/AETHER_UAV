#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_MLX90640.h>
#include <WebSocketsClient.h>

// WiFi credentials
const char* ssid = "lenovo";
const char* password = "123456789";

// WebSocket Server (Your PC's IP address)
const char* websocket_host = "192.168.137.1"; // replace with your PC IP
const uint16_t websocket_port = 8765;         // your server port
const char* websocket_path = "/";

WebSocketsClient webSocket;
Adafruit_MLX90640 mlx;
float frame[32 * 24];


// WebSocket event handler
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.println("Connected to WebSocket server");
      break;
    case WStype_DISCONNECTED:
      Serial.println("Disconnected from WebSocket server");
      break;
    case WStype_TEXT:
      Serial.printf("Server says: %s\n", payload);
      break;
    default:
      break;
  }
}


void setup() {
  Serial.begin(115200);
  delay(1000);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Init MLX90640
  Wire.begin(); // ESP32 default: SDA=21, SCL=22
  Wire.setClock(400000); // Fast I2C

  if (!mlx.begin(0x33, &Wire)) {
    Serial.println("MLX90640 not found!");
    while (1);
  }

  Serial.println("MLX90640 online");

  mlx.setMode(MLX90640_CHESS);
  mlx.setResolution(MLX90640_ADC_18BIT);
  mlx.setRefreshRate(MLX90640_8_HZ);

  // Setup WebSocket client
  webSocket.begin(websocket_host, websocket_port, websocket_path);
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000); // Retry every 5 sec
}

void loop() {
  webSocket.loop();

  static unsigned long lastSent = 0;
  if (millis() - lastSent >= 250 && webSocket.isConnected()) {
    lastSent = millis();

    if (mlx.getFrame(frame) != 0) {
      Serial.println("Failed to read MLX90640");
      return;
    }

    String data = "";
    for (int i = 0; i < 768; i++) {
      data += String(frame[i], 2);
      if (i < 767) data += ",";
    }

    webSocket.sendTXT(data);
    Serial.println("Frame sent");
  }
}
