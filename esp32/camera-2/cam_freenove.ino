#include "esp_camera.h"
#include "img_converters.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_now.h>
#include <esp_wifi.h>

const char* WIFI_SSID = "******";
const char* WIFI_PASSWORD = "******";

const char* API_URL = "http://X.X.X.X:8000/detectar";

#define CAMERA_ID 2

uint8_t mainMAC[] = {
    0x, 0x, 0x, 0x, 0x, 0x
};

// Freenove ESP32-S3-CAM pin definitions
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     15
#define SIOD_GPIO_NUM      4
#define SIOC_GPIO_NUM      5
#define Y9_GPIO_NUM       16
#define Y8_GPIO_NUM       17
#define Y7_GPIO_NUM       18
#define Y6_GPIO_NUM       12
#define Y5_GPIO_NUM       10
#define Y4_GPIO_NUM        8
#define Y3_GPIO_NUM        9
#define Y2_GPIO_NUM       11
#define VSYNC_GPIO_NUM     6
#define HREF_GPIO_NUM      7
#define PCLK_GPIO_NUM      13

bool sending = false;
unsigned long INTERVAL_MS = 3000;
unsigned long lastSend = 0;

WiFiClient client;
HTTPClient http;

typedef struct {
    uint8_t camera;
    int count;
} Result;

Result result;

typedef struct {
    uint8_t type;            // 0 = START, 1 = STOP, 2 = INTERVAL
    unsigned long intervalMs;
} CameraCommand;

#define CMD_START      0
#define CMD_STOP       1
#define CMD_INTERVAL   2

void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
    Serial.print("ESP-NOW callback: ");
    if (status == ESP_NOW_SEND_SUCCESS) {
        Serial.println("DELIVERED");
    } else {
        Serial.println("FAILED");
    }
}

void setupCamera() {
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
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_RGB565;
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 15;
    if (psramFound()) {
        config.fb_location = CAMERA_FB_IN_PSRAM;
        config.fb_count = 2;
    } else {
        config.fb_location = CAMERA_FB_IN_DRAM;
        config.fb_count = 1;
    }
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed: 0x%x\n", err);
        while (true) delay(1000);
    }
    Serial.println("Camera initialized!");
}

void connectWiFi() {
    WiFi.mode(WIFI_STA);
    Serial.println();
    Serial.println("Connecting to Wi-Fi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("Wi-Fi connected!");
    WiFi.setSleep(false);

    // Note: do not force a specific Wi-Fi channel here.
    // All ESP32s connect to the same router, so they automatically stay on the same channel.
    // Forcing a fixed channel can cause ESP-NOW packets to be missed.

    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC STA: ");
    Serial.println(WiFi.macAddress());
    Serial.print("WiFi channel: ");
    Serial.println(WiFi.channel());
}

void setupESPNow() {
    Serial.println();
    Serial.println("Initializing ESP-NOW...");
    if (esp_now_init() != ESP_OK) {
        Serial.println("ERROR: ESP-NOW init failed");
        while (true) delay(1000);
    }
    Serial.println("ESP-NOW initialized!");
    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onCommandRecv);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mainMAC, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_is_peer_exist(mainMAC)) {
        Serial.println("Peer already exists.");
    } else {
        esp_err_t result = esp_now_add_peer(&peerInfo);
        Serial.print("Peer exists: ");
        Serial.println(esp_now_is_peer_exist(mainMAC) ? "YES" : "NO");
        if (result != ESP_OK) {
            Serial.print("Error adding peer: ");
            Serial.println(esp_err_to_name(result));
            while (true) delay(1000);
        }
        Serial.println("ESP-NOW peer added!");
    }

    Serial.print("Destination MAC: ");
    for (int i = 0; i < 6; i++) {
        if (mainMAC[i] < 16) Serial.print("0");
        Serial.print(mainMAC[i], HEX);
        if (i < 5) Serial.print(":");
    }
    Serial.println();
    Serial.println("ESP-NOW configured!");
}

void sendResultESPNow(int count) {
    result.camera = CAMERA_ID;
    result.count = count;
    Serial.println();
    Serial.println("Sending result via ESP-NOW...");
    Serial.print("Camera: ");
    Serial.println(result.camera);
    Serial.print("Count: ");
    Serial.println(result.count);
    Serial.print("Packet size: ");
    Serial.println(sizeof(result));
    esp_err_t ret = esp_now_send(mainMAC, (uint8_t*)&result, sizeof(result));
    Serial.print("ESP-NOW return: ");
    Serial.println(esp_err_to_name(ret));
    if (ret == ESP_OK) {
        Serial.println("ESP-NOW: send accepted!");
    } else {
        Serial.println("ESP-NOW: send error!");
    }
}

void sendImageToAPI() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Wi-Fi disconnected, reconnecting...");
        WiFi.reconnect();
        return;
    }
    Serial.printf("Free heap before capture: %d\n", ESP.getFreeHeap());
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Image capture error");
        return;
    }
    Serial.print("Frame RGB565 captured: ");
    Serial.print(fb->len);
    Serial.println(" bytes");

    uint8_t* jpgBuf = NULL;
    size_t jpgLen = 0;
    bool converted = frame2jpg(fb, 15, &jpgBuf, &jpgLen);
    esp_camera_fb_return(fb);
    if (!converted || !jpgBuf) {
        Serial.println("Error converting RGB565 to JPEG");
        return;
    }
    Serial.print("Converted to JPEG: ");
    Serial.print(jpgLen);
    Serial.println(" bytes");

    bool validJpeg = jpgLen > 4 &&
                     jpgBuf[0] == 0xFF &&
                     jpgBuf[1] == 0xD8 &&
                     jpgBuf[jpgLen - 2] == 0xFF &&
                     jpgBuf[jpgLen - 1] == 0xD9;
    if (!validJpeg) {
        Serial.println("Invalid JPEG, discarding.");
        free(jpgBuf);
        return;
    }

    const char* boundary = "ESP32camBoundary7MA4YWxkTrZu0gW";
    String bodyStart = "--" + String(boundary) + "\r\n" +
                       "Content-Disposition: form-data; name=\"camera_id\"\r\n\r\n" +
                       String(CAMERA_ID) + "\r\n" +
                       "--" + String(boundary) + "\r\n" +
                       "Content-Disposition: form-data; name=\"imagem\"; filename=\"frame.jpg\"\r\n" +
                       "Content-Type: image/jpeg\r\n\r\n";
    String bodyEnd = "\r\n--" + String(boundary) + "--\r\n";
    size_t totalLen = bodyStart.length() + jpgLen + bodyEnd.length();
    uint8_t* body = (uint8_t*)malloc(totalLen);
    if (!body) {
        Serial.println("Error: not enough memory for multipart body");
        free(jpgBuf);
        return;
    }
    memcpy(body, bodyStart.c_str(), bodyStart.length());
    memcpy(body + bodyStart.length(), jpgBuf, jpgLen);
    memcpy(body + bodyStart.length() + jpgLen, bodyEnd.c_str(), bodyEnd.length());
    free(jpgBuf);

    http.begin(client, API_URL);
    http.addHeader("Content-Type", "multipart/form-data; boundary=" + String(boundary));
    http.setTimeout(10000);
    Serial.println("Sending image to API...");
    int httpCode = http.POST(body, totalLen);
    free(body);
    Serial.printf("Free heap after send: %d\n", ESP.getFreeHeap());

    if (httpCode <= 0) {
        Serial.print("HTTP error: ");
        Serial.println(http.errorToString(httpCode));
        http.end();
        return;
    }
    Serial.print("HTTP: ");
    Serial.println(httpCode);
    String response = http.getString();
    Serial.print("API response: ");
    Serial.println(response);
    http.end();

    StaticJsonDocument<768> doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        Serial.print("JSON error: ");
        Serial.println(error.c_str());
        return;
    }
    int count = doc["resultado"]["quantidade_deteccoes"] | 0;
    Serial.print("COUNT received: ");
    Serial.println(count);
    sendResultESPNow(count);
}

void executeCommand(uint8_t type, unsigned long intervalMs) {
    if (type == CMD_START) {
        sending = true;
        lastSend = 0;
        Serial.println("Sending STARTED.");
    } else if (type == CMD_STOP) {
        sending = false;
        Serial.println("Sending STOPPED.");
    } else if (type == CMD_INTERVAL) {
        if (intervalMs > 0) {
            INTERVAL_MS = intervalMs;
            Serial.print("Interval updated to (ms): ");
            Serial.println(INTERVAL_MS);
        } else {
            Serial.println("Invalid interval received.");
        }
    } else {
        Serial.println("Unknown command received via ESP-NOW.");
    }
}

void onCommandRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
    if (len != sizeof(CameraCommand)) return;
    CameraCommand command;
    memcpy(&command, incomingData, sizeof(command));
    Serial.println();
    Serial.println(">>> COMMAND RECEIVED VIA ESP-NOW <<<");
    executeCommand(command.type, command.intervalMs);
}

void checkSerialCommands() {
    if (!Serial.available()) return;
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();
    if (command == "start" || command == "iniciar") {
        executeCommand(CMD_START, 0);
    } else if (command == "stop" || command == "parar") {
        executeCommand(CMD_STOP, 0);
    } else if (command.startsWith("intervalo ")) {
        long newInterval = command.substring(10).toInt();
        executeCommand(CMD_INTERVAL, newInterval);
    } else {
        Serial.println("Unknown command.");
        Serial.println("Use: start | stop | intervalo <ms>");
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println();
    Serial.println("==========================");
    Serial.print("ESP32-CAM ");
    Serial.println(CAMERA_ID);
    Serial.println("==========================");
    Serial.print("PSRAM found: ");
    Serial.println(psramFound() ? "YES" : "NO");
    Serial.print("Free heap: ");
    Serial.println(ESP.getFreeHeap());
    Serial.print("Free PSRAM: ");
    Serial.println(ESP.getFreePsram());

    connectWiFi();
    setupCamera();
    setupESPNow();

    Serial.println();
    Serial.println("Enter a command:");
    Serial.println("start | stop | intervalo <ms>");
}

void loop() {
    checkSerialCommands();
    if (!sending) {
        delay(50);
        return;
    }
    unsigned long now = millis();
    if (now - lastSend >= INTERVAL_MS) {
        lastSend = now;
        sendImageToAPI();
    }
}