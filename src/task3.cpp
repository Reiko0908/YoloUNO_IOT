/*
 * BÀI 3: KẾT NỐI I2C VỚI CẢM BIẾN DHT20
 * ---------------------------------------------------------------------------
 * Mục tiêu : Quét bus I2C, đọc nhiệt độ - độ ẩm từ DHT20 định kỳ, in ra
 *            Serial Monitor và (khuyến khích) gửi lên dashboard CoreIoT.
 * Board    : YoloUNO (ESP32-S3)
 * Gọi từ   : main.cpp -> bai3_start()
 *
 * SDA: GPIO 11   SCL: GPIO 12   Địa chỉ I2C: 0x38
 * Chu kỳ đọc : 2000 ms      Chu kỳ gửi CoreIoT : mỗi 3 lần đọc (~6 giây)
 *
 * Thư viện: "DHT20" của Rob Tillaart
 *
 * LƯU Ý: Khi dán vào báo cáo phải giữ nguyên chuỗi "ACCESS_TOKEN" và
 *        "YOUR_WIFI_..." – không để lộ token / mật khẩu Wi-Fi thật.
 */

#include <Arduino.h>
#include <Wire.h>
#include <DHT20.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "../include/tasks.h"

// --- config I2C ---
#define SDA_PIN             11
#define SCL_PIN             12
#define DHT20_ADDR          0x38
#define READ_INTERVAL_MS    2000 
#define PUSH_INTERVAL_MS    5000

// --- CONFIG WIFI AND COREIOT (values come from include/secrets.h) ---
#include "../include/secrets.h"
#define COREIOT_TOKEN COREIOT_TOKEN_TASK3

static DHT20 dht;

static float g_temp  = 0.0f;
static float g_humid   = 0.0f;
static bool  g_valid = false;                           
static portMUX_TYPE g_mux = portMUX_INITIALIZER_UNLOCKED;  

// ---------------------------------------------------------------------------
// scan I2C: try address 1..126
// ---------------------------------------------------------------------------
static void i2c_scan() {
    Serial.println("[I2C] Scanning I2C...");
    int found = 0;

    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {  
            Serial.printf("[I2C] Found at 0x%02X%s\n",
                            addr, (addr == DHT20_ADDR) ? "  <-- DHT20" : "");
            found++;
        }
  }

    if (found == 0) {
        Serial.println("Cannot found any device!");
    } else {
        Serial.printf("Scan completed, found %d device.\n", found);
    }
}

// ---------------------------------------------------------------------------
// Read DHT20 + error detection. Return true if successful
// ---------------------------------------------------------------------------
static bool read_dht20(float &temp, float &hum) {
    int stat = dht.read();

    if (stat != DHT20_OK) {
        const char* msg;
        switch (stat) {
        case -10: msg = "checksum error";           break;
            case -11: msg = "I2C connection failed"; break;
            case -12: msg = "missing data bytes";    break;
            case -13: msg = "all bytes are zero";    break;
            case -14: msg = "read timeout";          break;
            case -15: msg = "read too fast (<1s)";   break;
            default:  msg = "unknown error";         break;
        }
        Serial.printf("Read error, code: %d (%s)\n", stat, msg);
        return false;
    }

    temp = dht.getTemperature();
    hum  = dht.getHumidity();
    return true;
}

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
// Task 2: Init I2C, scan bus, read DHT20 device
// ---------------------------------------------------------------------------
static void dht_task(void *pvParameters) {
    // 1.Init I2C bus
    Wire.begin(SDA_PIN, SCL_PIN);
    // 2. Delay 200ms
    vTaskDelay(pdMS_TO_TICKS(200));
    // 3. Scan bus
    i2c_scan();
    // 4. Init sensor
    dht.begin();
 
    while (1) {
        float temp, humid;
    
        if (read_dht20(temp, humid)) {
            Serial.printf("Temperature: %.2f °C\tHumidity: %.2f %%\n", temp, humid);
        
            // Save value for coreiot_task
            portENTER_CRITICAL(&g_mux);
            g_temp  = temp;
            g_humid   = humid;
            g_valid = true;
            portEXIT_CRITICAL(&g_mux);
        } 
        else {
            portENTER_CRITICAL(&g_mux);
            g_valid = false;
            portEXIT_CRITICAL(&g_mux);
        }
    
        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }
}
 
// ---------------------------------------------------------------------------
// Task 3: Send data DHT20 to CoreIoT
// ---------------------------------------------------------------------------
static void coreiot_task(void *pvParameters) {
    // URL telemetry
    String serverUrl = "http://app.coreiot.io/api/v1/";
    serverUrl += COREIOT_TOKEN;
    serverUrl += "/telemetry";
 
    while (1) {
        // Chỉ gửi dữ liệu khi WiFi đã kết nối
        if (WiFi.status() == WL_CONNECTED) {
        // Lấy bản sao giá trị mới nhất
        float temp, humid;
        bool valid;
        portENTER_CRITICAL(&g_mux);
        temp    = g_temp;
        humid   = g_humid;
        valid   = g_valid;
        portEXIT_CRITICAL(&g_mux);
    
        if (valid) {
            // pack data under JSON: {"temperature": 27.50, "humidity": 60.00}
            String jsonPayload = "{\"temperature\":" + String(temp, 2) +
                                ",\"humidity\":"    + String(humid, 2)  + "}";
    
            // init HTTP POST
            HTTPClient http;
            http.begin(serverUrl);
            http.addHeader("Content-Type", "application/json");
            http.setTimeout(5000);
    
            int httpCode = http.POST(jsonPayload);
    
            if (httpCode == HTTP_CODE_OK) {
            Serial.print("Send data successfully (HTTP 200) -> ");
            Serial.println(jsonPayload);
            } else if (httpCode > 0) {
                Serial.printf("Server rejected, HTTP code: %d\n", httpCode);
            } 
            else {
                Serial.printf("Send failed, lỗi: %s\n", http.errorToString(httpCode).c_str());
            }
    
            http.end();  
        } else {
            Serial.println("Invalid sensor data");
        }
        } else {
            Serial.println("Connecting WiFi...");
        }
    
        vTaskDelay(pdMS_TO_TICKS(PUSH_INTERVAL_MS));
    }
}

// ---------------------------------------------------------------------------
// Call in main.cpp
// ---------------------------------------------------------------------------
void task3_start() {
  Serial.println("\n=== TASK 3: READ DHT20 THROUGH I2C ===");

  xTaskCreate(wifi_connect_task, "B3 WiFi", 4096, NULL, 3, NULL);
  // Stack 8192 vì task có gọi HTTPClient
  xTaskCreate(dht_task, "B3 DHT20", 8192, NULL, 2, NULL);
  xTaskCreate(coreiot_task, "B3 CoreIoT", 8192, NULL, 2, NULL);
}