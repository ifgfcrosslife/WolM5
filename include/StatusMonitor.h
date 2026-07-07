#pragma once

#include "SupabaseClient.h"

class StatusMonitor {
 public:
  explicit StatusMonitor(SupabaseClient &supabase);
  void begin(const AppConfig &config);
  void tick();

 private:
  SupabaseClient &api;
  AppConfig cfg;
  uint32_t lastRun = 0;
};
