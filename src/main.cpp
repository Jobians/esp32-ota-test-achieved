#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Update.h>

// ====== WiFi Credentials ======
const char* ssid = "OPPO Reno12 5G";
const char* password = "qiwa7765";

// ====== OTA Config ======
const char* firmwareURL = "https://github.com/Jobians/esp32-ota-test/releases/download/v1.0.0/firmware.bin";
const unsigned long OTA_TIMEOUT = 120000; // 2 minutes

WebServer server(80); // Web server on port 80

#define PIN_FLASHLIGHT 4

// ====== CAMERA CONFIG (ESP32-CAM AI Thinker) ======
camera_config_t config = {
  .ledc_channel = LEDC_CHANNEL_0,
  .ledc_timer = LEDC_TIMER_0,
  .pin_d0 = 5,
  .pin_d1 = 18,
  .pin_d2 = 19,
  .pin_d3 = 21,
  .pin_d4 = 36,
  .pin_d5 = 39,
  .pin_d6 = 34,
  .pin_d7 = 35,
  .pin_xclk = 0,
  .pin_pclk = 22,
  .pin_vsync = 25,
  .pin_href = 23,
  .pin_sccb_sda = 26,
  .pin_sccb_scl = 27,
  .pin_pwdn = 32,
  .pin_reset = -1,
  .xclk_freq_hz = 10000000,
  .pixel_format = PIXFORMAT_JPEG,
  .frame_size = FRAMESIZE_QVGA,
  .jpeg_quality = 12,
  .fb_count = 2
};

// ====== Camera Stream ======
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static esp_err_t stream_handler(httpd_req_t* req) {
  camera_fb_t* fb = NULL;
  esp_err_t res = ESP_OK;
  char part_buf[64];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) return res;

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
      break;
    }

    size_t hlen = snprintf(part_buf, sizeof(part_buf), _STREAM_PART, fb->len);
    res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, part_buf, hlen);
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, (const char*)fb->buf, fb->len);

    esp_camera_fb_return(fb);
    if (res != ESP_OK) break;
  }
  return res;
}

void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 81;  // Run on port 81 to not conflict with OTA server
  httpd_handle_t stream_httpd = NULL;

  httpd_uri_t stream_uri = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = stream_handler,
    .user_ctx = NULL
  };

  if (httpd_start(&stream_httpd, &config) == ESP_OK)
    httpd_register_uri_handler(stream_httpd, &stream_uri);

  Serial.println("📷 Camera stream ready at:");
  Serial.println("👉 http://" + WiFi.localIP().toString() + ":81/");
}

// ====== WiFi ======
void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected!");
  Serial.println("IP: " + WiFi.localIP().toString());
}

// ====== OTA ======
bool otaUpdate(const char* url) {
  HTTPClient http;
  http.begin(url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  int httpCode = http.GET();
  if (httpCode != 200) {
    Serial.println("Failed to download firmware. HTTP code: " + String(httpCode));
    http.end();
    return false;
  }

  int contentLength = http.getSize();
  if (contentLength <= 0) {
    Serial.println("Invalid firmware size");
    http.end();
    return false;
  }

  Serial.printf("Firmware size: %d KB\n", contentLength / 1024);

  if (!Update.begin(contentLength)) {
    Serial.println("OTA begin failed!");
    Serial.println("Error: " + String(Update.errorString()));
    http.end();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();
  size_t written = 0;
  uint8_t buffer[128];
  unsigned long lastDataTime = millis();

  while (written < contentLength) {
    if (stream->available()) {
      size_t len = stream->read(buffer, sizeof(buffer));
      if (len > 0) {
        Update.write(buffer, len);
        written += len;
        int progress = (written * 100) / contentLength;
        Serial.printf("Progress: %d%%\n", progress);
        lastDataTime = millis();
      }
    }
    if (millis() - lastDataTime > OTA_TIMEOUT) {
      Serial.println("OTA Timeout!");
      Update.abort();
      http.end();
      return false;
    }
    yield();
  }

  if (!Update.end()) {
    Serial.println("OTA failed: " + String(Update.errorString()));
    return false;
  }

  Serial.println("✅ OTA completed. Rebooting...");
  delay(2000);
  ESP.restart();
  return true;
}

void handleUpdate() {
  server.send(200, "text/plain", "Starting OTA update. Check Serial for progress.");
  otaUpdate(firmwareURL);
}

// ====== Setup ======
void setup() {
  Serial.begin(115200);
  pinMode(PIN_FLASHLIGHT, OUTPUT);
  connectWiFi();

  // Start camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed 0x%x\n", err);
    return;
  }

  startCameraServer();

  // Start OTA web server
  server.on("/update", handleUpdate);
  server.begin();

  Serial.println("🟢 OTA endpoint: http://" + WiFi.localIP().toString() + "/update");
}

// ====== Loop ======
void loop() {
  server.handleClient();
}
