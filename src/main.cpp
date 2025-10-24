#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>

const char* ssid = "OPPO Reno12 5G";
const char* password = "qiwa7765";

const int ledPin = 33; // Onboard LED / flash
const char* firmwareURL = "https://github.com/Jobians/esp32-ota-test/releases/download/v1.0.0/firmware.bin";

void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
}

void blinkLED(int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(ledPin, HIGH);
    delay(delayMs);
    digitalWrite(ledPin, LOW);
    delay(delayMs);
  }
}

String getFinalURL(const char* url) {
  HTTPClient http;
  http.begin(url);
  int code = http.GET();

  String finalURL = url;

  if (code == 302 || code == 301) { // Redirect
    finalURL = http.getLocation();
    Serial.println("Redirected to: " + finalURL);
  } else if (code != 200) {
    Serial.println("HTTP request failed with code: " + String(code));
  }

  http.end();
  return finalURL;
}

void otaUpdate(const char* url) {
  String finalURL = getFinalURL(url);
  HTTPClient http;
  http.begin(finalURL);
  int httpCode = http.GET();

  if (httpCode != 200) {
    Serial.println("Failed to download firmware, HTTP code: " + String(httpCode));
    http.end();
    return;
  }

  int contentLength = http.getSize();
  if (contentLength <= 0) {
    Serial.println("Invalid content length");
    http.end();
    return;
  }

  bool canBegin = Update.begin(contentLength);
  if (!canBegin) {
    Serial.println("Not enough space for OTA");
    http.end();
    return;
  }

  WiFiClient* stream = http.getStreamPtr();
  size_t written = Update.writeStream(*stream);

  if (written == contentLength) {
    Serial.println("Written " + String(written) + " bytes successfully");
  } else {
    Serial.println("Written only " + String(written) + "/" + String(contentLength));
  }

  if (Update.end()) {
    if (Update.isFinished()) {
      Serial.println("Update successfully completed. Rebooting...");
      ESP.restart();
    } else {
      Serial.println("Update not finished? Something went wrong.");
    }
  } else {
    Serial.println("Update failed. Error #: " + String(Update.getError()));
  }

  http.end();
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  connectWiFi();

  // Blink LED to indicate OTA start
  blinkLED(3, 200);

  otaUpdate(firmwareURL);
}

void loop() {
  // Optional: blink LED slowly to indicate idle
  digitalWrite(ledPin, HIGH);
  delay(1000);
  digitalWrite(ledPin, LOW);
  delay(1000);
}
