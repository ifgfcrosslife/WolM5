#pragma once

#include <Arduino.h>
#include "SupabaseClient.h"

class FirmwareUpdater {
 public:
  bool updateFirmware(const String &url, const String &sha256, SupabaseClient &supabase, const String &commandId, String &message);
  bool updateFilesystem(const String &url, const String &sha256, SupabaseClient &supabase, const String &commandId, String &message);

 private:
  bool downloadAndApply(const String &url, const String &sha256, int command, const String &label, SupabaseClient &supabase, const String &commandId, int progressStart, int progressEnd, String &message);
  bool matchesSha256(const String &actual, const String &expected) const;
};
