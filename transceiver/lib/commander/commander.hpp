#pragma once

#include <Arduino.h>

#include <cstring>

#include "SerialCom.hpp"
#define c_cmp(a, b) (strcmp(a, b) == 0)

class Commander {
 public:
  Commander(SerialCom *serialCom);

 private:
  uint16_t m_timeout = 20'000;  // 20 second timeout for commands

  SerialCom *m_serialCom;

  typedef void (Commander::*Handler)();

  struct HandlerMap {
    const char *name;
    Handler handler;
  };

  void handle_help();    // Command handler for "help"
  void handle_update();  // Command handler for "update" parameters
  void handle_mode();    // Command handler for "mode" (eg: transmit, receive,
                         // transcieve, spectrum scan, etc")
  void test();

  static constexpr const HandlerMap command_handler[4] = {
      {"help", &Commander::handle_help},
      {"update", &Commander::handle_update},
      {"mode", &Commander::handle_mode},
      {nullptr, nullptr}};

  static constexpr const HandlerMap update_handler[2] = {
      {"test", &Commander::test}, {nullptr, nullptr}};

  void runMappedCommand(char *command, const HandlerMap *handler);

  static constexpr const char *TAG = "Commander";

 public:
  void checkCommand(char *buffer,
                    const HandlerMap *handler =
                        command_handler);  // Check the command and run
                                           // the appropriate handler

  char *readAndRemove(char **buffer);
};
