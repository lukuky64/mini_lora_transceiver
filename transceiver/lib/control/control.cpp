#include "control.hpp"

Control::Control() {
  m_serialCom = new SerialCom();  // Initialize SerialCom instance
  // m_LoRaCom = new LoRaCom();      // Initialize LoRaCom instance
  // Constructor implementation
}

void Control::setup() {
  m_serialCom->init(115200);  // Initialize serial communication
  //   m_LoRaCom.begin(SPI_CLK_RF, SPI_MISO_RF, SPI_MOSI_RF, SPI_CS_RF, RF_DIO,
  //                   915.0f, 20);

  ESP_LOGI(TAG, "Control setup complete");
}

void Control::begin() {
  // Begin method implementation
  ESP_LOGI(TAG, "Control beginning...");

  // Delete the previous taska if they exist
  if (SerialTaskHandle != nullptr) {
    vTaskDelete(SerialTaskHandle);
  }

  if (LoRaTaskHandle != nullptr) {
    vTaskDelete(LoRaTaskHandle);
  }

  if (RSSTaskHandle != nullptr) {
    vTaskDelete(RSSTaskHandle);
  }

  // Create new tasks for serial data handling, LoRa data handling, and status
  xTaskCreate(
      [](void *param) { static_cast<Control *>(param)->serialDataTask(); },
      "SerialDataTask", 8192, this, 1, &SerialTaskHandle);

  //   xTaskCreate(
  //       [](void *param) { static_cast<Control *>(param)->loRaDataTask(); },
  //       "LoRaDataTask", 8192, this, 1, &LoRaTaskHandle);

  xTaskCreate([](void *param) { static_cast<Control *>(param)->statusTask(); },
              "RssiTask", 8192, this, 1, &RSSTaskHandle);

  ESP_LOGI(TAG, "Control begun!");
}

void Control::serialDataTask() {
  char buffer[128];  // Buffer to store incoming data
  int rxIndex = 0;   // Index to track the length of the received message

  while (true) {
    // Check for incoming data from the serial interface
    if (m_serialCom->getData(buffer, sizeof(buffer), &rxIndex)) {
      ESP_LOGI(TAG, "Received: %s", buffer);  // Log the received data
      interpretMessage(buffer, rxIndex);      // Process the received message
      // clear the buffer for the next message
      memset(buffer, 0, sizeof(buffer));
      rxIndex = 0;  // Reset the index
    }
    vTaskDelay(pdMS_TO_TICKS(serial_Interval));  // Delay to avoid busy-waiting
  }
}

void Control::loRaDataTask() {
  char buffer[128];  // Buffer to store incoming data
  int rxIndex = 0;   // Index to track the length of the received message

  while (true) {
    // Check for incoming data from the LoRa interface
    if (m_LoRaCom->getData(buffer, sizeof(buffer), &rxIndex)) {
      // Send the received data over serial
      m_serialCom->sendData("Received: <");
      m_serialCom->sendData(buffer);
      m_serialCom->sendData(">\n");
    }

    vTaskDelay(pdMS_TO_TICKS(lora_Interval));
  }
}

void Control::statusTask() {
  pinMode(LED_PIN, OUTPUT);  // Set LED pin as output
  unsigned long lastMillis = 0;
  while (true) {
    // if (millis() - lastMillis >= RSSI_interval) {
    //   lastMillis = millis();
    //   int16_t rssi = m_LoRaCom.getRssi();
    //   if (rssi != -1) {  // Check if the RSSI value is valid
    //     String msg = "DOWNLINK_RSSI = " + String(rssi) + "\n";
    //     m_serialCom->sendData(msg.c_str());
    //   }
    // }
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    ESP_LOGI(TAG, "LED toggled");
    vTaskDelay(pdMS_TO_TICKS(status_Interval));
  }
}

void Control::interpretMessage(const char *buffer, int rxIndex) {
  // have a list of commands and switch case statement
  // m_LoRaCom.sendMessage(buffer);
}