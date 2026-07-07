#include "StatusMonitor.h"
#include <ping/ping_sock.h>
#include <lwip/inet.h>

namespace {
struct PingContext {
  bool finished = false;
  bool success = false;
  uint32_t latencyMs = 0;
};

void onPingSuccess(esp_ping_handle_t hdl, void *args) {
  auto *ctx = static_cast<PingContext *>(args);
  ctx->success = true;
  uint32_t timeMs = 0;
  if (esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &timeMs, sizeof(timeMs)) == ESP_OK) {
    ctx->latencyMs = timeMs;
  }
}

void onPingTimeout(esp_ping_handle_t, void *args) {
  auto *ctx = static_cast<PingContext *>(args);
  ctx->success = false;
}

void onPingEnd(esp_ping_handle_t, void *args) {
  auto *ctx = static_cast<PingContext *>(args);
  ctx->finished = true;
}

bool pingHost(const String &ipAddress, uint32_t timeoutMs, uint32_t &latencyMs) {
  PingContext ctx;
  esp_ping_handle_t handle = nullptr;

  esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
  config.count = 1;
  config.interval_ms = 1000;
  config.timeout_ms = timeoutMs;
  config.data_size = 32;
  config.ttl = 64;
  if (!ipaddr_aton(ipAddress.c_str(), &config.target_addr)) {
    return false;
  }

  esp_ping_callbacks_t callbacks = {};
  callbacks.cb_args = &ctx;
  callbacks.on_ping_success = onPingSuccess;
  callbacks.on_ping_timeout = onPingTimeout;
  callbacks.on_ping_end = onPingEnd;

  if (esp_ping_new_session(&config, &callbacks, &handle) != ESP_OK) {
    return false;
  }

  const bool started = esp_ping_start(handle) == ESP_OK;
  if (!started) {
    esp_ping_delete_session(handle);
    return false;
  }

  const uint32_t startedAt = millis();
  while (!ctx.finished && millis() - startedAt < timeoutMs + 1500) {
    delay(10);
  }

  esp_ping_delete_session(handle);
  latencyMs = ctx.latencyMs;
  return ctx.success;
}
}

StatusMonitor::StatusMonitor(SupabaseClient &supabase) : api(supabase) {}

void StatusMonitor::begin(const AppConfig &config) {
  cfg = config;
  lastRun = 0;
}

void StatusMonitor::tick() {
  const uint32_t now = millis();
  if (lastRun != 0 && now - lastRun < cfg.statusPollMs) {
    return;
  }
  lastRun = now;

  JsonDocument devices;
  if (!api.fetchDevices(devices)) {
    return;
  }

  for (JsonObject device : devices.as<JsonArray>()) {
    const String id = device["id"] | "";
    const String ip = device["ip_address"] | "";
    if (id.length() == 0 || ip.length() == 0) {
      continue;
    }

    uint32_t latency = 0;
    const bool online = pingHost(ip, 1000, latency);
    const int latencyValue = online ? static_cast<int>(latency) : -1;
    api.upsertDeviceStatus(id, online, ip, latencyValue);
    delay(50);
  }
}
