#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "AppConfig.h"
#include "ConfigStore.h"
#include "StatusMonitor.h"
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

  configStore.begin();
  config = configStore.load();

  wifiManager.beginAccessPoint(config);
  webPortal.begin(config);

  if (wifiManager.connectStation(config, 15000)) {
    Serial.print("WiFi connected: ");
    Serial.println(wifiManager.localIp());
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  } else {
    Serial.print("Setup AP active: ");
    Serial.println(wifiManager.apIp());
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
    wifiManager.connectStation(config, 3000);
  }

  pollCommands();
  statusMonitor.tick();
  delay(10);
}
