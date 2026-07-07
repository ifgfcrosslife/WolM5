#include "StatusMonitor.h"
#include <ESP32Ping.h>

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

    IPAddress target;
    const bool validIp = target.fromString(ip);
    const bool online = validIp && Ping.ping(target, cfg.pingCount);
    const int latency = online ? static_cast<int>(Ping.averageTime()) : -1;
    api.upsertDeviceStatus(id, online, ip, latency);
    delay(50);
  }
}
