#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_now.h>

const char* WIFI_SSID = "******";
const char* WIFI_PASSWORD = "******";

const char* API_STATE = "http://X.X.X.X:8000/estado";
const unsigned long API_INTERVAL = 1000;
unsigned long lastApiSend = 0;

#define AVENUE_RED     18
#define AVENUE_YELLOW  19
#define AVENUE_GREEN   21

#define ROAD_RED        25
#define ROAD_YELLOW     26
#define ROAD_GREEN      27

#define PEDESTRIAN_RED        22
#define PEDESTRIAN_GREEN      23
#define BUZZER         32
#define TOUCH_PIN      33

#define CAMERA_AVENUE    1
#define CAMERA_SECONDARY 2

typedef struct {
    uint8_t cameraId;
    int count;
} CameraData;

CameraData data;

uint8_t avenueCameraMac[] = {
    0x, 0x, 0x, 0x, 0x, 0x
};   

uint8_t secondaryCameraMac[] = {
    0x, 0x, 0x, 0x, 0x, 0x
};   

typedef struct {
    uint8_t type;            
    unsigned long intervalMs;
} CameraCommand;

#define CMD_START      0
#define CMD_STOP       1
#define CMD_INTERVAL  2

volatile int avenueCars = 0;
volatile int secondaryCars = 0;

unsigned long avenueWaitStart = 0;
unsigned long secondaryWaitStart = 0;

const unsigned long ISOLATED_CAR_TIME = 5000;
const unsigned long YELLOW_TIME = 2000;
const unsigned long TIME_TO_CLOSE = 10000;
const unsigned long PEDESTRIAN_TIME = 10000;

enum TrafficLightState {
    AVENUE_GREEN,
    AVENUE_YELLOW,
    ROAD_GREEN,
    ROAD_YELLOW,
    ALL_RED,
    PEDESTRIAN_GREEN
};

TrafficLightState state = AVENUE_GREEN;
unsigned long stateStart = 0;

bool pedestrianRequest = false;
unsigned long pedestrianRequestTime = 0;
bool previousTouch = false;

void turnOffAvenueLight() {
    digitalWrite(AVENUE_RED, LOW);
    digitalWrite(AVENUE_YELLOW, LOW);
    digitalWrite(AVENUE_GREEN, LOW);
}

void turnOffSecondaryLight() {
    digitalWrite(ROAD_RED, LOW);
    digitalWrite(ROAD_YELLOW, LOW);
    digitalWrite(ROAD_GREEN, LOW);
}

void avenueRed() {
    digitalWrite(AVENUE_RED, HIGH);
    digitalWrite(AVENUE_YELLOW, LOW);
    digitalWrite(AVENUE_GREEN, LOW);
}

void avenueYellow() {
    digitalWrite(AVENUE_RED, LOW);
    digitalWrite(AVENUE_YELLOW, HIGH);
    digitalWrite(AVENUE_GREEN, LOW);
}

void avenueGreen() {
    digitalWrite(AVENUE_RED, LOW);
    digitalWrite(AVENUE_YELLOW, LOW);
    digitalWrite(AVENUE_GREEN, HIGH);
}

void secondaryRed() {
    digitalWrite(ROAD_RED, HIGH);
    digitalWrite(ROAD_YELLOW, LOW);
    digitalWrite(ROAD_GREEN, LOW);
}

void secondaryYellow() {
    digitalWrite(ROAD_RED, LOW);
    digitalWrite(ROAD_YELLOW, HIGH);
    digitalWrite(ROAD_GREEN, LOW);
}

void secondaryGreen() {
    digitalWrite(ROAD_RED, LOW);
    digitalWrite(ROAD_YELLOW, LOW);
    digitalWrite(ROAD_GREEN, HIGH);
}

void startAvenueGreen() {
    avenueGreen();
    secondaryRed();
    state = AVENUE_GREEN;
    stateStart = millis();
    Serial.println();
    Serial.println(">>> AVENIDA: VERDE <<<");
}

void startSecondaryGreen() {
    avenueRed();
    secondaryGreen();
    state = ROAD_GREEN;
    stateStart = millis();
    Serial.println();
    Serial.println(">>> RUA SECUNDARIA: VERDE <<<");
}

void startAllRed() {
    avenueRed();
    secondaryRed();
    state = ALL_RED;
    stateStart = millis();
    Serial.println();
    Serial.println(">>> TODOS OS SEMÁFOROS VERMELHOS <<<");
}

void startPedestrian() {
    avenueRed();
    secondaryRed();
    digitalWrite(PEDESTRIAN_RED, LOW);
    digitalWrite(PEDESTRIAN_GREEN, HIGH);
    digitalWrite(BUZZER, HIGH);
    state = PEDESTRIAN_GREEN;
    stateStart = millis();
    Serial.println();
    Serial.println("=================================");
    Serial.println(">>> PEDESTRE: VERDE <<<");
    Serial.println(">>> BUZZER LIGADO <<<");
    Serial.println("=================================");
}

void finishPedestrian() {
    digitalWrite(PEDESTRIAN_GREEN, LOW);
    digitalWrite(PEDESTRIAN_RED, HIGH);
    digitalWrite(BUZZER, LOW);
    pedestrianRequest = false;
    Serial.println();
    Serial.println(">>> TRAVESSIA DE PEDESTRE ENCERRADA <<<");
}

unsigned long avenueWaitTime() {
    if (avenueCars <= 0) return 0;
    if (avenueWaitStart == 0) return 0;
    return (millis() - avenueWaitStart) / 1000;
}

unsigned long secondaryWaitTime() {
    if (secondaryCars <= 0) return 0;
    if (secondaryWaitStart == 0) return 0;
    return (millis() - secondaryWaitStart) / 1000;
}

long avenueWeight() {
    unsigned long waitTime = avenueWaitTime();
    return (avenueCars * 5L + waitTime);
}

long secondaryWeight() {
    unsigned long waitTime = secondaryWaitTime();
    return (secondaryCars * 5L + waitTime);
}

bool avenueMandatoryPriority() {
    return (avenueWaitTime() >= 60);
}

bool secondaryMandatoryPriority() {
    return (secondaryWaitTime() >= 60);
}

void showPriorities() {
    unsigned long avenueWait = avenueWaitTime();
    unsigned long secondaryWait = secondaryWaitTime();
    Serial.println();
    Serial.println("========= FILAS =========");
    Serial.print("Avenida: ");
    Serial.print(avenueCars);
    Serial.print(" carros | espera: ");
    Serial.print(avenueWait);
    Serial.print("s | peso: ");
    Serial.println(avenueWeight());
    Serial.print("Secundaria: ");
    Serial.print(secondaryCars);
    Serial.print(" carros | espera: ");
    Serial.print(secondaryWait);
    Serial.print("s | peso: ");
    Serial.println(secondaryWeight());
    Serial.println("=========================");
}

bool secondaryHasPriority() {
    if (secondaryCars <= 0) return false;
    if (avenueCars <= 0) return true;
    if (secondaryMandatoryPriority()) return true;
    if (avenueMandatoryPriority()) return false;
    return (secondaryWeight() > avenueWeight());
}

bool avenueHasPriority() {
    if (avenueCars <= 0) return false;
    if (secondaryCars <= 0) return true;
    if (avenueMandatoryPriority()) return true;
    if (secondaryMandatoryPriority()) return false;
    return (avenueWeight() > secondaryWeight());
}

void updateQueue(uint8_t camera, int count) {
    if (count < 0) count = 0;
    if (camera == CAMERA_AVENUE) {
        int previous = avenueCars;
        avenueCars = count;
        if (previous == 0 && count > 0) {
            avenueWaitStart = millis();
            Serial.println("Avenida: nova fila detectada.");
        }
        if (count == 0) {
            avenueWaitStart = 0;
            Serial.println("Avenida: fila esvaziada.");
        }
    } else if (camera == CAMERA_SECONDARY) {
        int previous = secondaryCars;
        secondaryCars = count;
        if (previous == 0 && count > 0) {
            secondaryWaitStart = millis();
            Serial.println("Secundária: nova fila detectada.");
        }
        if (count == 0) {
            secondaryWaitStart = 0;
            Serial.println("Secundária: fila esvaziada.");
        }
    }
    Serial.print("Camera ");
    Serial.print(camera);
    Serial.print(" -> ");
    Serial.print(count);
    Serial.println(" carros");
}

void sendApiState() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("API: WiFi desconectado.");
        return;
    }
    HTTPClient http;
    http.begin(API_STATE);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String stateText;
    switch (state) {
        case AVENUE_GREEN:   stateText = "AVENIDA_VERDE"; break;
        case AVENUE_YELLOW:  stateText = "AVENIDA_AMARELO"; break;
        case ROAD_GREEN:     stateText = "RUA_VERDE"; break;
        case ROAD_YELLOW:    stateText = "RUA_AMARELO"; break;
        case ALL_RED:        stateText = "TODOS_VERMELHOS"; break;
        case PEDESTRIAN_GREEN: stateText = "PEDESTRE_VERDE"; break;
        default:             stateText = "DESCONHECIDO"; break;
    }
    String data = "avenida_carros=" + String(avenueCars) +
                  "&avenida_espera=" + String(avenueWaitTime()) +
                  "&avenida_peso=" + String(avenueWeight()) +
                  "&secundaria_carros=" + String(secondaryCars) +
                  "&secundaria_espera=" + String(secondaryWaitTime()) +
                  "&secundaria_peso=" + String(secondaryWeight()) +
                  "&semaforo=" + stateText +
                  "&avenida_prioridade=" + String(avenueMandatoryPriority() ? "true" : "false") +
                  "&secundaria_prioridade=" + String(secondaryMandatoryPriority() ? "true" : "false");
    int code = http.POST(data);
    if (code > 0) {
    } else {
        Serial.print("API: erro no envio. Código: ");
        Serial.println(code);
    }
    http.end();
}

void sendCameraCommand(uint8_t* mac, uint8_t type, unsigned long intervalMs) {
    CameraCommand command;
    command.type = type;
    command.intervalMs = intervalMs;
    esp_err_t result = esp_now_send(mac, (uint8_t*)&command, sizeof(command));
    Serial.print("Comando -> ");
    for (int i = 0; i < 6; i++) {
        if (mac[i] < 16) Serial.print("0");
        Serial.print(mac[i], HEX);
        if (i < 5) Serial.print(":");
    }
    Serial.print(" | tipo=");
    Serial.print(type);
    Serial.print(" | resultado=");
    Serial.println(esp_err_to_name(result));
}

void sendAvenueCommand(uint8_t type, unsigned long intervalMs) {
    sendCameraCommand(avenueCameraMac, type, intervalMs);
}

void sendSecondaryCommand(uint8_t type, unsigned long intervalMs) {
    sendCameraCommand(secondaryCameraMac, type, intervalMs);
}

void sendAllCameraCommands(uint8_t type, unsigned long intervalMs) {
    sendAvenueCommand(type, intervalMs);
    sendSecondaryCommand(type, intervalMs);
}

void checkSerialCommands() {
    if (!Serial.available()) return;
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();
    bool avenueTarget = true;
    bool secondaryTarget = true;
    if (command.endsWith(" avenida")) {
        secondaryTarget = false;
        command = command.substring(0, command.length() - 8);
        command.trim();
    } else if (command.endsWith(" secundaria")) {
        avenueTarget = false;
        command = command.substring(0, command.length() - 11);
        command.trim();
    }
    if (command == "start" || command == "iniciar") {
        if (avenueTarget) sendAvenueCommand(CMD_START, 0);
        if (secondaryTarget) sendSecondaryCommand(CMD_START, 0);
        Serial.println("Comando START enviado.");
        return;
    }
    if (command == "stop" || command == "parar") {
        if (avenueTarget) sendAvenueCommand(CMD_STOP, 0);
        if (secondaryTarget) sendSecondaryCommand(CMD_STOP, 0);
        Serial.println("Comando STOP enviado.");
        return;
    }
    if (command.startsWith("intervalo ")) {
        long newInterval = command.substring(10).toInt();
        if (newInterval > 0) {
            if (avenueTarget) sendAvenueCommand(CMD_INTERVAL, newInterval);
            if (secondaryTarget) sendSecondaryCommand(CMD_INTERVAL, newInterval);
            Serial.print("Comando INTERVALO enviado: ");
            Serial.println(newInterval);
        } else {
            Serial.println("Intervalo invalido. Use: intervalo 3000");
        }
        return;
    }
    Serial.println("Comando desconhecido.");
    Serial.println("Use: start | stop | intervalo <ms> (opcional: 'avenida'/'secundaria' no final)");
}

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
    Serial.println();
    Serial.println(">>> PACOTE ESP-NOW RECEBIDO <<<");
    Serial.print("MAC remetente: ");
    for (int i = 0; i < 6; i++) {
        if (info->src_addr[i] < 16) Serial.print("0");
        Serial.print(info->src_addr[i], HEX);
        if (i < 5) Serial.print(":");
    }
    Serial.println();
    Serial.print("Tamanho recebido: ");
    Serial.println(len);
    Serial.print("Tamanho esperado: ");
    Serial.println(sizeof(CameraData));
    if (len != sizeof(CameraData)) {
        Serial.println("ERRO: tamanho inesperado!");
        return;
    }
    memcpy(&data, incomingData, sizeof(data));
    Serial.print("Camera: ");
    Serial.println(data.cameraId);
    Serial.print("Quantidade: ");
    Serial.println(data.count);
    updateQueue(data.cameraId, data.count);
}

void checkTouch() {
    bool currentTouch = digitalRead(TOUCH_PIN);
    if (currentTouch && !previousTouch && !pedestrianRequest && state != PEDESTRIAN_GREEN) {
        pedestrianRequest = true;
        pedestrianRequestTime = millis();
        Serial.println();
        Serial.println("=================================");
        Serial.println(">>> PEDIDO DE PEDESTRE <<<");
        Serial.println("10 segundos até fechar os carros.");
        Serial.println("=================================");
    }
    previousTouch = currentTouch;
}

void processPedestrian() {
    if (!pedestrianRequest) return;
    if (state == PEDESTRIAN_GREEN) {
        if (millis() - stateStart >= PEDESTRIAN_TIME) {
            finishPedestrian();
            chooseNextTrafficLight();
        }
        return;
    }
    if (millis() - pedestrianRequestTime < TIME_TO_CLOSE) return;
    if (state == AVENUE_GREEN) {
        avenueYellow();
        state = AVENUE_YELLOW;
        stateStart = millis();
        Serial.println("Pedestre: fechando avenida.");
        return;
    }
    if (state == ROAD_GREEN) {
        secondaryYellow();
        state = ROAD_YELLOW;
        stateStart = millis();
        Serial.println("Pedestre: fechando rua secundária.");
        return;
    }
    if (state == AVENUE_YELLOW || state == ROAD_YELLOW) {
        if (millis() - stateStart >= YELLOW_TIME) {
            startAllRed();
            delay(500);
            startPedestrian();
        }
        return;
    }
}

void chooseNextTrafficLight() {
    Serial.println();
    Serial.println("Escolhendo próxima via...");
    showPriorities();
    if (avenueCars <= 0 && secondaryCars <= 0) {
        Serial.println("Nenhuma fila. Avenida permanece verde.");
        startAvenueGreen();
        return;
    }
    if (secondaryHasPriority()) {
        startSecondaryGreen();
        return;
    }
    startAvenueGreen();
}

void processGreen() {
    if (pedestrianRequest) return;
    if (state == AVENUE_GREEN) {
        if (avenueCars == 0 && secondaryCars > 0) {
            avenueYellow();
            state = AVENUE_YELLOW;
            stateStart = millis();
            return;
        }
        if (avenueCars == 1) {
            if (millis() - stateStart >= ISOLATED_CAR_TIME) {
                if (secondaryCars > 0 && secondaryHasPriority()) {
                    avenueYellow();
                    state = AVENUE_YELLOW;
                    stateStart = millis();
                }
            }
            return;
        }
        if (secondaryMandatoryPriority()) {
            avenueYellow();
            state = AVENUE_YELLOW;
            stateStart = millis();
            Serial.println("Secundária atingiu prioridade OBRIGATÓRIA.");
            return;
        }
    }
    if (state == ROAD_GREEN) {
        if (secondaryCars == 0) {
            secondaryYellow();
            state = ROAD_YELLOW;
            stateStart = millis();
            Serial.println("Fila secundária esvaziou.");
            return;
        }
        if (secondaryCars == 1) {
            if (millis() - stateStart >= ISOLATED_CAR_TIME) {
                if (avenueCars > 0 && avenueHasPriority()) {
                    secondaryYellow();
                    state = ROAD_YELLOW;
                    stateStart = millis();
                }
            }
            return;
        }
        if (avenueHasPriority()) {
            secondaryYellow();
            state = ROAD_YELLOW;
            stateStart = millis();
            Serial.println("Avenida ganhou prioridade.");
            return;
        }
    }
}

void processYellow() {
    if (millis() - stateStart < YELLOW_TIME) return;
    if (state == AVENUE_YELLOW) {
        startAllRed();
        delay(300);
        if (pedestrianRequest) {
            startPedestrian();
            return;
        }
        chooseNextTrafficLight();
        return;
    }
    if (state == ROAD_YELLOW) {
        startAllRed();
        delay(300);
        if (pedestrianRequest) {
            startPedestrian();
            return;
        }
        chooseNextTrafficLight();
        return;
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println();
    Serial.println("==========================");
    Serial.println("ESP32 PRINCIPAL");
    Serial.println("==========================");
    pinMode(AVENUE_RED, OUTPUT);
    pinMode(AVENUE_YELLOW, OUTPUT);
    pinMode(AVENUE_GREEN, OUTPUT);
    pinMode(ROAD_RED, OUTPUT);
    pinMode(ROAD_YELLOW, OUTPUT);
    pinMode(ROAD_GREEN, OUTPUT);
    pinMode(PEDESTRIAN_RED, OUTPUT);
    pinMode(PEDESTRIAN_GREEN, OUTPUT);
    pinMode(BUZZER, OUTPUT);
    pinMode(TOUCH_PIN, INPUT);
    turnOffAvenueLight();
    turnOffSecondaryLight();
    digitalWrite(PEDESTRIAN_RED, HIGH);
    digitalWrite(PEDESTRIAN_GREEN, LOW);
    digitalWrite(BUZZER, LOW);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Conectando ao WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("WiFi conectado!");
    WiFi.setSleep(false);
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC STA: ");
    Serial.println(WiFi.macAddress());
    Serial.print("Canal WiFi: ");
    Serial.println(WiFi.channel());
    if (esp_now_init() != ESP_OK) {
        Serial.println("ERRO: falha ao inicializar ESP-NOW");
        while (true) delay(1000);
    }
    Serial.println("ESP-NOW inicializado!");
    esp_err_t cbResult = esp_now_register_recv_cb(onDataRecv);
    if (cbResult != ESP_OK) {
        Serial.print("Erro registrando callback: ");
        Serial.println(esp_err_to_name(cbResult));
        while (true) delay(1000);
    }
    Serial.println("Callback ESP-NOW registrado!");
    esp_now_peer_info_t avenuePeer = {};
    memcpy(avenuePeer.peer_addr, avenueCameraMac, 6);
    avenuePeer.channel = 0;
    avenuePeer.encrypt = false;
    if (!esp_now_is_peer_exist(avenueCameraMac)) {
        esp_err_t r1 = esp_now_add_peer(&avenuePeer);
        Serial.print("Peer avenida adicionado: ");
        Serial.println(esp_err_to_name(r1));
    }
    esp_now_peer_info_t secondaryPeer = {};
    memcpy(secondaryPeer.peer_addr, secondaryCameraMac, 6);
    secondaryPeer.channel = 0;
    secondaryPeer.encrypt = false;
    if (!esp_now_is_peer_exist(secondaryCameraMac)) {
        esp_err_t r2 = esp_now_add_peer(&secondaryPeer);
        Serial.print("Peer secundaria adicionado: ");
        Serial.println(esp_err_to_name(r2));
    }
    Serial.println();
    Serial.println("Digite um comando para controlar as cameras:");
    Serial.println("start | stop | intervalo <ms>  (adicione 'avenida' ou 'secundaria' no final para controlar so uma)");
    startAvenueGreen();
    Serial.println();
    Serial.println("Sistema pronto.");
}

void loop() {
    checkSerialCommands();
    checkTouch();
    processPedestrian();
    processYellow();
    processGreen();
    if (millis() - lastApiSend >= API_INTERVAL) {
        lastApiSend = millis();
        sendApiState();
    }
    delay(10);
}