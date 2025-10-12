#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_MLX90640.h>
#include <WebSocketsClient.h>

// WiFi credentials
const char *ssid = "ewaste";
const char *password = "ewaste1234";

//You might wonder why not hardcoding the server IP here.
//This is because I use my Windows PC as a server and hotspot.
//So the gateway IP is my PC IP. You can change it to your server IP if needed.
// More on this in the setup() function.
// IPAddress serverIP(192, 168, 137, 1); // Your server IP
// uint16_t serverPort = 8765;        // Your server port
// const char *serverPath = "/";
// End of WiFi credentials


WebSocketsClient webSocket;
Adafruit_MLX90640 mlx;
float frame[32 * 24];

// WebSocket Server details
uint16_t websocket_port = 8765; // Your server port
const char *websocket_path = "/";

IPAddress gatewayIP;

// WebSocket event handler
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
    Serial.printf("💬 Server says: %s\n", payload);
    break;
  default:
    break;
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  // Connect to WiFi
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

  // Get gateway (your PC IP)
  /*
    Did this because I was using my Windows PC and Hotspot.
    Gateway IP is the PC IP. You can change it to your server IP if needed. 
  */
  gatewayIP = WiFi.gatewayIP();
  Serial.print("🌐 Detected Gateway (PC) IP: ");
  Serial.println(gatewayIP);

  // Init MLX90640
  Wire.begin(); // SDA=21, SCL=22
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

  // Setup WebSocket client with detected gateway
  webSocket.begin(gatewayIP.toString().c_str(), websocket_port, websocket_path);
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000); // Retry every 5 sec
}

void loop()
{
  webSocket.loop();

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
