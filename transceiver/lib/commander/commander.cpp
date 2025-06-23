#include "commander.hpp"

Commander::Commander(SerialCom* serialCom) {
  m_serialCom = serialCom;  // Initialize the SerialCom instance
  ESP_LOGD(TAG, "Commander initialised");
}

void Commander::handle_help() {
  // Implementation for help command
  ESP_LOGI(TAG, "Available commands: help, update, mode");
}

void Commander::handle_update() {
  ESP_LOGD(TAG, "Update command executed");

  char command[128];  // Buffer to store incoming data
  int rxIndex = 0;    // Index to track the length of the received message

  m_serialCom->sendData(
      "Update command received. Please enter the parameters to update.\n");

  unsigned long startTime = millis();  // Start time for timeout

  while (true) {  // Wait for data or timeout
    if (m_serialCom->getData(command, sizeof(command), &rxIndex)) {
      checkCommand(command,
                   update_handler);  // Check the command and run the handler
      break;
    }

    if (millis() - startTime > m_timeout) {
      m_serialCom->sendData("Timeout reached. No data received.\n");
      break;
    }
    vTaskDelay(
        pdMS_TO_TICKS(50));  // Wait for a short period before checking again
  }
}

void Commander::handle_mode() {
  // Implementation for mode command
  ESP_LOGD(TAG, "Mode command executed");
}

void Commander::checkCommand(char* buffer, const HandlerMap* handler_) {
  char* token = readAndRemove(&buffer);

  if (token != nullptr) {
    runMappedCommand(token, handler_);  // Run the mapped command
  } else {
    ESP_LOGW(TAG, "No command provided");
  }
}

void Commander::runMappedCommand(char* command, const HandlerMap* handler) {
  // Call the mapped command handler
  for (const HandlerMap* cmd = handler; cmd->name != nullptr; ++cmd) {
    if (c_cmp(command, cmd->name)) {
      return (this->*cmd->handler)();  // Call the corresponding handler
    }
  }
}

void Commander::test() {
  // Implementation for test command
  ESP_LOGD(TAG, "Test command executed");
  m_serialCom->sendData("Test command executed successfully.\n");
}

char* Commander::readAndRemove(char** buffer) {
  if (buffer == nullptr || *buffer == nullptr) return nullptr;

  char* start = *buffer;

  // Skip leading spaces
  while (*start == ' ') start++;

  // If we reached the end, return nullptr
  if (*start == '\0') {
    *buffer = nullptr;
    return nullptr;
  }

  // Find the end of the current token
  char* end = start;
  while (*end != ' ' && *end != '\0') end++;

  // If we found a space, null-terminate the token and update buffer
  if (*end == ' ') {
    *end = '\0';        // Null-terminate the current token
    *buffer = end + 1;  // Point to the rest of the string
  } else {
    // No more tokens after this one
    *buffer = nullptr;
  }

  return start;
}