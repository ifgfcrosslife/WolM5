#pragma once

#include <Arduino.h>

struct AppConfig {
  String deviceName = "wolm5";
  String wifiSsid;
  String wifiPassword;
  String supabaseUrl;
  String supabaseKey;
  String bridgeId;
  String bridgeSecret;
  String apPassword;
  bool wifiHidden = false;
  uint32_t commandPollMs = 5000;
  uint32_t statusPollMs = 30000;
  uint32_t bridgeHeartbeatMs = 900000;
  uint8_t pingCount = 2;
};

bool isConfigUsable(const AppConfig &config);
