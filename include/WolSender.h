#pragma once

#include <Arduino.h>

class WolSender {
 public:
  bool send(const String &macAddress, const String &broadcastIp, uint16_t port = 9);

 private:
  bool parseMac(const String &macAddress, uint8_t out[6]);
};
