#include "ConfigStore.h"
#include <Preferences.h>

namespace {
Preferences prefs;
constexpr const char *NAMESPACE = "wolm5";
}

bool ConfigStore::begin() {
  return prefs.begin(NAMESPACE, false);
}

AppConfig ConfigStore::load() {
  AppConfig config;
  config.wifiSsid = prefs.getString("wifiSsid", "");
  config.wifiPassword = prefs.getString("wifiPass", "");
  config.supabaseUrl = prefs.getString("supaUrl", "");
  config.supabaseKey = prefs.getString("supaKey", "");
  config.bridgeId = prefs.getString("bridgeId", "m5-atom-s3");
  config.apPassword = prefs.getString("apPass", "wolm5setup");
  config.commandPollMs = prefs.getUInt("cmdPoll", 5000);
  config.statusPollMs = prefs.getUInt("statusPoll", 30000);
  config.pingCount = prefs.getUChar("pingCount", 2);
  return config;
}

bool ConfigStore::save(const AppConfig &config) {
  bool ok = true;
  ok &= prefs.putString("wifiSsid", config.wifiSsid) > 0 || config.wifiSsid.length() == 0;
  ok &= prefs.putString("wifiPass", config.wifiPassword) > 0 || config.wifiPassword.length() == 0;
  ok &= prefs.putString("supaUrl", config.supabaseUrl) > 0 || config.supabaseUrl.length() == 0;
  ok &= prefs.putString("supaKey", config.supabaseKey) > 0 || config.supabaseKey.length() == 0;
  ok &= prefs.putString("bridgeId", config.bridgeId) > 0 || config.bridgeId.length() == 0;
  ok &= prefs.putString("apPass", config.apPassword) > 0 || config.apPassword.length() == 0;
  ok &= prefs.putUInt("cmdPoll", config.commandPollMs) > 0;
  ok &= prefs.putUInt("statusPoll", config.statusPollMs) > 0;
  ok &= prefs.putUChar("pingCount", config.pingCount) > 0;
  return ok;
}

void ConfigStore::clear() {
  prefs.clear();
}
