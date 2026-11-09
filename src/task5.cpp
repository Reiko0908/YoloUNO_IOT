/*
 * LAB 5: WEBSERVER ON YOLOUNO (Phiên bản đơn giản, dễ hiểu)
 * Giao diện web trực quan, đọc DHT20 và điều khiển LED trực tiếp.
 */

#include <Arduino.h>
#include <Wire.h>
#include <DHT20.h>
#include <WiFi.h>
#include <WebServer.h>
#include "../include/tasks.h"

#define LED_PIN      48
#define SDA_PIN      11
#define SCL_PIN      12

// --- CONFIG WIFI AND COREIOT (values come from include/secrets.h) ---
#include "../include/secrets.h"

DHT20 dht;
WebServer server(80);

bool ledState = false;

// Web HTML UI
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>YoloUNO Sensor Monitor</title>
    <style>
        body { font-family: sans-serif; text-align: center; background: #eef2f3; margin-top: 50px; }
        .card { background: white; padding: 20px; margin: 10px auto; width: 280px; border-radius: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }
        h2 { color: #333; }
        .val { font-size: 28px; font-weight: bold; color: #007bff; }
        .btn { background: #28a745; color: white; border: none; padding: 12px 24px; font-size: 16px; border-radius: 5px; cursor: pointer; margin-top: 10px; }
        .btn.off { background: #dc3545; }
    </style>
</head>
<body>
    <h2>YoloUNO WebServer</h2>
    
    <div class="card">
        <p>Nhiệt độ</p>
        <div class="val"><span id="temp">--</span> &deg;C</div>
    </div>

    <div class="card">
        <p>Độ ẩm</p>
        <div class="val"><span id="hum">--</span> %</div>
    </div>

    <div class="card">
        <p>Trạng thái Đèn LED</p>
        <button class="btn" id="ledBtn" onclick="toggleLed()">Đang tải...</button>
    </div>

<script>
    // Tự động cập nhật dữ liệu mỗi 2 giây mà không cần load lại trang
    setInterval(updateData, 2000);

    function updateData() {
        fetch('/data')
        .then(response => response.json())
        .then(data => {
            document.getElementById('temp').innerText = data.temp.toFixed(1);
            document.getElementById('hum').innerText = data.hum.toFixed(1);
            
            let btn = document.getElementById('ledBtn');
            if (data.led) {
            btn.innerText = "TẮT ĐÈN";
            btn.className = "btn off";
            } else {
            btn.innerText = "BẬT ĐÈN";
            btn.className = "btn";
            }
        });
    }

    function toggleLed() {
        fetch('/toggle')
        .then(response => response.json())
        .then(data => {
            updateData();
        });
    }

    // Gọi ngay khi mở trang
    updateData();
</script>
</body>
</html>
)rawliteral";

void handleRoot() {
    server.send(200, "text/html", INDEX_HTML);
}

// Trả về dữ liệu cảm biến dưới dạng JSON cho Javascript gọi.
// DHT20 từ chối đọc nếu hai lần đọc cách nhau < 1 s (lỗi -15), nên chỉ đọc
// lại tối đa mỗi 1 s và trả về giá trị hợp lệ gần nhất cho các request khác.
static float lastTemp = 0.0f, lastHum = 0.0f;
static uint32_t lastRead = 0;

void handleData() {
    if (millis() - lastRead >= 1000) {
        if (dht.read() == DHT20_OK) {
            lastTemp = dht.getTemperature();
            lastHum = dht.getHumidity();
        }
        lastRead = millis();
    }

    String json = "{ \"temp\": " + String(lastTemp, 1) +
                    ", \"hum\": " + String(lastHum, 1) +
                    ", \"led\": " + (ledState ? "true" : "false") + " }";
    server.send(200, "application/json", json);
}

// Xử lý bật/tắt đèn khi bấm nút trên web
void handleToggle() {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

static void web_task(void *pvParameters) {
    while (1) {
        server.handleClient();
        vTaskDelay(pdMS_TO_TICKS(2));   // nghỉ ngắn để các task khác chạy
    }
}
 
// Hàm được gọi từ main.cpp (thay cho setup() của bản cũ)
void task5_start() {
    Serial.println("\n=== LAB 5: WEBSERVER ON YOLOUNO ===");
 
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
 
    // Khởi tạo I2C và cảm biến DHT20
    Wire.begin(SDA_PIN, SCL_PIN);
    dht.begin();
 
    // Kết nối WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Dang ket noi WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\nKet noi WiFi thanh cong!");
    Serial.print("Dia chi IP truy cập Web: http://");
    Serial.println(WiFi.localIP());
 
    // Đăng ký các đường dẫn (Routes) cho Web Server
    server.on("/", handleRoot);
    server.on("/data", handleData);
    server.on("/toggle", handleToggle);
    
    server.begin();
    Serial.println("Web Server đã khởi động!");
 
    // Tạo task xử lý request (stack lớn vì có xử lý String và gửi trang web)
    xTaskCreate(web_task, "Web", 8192, NULL, 2, NULL);
}