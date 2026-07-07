#pragma once

#include <WebServer.h>
#include "AppConfig.h"
#include "ConfigStore.h"

class SupabaseClient;
class LanDiscoveryManager;

class WebPortal {
 public:
  WebPortal(ConfigStore &store, SupabaseClient &supabase, LanDiscoveryManager &lanDiscovery);
  void begin(AppConfig &config);
  void handle();
  bool restartRequested() const;

 private:
  WebServer server;
  ConfigStore &store;
  SupabaseClient &supabase;
  LanDiscoveryManager &lanDiscovery;
  AppConfig *configRef = nullptr;
  bool shouldRestart = false;

  void handleRoot();
  void handleSave();
  void handleReset();
  void handleStatus();
  void handleSupabaseTest();
  void handleRegisterBridge();
  void handleLanScanStart();
  void handleLanScanStatus();
  void handleLanDevices();
  void handleRegisterDevice();
  void handleFavicon();
  void handleNotFound();
};
