#include "WifiManager.h"
#include <WiFi.h>

namespace {
String apName() {
  uint64_t chipId = ESP.getEfuseMac();
  char suffix[7];
  snprintf(suffix, sizeof(suffix), "%06X", static_cast<uint32_t>(chipId & 0xFFFFFF));
  return "WOLM5-" + String(suffix);
}
}

void WifiManager::beginAccessPoint(const AppConfig &config) {
  WiFi.mode(WIFI_AP_STA);
  const String password = config.apPassword.length() >= 8 ? config.apPassword : "wolm5setup";
  WiFi.softAP(apName().c_str(), password.c_str());
}

bool WifiManager::connectStation(const AppConfig &config, uint32_t timeoutMs) {
  if (config.wifiSsid.isEmpty()) {
    return false;
  }

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(config.wifiSsid.c_str(), config.wifiPassword.c_str());

  const uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < timeoutMs) {
    delay(250);
  }

  return WiFi.status() == WL_CONNECTED;
}

bool WifiManager::isStationConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

String WifiManager::localIp() const {
  return WiFi.localIP().toString();
}

String WifiManager::apIp() const {
  return WiFi.softAPIP().toString();
}
