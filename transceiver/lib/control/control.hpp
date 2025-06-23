#pragma once

#include <Arduino.h>

#include <cstring>

#include "LoRaCom.hpp"
#include "SerialCom.hpp"
#include "commander.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pin_defs.hpp"

#define c_cmp(a, b) (strcmp(a, b) == 0)

class Control {
 public:
  Control();
  void setup();
  void begin();

 private:
  SerialCom *m_serialCom;
  LoRaCom *m_LoRaCom;
  Commander *m_commander;

  unsigned long serial_Interval = 100;
  unsigned long lora_Interval = 100;
  unsigned long status_Interval = 1000;
  unsigned long RSSI_interval = 4000;

  static constexpr const char *TAG = "Control";

  TaskHandle_t SerialTaskHandle = nullptr;
  TaskHandle_t LoRaTaskHandle = nullptr;
  TaskHandle_t RSSTaskHandle = nullptr;

  void serialDataTask();
  void loRaDataTask();
  void statusTask();
  void interpretMessage(char *buffer);
};