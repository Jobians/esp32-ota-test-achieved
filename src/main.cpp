#include <WiFi.h>
#include <ArduinoOTA.h>

const char* ssid = "YourWiFiName";
const char* password = "YourWiFiPassword";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  ArduinoOTA.setHostname("esp32-ota");
  ArduinoOTA.begin();

  Serial.println("Ready for OTA updates!");
}

void loop() {
  ArduinoOTA.handle();
  // Your normal loop code here
}