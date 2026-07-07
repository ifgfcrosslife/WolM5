#pragma once

#include <Arduino.h>
#include <array>
#include "AppConfig.h"

struct LanHostRecord {
  String ipAddress;
  String macAddress;
  String displayName;
  int latencyMs = -1;
  bool online = false;
};

class LanDiscoveryManager {
 public:
  static constexpr size_t kMaxResults = 24;

  void begin();
  void startScan();
  void tick();
  bool isScanning() const;
  bool hasResults() const;
  uint8_t progressPercent() const;
  size_t resultCount() const;
  const LanHostRecord &result(size_t index) const;
  String subnetLabel() const;

 private:
  enum class State { Idle, Scanning, Finished };

  State state = State::Idle;
  std::array<LanHostRecord, kMaxResults> results{};
  size_t resultCountValue = 0;
  uint32_t scanStartIp = 0;
  uint32_t scanEndIp = 0;
  uint32_t currentIp = 0;
  uint32_t lastTickMs = 0;

  bool pingHost(const String &ipAddress, uint32_t timeoutMs, uint32_t &latencyMs);
  String macForIp(const String &ipAddress) const;
  void pushResult(const String &ipAddress, const String &macAddress, int latencyMs);
  uint32_t ipToU32(const IPAddress &ip) const;
  IPAddress u32ToIp(uint32_t value) const;
};
