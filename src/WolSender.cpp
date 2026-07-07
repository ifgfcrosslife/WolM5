#include "WolSender.h"
#include <WiFiUdp.h>

bool WolSender::send(const String &macAddress, const String &broadcastIp, uint16_t port) {
  uint8_t mac[6];
  if (!parseMac(macAddress, mac)) {
    return false;
  }

  uint8_t packet[102];
  memset(packet, 0xFF, 6);
  for (int i = 1; i <= 16; i++) {
    memcpy(&packet[i * 6], mac, 6);
  }

  IPAddress target;
  if (!target.fromString(broadcastIp.length() ? broadcastIp : "255.255.255.255")) {
    target.fromString("255.255.255.255");
  }

  WiFiUDP udp;
  udp.begin(0);
  const bool ok = udp.beginPacket(target, port) && udp.write(packet, sizeof(packet)) == sizeof(packet) && udp.endPacket();
  udp.stop();
  return ok;
}

bool WolSender::parseMac(const String &macAddress, uint8_t out[6]) {
  String clean = macAddress;
  clean.replace(":", "");
  clean.replace("-", "");
  clean.toUpperCase();

  if (clean.length() != 12) {
    return false;
  }

  for (int i = 0; i < 6; i++) {
    char byteChars[3] = {clean[i * 2], clean[i * 2 + 1], '\0'};
    char *end = nullptr;
    const long value = strtol(byteChars, &end, 16);
    if (end == byteChars || value < 0 || value > 255) {
      return false;
    }
    out[i] = static_cast<uint8_t>(value);
  }

  return true;
}
