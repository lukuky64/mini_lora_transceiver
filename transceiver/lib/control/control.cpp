#include "control.hpp"

Control::Control() {
  m_serialCom = new SerialCom();  // Initialize SerialCom instance
  m_LoRaCom = new LoRaCom();      // Initialize LoRaCom instance
  m_commander =
      new Commander(m_serialCom, m_LoRaCom);  // Initialize Commander instance
  // Constructor implementation
}

void Control::setup() {
  m_serialCom->init(115200);  // Initialize serial communication

  m_LoRaCom->begin(SPI_CLK_RF, SPI_MISO_RF, SPI_MOSI_RF, SPI_CS_RF, RF_DIO,
                   RF_RST, RF_BUSY, 915.0f, 22);

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

  if (StatusTaskHandle != nullptr) {
    vTaskDelete(StatusTaskHandle);
  }

  // Create new tasks for serial data handling, LoRa data handling, and status
  xTaskCreate(
      [](void *param) { static_cast<Control *>(param)->serialDataTask(); },
      "SerialDataTask", 8192, this, 1, &SerialTaskHandle);

  xTaskCreate(
      [](void *param) { static_cast<Control *>(param)->loRaDataTask(); },
      "LoRaDataTask", 8192, this, 1, &LoRaTaskHandle);

  xTaskCreate([](void *param) { static_cast<Control *>(param)->statusTask(); },
              "StatusTask", 8192, this, 1, &StatusTaskHandle);

  xTaskCreate(
      [](void *param) { static_cast<Control *>(param)->heartBeatTask(); },
      "SerialDataTask", 512, this, 1, &heartBeatTaskHandle);

  ESP_LOGI(TAG, "Control begun!\n");

  ESP_LOGI(TAG, "Type <help> for a list of commands");
}

void Control::heartBeatTask() {
  pinMode(LED_PIN, OUTPUT);  // Set LED pin as output
  while (true) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    ESP_LOGD(TAG, "LED toggled");
    vTaskDelay(pdMS_TO_TICKS(heartBeat_Interval));
  }
}

void Control::serialDataTask() {
  char buffer[128];  // Buffer to store incoming data
  int rxIndex = 0;   // Index to track the length of the received message

  while (true) {
    // Check for incoming data from the serial interface
    if (m_serialCom->getData(buffer, sizeof(buffer), &rxIndex)) {
      ESP_LOGI(TAG, "Received: %s", buffer);  // Log the received data
      interpretMessage(buffer);               // Process the message
      // clear the buffer for the next message
      memset(buffer, 0, sizeof(buffer));
      rxIndex = 0;  // Reset the index
    }
    // vTaskDelay(pdMS_TO_TICKS(serial_Interval));  // Delay to avoid
    // busy-waiting
    yield();  // Yield to allow other tasks to run
  }
}

void Control::loRaDataTask() {
  char buffer[128];  // Buffer to store incoming data
  int rxIndex = 0;   // Index to track the length of the received message

  while (true) {
    // Check for incoming data from the LoRa interface
    if (m_LoRaCom->getData(buffer, sizeof(buffer), &rxIndex)) {
      ESP_LOGI(TAG, "Received: %s", buffer);  // Log the received data
      interpretMessage(buffer);               // Process the message
      // Send the received data over serial
      m_serialCom->sendData("Received: <");
      m_serialCom->sendData(buffer);
      m_serialCom->sendData(">\n");

      memset(buffer, 0, sizeof(buffer));
      rxIndex = 0;  // Reset the index
    }

    // vTaskDelay(pdMS_TO_TICKS(lora_Interval));
    yield();  // Yield to allow other tasks to run
  }
}

void Control::statusTask() {
  pinMode(LED_PIN, OUTPUT);  // Set LED pin as output
  while (true) {
    int32_t rssi = m_LoRaCom->getRssi();
    String msg = String("status ") + "ID:" + deviceID +
                 " RSSI:" + String(rssi) +
                 " batteryLevel:" + String(m_batteryLevel) + " mode:" + m_mode +
                 " status:" + m_status;
    m_serialCom->sendData(((msg + "\n").c_str()));
    m_LoRaCom->sendMessage(msg.c_str());

    vTaskDelay(pdMS_TO_TICKS(status_Interval));
  }
}

void Control::interpretMessage(const char *buffer) {
  m_commander->setCommand(buffer);  // Set the command in the commander
  char *token = m_commander->readAndRemove();

  // eg: "command update gain 22"
  // eg: "status <deviceID> <RSSI> <batteryLevel> <mode> <status>"
  // eg: "data <payload>"

  if (c_cmp(token, "command")) {
    // send to other devices to sync parameters
    m_LoRaCom->sendMessage(buffer);
    // should probably wait for a success reply before changing THIS device

    ESP_LOGD(TAG, "Processing command: %s", buffer);
    m_commander->checkCommand();
  } else if (c_cmp(token, "data")) {
    processData(buffer);
  } else if (c_cmp(token, "status")) {
    processData(buffer);
  } else if (c_cmp(token, "help")) {
    ESP_LOGI(TAG,
             "Message format: <type> <data1> <data2> ...\n"
             "Valid types:\n"
             "  - command: for device control\n"
             "  - data: for data transmission\n"
             "  - message: for standard messages\n"
             "  - help: for displaying help information");
  }
}

void Control::processData(const char *buffer) {
  // Process the data message
  ESP_LOGD(TAG, "Processing data");

  // remove the "data" prefix
  const char *dataStart = strchr(buffer, ' ') + 1;  // Find the first space
  if (dataStart == nullptr) {
    ESP_LOGE(TAG, "Invalid data format: %s", buffer);
    return;  // Invalid format, return early
  }

  m_serialCom->sendData(buffer);  // Send the data part over serial
  m_serialCom->sendData("\n");

  // save dataStart to flash

  ESP_LOGI(TAG, "Data processing complete");
}