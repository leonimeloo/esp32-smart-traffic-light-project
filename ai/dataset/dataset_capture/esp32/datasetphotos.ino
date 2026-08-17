#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>

const char* wifiSsid = "******";
const char* wifiPassword = "*****";
const char* serverUrl = "http://X.X.X.X:8080/upload";

void checkSerial() {
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'r' || cmd == 'R') {
      Serial.println("Rebooting ESP32...");
      delay(100);
      ESP.restart();
    }
  }
}

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

WebServer server(80);

int photosPerSequence = 10;
unsigned long photoInterval = 1500;
int sequenceNumber = 1;
int currentPhoto = 0;
bool capturing = false;
unsigned long lastCaptureTime = 0;
String lastStatus = "Ready";
int singlePhotoCounter = 1;

bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 10;
  if (psramFound()) {
    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  }
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init error: 0x%x\n", err);
    return false;
  }
  sensor_t *s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);
  s->set_vflip(s, 1);
  return true;
}

void connectWiFi() {
  WiFi.begin(wifiSsid, wifiPassword);
  Serial.println();
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("ESP32-CAM IP: ");
  Serial.println(WiFi.localIP());
}

bool sendImage(camera_fb_t* fb, int seq, int photoNum) {
  if (WiFi.status() != WL_CONNECTED) {
    lastStatus = "WiFi disconnected";
    return false;
  }
  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "image/jpeg");
  char filename[50];
  sprintf(filename, "photo_%03d.jpg", photoNum);
  http.addHeader("X-Filename", filename);
  char sequence[20];
  sprintf(sequence, "%03d", seq);
  http.addHeader("X-Sequence", sequence);
  int code = http.POST(fb->buf, fb->len);
  if (code > 0) {
    Serial.printf("Upload: seq=%03d photo=%03d HTTP=%d size=%d bytes\n", seq, photoNum, code, fb->len);
    http.end();
    if (code >= 200 && code < 300) return true;
  } else {
    Serial.printf("HTTP error: %s\n", http.errorToString(code).c_str());
  }
  http.end();
  return false;
}

bool capturePhoto() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("ERROR: camera_fb_get() returned NULL");
    lastStatus = "Capture error";
    return false;
  }
  Serial.printf("Image captured: %dx%d - %d bytes\n", fb->width, fb->height, fb->len);
  bool success = sendImage(fb, sequenceNumber, currentPhoto);
  esp_camera_fb_return(fb);
  if (success) {
    lastStatus = "Photo " + String(currentPhoto) + "/" + String(photosPerSequence) + " sent";
    return true;
  }
  lastStatus = "Upload error";
  return false;
}

const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32-CAM Dataset</title>
<style>
body { font-family: Arial, sans-serif; background: #111; color: white; margin: 0; padding: 20px; }
.container { max-width: 700px; margin: auto; }
h1 { text-align: center; }
.card { background: #222; padding: 20px; border-radius: 12px; margin-top: 15px; }
input { width: 100%; box-sizing: border-box; padding: 12px; margin-top: 5px; margin-bottom: 15px; border-radius: 6px; border: none; font-size: 16px; }
button { width: 100%; padding: 15px; margin-top: 8px; border: none; border-radius: 8px; font-size: 17px; cursor: pointer; }
.start { background: #00a86b; color: white; }
.stop { background: #d9534f; color: white; }
.single { background: #337ab7; color: white; }
.refresh { background: #555; color: white; }
.progress { width: 100%; height: 25px; background: #444; border-radius: 10px; overflow: hidden; margin-top: 15px; }
.bar { height: 100%; width: 0%; background: #00a86b; transition: width 0.3s; }
.status { font-size: 18px; margin-top: 15px; }
.preview { width: 100%; margin-top: 15px; border-radius: 8px; }
.info { line-height: 1.6; }
</style>
</head>
<body>
<div class="container">
<h1>ESP32-CAM Dataset</h1>
<div class="card">
<h2>Settings</h2>
<label>Photos per sequence</label>
<input id="photos" type="number" min="1" max="1000" value="10">
<label>Interval between photos (ms)</label>
<input id="interval" type="number" min="500" max="60000" value="1500">
<button class="start" onclick="startSequence()">📸 START SEQUENCE</button>
<button class="stop" onclick="stopSequence()">⏹ STOP</button>
<button class="single" onclick="singlePhoto()">📷 TAKE ONE PHOTO</button>
</div>
<div class="card">
<h2>Status</h2>
<div class="info">
<div>Sequence: <span id="sequence">-</span></div>
<div>Photos: <span id="photo">0</span></div>
<div>Status: <span id="status">Ready</span></div>
</div>
<div class="progress"><div id="bar" class="bar"></div></div>
</div>
<div class="card">
<h2>Preview</h2>
<img id="preview" class="preview" src="/capture">
<button class="refresh" onclick="refreshImage()">🔄 Refresh</button>
</div>
</div>
<script>
function startSequence() {
  let photos = document.getElementById("photos").value;
  let interval = document.getElementById("interval").value;
  fetch("/start?photos=" + photos + "&interval=" + interval);
}
function stopSequence() {
  fetch("/stop");
}
function singlePhoto() {
  fetch("/single").then(() => { refreshImage(); });
}
function refreshImage() {
  document.getElementById("preview").src = "/capture?t=" + new Date().getTime();
}
function updateStatus() {
  fetch("/status")
  .then(response => response.json())
  .then(data => {
    document.getElementById("sequence").innerText = data.sequence;
    document.getElementById("photo").innerText = data.photo + " / " + data.total;
    document.getElementById("status").innerText = data.status;
    let percent = 0;
    if (data.total > 0) percent = (data.photo / data.total) * 100;
    document.getElementById("bar").style.width = percent + "%";
    if (data.capturing) setTimeout(updateStatus, 500);
    else setTimeout(updateStatus, 1000);
  });
}
setInterval(refreshImage, 3000);
updateStatus();
</script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", PAGE);
}

void handleStart() {
  if (capturing) {
    server.send(409, "text/plain", "A sequence is already running.");
    return;
  }
  if (server.hasArg("photos")) photosPerSequence = server.arg("photos").toInt();
  if (server.hasArg("interval")) photoInterval = server.arg("interval").toInt();
  if (photosPerSequence < 1) photosPerSequence = 1;
  if (photosPerSequence > 1000) photosPerSequence = 1000;
  if (photoInterval < 500) photoInterval = 500;
  currentPhoto = 0;
  capturing = true;
  lastCaptureTime = millis();
  lastStatus = "Sequence started";
  Serial.println();
  Serial.println("==============================");
  Serial.printf("SEQUENCE %03d\n", sequenceNumber);
  Serial.printf("Photos: %d\n", photosPerSequence);
  Serial.printf("Interval: %lu ms\n", photoInterval);
  Serial.println("==============================");
  server.send(200, "text/plain", "OK");
}

void handleStop() {
  capturing = false;
  lastStatus = "Sequence stopped";
  Serial.println("Sequence stopped by user.");
  server.send(200, "text/plain", "OK");
}

void handleSingle() {
  if (capturing) {
    server.send(409, "text/plain", "Sequence in progress.");
    return;
  }
  currentPhoto = singlePhotoCounter;
  bool success = capturePhoto();
  if (success) {
    singlePhotoCounter++;
    server.send(200, "text/plain", "Photo sent");
  } else {
    server.send(500, "text/plain", "Capture error");
  }
}

void handleStatus() {
  String json = "{";
  json += "\"capturing\":" + String(capturing ? "true" : "false") + ",";
  json += "\"sequence\":" + String(sequenceNumber) + ",";
  json += "\"photo\":" + String(currentPhoto) + ",";
  json += "\"total\":" + String(photosPerSequence) + ",";
  json += "\"status\":\"" + lastStatus + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handleCapture() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "text/plain", "Capture failed");
    return;
  }
  server.send_P(200, "image/jpeg", (const char*)fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

void processSequence() {
  if (!capturing) return;
  unsigned long now = millis();
  if (now - lastCaptureTime < photoInterval) return;
  currentPhoto++;
  Serial.printf("Capturing photo %d/%d...\n", currentPhoto, photosPerSequence);
  bool success = capturePhoto();
  if (!success) {
    Serial.println("Failed. Photo not counted.");
    currentPhoto--;
    lastCaptureTime = millis();
    return;
  }
  lastCaptureTime = millis();
  if (currentPhoto >= photosPerSequence) {
    capturing = false;
    lastStatus = "Sequence completed";
    Serial.println();
    Serial.printf("Sequence %03d completed!\n", sequenceNumber);
    Serial.println();
    sequenceNumber++;
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println();
  Serial.println("==============================");
  Serial.println("ESP32-CAM DATASET SERVER");
  Serial.println("==============================");
  if (!initCamera()) {
    Serial.println("Camera init failed.");
    while (true) delay(1000);
  }
  Serial.println("Camera started!");
  connectWiFi();
  server.on("/", HTTP_GET, handleRoot);
  server.on("/start", HTTP_GET, handleStart);
  server.on("/stop", HTTP_GET, handleStop);
  server.on("/single", HTTP_GET, handleSingle);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/capture", HTTP_GET, handleCapture);
  server.begin();
  Serial.println();
  Serial.println("Server started!");
  Serial.print("Open browser at: http://");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void loop() {
  checkSerial();
  server.handleClient();
  processSequence();
  delay(2);
}