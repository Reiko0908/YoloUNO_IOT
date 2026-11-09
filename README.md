# YoloUNO IoT Labs (PlatformIO)

Five practice labs for the IoT course, running on a **YoloUNO (ESP32-S3)** board
with the Arduino framework and FreeRTOS.

| Lab | File | What it does |
|-----|------|--------------|
| 1 | `src/task1.cpp` | Connects to Wi-Fi and POSTs simulated temperature/humidity telemetry to CoreIoT every 5 s |
| 2 | `src/task2.cpp` | LED blinky on GPIO 48 (1 s on / 1 s off) with serial log |
| 3 | `src/task3.cpp` | I2C scan, DHT20 readings every 2 s, upload to CoreIoT every 5 s |
| 4 | `src/task4.cpp` | Two FreeRTOS tasks synchronised with a binary semaphore + mutex; LED on when temp >= 30 C |
| 5 | `src/task5.cpp` | Web server (routes `/`, `/data`, `/toggle`) showing DHT20 data and toggling the LED |

## Hardware

| Item | Pin |
|------|-----|
| LED | GPIO 48 |
| DHT20 SDA | GPIO 11 |
| DHT20 SCL | GPIO 12 (I2C address `0x38`) |

## Setup

1. Install [PlatformIO](https://platformio.org) (VS Code extension or CLI).
2. Create your credentials file (it is git-ignored):
   ```bash
   cp include/secrets.example.h include/secrets.h
   ```
   then edit `include/secrets.h` with your Wi-Fi SSID/password and the access
   token of each CoreIoT device. Never commit this file.
3. In `src/main.cpp`, uncomment exactly one `taskN_start();` line in `setup()`.
4. Build, upload and open the serial monitor (115200 baud):
   ```bash
   pio run -t upload
   pio device monitor
   ```

## Notes

- CoreIoT telemetry is sent over HTTP: `POST http://app.coreiot.io/api/v1/<ACCESS_TOKEN>/telemetry`
  with a JSON body such as `{"temperature": 30.5, "humidity": 65.0}`.
- The DHT20 refuses reads closer than 1 s apart. Task 5 therefore reads the
  sensor at most once per second and serves the last good values to the web page.
- The web page (task 5) refreshes its data every 2 s with `fetch('/data')`.
