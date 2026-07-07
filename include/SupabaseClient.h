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

class SupabaseClient {
 public:
  void configure(const AppConfig &config);
  bool isConfigured() const;
  bool fetchNextCommand(WolCommand &command);
  bool markCommand(const String &commandId, const String &status, const String &message);
  bool upsertDeviceStatus(const String &deviceId, bool online, const String &ipAddress, int latencyMs);
  bool fetchDevices(JsonDocument &doc);

 private:
  AppConfig cfg;
  WiFiClientSecure secureClient;

  String restUrl(const String &pathAndQuery) const;
  bool request(const String &method, const String &url, const String &body, int &statusCode, String &response);
  void addHeaders(HTTPClient &http, bool preferReturnMinimal = false);
};
