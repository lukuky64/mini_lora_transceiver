#include "saveFlash.hpp"

SaveFlash::SaveFlash(SerialCom *serialCom) {
  m_serialCom = serialCom;  // Initialize the SerialCom pointer
  // Constructor implementation
}

void SaveFlash::begin() {
  if (!LittleFS.begin()) {
    ESP_LOGW(TAG, "LittleFS Mount Failed, attempting to format...");
    if (LittleFS.format()) {
      ESP_LOGI(TAG, "LittleFS formatted successfully");
      if (!LittleFS.begin()) {
        ESP_LOGE(TAG, "LittleFS Mount Failed even after format");
        return;
      }
    } else {
      ESP_LOGE(TAG, "LittleFS format failed");
      return;
    }
  }
  m_initialised = true;

  updateStorage();

  writeData("New Log\n");
}

void SaveFlash::writeData(const String &data) {
  if (!m_initialised) {
    ESP_LOGW(TAG, "File system not initialised");
    return;
  }

  if (data.length() < m_free) {
    File file = LittleFS.open(fileName, FILE_APPEND);
    if (!file) {
      ESP_LOGE(TAG, "Failed to open file for appending");
      return;
    }
    if (file.print(data)) {
      ESP_LOGD(TAG, "Data written successfully");
      m_free -= data.length();
    } else {
      ESP_LOGE(TAG, "Write failed");
    }
    file.close();
  } else {
    ESP_LOGE(TAG, "Not enough space to write data");
    return;
  }
}

void SaveFlash::removeFile() {
  if (!m_initialised) {
    ESP_LOGW(TAG, "File system not initialised");
    return;
  }

  if (LittleFS.remove(fileName)) {
    ESP_LOGI(TAG, "File removed successfully: %s", fileName);
    updateStorage();
  } else {
    ESP_LOGE(TAG, "Failed to remove file: %s", fileName);
  }
}

void SaveFlash::updateStorage() {
  if (!m_initialised) {
    ESP_LOGW(TAG, "File system not initialised");
    return;
  }

  m_total = LittleFS.totalBytes();
  m_used = LittleFS.usedBytes();
  m_free = m_total - m_used;

  ESP_LOGI(TAG, "\nTotal: %d bytes\nUsed: %d bytes\nFree: %d bytes", m_total,
           m_used, m_free);
}

void SaveFlash::readFile() {
  if (!m_initialised) {
    ESP_LOGW(TAG, "File system not initialised");
    return;
  }

  File file = LittleFS.open(fileName, FILE_READ);
  if (!file) {
    ESP_LOGE(TAG, "Failed to open file for reading");
    return;
  }
  ESP_LOGI(TAG, "Reading file: %s", fileName);
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line += '\n';  // Add the newline back
    m_serialCom->sendData(line.c_str());
  }
  file.close();
}

// void SaveFlash::sendCommand(const char *command) {
//   if (!m_initialised) {
//     ESP_LOGW(TAG, "File system not initialised");
//     return;
//   }

//   if (strcmp(command, "read") == 0) {
//     readFile();
//   } else if (strcmp(command, "remove") == 0) {
//     removeFile();
//   } else if (strcmp(command, "update") == 0) {
//     updateStorage();
//   } else {
//     ESP_LOGW(TAG, "Unknown command: %s", command);
//   }
// }