#pragma once

#include <Arduino.h>

#include <cstring>

#include "LoRaCom.hpp"
#include "SerialCom.hpp"

#define c_cmp(a, b) (strcmp(a, b) == 0)

class Commander {
 public:
  Commander(SerialCom *serialCom, LoRaCom *loraCom);

 private:
  char *m_command[128];  // Buffer to store the command

  uint16_t m_timeout = 20'000;  // 20 second timeout for commands

  SerialCom *m_serialCom;  // Pointer to SerialCom instance
  LoRaCom *m_loraCom;      // Pointer to LoRaCom instance

  typedef void (Commander::*Handler)();

  struct HandlerMap {
    const char *name;
    Handler handler;
  };

  // ----- Command Handlers -----
  void handle_command_help();  // Command handler for "help"
  void handle_update();        // Command handler for "update" parameters
  void handle_mode();  // Command handler for "mode" (eg: transmit, receive,
                       // transcieve, spectrum scan, etc")

  // ----- Update Handlers -----
  void handle_update_help();             // Command handler for "help"
  void handle_update_gain();             // Command handler for "update gain"
  void handle_update_freqMhz();          // Command handler for "update freqMhz"
  void handle_update_spreadingFactor();  // Command handler for "update
                                         // spreading factor"
  void handle_update_bandwidthKHz();  // Command handler for "update bandwidth"

  void handle_help(const HandlerMap *handler);

  static constexpr const HandlerMap command_handler[4] = {
      {"help", &Commander::handle_command_help},
      {"update", &Commander::handle_update},
      {"mode", &Commander::handle_mode},
      {nullptr, nullptr}};

  static constexpr const HandlerMap update_handler[6] = {
      {"help", &Commander::handle_update_help},
      {"gain", &Commander::handle_update_gain},
      {"freqMhz", &Commander::handle_update_freqMhz},
      {"sf", &Commander::handle_update_spreadingFactor},
      {"bwKHz", &Commander::handle_update_bandwidthKHz},
      {nullptr, nullptr}};

  void runMappedCommand(char *command, const HandlerMap *handler);

  static constexpr const char *TAG = "Commander";

 public:
  void checkCommand(const HandlerMap *handler =
                        command_handler);  // Check the command and run
                                           // the appropriate handler

  void setCommand(const char *buffer);

  char *readAndRemove();
};
