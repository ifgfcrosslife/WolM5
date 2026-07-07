#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "AppConfig.h"
#include "ConfigStore.h"
#include "StatusMonitor.h"
#include "DisplayManager.h"
#include "NetworkDiscoveryManager.h"
#include "SupabaseClient.h"
#include "WebPortal.h"
#include "WifiManager.h"
#include "WolSender.h"

ConfigStore configStore;
WifiManager wifiManager;
SupabaseClient supabase;
StatusMonitor statusMonitor(supabase);
WolSender wolSender;
WebPortal webPortal(configStore);
DisplayManager display;
NetworkDiscoveryManager discovery;

AppConfig config;
uint32_t lastCommandPoll = 0;

void pollCommands() {
  if (!wifiManager.isStationConnected() || !supabase.isConfigured()) {
    return;
  }

  const uint32_t now = millis();
  if (lastCommandPoll != 0 && now - lastCommandPoll < config.commandPollMs) {
    return;
  }
  lastCommandPoll = now;

  WolCommand command;
  if (!supabase.fetchNextCommand(command)) {
    return;
  }

  supabase.markCommand(command.id, "processing", "M5 bridge accepted command");
  const bool sent = wolSender.send(command.macAddress, command.broadcastIp, command.port);
  supabase.markCommand(command.id, sent ? "done" : "failed", sent ? "Magic packet sent" : "Failed to send magic packet");
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("WOL M5 Atom S3 starting");

  display.begin();
  configStore.begin();
  config = configStore.load();
  display.showBoot(config);

  wifiManager.beginAccessPoint(config);
  webPortal.begin(config);

  if (wifiManager.connectStation(config, 15000)) {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    discovery.begin(config.deviceName, 80);
    display.showConnected(wifiManager.localIp(), wifiManager.apIp());
  } else {
    Serial.print("Setup AP active: ");
    Serial.println(wifiManager.apIp());
    display.showSetupMode(wifiManager.apIp());
  }

  supabase.configure(config);
  statusMonitor.begin(config);
}

void loop() {
  webPortal.handle();

  if (webPortal.restartRequested()) {
    delay(800);
    ESP.restart();
  }

  if (WiFi.status() != WL_CONNECTED && config.wifiSsid.length() > 0) {
    display.showConnecting(config.wifiSsid, config.wifiHidden);
    if (wifiManager.connectStation(config, 3000)) {
      discovery.begin(config.deviceName, 80);
      display.showConnected(wifiManager.localIp(), wifiManager.apIp());
    } else {
      discovery.end();
      display.showSetupMode(wifiManager.apIp());
    }
  }

  pollCommands();
  statusMonitor.tick();
  display.tick();
  delay(10);
}
