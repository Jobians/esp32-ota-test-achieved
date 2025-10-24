#include <WiFi.h>
#include <ArduinoOTA.h>

const char* ssid = "OPPO Reno12 5G";
const char* password = "qiwa7765";

const int ledPin = 33; // Onboard LED / flash pin

void setup() {
  Serial.begin(115200);
  
  // Set LED pin as output
  pinMode(ledPin, OUTPUT);

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

  // Blink the onboard LED
  digitalWrite(ledPin, HIGH);
  delay(500);
  digitalWrite(ledPin, LOW);
  delay(500);
}
