#pragma once

#include <Arduino.h>

class NetworkDiscoveryManager {
 public:
  void begin(const String &hostName, uint16_t httpPort);
  void end();
  bool isActive() const;
  String hostName() const;

 private:
  String currentHostName;
  bool active = false;
};
