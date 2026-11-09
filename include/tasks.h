#pragma once

// each function creates (xTaskCreate) 
// Call in setup() of main.cpp, after Serial.begin().

void task1_start();   // CoreIoT - send telemetry data
void task2_start();   // LED Blinky
void task3_start();   // I2C + DHT20
void task4_start();   // Semaphore
void task5_start();   // WebServer