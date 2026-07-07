#include "WebPortal.h"
#include <ArduinoJson.h>
#include <WiFi.h>

namespace {
String htmlEscape(String value) {
  value.replace("&", "&amp;");
  value.replace("\"", "&quot;");
  value.replace("'", "&#39;");
  value.replace("<", "&lt;");
  value.replace(">", "&gt;");
  return value;
}
}

WebPortal::WebPortal(ConfigStore &store) : server(80), store(store) {}

void WebPortal::begin(AppConfig &config) {
  configRef = &config;

  server.on("/", HTTP_GET, [this]() { handleRoot(); });
  server.on("/save", HTTP_POST, [this]() { handleSave(); });
  server.on("/reset", HTTP_POST, [this]() { handleReset(); });
  server.on("/status", HTTP_GET, [this]() { handleStatus(); });
  server.begin();
}

void WebPortal::handle() {
  server.handleClient();
}

bool WebPortal::restartRequested() const {
  return shouldRestart;
}

void WebPortal::handleRoot() {
  AppConfig &cfg = *configRef;
  String html;
  html.reserve(7000);
  html += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>WOL M5 Setup</title><style>");
  html += F("body{font-family:Arial,sans-serif;margin:0;background:#f7f7f3;color:#202124}main{max-width:760px;margin:auto;padding:28px}");
  html += F("h1{font-size:28px}label{display:block;margin-top:14px;font-weight:700}input{width:100%;box-sizing:border-box;padding:11px;border:1px solid #bbb;border-radius:6px;margin-top:6px}");
  html += F(".row{display:grid;grid-template-columns:1fr 1fr;gap:14px}.actions{display:flex;gap:10px;margin-top:18px;flex-wrap:wrap}");
  html += F("button{border:0;border-radius:6px;padding:11px 16px;background:#155e75;color:white;font-weight:700}.danger{background:#a33}");
  html += F(".meta{background:white;border:1px solid #ddd;border-radius:8px;padding:14px;margin:16px 0}@media(max-width:620px){.row{grid-template-columns:1fr}}");
  html += F("</style></head><body><main><h1>WOL M5 Atom S3</h1>");
  html += F("<div class='meta'>AP IP: ");
  html += WiFi.softAPIP().toString();
  html += F("<br>Local IP: ");
  html += WiFi.localIP().toString();
  html += F("<br>WiFi: ");
  html += (WiFi.status() == WL_CONNECTED ? "connected" : "not connected");
  html += F("</div><form method='post' action='/save'>");
  html += F("<label>WiFi SSID<input name='wifiSsid' value='");
  html += htmlEscape(cfg.wifiSsid);
  html += F("'></label><label>WiFi Password<input name='wifiPassword' type='password' value='");
  html += htmlEscape(cfg.wifiPassword);
  html += F("'></label><label>Supabase URL<input name='supabaseUrl' placeholder='https://xxxxx.supabase.co' value='");
  html += htmlEscape(cfg.supabaseUrl);
  html += F("'></label><label>Supabase API Key<input name='supabaseKey' type='password' value='");
  html += htmlEscape(cfg.supabaseKey);
  html += F("'></label><div class='row'><label>Bridge ID<input name='bridgeId' value='");
  html += htmlEscape(cfg.bridgeId);
  html += F("'></label><label>AP Password<input name='apPassword' value='");
  html += htmlEscape(cfg.apPassword);
  html += F("'></label></div><div class='row'><label>Command Poll ms<input name='commandPollMs' type='number' min='1000' value='");
  html += String(cfg.commandPollMs);
  html += F("'></label><label>Status Poll ms<input name='statusPollMs' type='number' min='10000' value='");
  html += String(cfg.statusPollMs);
  html += F("'></label></div><label>Ping Count<input name='pingCount' type='number' min='1' max='5' value='");
  html += String(cfg.pingCount);
  html += F("'></label><div class='actions'><button type='submit'>Save & Restart</button></div></form>");
  html += F("<form method='post' action='/reset' class='actions'><button class='danger' type='submit'>Reset Config</button></form></main></body></html>");
  server.send(200, "text/html", html);
}

void WebPortal::handleSave() {
  AppConfig next = *configRef;
  next.wifiSsid = server.arg("wifiSsid");
  next.wifiPassword = server.arg("wifiPassword");
  next.supabaseUrl = server.arg("supabaseUrl");
  next.supabaseKey = server.arg("supabaseKey");
  next.bridgeId = server.arg("bridgeId");
  next.apPassword = server.arg("apPassword");
  next.commandPollMs = server.arg("commandPollMs").toInt();
  next.statusPollMs = server.arg("statusPollMs").toInt();
  next.pingCount = static_cast<uint8_t>(server.arg("pingCount").toInt());

  if (next.commandPollMs < 1000) next.commandPollMs = 5000;
  if (next.statusPollMs < 10000) next.statusPollMs = 30000;
  if (next.pingCount < 1 || next.pingCount > 5) next.pingCount = 2;
  if (next.apPassword.length() < 8) next.apPassword = "wolm5setup";

  store.save(next);
  *configRef = next;
  shouldRestart = true;
  server.send(200, "text/html", "<p>Saved. Restarting...</p>");
}

void WebPortal::handleReset() {
  store.clear();
  shouldRestart = true;
  server.send(200, "text/html", "<p>Config reset. Restarting...</p>");
}

void WebPortal::handleStatus() {
  JsonDocument doc;
  doc["wifi"] = WiFi.status() == WL_CONNECTED;
  doc["localIp"] = WiFi.localIP().toString();
  doc["apIp"] = WiFi.softAPIP().toString();
  doc["heap"] = ESP.getFreeHeap();
  String body;
  serializeJson(doc, body);
  server.send(200, "application/json", body);
}
