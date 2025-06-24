// LoRaCom.h
#ifndef LoRaCom_h
#define LoRaCom_h

#include <Arduino.h>
#include <RadioLib.h>

#include "esp_log.h"

class LoRaCom {
 public:
  LoRaCom();
  bool begin(uint8_t CLK, uint8_t MISO, int8_t MOSI, uint8_t csPin,
             uint8_t intPin, int8_t RST, int8_t BUSY_, float freqMHz,
             int8_t power);
  // bool createMessage();
  // void sendMessage();
  void sendMessage(const char *inputmsg);  // overloaded function
  String checkForReply();
  int32_t getRssi();

  bool setOutGain(int8_t gain);
  bool setFrequency(float freqMHz);
  bool setSpreadingFactor(uint8_t spreadingFactor);
  bool setBandwidth(float bandwidth);

  bool getData(char *buffer, const size_t bufferSize, int *_rxIndex);

 private:
  SX1262 *radio;

  // Flag to indicate if an operation is done
  bool operationDone = false;

  // Flag to indicate transmission or reception state
  bool transmitFlag = false;

  // Flag to indicate if a packet was received
  volatile bool receivedFlag = false;

  // save transmission states between loops
  int transmissionState = RADIOLIB_ERR_NONE;

  static const int MAX_INPUT_LENGTH = 128;
  char inputArray[MAX_INPUT_LENGTH];

  int INT_PIN;
  int CS_PIN;
  float RF95_FREQ;

  static void setFlag(void);

  static constexpr const char *TAG = "LORA_COMM";
};

#endif
