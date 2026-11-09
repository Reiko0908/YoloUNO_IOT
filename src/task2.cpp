#include <Arduino.h>
#include "../include/tasks.h"

#define LED_PIN      48      // LED on YoloUNO
#define BLINK_MS     1000    // time interval on/off

static void led_blinky(void *pvParameters) {
  pinMode(LED_PIN, OUTPUT);
 
  while (1) {
    digitalWrite(LED_PIN, HIGH);                       // turn on LED
    Serial.println("LED ON");     // Log every one change status
    vTaskDelay(pdMS_TO_TICKS(BLINK_MS));
 
    digitalWrite(LED_PIN, LOW);                        // turn off LED
    Serial.println("LED OFF");
    vTaskDelay(pdMS_TO_TICKS(BLINK_MS));
  }
}
 
// ---------------------------------------------------------------------------
// Call in main.cpp
// ---------------------------------------------------------------------------
void task2_start() {
  Serial.println("\n=== TASK 2: LED BLINKY ON YOLOUNO ===");
 
  xTaskCreate(led_blinky, "LED Control", 4096, NULL, 2, NULL);
}