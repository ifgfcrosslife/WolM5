#include "SupabaseClient.h"
#include <ctype.h>
#include <time.h>

namespace {
String jsonEscape(String value) {
  value.replace("\\", "\\\\");
  value.replace("\"", "\\\"");
  value.replace("\n", " ");
  value.replace("\r", " ");
  return value;
}

String queryEscape(const String &value) {
  String escaped;
  const char *hex = "0123456789ABCDEF";
  for (size_t i = 0; i < value.length(); i++) {
    const uint8_t c = static_cast<uint8_t>(value.charAt(i));
    const bool safe = isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~';
    if (safe) {
      escaped += static_cast<char>(c);
    } else {
      escaped += '%';
      escaped += hex[(c >> 4) & 0x0F];
      escaped += hex[c & 0x0F];
    }
  }
  return escaped;
}

String currentIsoTime() {
  time_t now = time(nullptr);
  if (now < 1700000000) {
    return "";
  }

  tm timeInfo;
  gmtime_r(&now, &timeInfo);
  char buffer[25];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &timeInfo);
  return String(buffer);
}
}

void SupabaseClient::configure(const AppConfig &config) {
  cfg = config;
  cfg.supabaseUrl.trim();
  if (cfg.supabaseUrl.endsWith("/")) {
    cfg.supabaseUrl.remove(cfg.supabaseUrl.length() - 1);
  }
  secureClient.setInsecure();
}

void SupabaseClient::setBridgeSecret(const String &bridgeSecret) {
  cfg.bridgeSecret = bridgeSecret;
  cfg.bridgeSecret.trim();
}

bool SupabaseClient::isConfigured() const {
  return cfg.supabaseUrl.length() > 0 &&
         cfg.supabaseKey.length() > 0 &&
         cfg.bridgeId.length() > 0 &&
         cfg.bridgeSecret.length() > 0;
}

bool SupabaseClient::fetchNextCommand(WolCommand &command) {
  if (!isConfigured()) return false;

  const String url = restUrl("/wol_commands?select=id,device_id,mac_address,broadcast_ip,port&bridge_id=eq." +
                            queryEscape(cfg.bridgeId) + "&status=eq.pending&order=created_at.asc&limit=1");
  int code = 0;
  String response;
  if (!request("GET", url, "", code, response) || code < 200 || code >= 300) {
    return false;
  }

  JsonDocument doc;
  if (deserializeJson(doc, response) != DeserializationError::Ok || !doc.is<JsonArray>() || doc.size() == 0) {
    return false;
  }

  JsonObject row = doc[0];
  command.id = row["id"] | "";
  command.deviceId = row["device_id"] | "";
  command.macAddress = row["mac_address"] | "";
  command.broadcastIp = row["broadcast_ip"] | "255.255.255.255";
  command.port = row["port"] | 9;
  return command.id.length() > 0 && command.macAddress.length() > 0;
}

bool SupabaseClient::markCommand(const String &commandId, const String &status, const String &message) {
  if (!isConfigured() || commandId.length() == 0) return false;

  const String url = restUrl("/wol_commands?id=eq." + queryEscape(commandId));
  String body = "{\"status\":\"" + jsonEscape(status) + "\",\"message\":\"" + jsonEscape(message) + "\"";
  const String timestamp = currentIsoTime();
  if (timestamp.length() > 0) {
    body += ",\"processed_at\":\"" + timestamp + "\"";
  }
  body += "}";
  int code = 0;
  String response;
  return request("PATCH", url, body, code, response) && code >= 200 && code < 300;
}

bool SupabaseClient::upsertDeviceStatus(const String &deviceId, bool online, const String &ipAddress, int latencyMs) {
  if (!isConfigured() || deviceId.length() == 0) return false;

  const String url = restUrl("/wol_device_status?on_conflict=device_id");
  String body = "{\"device_id\":\"" + jsonEscape(deviceId) + "\",\"bridge_id\":\"" + jsonEscape(cfg.bridgeId) +
                "\",\"online\":" + String(online ? "true" : "false") +
                ",\"ip_address\":\"" + jsonEscape(ipAddress) + "\",\"latency_ms\":" + String(latencyMs);
  const String timestamp = currentIsoTime();
  if (timestamp.length() > 0) {
    body += ",\"checked_at\":\"" + timestamp + "\"";
  }
  body += "}";
  int code = 0;
  String response;
  return request("POST", url, body, code, response) && code >= 200 && code < 300;
}

bool SupabaseClient::upsertBridge(const String &bridgeId, const String &name, const String &localIp, const String &apIp, bool wifiConnected) {
  if (!isConfigured() || bridgeId.length() == 0) return false;

  const String url = restUrl("/wol_bridges?on_conflict=id");
  String body = "{\"id\":\"" + jsonEscape(bridgeId) + "\",\"name\":\"" + jsonEscape(name) +
                "\",\"bridge_secret\":\"" + jsonEscape(cfg.bridgeSecret) +
                "\",\"local_ip\":\"" + jsonEscape(localIp) + "\",\"ap_ip\":\"" + jsonEscape(apIp) +
                "\",\"wifi_connected\":" + String(wifiConnected ? "true" : "false");
  const String timestamp = currentIsoTime();
  if (timestamp.length() > 0) {
    body += ",\"last_seen_at\":\"" + timestamp + "\"";
  }
  body += "}";
  int code = 0;
  String response;
  return request("POST", url, body, code, response) && code >= 200 && code < 300;
}

bool SupabaseClient::bridgeExists(const String &bridgeId) {
  if (!isConfigured() || bridgeId.length() == 0) return false;

  const String url = restUrl("/wol_bridges?select=id&id=eq." + queryEscape(bridgeId) + "&limit=1");
  int code = 0;
  String response;
  if (!request("GET", url, "", code, response) || code < 200 || code >= 300) {
    return false;
  }

  JsonDocument doc;
  return deserializeJson(doc, response) == DeserializationError::Ok && doc.is<JsonArray>() && doc.size() > 0;
}

bool SupabaseClient::upsertDeviceByMac(const String &bridgeId, const String &name, const String &macAddress, const String &ipAddress, const String &broadcastIp, uint16_t port) {
  if (!isConfigured() || bridgeId.length() == 0 || macAddress.length() == 0) return false;

  const String findUrl = restUrl("/wol_devices?select=id&bridge_id=eq." + queryEscape(bridgeId) +
                                 "&mac_address=eq." + queryEscape(macAddress) + "&limit=1");
  int code = 0;
  String response;
  if (!request("GET", findUrl, "", code, response) || code < 200 || code >= 300) {
    return false;
  }

  JsonDocument doc;
  String deviceId;
  if (deserializeJson(doc, response) == DeserializationError::Ok && doc.is<JsonArray>() && doc.size() > 0) {
    deviceId = doc[0]["id"] | "";
  }

  const String safeName = name.length() > 0 ? name : String("PC");
  const String safeBroadcast = broadcastIp.length() > 0 ? broadcastIp : String("255.255.255.255");

  if (deviceId.length() > 0) {
    const String url = restUrl("/wol_devices?id=eq." + queryEscape(deviceId));
    String body = "{\"name\":\"" + jsonEscape(safeName) + "\",\"mac_address\":\"" + jsonEscape(macAddress) +
                  "\",\"ip_address\":\"" + jsonEscape(ipAddress) + "\",\"broadcast_ip\":\"" + jsonEscape(safeBroadcast) +
                  "\",\"port\":" + String(port) + ",\"enabled\":true,\"updated_at\":\"" + currentIsoTime() + "\"}";
    int patchCode = 0;
    String patchResponse;
    return request("PATCH", url, body, patchCode, patchResponse) && patchCode >= 200 && patchCode < 300;
  }

  const String url = restUrl("/wol_devices");
  String body = "{\"bridge_id\":\"" + jsonEscape(bridgeId) + "\",\"name\":\"" + jsonEscape(safeName) +
                "\",\"mac_address\":\"" + jsonEscape(macAddress) + "\",\"ip_address\":\"" + jsonEscape(ipAddress) +
                "\",\"broadcast_ip\":\"" + jsonEscape(safeBroadcast) + "\",\"port\":" + String(port) + ",\"enabled\":true}";
  int insertCode = 0;
  String insertResponse;
  return request("POST", url, body, insertCode, insertResponse) && insertCode >= 200 && insertCode < 300;
}

bool SupabaseClient::fetchDevices(JsonDocument &doc) {
  if (!isConfigured()) return false;

  const String url = restUrl("/wol_devices?select=id,name,ip_address,mac_address,broadcast_ip,port,enabled&bridge_id=eq." +
                            queryEscape(cfg.bridgeId) + "&enabled=eq.true");
  int code = 0;
  String response;
  if (!request("GET", url, "", code, response) || code < 200 || code >= 300) {
    return false;
  }

  return deserializeJson(doc, response) == DeserializationError::Ok && doc.is<JsonArray>();
}

String SupabaseClient::restUrl(const String &pathAndQuery) const {
  return cfg.supabaseUrl + "/rest/v1" + pathAndQuery;
}

bool SupabaseClient::request(const String &method, const String &url, const String &body, int &statusCode, String &response) {
  HTTPClient http;
  if (!http.begin(secureClient, url)) {
    return false;
  }

  addHeaders(http, method == "PATCH" || method == "POST");
  if (method == "GET") {
    statusCode = http.GET();
  } else if (method == "POST") {
    statusCode = http.POST(body);
  } else if (method == "PATCH") {
    statusCode = http.PATCH(body);
  } else {
    http.end();
    return false;
  }

  response = http.getString();
  http.end();
  return statusCode > 0;
}

void SupabaseClient::addHeaders(HTTPClient &http, bool preferReturnMinimal) {
  http.addHeader("apikey", cfg.supabaseKey);
  http.addHeader("Authorization", "Bearer " + cfg.supabaseKey);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-bridge-secret", cfg.bridgeSecret);
  if (preferReturnMinimal) {
    http.addHeader("Prefer", "return=minimal,resolution=merge-duplicates");
  }
}
