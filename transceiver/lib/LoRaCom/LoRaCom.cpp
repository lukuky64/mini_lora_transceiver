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

int32_t LoRaCom::getRssi() { return -40.0; }

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

bool LoRaCom::setOutGain(int8_t gain) {
  // value should be bewteen -9 and 22 dBm
  if (gain < -9 || gain > 22) {
    ESP_LOGE(TAG, "Invalid gain value: %d. Gain must be between -9 and 22 dBm",
             gain);
    return false;
  } else {
    ESP_LOGI(TAG, "Gain set to %d", gain);
    return true;
  }
}

bool LoRaCom::setFrequency(float freqMHz) {
  // Simulate setting frequency

  if (freqMHz < 100.0f || freqMHz > 1000.0f) {
    ESP_LOGE(TAG,
             "Invalid frequency: %.2f MHz. Frequency must be between 100.0 "
             "and 1000.0 MHz",
             freqMHz);
    return false;
  }

  ESP_LOGI(TAG, "Frequency set to %.2f MHz", freqMHz);
  return true;  // Simulate successful frequency setting
}

bool LoRaCom::setSpreadingFactor(uint8_t spreadingFactor) {
  // Simulate setting spreading factor
  if (spreadingFactor < 5 || spreadingFactor > 12) {
    ESP_LOGE(TAG,
             "Invalid spreading factor: %d. Spreading factor must be between 5 "
             "and 12",
             spreadingFactor);
    return false;
  }
  ESP_LOGI(TAG, "Spreading factor set to %d", spreadingFactor);
  return true;  // Simulate successful spreading factor setting
}

bool LoRaCom::setBandwidth(float bandwidth) {
  // Simulate setting bandwidth
  if (bandwidth < 0 || bandwidth > 510) {
    ESP_LOGE(TAG,
             "Invalid bandwidth: %.2f kHz. Bandwidth must be between 0 and 510 "
             "kHz",
             bandwidth);
    return false;
  }
  ESP_LOGI(TAG, "Bandwidth set to %.2f kHz", bandwidth);
  return true;  // Simulate successful spreading factor setting
}

#else
bool LoRaCom::begin(uint8_t CLK, uint8_t MISO, int8_t MOSI, uint8_t csPin,
                    uint8_t intPin, int8_t RST, int8_t BUSY_, float freqMHz,
                    int8_t power) {
  ESP_LOGI(TAG,
           "Initializing Heltec ESP32-C3+SX1262 with pins: CLK=%d, MISO=%d, "
           "MOSI=%d, CS=%d",
           CLK, MISO, MOSI, csPin);
  ESP_LOGI(TAG, "Datasheet mapping: DIO1=%d, BUSY=%d, RST=%d", intPin, BUSY_,
           RST);

  // Initialize SPI with custom pins
  SPI.begin(10, 6, 7, 8);     // CLK, MISO, MOSI, csPin
  SPI.setFrequency(1000000);  // Start conservative with 1MHz
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);

  ESP_LOGI(TAG, "Creating SX1262 module...");

  radio = new SX1262(new Module(8, 3, 5, 4));

  // Manual reset sequence is critical for SX1262
  // ESP_LOGI(TAG, "Performing manual reset...");
  // pinMode(RST, OUTPUT);
  // digitalWrite(RST, LOW);
  // delay(50);  // Longer reset pulse
  // digitalWrite(RST, HIGH);
  // delay(200);  // Longer startup delay

  ESP_LOGI(TAG, "Starting radio initialization...");
  // carrier frequency:           915.0 MHz
  // bandwidth:                   125 kHz
  // spreading factor:            12
  // coding rate:                 5
  // sync word:                   0x34 (public network/LoRaWAN)
  // output power:                22 dBm
  // preamble length:             20 symbols

  // Add a watchdog timer to prevent infinite hang
  unsigned long startTime = millis();
  const unsigned long timeout = 5000;  // 5 second timeout

  int state = radio->begin(freqMHz, 125, 12, 5, 0x34, 22, 20);

  if (millis() - startTime > timeout) {
    ESP_LOGE(TAG, "Radio initialization timed out!");
    return false;
  }

  ESP_LOGD(TAG, "Radio begin returned state: %d", state);

  if (state == RADIOLIB_ERR_NONE) {
    ESP_LOGI(TAG, "LoRaCom begin success with frequency: %.2f MHz", freqMHz);
    radioInitialized = true;
  } else {
    ESP_LOGE(TAG, "LoRa Initialisation failed with code: %d", state);
    ESP_LOGE(TAG,
             "Common error codes: -2=SPI_CMD_INVALID, -8=CHIP_NOT_FOUND, "
             "-16=INVALID_FREQUENCY");
    ESP_LOGE(TAG, "Check your wiring! CS=%d, INT=%d, RST=%d, BUSY=%d", csPin,
             intPin, RST, BUSY_);
    radioInitialized = false;
    return false;
  }

  radio->setPacketReceivedAction(setFlag);
  radio->setPacketSentAction(setFlag);  // Also handle transmission complete

  // Start listening for incoming packets
  int rxState = radio->startReceive();
  if (rxState == RADIOLIB_ERR_NONE) {
    ESP_LOGI(TAG, "LoRa started listening for packets");
  } else {
    ESP_LOGW(TAG, "Failed to start receive mode: %d", rxState);
  }

  return true;
}

void LoRaCom::setFlag(void) {
  if (instance) {
    instance->operationDone = true;
    ESP_LOGD("LoRaCom", "Interrupt callback triggered - operationDone set");
    // We'll determine if it's RX or TX in the main code by checking the radio
    // state
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

    // Check if radio is properly initialized
    if (!radioInitialized) {
      ESP_LOGE(TAG, "Cannot send message - radio not initialized");
      return;
    }

    // Process any pending operations first (like completed transmissions)
    processOperations();

    // Check if a transmission is already in progress
    if (transmitFlag) {
      ESP_LOGW(TAG, "Transmission already in progress, skipping message: [%s]",
               inputmsg);
      return;
    }

    ESP_LOGI(TAG, "Transmitting [%s]", inputmsg);

    // Set transmission flag
    transmitFlag = true;
    transmitStartTime = millis();  // Record when transmission started

    // Use startTransmit for non-blocking operation
    int state = radio->startTransmit(inputmsg);

    if (state == RADIOLIB_ERR_NONE) {
      ESP_LOGD(TAG, "LoRa transmission started successfully");
      // Note: transmission will complete in background
      // The setFlag callback will be called when done, and we should clear
      // transmitFlag there
    } else {
      ESP_LOGE(TAG, "LoRa send failed to start with code: %d", state);
      transmitFlag = false;  // Clear flag on error
    }
  }
}

// return message
String LoRaCom::checkForReply() {
  String message = "";

  if (operationDone) {
    ESP_LOGD(TAG, "Processing completed operation...");
    operationDone = false;

    // Check if we were transmitting
    if (transmitFlag) {
      transmitFlag = false;
      ESP_LOGI(TAG, "LoRa transmission completed successfully");

      // Restart receive mode after transmission
      int rxState = radio->startReceive();
      if (rxState != RADIOLIB_ERR_NONE) {
        ESP_LOGW(TAG, "Failed to restart receive mode after TX: %d", rxState);
      } else {
        ESP_LOGD(TAG, "Restarted receive mode after transmission");
      }

      // No message to return for transmission complete
      return message;
    }

    // Otherwise it was a reception
    int state = radio->readData(message);

    if (state == RADIOLIB_ERR_NONE) {
      ESP_LOGI(TAG, "LoRa receive successful!");
      receivedFlag = true;  // Set the old flag for compatibility
    } else {
      ESP_LOGE(TAG, "LoRa receive failed with code: %d", state);
    }
  }

  return message;
}

void LoRaCom::processOperations() {
  // Check for transmission timeout
  if (transmitFlag && (millis() - transmitStartTime > TRANSMIT_TIMEOUT_MS)) {
    ESP_LOGE(TAG, "Transmission timeout! Forcing reset of transmit flag");
    transmitFlag = false;

    // Try to restart receive mode
    int rxState = radio->startReceive();
    if (rxState != RADIOLIB_ERR_NONE) {
      ESP_LOGW(TAG, "Failed to restart receive mode after timeout: %d",
               rxState);
    }
  }

  // Simply call checkForReply which handles both TX complete and RX
  checkForReply();
}

int32_t LoRaCom::getRssi() {
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

bool LoRaCom::setOutGain(int8_t gain) {
  // value should be bewteen -9 and 22 dBm
  int state = radio->setOutputPower(gain);
  if (state == RADIOLIB_ERR_NONE) {
    ESP_LOGI(TAG, "Gain set to %d", gain);
    return true;
  } else {
    ESP_LOGE(TAG, "Failed to set gain with code: %d", state);
    return false;
  }
}

bool LoRaCom::setFrequency(float freqMHz) {
  // Set the frequency of the radio
  int state = radio->setFrequency(freqMHz);
  if (state == RADIOLIB_ERR_NONE) {
    ESP_LOGI(TAG, "Frequency set to %.2f MHz", freqMHz);
    return true;
  } else {
    ESP_LOGE(TAG, "Failed to set frequency with code: %d", state);
    return false;
  }
}

bool LoRaCom::setSpreadingFactor(uint8_t spreadingFactor) {
  // Set the spreading factor of the radio
  int state = radio->setSpreadingFactor(spreadingFactor);
  if (state == RADIOLIB_ERR_NONE) {
    ESP_LOGI(TAG, "Spreading factor set to %d", spreadingFactor);
    return true;
  } else {
    ESP_LOGE(TAG, "Failed to set spreading factor with code: %d", state);
    return false;
  }
}

bool LoRaCom::setBandwidth(float bandwidth) {
  // Set the spreading factor of the radio
  int state = radio->setBandwidth(bandwidth);
  if (state == RADIOLIB_ERR_NONE) {
    ESP_LOGI(TAG, "Spreading factor set to %.2f kHz", bandwidth);
    return true;
  } else {
    ESP_LOGE(TAG, "Failed to set spreading factor with code: %d", state);
    return false;
  }
}

#endif
