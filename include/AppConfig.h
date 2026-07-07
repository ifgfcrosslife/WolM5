#pragma once

#include <Arduino.h>

struct AppConfig {
  String wifiSsid;
  String wifiPassword;
  String supabaseUrl;
  String supabaseKey;
  String bridgeId;
  String apPassword;
  uint32_t commandPollMs = 5000;
  uint32_t statusPollMs = 30000;
  uint8_t pingCount = 2;
};

bool isConfigUsable(const AppConfig &config);
