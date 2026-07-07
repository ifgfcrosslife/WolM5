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

const char *checkedAttr(bool value) {
  return value ? " checked" : "";
}

String boolText(bool value, const char *yesText, const char *noText) {
  return value ? yesText : noText;
}
}

WebPortal::WebPortal(ConfigStore &store) : server(80), store(store) {}

void WebPortal::begin(AppConfig &config) {
  configRef = &config;

  server.on("/", HTTP_GET, [this]() { handleRoot(); });
  server.on("/favicon.ico", HTTP_GET, [this]() { handleFavicon(); });
  server.on("/save", HTTP_POST, [this]() { handleSave(); });
  server.on("/reset", HTTP_POST, [this]() { handleReset(); });
  server.on("/status", HTTP_GET, [this]() { handleStatus(); });
  server.onNotFound([this]() { handleNotFound(); });
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
  html.reserve(14000);
  html += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>WOL M5 Setup</title><style>");
  html += F(":root{--bg:#eef2f0;--panel:#ffffff;--panel-2:#f7faf9;--text:#14211d;--muted:#5e7069;--line:#d6ddd9;--accent:#0f766e;--accent-2:#115e59;--danger:#b42318;--shadow:0 12px 30px rgba(16,24,20,.08);--radius:16px}");
  html += F("*{box-sizing:border-box}body{margin:0;font-family:Inter,Segoe UI,Arial,sans-serif;background:linear-gradient(180deg,#f7f9f8 0%,#eef2f0 100%);color:var(--text)}");
  html += F("main{max-width:1080px;margin:0 auto;padding:20px 16px 28px}.shell{display:grid;gap:16px}.hero{display:flex;justify-content:space-between;align-items:flex-start;gap:16px;padding:18px 18px 14px;background:rgba(255,255,255,.8);backdrop-filter:blur(8px);border:1px solid var(--line);border-radius:var(--radius);box-shadow:var(--shadow)}");
  html += F("h1{margin:0;font-size:28px;line-height:1.1;letter-spacing:0}.subtitle{margin:8px 0 0;color:var(--muted);font-size:14px;max-width:64ch}.badge-row{display:flex;flex-wrap:wrap;gap:8px;justify-content:flex-end}");
  html += F(".badge{display:inline-flex;align-items:center;gap:8px;padding:8px 10px;border:1px solid var(--line);border-radius:999px;background:#fff;color:var(--text);font-size:12px;font-weight:700;white-space:nowrap}.badge-dot{width:8px;height:8px;border-radius:999px;background:#8b9a95}.ok{background:#dcfce7}.ok .badge-dot{background:#16a34a}.warn{background:#fef3c7}.warn .badge-dot{background:#d97706}.muted{background:#f3f4f6}.muted .badge-dot{background:#6b7280}");
  html += F(".grid{display:grid;grid-template-columns:minmax(0,1.05fr) minmax(0,.95fr);gap:16px}.card{background:var(--panel);border:1px solid var(--line);border-radius:var(--radius);box-shadow:var(--shadow);overflow:hidden}.card-head{padding:16px 18px;border-bottom:1px solid var(--line);background:linear-gradient(180deg,#fff, #fbfcfb)}");
  html += F(".card-head h2{margin:0;font-size:18px}.card-head p{margin:6px 0 0;color:var(--muted);font-size:13px;line-height:1.4}.card-body{padding:18px}.status-grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:10px}");
  html += F(".status-tile{padding:12px;border:1px solid var(--line);border-radius:14px;background:var(--panel-2)}.status-label{color:var(--muted);font-size:12px}.status-value{margin-top:6px;font-size:15px;font-weight:800;word-break:break-word}.form-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:14px}.span-2{grid-column:1/-1}");
  html += F("label{display:block;font-weight:800;font-size:13px;color:var(--text)}.field{margin-top:14px}input[type=text],input[type=password],input[type=number]{width:100%;margin-top:8px;padding:13px 14px;border:1px solid var(--line);border-radius:12px;background:#fff;color:var(--text);font-size:15px;outline:none;transition:border-color .15s,box-shadow .15s}input:focus{border-color:#7ab8b0;box-shadow:0 0 0 4px rgba(15,118,110,.12)}");
  html += F(".hint{margin-top:8px;color:var(--muted);font-size:12px;line-height:1.4}.check{display:flex;align-items:center;gap:10px;padding:12px 14px;border:1px solid var(--line);border-radius:12px;background:#fff}.check input{width:18px;height:18px;margin:0}.actions{display:flex;gap:10px;flex-wrap:wrap;margin-top:18px}.actions form{margin:0}");
  html += F("button{appearance:none;border:0;border-radius:12px;padding:13px 16px;font-size:14px;font-weight:800;cursor:pointer}.primary{background:linear-gradient(180deg,var(--accent),var(--accent-2));color:#fff;box-shadow:0 10px 18px rgba(17,94,89,.22)}.secondary{background:#fff;color:var(--text);border:1px solid var(--line)}.danger{background:var(--danger);color:#fff}.footer-note{padding:14px 2px 0;color:var(--muted);font-size:12px;line-height:1.5}");
  html += F(".mini{display:grid;gap:8px}.mini-row{display:flex;justify-content:space-between;gap:12px;color:var(--muted);font-size:13px}.mini-row strong{color:var(--text)}@media(max-width:860px){.grid{grid-template-columns:1fr}.status-grid{grid-template-columns:1fr 1fr}}@media(max-width:620px){main{padding:12px}.hero{padding:16px;border-radius:14px}.badge-row{justify-content:flex-start}.status-grid,.form-grid{grid-template-columns:1fr}.span-2{grid-column:auto}.actions{flex-direction:column}.actions button{width:100%}}");
  html += F("</style></head><body><main><div class='shell'>");
  html += F("<section class='hero'><div><h1>WOL M5 Atom S3</h1><p class='subtitle'>Bridge Wake-on-LAN lokal dengan setup AP, koneksi router, dan sinkronisasi Supabase. Tampilan ini dibuat nyaman dipakai dari HP maupun layar kecil.</p></div><div class='badge-row' id='badges'>");
  html += F("<span class='badge ");
  html += (WiFi.status() == WL_CONNECTED ? "ok" : "warn");
  html += F("'><span class='badge-dot'></span><span id='wifi-badge'>");
  html += boolText(WiFi.status() == WL_CONNECTED, "WiFi Connected", "WiFi Setup");
  html += F("</span></span>");
  html += F("<span class='badge muted'><span class='badge-dot'></span><span id='ap-badge'>AP ");
  html += WiFi.softAPIP().toString();
  html += F("</span></span>");
  html += F("<span class='badge muted'><span class='badge-dot'></span><span id='ip-badge'>IP ");
  html += WiFi.localIP().toString();
  html += F("</span></span></div></section>");

  html += F("<section class='card'><div class='card-head'><h2>Connection Status</h2><p>Ringkasan koneksi aktif dan informasi jaringan yang dipakai sekarang.</p></div><div class='card-body'><div class='status-grid'>");
  html += F("<div class='status-tile'><div class='status-label'>Router SSID</div><div class='status-value'>");
  html += htmlEscape(cfg.wifiSsid);
  html += F("</div></div><div class='status-tile'><div class='status-label'>Local IP</div><div class='status-value' id='status-local-ip'>");
  html += WiFi.localIP().toString();
  html += F("</div></div><div class='status-tile'><div class='status-label'>AP IP</div><div class='status-value' id='status-ap-ip'>");
  html += WiFi.softAPIP().toString();
  html += F("</div></div></div></div></section>");

  html += F("<form id='config-form' method='post' action='/save'><section class='grid'>");
  html += F("<div class='card'><div class='card-head'><h2>WiFi Bridge</h2><p>Gunakan bagian ini untuk koneksi ke router lokal dan mode AP setup.</p></div><div class='card-body'><div class='form-grid'>");
  html += F("<div class='field span-2'><label for='deviceName'>Device Name</label><input id='deviceName' name='deviceName' type='text' autocomplete='off' value='");
  html += htmlEscape(cfg.deviceName);
  html += F("'><div class='hint'>Pakai nama pendek seperti <strong>wolm5</strong>. Nanti bisa dibuka lewat <strong>wolm5.local</strong>.</div></div>");
  html += F("<div class='field span-2'><label for='wifiSsid'>WiFi SSID</label><input id='wifiSsid' name='wifiSsid' type='text' autocomplete='off' value='");
  html += htmlEscape(cfg.wifiSsid);
  html += F("'><div class='hint'>Masukkan nama jaringan 2.4 GHz, termasuk jika SSID disembunyikan.</div></div>");
  html += F("<div class='field span-2'><label for='wifiPassword'>WiFi Password</label><input id='wifiPassword' name='wifiPassword' type='password' value='");
  html += htmlEscape(cfg.wifiPassword);
  html += F("'></div>");
  html += F("<div class='field'><label for='bridgeId'>Bridge ID</label><input id='bridgeId' name='bridgeId' type='text' value='");
  html += htmlEscape(cfg.bridgeId);
  html += F("'></div>");
  html += F("<div class='field'><label for='apPassword'>AP Password</label><input id='apPassword' name='apPassword' type='text' value='");
  html += htmlEscape(cfg.apPassword);
  html += F("'></div>");
  html += F("<div class='field span-2'><div class='check'><input id='wifiHidden' name='wifiHidden' type='checkbox'");
  html += checkedAttr(cfg.wifiHidden);
  html += F("><label for='wifiHidden'>Hidden SSID</label></div><div class='hint'>Aktifkan ini bila nama WiFi tidak disiarkan oleh router.</div></div>");
  html += F("</div></div></div>");

  html += F("<div class='card'><div class='card-head'><h2>Supabase & Timing</h2><p>Semua setting yang berhubungan dengan backend diletakkan di sini agar fleksibel nanti.</p></div><div class='card-body'><div class='form-grid'>");
  html += F("<div class='field span-2'><label for='supabaseUrl'>Supabase URL</label><input id='supabaseUrl' name='supabaseUrl' type='text' placeholder='https://xxxxx.supabase.co' value='");
  html += htmlEscape(cfg.supabaseUrl);
  html += F("'></div>");
  html += F("<div class='field span-2'><label for='supabaseKey'>Supabase API Key</label><input id='supabaseKey' name='supabaseKey' type='password' value='");
  html += htmlEscape(cfg.supabaseKey);
  html += F("'></div>");
  html += F("<div class='field'><label for='commandPollMs'>Command Poll ms</label><input id='commandPollMs' name='commandPollMs' type='number' min='1000' step='500' value='");
  html += String(cfg.commandPollMs);
  html += F("'></div>");
  html += F("<div class='field'><label for='statusPollMs'>Status Poll ms</label><input id='statusPollMs' name='statusPollMs' type='number' min='10000' step='1000' value='");
  html += String(cfg.statusPollMs);
  html += F("'></div>");
  html += F("<div class='field span-2'><label for='pingCount'>Ping Count</label><input id='pingCount' name='pingCount' type='number' min='1' max='5' value='");
  html += String(cfg.pingCount);
  html += F("'><div class='hint'>Angka kecil cukup untuk status ringan; kita menjaga jaringan tetap santai.</div></div>");
  html += F("</div></div></div></section><div class='actions'><button class='primary' type='submit'>Save & Restart</button></form><form method='post' action='/reset'><button class='secondary' type='submit'>Reset Config</button></form></div><div class='footer-note'>Status di atas akan menyala otomatis setelah WiFi aktif. Halaman ini tetap ringan supaya nyaman di mobile dan tetap enak dibaca dari browser kecil.</div></div></main>");

  html += F("<script>");
  html += F("(async()=>{const wifi=document.getElementById('wifi-badge');const localIp=document.getElementById('status-local-ip');const apIp=document.getElementById('status-ap-ip');const ipBadge=document.getElementById('ip-badge');const apBadge=document.getElementById('ap-badge');try{const res=await fetch('/status',{cache:'no-store'});if(!res.ok)return;const data=await res.json();if(localIp)localIp.textContent=data.localIp||'-';if(apIp)apIp.textContent=data.apIp||'-';if(ipBadge)ipBadge.textContent='IP '+(data.localIp||'-');if(apBadge)apBadge.textContent='AP '+(data.apIp||'-');if(wifi)wifi.textContent=data.wifi?'WiFi Connected':'WiFi Setup';}catch(e){}})();");
  html += F("setInterval(async()=>{try{const res=await fetch('/status',{cache:'no-store'});if(!res.ok)return;const data=await res.json();document.getElementById('status-local-ip').textContent=data.localIp||'-';document.getElementById('status-ap-ip').textContent=data.apIp||'-';document.getElementById('ip-badge').textContent='IP '+(data.localIp||'-');document.getElementById('ap-badge').textContent='AP '+(data.apIp||'-');document.getElementById('wifi-badge').textContent=data.wifi?'WiFi Connected':'WiFi Setup';}catch(e){}},5000);");
  html += F("</script></body></html>");
  server.send(200, "text/html", html);
}

void WebPortal::handleSave() {
  AppConfig next = *configRef;
  next.deviceName = server.arg("deviceName");
  next.wifiSsid = server.arg("wifiSsid");
  next.wifiPassword = server.arg("wifiPassword");
  next.supabaseUrl = server.arg("supabaseUrl");
  next.supabaseKey = server.arg("supabaseKey");
  next.bridgeId = server.arg("bridgeId");
  next.apPassword = server.arg("apPassword");
  next.wifiHidden = server.hasArg("wifiHidden");
  next.commandPollMs = server.arg("commandPollMs").toInt();
  next.statusPollMs = server.arg("statusPollMs").toInt();
  next.pingCount = static_cast<uint8_t>(server.arg("pingCount").toInt());

  if (next.commandPollMs < 1000) next.commandPollMs = 5000;
  if (next.statusPollMs < 10000) next.statusPollMs = 30000;
  if (next.pingCount < 1 || next.pingCount > 5) next.pingCount = 2;
  if (next.apPassword.length() < 8) next.apPassword = "wolm5setup";
  if (next.deviceName.isEmpty()) next.deviceName = "wolm5";

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

void WebPortal::handleFavicon() {
  server.send(204);
}

void WebPortal::handleNotFound() {
  server.send(404, "text/plain", "Not found");
}
