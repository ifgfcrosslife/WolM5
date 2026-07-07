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
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.mode(WIFI_AP_STA);
  const String password = config.apPassword.length() >= 8 ? config.apPassword : "wolm5setup";
  WiFi.softAP(apName().c_str(), password.c_str(), 6);
  Serial.print("AP started: ");
  Serial.println(WiFi.softAPIP());
}

bool WifiManager::connectStation(const AppConfig &config, uint32_t timeoutMs) {
  if (config.wifiSsid.isEmpty()) {
    return false;
  }

  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect(true, true);
  delay(200);
  const String hostName = config.deviceName.length() ? config.deviceName : "wolm5";
  WiFi.setHostname(hostName.c_str());
  if (config.wifiHidden) {
    Serial.println("Hidden SSID mode enabled");
    WiFi.begin(config.wifiSsid.c_str(), config.wifiPassword.c_str(), 0, nullptr, true);
  } else {
    WiFi.begin(config.wifiSsid.c_str(), config.wifiPassword.c_str());
  }

  Serial.print("Connecting to WiFi SSID: ");
  Serial.println(config.wifiSsid);

  const uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < timeoutMs) {
    Serial.print('.');
    delay(250);
  }
  Serial.println();

  const bool connected = WiFi.status() == WL_CONNECTED;
  if (connected) {
    Serial.print("WiFi connected, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.print("WiFi connect failed, status=");
    Serial.println(static_cast<int>(WiFi.status()));
  }

  return connected;
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
