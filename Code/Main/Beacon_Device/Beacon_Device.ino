#include <LoRa.h>
#include <SPI.h>
#include <HardwareSerial.h>
#include <TinyGPS++.h>
#include <Adafruit_NeoPixel.h>

// -----------------------------
// LoRa Module Pins
// -----------------------------
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 2

// -----------------------------
// GPS Pins (TX2 = GPIO17, RX2 = GPIO16)
// -----------------------------
#define GPS_RX 16
#define GPS_TX 17
HardwareSerial gpsSerial(2);  // UART2
TinyGPSPlus gps;

// -----------------------------
// Buzzer and NeoPixel
// -----------------------------
const int buzzerPin = 13;

// NeoPixel configuration
#define NEOPIXEL_PIN 21
#define NUMPIXELS 8
Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// -----------------------------
// Timers and States
// -----------------------------
unsigned long previousMillisBuzzer = 0;
unsigned long previousMillisPixels = 0;
bool buzzerState = false;
bool colorToggle = false;  // For alternating red/blue flashes

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  Serial.println("GPS + LoRa Started");

  pinMode(25, INPUT_PULLUP);  // Wait pin

  Serial.println("Waiting for D33 to be pulled LOW...");
  while (digitalRead(25) == LOW) {
    delay(10);  // Short delay to prevent CPU hogging
  }

  Serial.println("D33 LOW detected, continuing...");

  // -----------------------------
  // Buzzer setup
  // -----------------------------
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, HIGH);
  delay(500);
  digitalWrite(buzzerPin, LOW);

  // -----------------------------
  // NeoPixel setup
  // -----------------------------
  pixels.begin();
  pixels.clear();
  pixels.show();

  // -----------------------------
  // LoRa setup
  // -----------------------------
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  while (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed. Retrying...");
    delay(1000);
  }

  LoRa.setSyncWord(0xA5);
  Serial.println("LoRa Initialized");

  LoRa.beginPacket();
  LoRa.print("Beacon_Active");
  LoRa.endPacket();
}

void loop() {
  static char buffer[7] = {0};
  static int index = 0;

  // -----------------------------
  // GPS Reading
  // -----------------------------
  while (gpsSerial.available()) {
    char c = gpsSerial.read();
    buffer[index++] = c;
    if (index >= 6) index = 0;

    if (buffer[(index + 5) % 6] == '$' && buffer[(index + 0) % 6] == 'G' && buffer[(index + 1) % 6] == 'N') {
      gps.encode('$');
      gps.encode('G');
      gps.encode('P');
    } else {
      gps.encode(c);
    }
  }

  if (gps.location.isUpdated()) {
    float lat = gps.location.lat();
    float lng = gps.location.lng();

    LoRa.beginPacket();
    LoRa.print("Lat:");
    LoRa.print(lat, 6);
    LoRa.print(",Lng:");
    LoRa.print(lng, 6);
    LoRa.endPacket();

    delay(1000);
  }

  // -----------------------------
  // Buzzer non-blocking toggle
  // -----------------------------
  unsigned long currentMillis = millis();

  if (buzzerState && (currentMillis - previousMillisBuzzer >= 500)) {
    buzzerState = false;
    digitalWrite(buzzerPin, LOW);
    previousMillisBuzzer = currentMillis;
    pixels.clear();  // Turn off LEDs when buzzer off
    pixels.show();
  } else if (!buzzerState && (currentMillis - previousMillisBuzzer >= 2000)) {
    buzzerState = true;
    digitalWrite(buzzerPin, HIGH);
    previousMillisBuzzer = currentMillis;
  }

  // -----------------------------
  // Aggressive Red-Blue flashing while buzzer active
  // -----------------------------
  if (buzzerState) {
    if (currentMillis - previousMillisPixels >= 200) {  // Rapid flashing every 100ms
      previousMillisPixels = currentMillis;
      colorToggle = !colorToggle;

      uint32_t color = colorToggle ? pixels.Color(255, 0, 0) : pixels.Color(0, 0, 255);  // Red / Blue
      for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, color);
      }
      pixels.show();
    }
  }
}
