#pragma once

#include <WebServer.h>
#include "AppConfig.h"
#include "ConfigStore.h"

class WebPortal {
 public:
  explicit WebPortal(ConfigStore &store);
  void begin(AppConfig &config);
  void handle();
  bool restartRequested() const;

 private:
  WebServer server;
  ConfigStore &store;
  AppConfig *configRef = nullptr;
  bool shouldRestart = false;

  void handleRoot();
  void handleSave();
  void handleReset();
  void handleStatus();
  void handleFavicon();
  void handleNotFound();
};
