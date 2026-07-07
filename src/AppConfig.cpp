#include "AppConfig.h"

bool isConfigUsable(const AppConfig &config) {
  return config.wifiSsid.length() > 0 &&
         config.supabaseUrl.length() > 0 &&
         config.supabaseKey.length() > 0 &&
         config.bridgeId.length() > 0 &&
         config.bridgeSecret.length() > 0;
}
