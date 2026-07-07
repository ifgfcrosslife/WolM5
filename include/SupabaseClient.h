#pragma once

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "AppConfig.h"

struct WolCommand {
  String id;
  String deviceId;
  String macAddress;
  String broadcastIp;
  uint16_t port = 9;
};

struct WolDeviceRecord {
  String id;
  String name;
  String ipAddress;
  String macAddress;
  String broadcastIp = "255.255.255.255";
  uint16_t port = 9;
  bool enabled = true;
};

class SupabaseClient {
 public:
  void configure(const AppConfig &config);
  bool isConfigured() const;
  bool fetchNextCommand(WolCommand &command);
  bool markCommand(const String &commandId, const String &status, const String &message);
  bool upsertDeviceStatus(const String &deviceId, bool online, const String &ipAddress, int latencyMs);
  bool upsertBridge(const String &bridgeId, const String &name, const String &localIp, const String &apIp, bool wifiConnected);
  bool bridgeExists(const String &bridgeId);
  bool fetchDevices(JsonDocument &doc);
  bool upsertDeviceByMac(const String &bridgeId, const String &name, const String &macAddress, const String &ipAddress, const String &broadcastIp, uint16_t port);

 private:
  AppConfig cfg;
  WiFiClientSecure secureClient;

  String restUrl(const String &pathAndQuery) const;
  bool request(const String &method, const String &url, const String &body, int &statusCode, String &response);
  void addHeaders(HTTPClient &http, bool preferReturnMinimal = false);
};
