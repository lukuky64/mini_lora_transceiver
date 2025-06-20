// LoRaCom.cpp
#include "LoRaCom.hpp"

// Issues:
// 1. If two devices transmit at the exact same time, neither message will be
// received

static LoRaCom *instance = nullptr;

LoRaCom::LoRaCom() {
  ESP_LOGI(TAG, "LoRaCom constructor called");
  instance = this;
}

#ifdef FAKE_LORA
bool LoRaCom::begin(uint8_t CLK, uint8_t MISO, int8_t MOSI, uint8_t csPin,
                    uint8_t intPin, int8_t RST, int8_t BUSY_, float freqMHz,
                    int8_t power) {
  // This is a fake LoRa implementation for testing purposes
  ESP_LOGI(TAG, "Fake LoRaCom begin called with frequency: %.2f MHz", freqMHz);
  return true;  // Simulate successful initialization
}

void LoRaCom::sendMessage(const char *inputmsg) {
  if (inputmsg[0] != '\0') {  // Check the message is not empty
    ESP_LOGI(TAG, "Transmitting [%s]", inputmsg);
    int state = radio->transmit(inputmsg);  // Start the transmission process
  }
}

// return message
String LoRaCom::checkForReply() {
  String message = "";

  if (receivedFlag) {
    receivedFlag = false;
    message = "Fake reply message";
  }
  return message;
}

float LoRaCom::getRssi() { return -40.0; }

bool LoRaCom::getData(char *buffer, const size_t bufferSize, int *_rxIndex) {
  String message = checkForReply();
  if (message.length() > 0) {
    // Check if the message fits in the buffer
    if (message.length() < bufferSize) {
      // Copy the message to the buffer
      strncpy(buffer, message.c_str(), bufferSize - 1);
      buffer[bufferSize - 1] = '\0';  // Ensure null-termination
      *_rxIndex =
          message.length();  // Update the index with the length of the message
      return true;           // Data received successfully
    } else {
      ESP_LOGW(TAG, "Buffer overflow: Message too long");
    }
  }
  return false;
}

#else
bool LoRaCom::begin(uint8_t CLK, uint8_t MISO, int8_t MOSI, uint8_t csPin,
                    uint8_t intPin, int8_t RST, int8_t BUSY_, float freqMHz,
                    int8_t power) {
  //
  radio = new SX1262(new Module(csPin, intPin, RST, BUSY_));

  // carrier frequency:           915.0 MHz
  // bandwidth:                   7.8 kHz
  // spreading factor:            12
  // coding rate:                 5
  // sync word:                   0x34 (public network/LoRaWAN)
  // output power:                22 dBm
  // preamble length:             20 symbols
  int state = radio->begin(freqMHz, 7.8, 12, 5, 0x34, power, 20);

  if (state == RADIOLIB_ERR_NONE) {
    ESP_LOGI(TAG, "LoRaCom begin called with frequency: %.2f MHz", freqMHz);

    ESP_LOGI(TAG, "LoRa radio init successful!");
  } else {
    ESP_LOGE(TAG, "LoRa Initialisation failed with code: %d", state);
    return false;
  }

  radio->setPacketReceivedAction(setFlag);
  return true;
}

void LoRaCom::setFlag(void) {
  if (instance) {
    instance->receivedFlag = true;
    // instance->operationDone = true;
  }
}

// bool LoRaCom::createMessage() {
//   bool messageCreated = false;
//   if (Serial.available() > 0) {
//     memset(inputArray, 0, MAX_INPUT_LENGTH);  // Clear the buffer
//     int Index = 0;

//     while (Serial.available() > 0 && Index < MAX_INPUT_LENGTH - 1) {
//       char incomingByte = Serial.read();
//       if (incomingByte == '\n') {
//         inputArray[Index] = '\0';
//         messageCreated = true;
//         break;
//       } else {
//         inputArray[Index] = incomingByte;
//         Index++;
//         vTaskDelay(pdMS_TO_TICKS(5));  // this fixes reliablity issues with
//         the data somehow
//       }
//     }
//   }

//   return messageCreated;
// }

// void LoRaCom::sendMessage() {
//   ESP_LOGE(TAG, "Trying to send message");
//   if (inputArray[0] != '\0') {  // Check if the message is not empty
//     ESP_LOGI(TAG, "Transmit: ");
//     ESP_LOGI(TAG, inputArray);
//     rf95->send((uint8_t *)inputArray, strlen(inputArray));
//     vTaskDelay(pdMS_TO_TICKS(10));
//     rf95->waitPacketSent();
//     vTaskDelay(pdMS_TO_TICKS(10));
//   }
// }

void LoRaCom::sendMessage(const char *inputmsg) {
  if (inputmsg[0] != '\0') {  // Check the message is not empty
    ESP_LOGI(TAG, "Transmitting [%s]", inputmsg);
    int state = radio->transmit(inputmsg);  // Start the transmission process

    if (state == RADIOLIB_ERR_NONE) {
      return;
    } else {
      ESP_LOGE(TAG, "LoRa send failed with code: %d", state);
    }
  }
}

// return message
String LoRaCom::checkForReply() {
  String message = "";

  if (receivedFlag) {
    receivedFlag = false;
    int state = radio->readData(message);

    if (state == RADIOLIB_ERR_NONE) {
      ESP_LOGI(TAG, "LoRa receive successful!");
    } else {
      ESP_LOGE(TAG, "LoRa receive failed with code: %d", state);
    }
    // ESP_LOGI(TAG, "\t RSSI [%d]", rf95->lastRssi());
    // ESP_LOGI(TAG, "\t SNR [%d]", rf95->lastSNR());
  }

  return message;
}

float LoRaCom::getRssi() {
  return radio->getRSSI();  // Return the last received signal strength
}

bool LoRaCom::getData(char *buffer, const size_t bufferSize, int *_rxIndex) {
  String message = checkForReply();
  if (message.length() > 0) {
    // Check if the message fits in the buffer
    if (message.length() < bufferSize) {
      // Copy the message to the buffer
      strncpy(buffer, message.c_str(), bufferSize - 1);
      buffer[bufferSize - 1] = '\0';  // Ensure null-termination
      *_rxIndex =
          message.length();  // Update the index with the length of the message
      return true;           // Data received successfully
    } else {
      ESP_LOGW(TAG, "Buffer overflow: Message too long");
    }
  }
  return false;
}
#endif
