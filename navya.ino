#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Hardware Configuration
#define BATTERY_PIN 34
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Network Configuration
const char* ssid = "Pixel_2381";
const char* password = "maheshn5";
const char* serverIP = "10.65.92.152";
const int serverPort = 5000;

// Global Variables
unsigned long lastReconnectAttempt = 0;
const unsigned long reconnectInterval = 10000;
WiFiClient client;

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for serial connection

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("[OLED] Initialization failed!");
    while (1); // Halt if display fails
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Initializing...");
  display.display();

  // Configure ADC
  analogSetPinAttenuation(BATTERY_PIN, ADC_11db);
  
  connectToWiFi();
}

void connectToWiFi() {
  Serial.print("[WiFi] Connecting to ");
  Serial.println(ssid);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Connecting to:");
  display.println(ssid);
  display.display();

  WiFi.begin(ssid, password);
  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 30000) {
    delay(500);
    Serial.print(".");
    display.print(".");
    display.display();
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WiFi Connected");
    display.print("IP: ");
    display.println(WiFi.localIP());
    display.display();
  } else {
    Serial.println("\n[WiFi] Connection Failed.");
    display.println("\nFailed!");
    display.display();
    ESP.restart();
  }
}

void sendData(float voltage, int percentage, bool charging) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Not connected, attempting reconnect");
    connectToWiFi();
    return;
  }

  HTTPClient http;
  
  String payload = "{\"voltage\":" + String(voltage, 2) +
                 ",\"percentage\":" + String(percentage) +
                 ",\"charging\":" + String(charging ? "true" : "false") + "}";

  String url = "http://" + String(serverIP) + ":" + String(serverPort) + "/data";
  Serial.print("[HTTP] Attempting to connect to: ");
  Serial.println(url);

  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(5000);
  
  int httpCode = http.POST(payload);

  if (httpCode > 0) {
    Serial.printf("[HTTP] Response code: %d\n", httpCode);
    if (httpCode == HTTP_CODE_OK) {
      String response = http.getString();
      Serial.println("[HTTP] Response: " + response);
    }
  } else {
    Serial.print("[HTTP] Failed, error: ");
    Serial.println(http.errorToString(httpCode));
  }

  http.end();
}

void loop() {  // <-- This is the critical function that was missing
  static unsigned long lastUpdate = 0;
  const unsigned long updateInterval = 5000;

  if (millis() - lastUpdate >= updateInterval) {
    lastUpdate = millis();

    int raw = 0;
    for (int i = 0; i < 10; i++) {
      raw += analogRead(BATTERY_PIN);
      delay(10);
    }
    raw /= 10;

    float voltage = raw * (3.3 / 4095.0) * 2.0;
    int percentage = map(raw, 1500, 2500, 0, 100);
    percentage = constrain(percentage, 0, 100);
    bool charging = (voltage > 3.7);

    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 5);
    display.print("V:");
    display.print(voltage, 1);
    display.println("V");
    display.setCursor(0, 35);
    display.print("B:");
    display.print(percentage);
    display.println("%");
    if (charging) {
      display.print("CHG");
    }
    display.display();

    Serial.println("---------------------");
    Serial.print("Voltage: ");
    Serial.print(voltage, 2);
    Serial.println(" V");
    Serial.print("Battery: ");
    Serial.print(percentage);
    Serial.println(" %");
    Serial.print("Charging: ");
    Serial.println(charging ? "Yes" : "No");
    Serial.println("---------------------");

    sendData(voltage, percentage, charging);
  }
}