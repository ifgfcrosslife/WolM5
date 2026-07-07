#include "LanDiscoveryManager.h"
#include <WiFi.h>
#include <lwip/etharp.h>
#include <lwip/ip4_addr.h>
#include <ping/ping_sock.h>

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
}

void LanDiscoveryManager::begin() {
  state = State::Idle;
  resultCountValue = 0;
  scanStartIp = 0;
  scanEndIp = 0;
  currentIp = 0;
  lastTickMs = 0;
}

void LanDiscoveryManager::startScan() {
  IPAddress localIp = WiFi.localIP();
  IPAddress mask = WiFi.subnetMask();
  const uint32_t local = ipToU32(localIp);
  const uint32_t netmask = ipToU32(mask);
  scanStartIp = (local & netmask) + 1;
  scanEndIp = (local | (~netmask)) - 1;
  currentIp = scanStartIp;
  resultCountValue = 0;
  state = State::Scanning;
  lastTickMs = 0;
}

void LanDiscoveryManager::tick() {
  if (state != State::Scanning) {
    return;
  }

  const uint32_t now = millis();
  if (lastTickMs != 0 && now - lastTickMs < 35) {
    return;
  }
  lastTickMs = now;

  if (currentIp == 0 || currentIp > scanEndIp) {
    state = State::Finished;
    return;
  }

  const IPAddress ip = u32ToIp(currentIp);
  currentIp++;

  uint32_t latency = 0;
  const bool online = pingHost(ip.toString(), 120, latency);
  if (!online) {
    return;
  }

  const String ipText = ip.toString();
  const String macText = macForIp(ipText);
  pushResult(ipText, macText, static_cast<int>(latency));
}

bool LanDiscoveryManager::isScanning() const {
  return state == State::Scanning;
}

bool LanDiscoveryManager::hasResults() const {
  return resultCountValue > 0;
}

uint8_t LanDiscoveryManager::progressPercent() const {
  if (state != State::Scanning || scanEndIp <= scanStartIp) {
    return state == State::Finished ? 100 : 0;
  }
  const uint32_t total = scanEndIp - scanStartIp + 1;
  const uint32_t completed = currentIp > scanStartIp ? (currentIp - scanStartIp) : 0;
  const uint32_t pct = (completed * 100UL) / total;
  return pct > 100 ? 100 : static_cast<uint8_t>(pct);
}

size_t LanDiscoveryManager::resultCount() const {
  return resultCountValue;
}

const LanHostRecord &LanDiscoveryManager::result(size_t index) const {
  return results[index];
}

String LanDiscoveryManager::subnetLabel() const {
  if (scanEndIp <= scanStartIp) {
    return String();
  }
  IPAddress start = u32ToIp(scanStartIp);
  IPAddress end = u32ToIp(scanEndIp);
  return start.toString() + " - " + end.toString();
}

bool LanDiscoveryManager::pingHost(const String &ipAddress, uint32_t timeoutMs, uint32_t &latencyMs) {
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
  while (!ctx.finished && millis() - startedAt < timeoutMs + 500) {
    delay(5);
  }

  esp_ping_delete_session(handle);
  latencyMs = ctx.latencyMs;
  return ctx.success;
}

String LanDiscoveryManager::macForIp(const String &ipAddress) const {
  ip_addr_t target;
  if (!ipaddr_aton(ipAddress.c_str(), &target)) {
    return "";
  }

  for (size_t i = 0; i < ARP_TABLE_SIZE; i++) {
    ip4_addr_t *entryIp = nullptr;
    struct netif *entryNetif = nullptr;
    struct eth_addr *entryEth = nullptr;
    if (etharp_get_entry(i, &entryIp, &entryNetif, &entryEth) < 0 || entryIp == nullptr || entryEth == nullptr) {
      continue;
    }
    if (entryIp->addr != target.u_addr.ip4.addr) {
      continue;
    }

    char buffer[18];
    snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X",
             entryEth->addr[0], entryEth->addr[1], entryEth->addr[2],
             entryEth->addr[3], entryEth->addr[4], entryEth->addr[5]);
    return String(buffer);
  }

  return "";
}

void LanDiscoveryManager::pushResult(const String &ipAddress, const String &macAddress, int latencyMs) {
  for (size_t i = 0; i < resultCountValue; i++) {
    if (results[i].ipAddress == ipAddress) {
      results[i].macAddress = macAddress;
      results[i].latencyMs = latencyMs;
      results[i].online = true;
      if (results[i].displayName.isEmpty()) {
        results[i].displayName = ipAddress;
      }
      return;
    }
  }

  if (resultCountValue >= kMaxResults) {
    return;
  }

  LanHostRecord &slot = results[resultCountValue++];
  slot.ipAddress = ipAddress;
  slot.macAddress = macAddress;
  slot.displayName = ipAddress;
  slot.latencyMs = latencyMs;
  slot.online = true;
}

uint32_t LanDiscoveryManager::ipToU32(const IPAddress &ip) const {
  return static_cast<uint32_t>(ip);
}

IPAddress LanDiscoveryManager::u32ToIp(uint32_t value) const {
  return IPAddress((value >> 24) & 0xFF, (value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF);
}
