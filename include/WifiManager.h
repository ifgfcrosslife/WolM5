#pragma once

#include "AppConfig.h"

class WifiManager {
 public:
  void beginAccessPoint(const AppConfig &config);
  bool connectStation(const AppConfig &config, uint32_t timeoutMs);
  bool isStationConnected() const;
  String localIp() const;
  String apIp() const;
};
