#include "NetworkDiscoveryManager.h"
#include <ESPmDNS.h>
#include <NetBIOS.h>

namespace {
String sanitizeHostName(String value) {
  value.toLowerCase();
  for (size_t i = 0; i < value.length(); i++) {
    const char c = value.charAt(i);
    const bool ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
    if (!ok) {
      value.setCharAt(i, '-');
    }
  }
  while (value.startsWith("-")) value.remove(0, 1);
  while (value.endsWith("-")) value.remove(value.length() - 1);
  if (value.isEmpty()) {
    value = "wolm5";
  }
  if (value.length() > 15) {
    value = value.substring(0, 15);
  }
  return value;
}
}

void NetworkDiscoveryManager::begin(const String &hostName, uint16_t httpPort) {
  end();

  currentHostName = sanitizeHostName(hostName);
  MDNS.begin(currentHostName.c_str());
  MDNS.addService("http", "tcp", httpPort);
  NBNS.begin(currentHostName.c_str());
  active = true;
}

void NetworkDiscoveryManager::end() {
  if (!active) {
    return;
  }

  MDNS.end();
  NBNS.end();
  active = false;
}

bool NetworkDiscoveryManager::isActive() const {
  return active;
}

String NetworkDiscoveryManager::hostName() const {
  return currentHostName;
}
