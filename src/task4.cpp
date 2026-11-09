#include <Arduino.h>
#include <Wire.h>
#include <DHT20.h>
#include <stdarg.h>
#include "../include/tasks.h"

#define LED_PIN              48
#define SDA_PIN              11
#define SCL_PIN              12
#define TEMP_THRESHOLD       30.0f   // LED turns ON above this temperature
#define SENSOR_PERIOD_MS     2000    // TaskSensor reads every 2s

// Data shared between TaskSensor (writer) and TaskDisplay (reader)
typedef struct {
  float    temp;     // temperature (deg C)
  float    humid;     // humidity (%)
} SensorData;

static DHT20 dht(&Wire);

static SemaphoreHandle_t dataReady;     // binary: sensor to display signal
static SemaphoreHandle_t dataMutex;     // mutex: protects 'shared'

// Shared data: written by sensor_task, read by display_task
static float shared_temp = 0.0f;
static float shared_hum  = 0.0f;
 
// ---------------------------------------------------------------------------
// sensor_task: read the sensor, save the result, signal display_task
// ---------------------------------------------------------------------------
static void sensor_task(void *pvParameters) {
  while (1) {
    int status = dht.read();
    
    if (status == DHT20_OK) {
      float temp = dht.getTemperature();
      float hum  = dht.getHumidity();
      
      // Lock mutex, write shared data, unlock
      xSemaphoreTake(dataMutex, portMAX_DELAY); 
      shared_temp = temp;
      shared_hum  = hum;
      xSemaphoreGive(dataMutex);
      
      Serial.printf("[DHT20] Read OK: temp=%.2f C, humid=%.2f %% -> GIVE dataReady\n", temp, hum);
      
      // Signal display_task that new data is ready
      xSemaphoreGive(dataReady);
    } 
    else
    {
      Serial.printf("[DHT20] Read error, code: %d (no GIVE)\n", status);
    }
    
    vTaskDelay(pdMS_TO_TICKS(SENSOR_PERIOD_MS));
  }
}
 
// ---------------------------------------------------------------------------
// display_task: wait for the signal, print the data, control the LED
// ---------------------------------------------------------------------------
static void display_task(void *pvParameters) {
  while (1) {
    Serial.println("[Display] Waiting: TAKE dataReady...");
 
    // The task sleeps here (uses no CPU) until sensor_task gives dataReady
    xSemaphoreTake(dataReady, portMAX_DELAY);
 
    // Lock the mutex, copy the shared data, unlock
    float temp, hum;
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    temp = shared_temp;
    hum  = shared_hum;
    xSemaphoreGive(dataMutex);
 
    // LED is ON when the temperature is above the threshold
    bool ledOn = (temp >= TEMP_THRESHOLD);
    digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
 
    Serial.printf("[Display] TAKE OK: temp=%.2f C, humid=%.2f %%, LED=%s\n", temp, hum, ledOn ? "ON" : "OFF");
  }
}
 
// ---------------------------------------------------------------------------
// Called from main.cpp: set up hardware, create semaphores and tasks
// ---------------------------------------------------------------------------
void task4_start() {
  Serial.println("\n=== LAB 4: FREERTOS SEMAPHORES ===");
 
  // Hardware setup
  pinMode(LED_PIN, OUTPUT);
  Wire.begin(SDA_PIN, SCL_PIN);
  delay(200);  // delay to wait sensor settled
  dht.begin();
 
  // Create the semaphores
  dataReady = xSemaphoreCreateBinary();   // starts empty, so the first Take waits
  dataMutex = xSemaphoreCreateMutex();    // starts available
 
  if (dataReady == NULL || dataMutex == NULL) {
    Serial.println("Failed to create semaphores!");
    return;
  }
 
  // Create the two tasks
  xTaskCreate(sensor_task,  "Sensor",  4096, NULL, 3, NULL);
  xTaskCreate(display_task, "Display", 4096, NULL, 2, NULL);
}