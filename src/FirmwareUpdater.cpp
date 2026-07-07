#include "FirmwareUpdater.h"
#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClientSecure.h>
#include <mbedtls/sha256.h>

namespace {
String toHex(const uint8_t *bytes, size_t len) {
  const char *hex = "0123456789abcdef";
  String out;
  out.reserve(len * 2);
  for (size_t i = 0; i < len; i++) {
    out += hex[(bytes[i] >> 4) & 0x0F];
    out += hex[bytes[i] & 0x0F];
  }
  return out;
}
}

bool FirmwareUpdater::matchesSha256(const String &actual, const String &expected) const {
  if (expected.length() == 0) return true;

  String a = actual;
  String e = expected;
  a.toLowerCase();
  e.toLowerCase();
  e.replace(" ", "");
  return a == e;
}

bool FirmwareUpdater::updateFirmware(const String &url, const String &sha256, SupabaseClient &supabase, const String &commandId, String &message) {
  return downloadAndApply(url, sha256, U_FLASH, "firmware", supabase, commandId, 40, 100, message);
}

bool FirmwareUpdater::updateFilesystem(const String &url, const String &sha256, SupabaseClient &supabase, const String &commandId, String &message) {
  if (url.length() == 0) {
    message = "Filesystem URL kosong";
    return false;
  }
  return downloadAndApply(url, sha256, U_SPIFFS, "filesystem", supabase, commandId, 0, 40, message);
}

bool FirmwareUpdater::downloadAndApply(const String &url, const String &sha256, int command, const String &label, SupabaseClient &supabase, const String &commandId, int progressStart, int progressEnd, String &message) {
  if (url.length() == 0) {
    message = label + " URL kosong";
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(client, url)) {
    message = "Gagal membuka " + label + " URL";
    return false;
  }

  const int code = http.GET();
  if (code < 200 || code >= 300) {
    message = "Download " + label + " gagal HTTP " + String(code);
    http.end();
    return false;
  }

  const int total = http.getSize();
  if (!Update.begin(total > 0 ? total : UPDATE_SIZE_UNKNOWN, command)) {
    message = "OTA begin gagal: " + String(Update.errorString());
    http.end();
    return false;
  }

  mbedtls_sha256_context sha;
  mbedtls_sha256_init(&sha);
  mbedtls_sha256_starts(&sha, 0);

  WiFiClient *stream = http.getStreamPtr();
  uint8_t buffer[1024];
  size_t written = 0;
  int lastProgress = -1;
  uint32_t lastReport = 0;

  while (http.connected() && (total <= 0 || written < static_cast<size_t>(total))) {
    const size_t available = stream->available();
    if (available == 0) {
      delay(1);
      continue;
    }

    const size_t readLen = stream->readBytes(buffer, min(available, sizeof(buffer)));
    if (readLen == 0) {
      delay(1);
      continue;
    }

    const size_t updateWritten = Update.write(buffer, readLen);
    if (updateWritten != readLen) {
      message = "OTA write gagal: " + String(Update.errorString());
      Update.abort();
      http.end();
      mbedtls_sha256_free(&sha);
      return false;
    }

    mbedtls_sha256_update(&sha, buffer, readLen);
    written += readLen;

    const int relativeProgress = total > 0 ? static_cast<int>((written * 100) / total) : 50;
    int progress = progressStart + ((progressEnd - progressStart) * relativeProgress / 100);
    if (progress >= progressEnd) progress = progressEnd - 1;
    const uint32_t now = millis();
    if (progress != lastProgress && (progress % 5 == 0 || now - lastReport > 1500)) {
      lastProgress = progress;
      lastReport = now;
      supabase.markCommandProgress(commandId, progress, "Downloading " + label + " " + String(progress) + "%");
    }
  }

  uint8_t digest[32];
  mbedtls_sha256_finish(&sha, digest);
  mbedtls_sha256_free(&sha);

  const String actualSha = toHex(digest, sizeof(digest));
  if (!matchesSha256(actualSha, sha256)) {
    message = "SHA256 " + label + " tidak cocok";
    Update.abort();
    http.end();
    return false;
  }

  if (!Update.end(true)) {
    message = "OTA end gagal: " + String(Update.errorString());
    http.end();
    return false;
  }

  if (!Update.isFinished()) {
    message = "OTA belum selesai";
    http.end();
    return false;
  }

  http.end();
  supabase.markCommandProgress(commandId, progressEnd, label + " update selesai");
  message = label + " update selesai";
  return true;
}
