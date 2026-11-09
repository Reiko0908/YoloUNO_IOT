#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "../include/tasks.h"

// --- CONFIG WIFI AND COREIOT (values come from include/secrets.h) ---
#include "../include/secrets.h"
#define COREIOT_TOKEN COREIOT_TOKEN_TASK1

#define SEND_INTERVAL_MS 5000

// ---------------------------------------------------------------------------
// Task 1: Connect WiFi 
// ---------------------------------------------------------------------------
static void wifi_connect_task(void *pvParameters) {
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Connecting WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
    }
    Serial.println("\nWiFi is conencted successfully!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    vTaskDelete(NULL); //delete after completion
}

// ---------------------------------------------------------------------------
// Task 2: Create telemetry data and push to CoreIoT
//  - Temperature in range 25-35 Celcius
//  - Humidity in range 50-80 %
// ---------------------------------------------------------------------------
static void telemetry_task(void *pvParameters) {
    float temp = 36.0f;   // giá trị khởi đầu
    float hum  = 80.0f;

    // URL telemetry
    String serverUrl = "http://app.coreiot.io/api/v1/";
    serverUrl += COREIOT_TOKEN;
    serverUrl += "/telemetry";

    while (1) {
        if (WiFi.status() == WL_CONNECTED) {
        // 1. Create data randomly
        temp += random(-10, 11) / 10.0f;   // ±1.0 °C
        hum  += random(-20, 21) / 10.0f;   // ±2.0 %
        temp = constrain(temp, 25.0f, 35.0f);
        hum  = constrain(hum,  50.0f, 80.0f);

        // 2. Đóng gói JSON
        String tele_dat = "{\"temperature\":" + String(temp, 2) +
                        ",\"humidity\":"    + String(hum, 2)  + "}";

        // 3. Gửi HTTP POST
        HTTPClient http;
        http.begin(serverUrl);
        http.addHeader("Content-Type", "application/json");
        int httpCode = http.POST(tele_dat);

        if (httpCode == HTTP_CODE_OK) {
            Serial.print("[CoreIoT] Send successfully (HTTP 200) -> ");
            Serial.println(tele_dat);
        } else if (httpCode > 0) {
            Serial.printf("[CoreIoT] Server rejected, HTTP code: %d \n", httpCode);
        } else {
            Serial.printf("[CoreIoT] Send failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();   
    } 
    else {
        Serial.println("Connecting WiFi...");
    }

    vTaskDelay(pdMS_TO_TICKS(SEND_INTERVAL_MS));
  }
}

// ---------------------------------------------------------------------------
// Call in main.cpp
// ---------------------------------------------------------------------------
void task1_start() {
  Serial.println("\n=== TASK 1: SEND TELEMETRY DATA TO COREIOT ===");

  xTaskCreate(wifi_connect_task, "B1 WiFi", 4096, NULL, 3, NULL);
  xTaskCreate(telemetry_task, "B1 CoreIoT", 8192, NULL, 2, NULL);
}