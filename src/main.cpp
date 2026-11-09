#include <Arduino.h>
#include "../include/tasks.h"

void setup() {
  Serial.begin(115200);

  unsigned long start = millis();
  while (!Serial && (millis() - start < 2000));

  Serial.println("\n=== IoT exercise ===");

  // ---- Select the lab to run: uncomment ONE line ----
  // task1_start();   // CoreIoT telemetry
  // task2_start();   // LED blinky
  // task3_start();   // I2C + DHT20
  // task4_start();   // Semaphores
  // task5_start();   // WebServer
}

void loop() {
}
