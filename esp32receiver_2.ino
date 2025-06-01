#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#define RELAY_BOM 18
#define RELAY_HUT 19
#define LED 2

bool airSystemActive = false;


//unsigned long snore_flag_count = 0;
//unsigned long pump_activation_count = 0;

// Ghi ra Serial1 khi nhận được tin nhắn
void sendToSerial1(const String& message) {
  Serial1.println(message);
}

AsyncWebServer server(80);

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    char msg[32];
    memcpy(msg, incomingData, len);
    msg[len] = '\0';

    Serial.print("Received message: ");
    Serial.println(msg);

    if (strcmp(msg, "pump") == 0) {
        Serial.println("→ Bật bơm");
        digitalWrite(RELAY_BOM, LOW);
        sendToSerial1("pump");

    } else if (strcmp(msg, "no pump") == 0) {
        Serial.println("→ Tắt bơm");
        digitalWrite(RELAY_BOM, HIGH);

    } else if (strcmp(msg, "valve on") == 0) {
        Serial.println("→ Đóng van");
        digitalWrite(RELAY_HUT, LOW); // Đóng van
        sendToSerial1("valve on");

    } else if (strcmp(msg, "valve off") == 0) {
        Serial.println("→ Mở van");
        digitalWrite(RELAY_HUT, HIGH); // Mở van
        sendToSerial1("valve off");

    } else if (strcmp(msg, "snoring episode") == 0) {
        sendToSerial1("snoring episode");

    } else if (strcmp(msg, "snoring") == 0) {
        sendToSerial1("snoring");
    }
}


void setup() {
    Serial.begin(115200);
    Serial1.begin(9600, SERIAL_8N1, 16, 17);  // RX=16, TX=17
    //setCpuFrequencyMhz(80);
    WiFi.mode(WIFI_STA);
    
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed!");
        return;
    }

    esp_now_register_recv_cb(OnDataRecv);

    pinMode(LED, OUTPUT);
    pinMode(RELAY_BOM, OUTPUT);
    pinMode(RELAY_HUT, OUTPUT);
    digitalWrite(RELAY_BOM, HIGH); // Tắt bơm mặc định
    digitalWrite(RELAY_HUT, HIGH); // Tắt hút mặc định

    // Giao diện Web

    Serial.println("ESP32 Receiver Ready");
}

void loop() {
    // Không cần làm gì trong loop, mọi xử lý nằm trong callback
    delay(100);
}
