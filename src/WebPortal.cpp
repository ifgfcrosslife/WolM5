#include "WebPortal.h"
#include "LanDiscoveryManager.h"
#include "SupabaseClient.h"
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

String jsonEscape(String value) {
  value.replace("\\", "\\\\");
  value.replace("\"", "\\\"");
  value.replace("\n", " ");
  value.replace("\r", " ");
  return value;
}

String defaultBroadcastIp() {
  IPAddress local = WiFi.localIP();
  IPAddress mask = WiFi.subnetMask();
  const uint32_t network = static_cast<uint32_t>(local) & static_cast<uint32_t>(mask);
  const uint32_t broadcast = network | ~static_cast<uint32_t>(mask);
  return IPAddress((broadcast >> 24) & 0xFF, (broadcast >> 16) & 0xFF, (broadcast >> 8) & 0xFF, broadcast & 0xFF).toString();
}
}

WebPortal::WebPortal(ConfigStore &store, SupabaseClient &supabase, LanDiscoveryManager &lanDiscovery)
    : server(80), store(store), supabase(supabase), lanDiscovery(lanDiscovery) {}

void WebPortal::begin(AppConfig &config) {
  configRef = &config;

  server.on("/", HTTP_GET, [this]() { handleRoot(); });
  server.on("/favicon.ico", HTTP_GET, [this]() { handleFavicon(); });
  server.on("/save", HTTP_POST, [this]() { handleSave(); });
  server.on("/reset", HTTP_POST, [this]() { handleReset(); });
  server.on("/status", HTTP_GET, [this]() { handleStatus(); });
  server.on("/test-supabase", HTTP_POST, [this]() { handleSupabaseTest(); });
  server.on("/register-bridge", HTTP_POST, [this]() { handleRegisterBridge(); });
  server.on("/lan-scan/start", HTTP_POST, [this]() { handleLanScanStart(); });
  server.on("/lan-scan/status", HTTP_GET, [this]() { handleLanScanStatus(); });
  server.on("/lan-devices", HTTP_GET, [this]() { handleLanDevices(); });
  server.on("/register-device", HTTP_POST, [this]() { handleRegisterDevice(); });
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
  const bool bridgeRegistered = supabase.bridgeExists(cfg.bridgeId);
  const char *registerLabel = bridgeRegistered ? "Register Ulang" : "Register Device";
  const char *registerHint = bridgeRegistered
                                 ? "Bridge ini sudah terdaftar. Klik Register Ulang untuk menimpa data dengan setting terbaru."
                                 : "Daftarkan bridge ini ke tabel wol_bridges dari portal M5.";
  String html;
  html.reserve(22000);
  html += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>WOL M5 Setup</title><style>");
  html += F(":root{--bg:#eef2f0;--panel:#ffffff;--panel-2:#f7faf9;--text:#14211d;--muted:#5e7069;--line:#d6ddd9;--accent:#0f766e;--accent-2:#115e59;--danger:#b42318;--shadow:0 12px 30px rgba(16,24,20,.08);--radius:14px}");
  html += F("*{box-sizing:border-box}body{margin:0;font-family:Inter,Segoe UI,Arial,sans-serif;background:linear-gradient(180deg,#f7f9f8 0%,#eef2f0 100%);color:var(--text)}");
  html += F("main{max-width:1160px;margin:0 auto;padding:18px 14px 28px}.shell{display:grid;gap:14px}.hero{display:flex;justify-content:space-between;align-items:flex-start;gap:16px;padding:16px 16px 14px;background:rgba(255,255,255,.8);backdrop-filter:blur(8px);border:1px solid var(--line);border-radius:var(--radius);box-shadow:var(--shadow)}");
  html += F("h1{margin:0;font-size:26px;line-height:1.1;letter-spacing:0}.subtitle{margin:8px 0 0;color:var(--muted);font-size:13px;max-width:64ch}.badge-row{display:flex;flex-wrap:wrap;gap:8px;justify-content:flex-end}");
  html += F(".badge{display:inline-flex;align-items:center;gap:8px;padding:8px 10px;border:1px solid var(--line);border-radius:999px;background:#fff;color:var(--text);font-size:12px;font-weight:700;white-space:nowrap}.badge-dot{width:8px;height:8px;border-radius:999px;background:#8b9a95}.ok{background:#dcfce7}.ok .badge-dot{background:#16a34a}.warn{background:#fef3c7}.warn .badge-dot{background:#d97706}.muted{background:#f3f4f6}.muted .badge-dot{background:#6b7280}");
  html += F(".tabs{display:flex;gap:8px;flex-wrap:wrap;background:rgba(255,255,255,.75);border:1px solid var(--line);border-radius:var(--radius);padding:10px;box-shadow:var(--shadow)}.tab-btn{border:1px solid var(--line);background:#fff;color:var(--muted);font-size:13px;font-weight:800;padding:10px 14px;border-radius:999px;cursor:pointer}.tab-btn.active{background:linear-gradient(180deg,var(--accent),var(--accent-2));color:#fff;border-color:transparent}.tab-panel{display:none}.tab-panel.active{display:block}.stack{display:grid;gap:12px}.grid{display:grid;grid-template-columns:minmax(0,1.05fr) minmax(0,.95fr);gap:14px}.card{background:var(--panel);border:1px solid var(--line);border-radius:var(--radius);box-shadow:var(--shadow);overflow:hidden}.card-head{padding:14px 16px;border-bottom:1px solid var(--line);background:linear-gradient(180deg,#fff,#fbfcfb)}");
  html += F(".card-head h2{margin:0;font-size:18px}.card-head p{margin:6px 0 0;color:var(--muted);font-size:13px;line-height:1.4}.card-body{padding:16px}.status-grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:10px}.status-tile{padding:12px;border:1px solid var(--line);border-radius:12px;background:var(--panel-2)}.status-label{color:var(--muted);font-size:12px}.status-value{margin-top:6px;font-size:15px;font-weight:800;word-break:break-word}.form-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:14px}.span-2{grid-column:1/-1}");
  html += F("label{display:block;font-weight:800;font-size:13px;color:var(--text)}.field{margin-top:14px}input[type=text],input[type=password],input[type=number]{width:100%;margin-top:8px;padding:12px 13px;border:1px solid var(--line);border-radius:12px;background:#fff;color:var(--text);font-size:15px;outline:none;transition:border-color .15s,box-shadow .15s}input:focus{border-color:#7ab8b0;box-shadow:0 0 0 4px rgba(15,118,110,.12)}");
  html += F(".hint{margin-top:8px;color:var(--muted);font-size:12px;line-height:1.4}.check{display:flex;align-items:center;gap:10px;padding:12px 14px;border:1px solid var(--line);border-radius:12px;background:#fff}.check input{width:18px;height:18px;margin:0}.actions{display:flex;gap:10px;flex-wrap:wrap;margin-top:18px}.actions form{margin:0}.inline-actions{display:flex;gap:10px;flex-wrap:wrap;margin-top:12px;align-items:flex-start}.inline-actions .stack{display:grid;gap:10px;flex:1 1 320px}.inline-actions .stack > div{margin:0}");
  html += F("button{appearance:none;border:0;border-radius:12px;padding:12px 15px;font-size:14px;font-weight:800;cursor:pointer}.primary{background:linear-gradient(180deg,var(--accent),var(--accent-2));color:#fff;box-shadow:0 10px 18px rgba(17,94,89,.22)}.secondary{background:#fff;color:var(--text);border:1px solid var(--line)}.danger{background:var(--danger);color:#fff}.footer-note{padding:14px 2px 0;color:var(--muted);font-size:12px;line-height:1.5}.test-result{padding:12px 14px;border-radius:12px;border:1px solid var(--line);background:#f8faf9;font-size:13px;line-height:1.5;color:var(--text)}.test-result.ok{border-color:#86efac;background:#f0fdf4}.test-result.fail{border-color:#fca5a5;background:#fef2f2}.table{width:100%;border-collapse:collapse}.table th,.table td{padding:10px 8px;border-bottom:1px solid var(--line);text-align:left;font-size:13px;vertical-align:top}.table th{color:var(--muted);font-size:12px;text-transform:uppercase;letter-spacing:.02em}.chip{display:inline-flex;align-items:center;padding:5px 8px;border-radius:999px;background:#eef7f5;border:1px solid #cfe8e2;font-size:12px;font-weight:800;color:#0f766e}.muted-text{color:var(--muted)}.device-tools{display:flex;gap:8px;flex-wrap:wrap;align-items:center}.device-tools input{max-width:220px}.scan-box{display:grid;gap:10px}.scan-meta{display:flex;gap:8px;flex-wrap:wrap;align-items:center}.empty{padding:14px;border:1px dashed var(--line);border-radius:12px;color:var(--muted);background:#fafcfc}");
  html += F("@media(max-width:860px){.grid,.status-grid{grid-template-columns:1fr 1fr}.grid{grid-template-columns:1fr}}@media(max-width:620px){main{padding:12px}.hero{padding:14px;border-radius:14px}.badge-row{justify-content:flex-start}.status-grid,.form-grid{grid-template-columns:1fr}.span-2{grid-column:auto}.actions{flex-direction:column}.actions button{width:100%}.tabs{gap:6px}.tab-btn{padding:9px 12px}}");
  html += F("</style></head><body><main><div class='shell'>");
  html += F("<section class='hero'><div><h1>WOL M5 Atom S3</h1><p class='subtitle'>Bridge Wake-on-LAN lokal dengan setup AP, koneksi router, scan host LAN, dan sinkronisasi Supabase.</p></div><div class='badge-row' id='badges'>");
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

  html += F("<nav class='tabs' id='tabs'>");
  html += F("<button type='button' class='tab-btn active' data-tab='overview'>Overview</button>");
  html += F("<button type='button' class='tab-btn' data-tab='wifi'>WiFi</button>");
  html += F("<button type='button' class='tab-btn' data-tab='supabase'>Supabase</button>");
  html += F("<button type='button' class='tab-btn' data-tab='devices'>Devices</button>");
  html += F("<button type='button' class='tab-btn' data-tab='advanced'>Advanced</button>");
  html += F("</nav>");
  html += F("<form id='config-form' method='post' action='/save'>");

  html += F("<section class='tab-panel active' data-tab='overview'><div class='grid'><div class='card'><div class='card-head'><h2>Status</h2><p>Ringkasan koneksi aktif dan kondisi bridge sekarang.</p></div><div class='card-body'><div class='status-grid'>");
  html += F("<div class='status-tile'><div class='status-label'>Router SSID</div><div class='status-value'>");
  html += htmlEscape(cfg.wifiSsid.length() > 0 ? cfg.wifiSsid : String("-"));
  html += F("</div></div><div class='status-tile'><div class='status-label'>Local IP</div><div class='status-value' id='status-local-ip'>");
  html += WiFi.localIP().toString();
  html += F("</div></div><div class='status-tile'><div class='status-label'>AP IP</div><div class='status-value' id='status-ap-ip'>");
  html += WiFi.softAPIP().toString();
  html += F("</div></div></div></div></div><div class='card'><div class='card-head'><h2>Quick Actions</h2><p>Registrasi bridge dan cek koneksi Supabase dari M5.</p></div><div class='card-body'><div class='inline-actions'><div class='stack'><button class='secondary' type='button' id='test-supabase-btn'>Test Supabase</button><div id='test-supabase-result' class='test-result'>Tekan tombol untuk cek koneksi Supabase dari M5.</div></div><div class='stack'><button class='primary' type='button' id='register-bridge-btn'>");
  html += registerLabel;
  html += F("</button><div id='register-bridge-result' class='test-result'>");
  html += registerHint;
  html += F("</div></div></div></div></div></div></section>");

  html += F("<section class='tab-panel' data-tab='wifi'><div class='card'><div class='card-head'><h2>WiFi Bridge</h2><p>Koneksi ke router lokal dan AP setup.</p></div><div class='card-body'><div class='form-grid'>");
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
  html += F("</div></div></div></section>");

  html += F("<section class='tab-panel' data-tab='supabase'><div class='card'><div class='card-head'><h2>Supabase & Timing</h2><p>Backend dan polling ada di sini.</p></div><div class='card-body'><div class='form-grid'>");
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
  html += F("</div></div></div></section>");

  html += F("<section class='tab-panel' data-tab='devices'><div class='grid'><div class='card'><div class='card-head'><h2>LAN Scan</h2><p>Scan host aktif di subnet lokal dan pilih untuk register ke Supabase.</p></div><div class='card-body'><div class='scan-box'><div class='scan-meta'><button class='primary' type='button' id='scan-start-btn'>Scan LAN</button><span class='chip' id='scan-progress'>Siap scan</span><span class='muted-text' id='scan-subnet'></span></div><div id='scan-status' class='muted-text'>Tekan scan untuk mencari komputer yang sedang aktif.</div><div id='scan-results' class='stack'></div></div></div></div><div class='card'><div class='card-head'><h2>Registered PCs</h2><p>Daftar PC yang sudah disimpan di Supabase untuk bridge ini.</p></div><div class='card-body'><div id='registered-devices' class='stack'></div></div></div></div></section>");

  html += F("<section class='tab-panel' data-tab='advanced'><div class='card'><div class='card-head'><h2>Advanced</h2><p>Kontrol tambahan untuk penyimpanan dan reset.</p></div><div class='card-body'><div class='actions'><button class='primary' type='submit'>Save & Restart</button></form><form method='post' action='/reset'><button class='secondary' type='submit'>Reset Config</button></form></div><div class='footer-note'>Status di atas akan menyala otomatis setelah WiFi aktif. Halaman ini dibuat lebih ringan dan dibagi tab supaya layar kecil tidak penuh.</div></div></div></section>");

  html += F("<script>");
  html += F("const tabs=[...document.querySelectorAll('.tab-btn')];const panels=[...document.querySelectorAll('.tab-panel')];const setTab=(name)=>{tabs.forEach(b=>b.classList.toggle('active',b.dataset.tab===name));panels.forEach(p=>p.classList.toggle('active',p.dataset.tab===name));try{localStorage.setItem('wolm5-tab',name);}catch(e){}};tabs.forEach(b=>b.addEventListener('click',()=>setTab(b.dataset.tab)));setTab((()=>{try{return localStorage.getItem('wolm5-tab')||'overview';}catch(e){return 'overview';}})());");
  html += F("(async()=>{const wifi=document.getElementById('wifi-badge');const localIp=document.getElementById('status-local-ip');const apIp=document.getElementById('status-ap-ip');const ipBadge=document.getElementById('ip-badge');const apBadge=document.getElementById('ap-badge');try{const res=await fetch('/status',{cache:'no-store'});if(!res.ok)return;const data=await res.json();if(localIp)localIp.textContent=data.localIp||'-';if(apIp)apIp.textContent=data.apIp||'-';if(ipBadge)ipBadge.textContent='IP '+(data.localIp||'-');if(apBadge)apBadge.textContent='AP '+(data.apIp||'-');if(wifi)wifi.textContent=data.wifi?'WiFi Connected':'WiFi Setup';}catch(e){}})();");
  html += F("setInterval(async()=>{try{const res=await fetch('/status',{cache:'no-store'});if(!res.ok)return;const data=await res.json();document.getElementById('status-local-ip').textContent=data.localIp||'-';document.getElementById('status-ap-ip').textContent=data.apIp||'-';document.getElementById('ip-badge').textContent='IP '+(data.localIp||'-');document.getElementById('ap-badge').textContent='AP '+(data.apIp||'-');document.getElementById('wifi-badge').textContent=data.wifi?'WiFi Connected':'WiFi Setup';}catch(e){}},5000);");
  html += F("const testBtn=document.getElementById('test-supabase-btn');const testBox=document.getElementById('test-supabase-result');if(testBtn&&testBox){testBtn.addEventListener('click',async()=>{testBtn.disabled=true;const oldText=testBtn.textContent;testBtn.textContent='Checking...';testBox.className='test-result';testBox.textContent='Mengecek koneksi ke Supabase...';try{const res=await fetch('/test-supabase',{method:'POST',cache:'no-store'});const data=await res.json();if(data.ok){testBox.className='test-result ok';testBox.textContent=data.message||'Koneksi Supabase OK';}else{testBox.className='test-result fail';testBox.textContent=data.message||'Koneksi Supabase gagal';}}catch(e){testBox.className='test-result fail';testBox.textContent='Gagal menghubungi endpoint test di device.';}testBtn.disabled=false;testBtn.textContent=oldText;});}");
  html += F("const registerBtn=document.getElementById('register-bridge-btn');const registerBox=document.getElementById('register-bridge-result');if(registerBtn&&registerBox){registerBtn.addEventListener('click',async()=>{registerBtn.disabled=true;const oldText=registerBtn.textContent;registerBtn.textContent='Registering...';registerBox.className='test-result';registerBox.textContent='Mendaftarkan bridge ke Supabase...';try{const res=await fetch('/register-bridge',{method:'POST',cache:'no-store'});const data=await res.json();if(data.ok){registerBox.className='test-result ok';registerBox.textContent=data.message||'Bridge terdaftar di Supabase';registerBtn.textContent='Register Ulang';}else{registerBox.className='test-result fail';registerBox.textContent=data.message||'Bridge gagal didaftarkan';registerBtn.textContent=oldText;}}catch(e){registerBox.className='test-result fail';registerBox.textContent='Gagal menghubungi endpoint register di device.';registerBtn.textContent=oldText;}registerBtn.disabled=false;});}");
  html += F("const scanBtn=document.getElementById('scan-start-btn');const scanProgress=document.getElementById('scan-progress');const scanStatus=document.getElementById('scan-status');const scanSubnet=document.getElementById('scan-subnet');const scanResults=document.getElementById('scan-results');const registeredBox=document.getElementById('registered-devices');const esc=s=>String(s||'').replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/\"/g,'&quot;').replace(/'/g,'&#39;');async function renderRegistered(){if(!registeredBox)return;try{const res=await fetch('/lan-devices',{cache:'no-store'});if(!res.ok)throw new Error('bad');const data=await res.json();const rows=(data&&Array.isArray(data))?data:[];if(rows.length===0){registeredBox.innerHTML='<div class=\"empty\">Belum ada PC yang tersimpan di Supabase.</div>';return;}registeredBox.innerHTML='<table class=\"table\"><thead><tr><th>Name</th><th>IP</th><th>MAC</th><th>Status</th></tr></thead><tbody>'+rows.map(r=>'<tr><td>'+esc(r.name||'')+'</td><td>'+esc(r.ip_address||'')+'</td><td>'+esc(r.mac_address||'')+'</td><td>'+(r.enabled?'<span class=\"chip\">enabled</span>':'<span class=\"muted-text\">disabled</span>')+'</td></tr>').join('')+'</tbody></table>'; }catch(e){registeredBox.innerHTML='<div class=\"empty\">Gagal memuat daftar PC dari Supabase.</div>';}}async function refreshScan(){if(!scanResults)return;try{const res=await fetch('/lan-scan/status',{cache:'no-store'});if(!res.ok)throw new Error('bad');const data=await res.json();if(scanProgress)scanProgress.textContent=data.scanning?('Scan '+(data.progress||0)+'%'):'Siap scan';if(scanStatus)scanStatus.textContent=data.scanning?'Mencari host aktif di LAN...':'Scan selesai.';if(scanSubnet)scanSubnet.textContent=data.subnet||'';const rows=(data.results||[]);if(rows.length===0){scanResults.innerHTML='<div class=\"empty\">Belum ada host ditemukan.</div>';return;}scanResults.innerHTML=rows.map((r,i)=>'<div class=\"card\" style=\"box-shadow:none\"><div class=\"card-body\"><div class=\"device-tools\"><span class=\"chip\">'+esc(r.ipAddress)+'</span><span class=\"muted-text\">'+esc(r.macAddress||'MAC belum kebaca')+'</span><span class=\"muted-text\">'+esc(r.latencyMs>=0?r.latencyMs+'ms':'-')+'</span></div><div class=\"field\"><label>Nama perangkat</label><input type=\"text\" data-name=\"'+i+'\" value=\"'+esc(r.displayName||r.ipAddress)+'\"></div><div class=\"device-tools\" style=\"margin-top:10px\"><button type=\"button\" class=\"secondary\" data-register=\"'+i+'\">Daftar ke Supabase</button></div></div></div>').join('');}catch(e){if(scanResults)scanResults.innerHTML='<div class=\"empty\">Gagal memuat hasil scan.</div>';}}if(scanBtn){scanBtn.addEventListener('click',async()=>{scanBtn.disabled=true;scanBtn.textContent='Scanning...';try{const res=await fetch('/lan-scan/start',{method:'POST',cache:'no-store'});const data=await res.json();if(!data.ok){if(scanStatus)scanStatus.textContent=data.message||'Scan gagal dimulai';}else{if(scanStatus)scanStatus.textContent='Scan dimulai...';setTab('devices');}}catch(e){if(scanStatus)scanStatus.textContent='Gagal memulai scan';}scanBtn.disabled=false;scanBtn.textContent='Scan LAN';});}if(scanResults){scanResults.addEventListener('click',async(ev)=>{const btn=ev.target.closest('[data-register]');if(!btn)return;const idx=parseInt(btn.getAttribute('data-register'),10);const row=btn.closest('.card');const nameInput=row?row.querySelector('[data-name=\"'+idx+'\"]'):null;const name=nameInput?nameInput.value:'';const rows=(await (await fetch('/lan-scan/status',{cache:'no-store'})).json()).results||[];const host=rows[idx];if(!host)return;btn.disabled=true;btn.textContent='Saving...';try{const form=new URLSearchParams();form.set('name',name||host.ipAddress);form.set('macAddress',host.macAddress||'');form.set('ipAddress',host.ipAddress||'');form.set('broadcastIp',host.broadcastIp||'');form.set('port','9');const res=await fetch('/register-device',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:form.toString()});const data=await res.json();if(data.ok){await renderRegistered();scanStatus.textContent=data.message||'Device tersimpan';}else{scanStatus.textContent=data.message||'Gagal register';}}catch(e){scanStatus.textContent='Gagal register device';}btn.disabled=false;btn.textContent='Daftar ke Supabase';});}renderRegistered();refreshScan();setInterval(refreshScan,2000);setInterval(renderRegistered,10000);");
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

  String html;
  html.reserve(2200);
  html += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<meta http-equiv='refresh' content='15;url=/'><title>Saving...</title><style>");
  html += F("body{margin:0;min-height:100vh;display:grid;place-items:center;background:#eef2f0;color:#14211d;font-family:Inter,Segoe UI,Arial,sans-serif}main{max-width:420px;padding:24px;text-align:center}h1{margin:0 0 8px;font-size:28px;line-height:1.1}p{margin:0;color:#5e7069;font-size:14px;line-height:1.5}.chip{display:inline-flex;align-items:center;gap:8px;padding:8px 12px;border-radius:999px;background:#fff;border:1px solid #d6ddd9;margin-bottom:16px;font-size:12px;font-weight:700}.dot{width:8px;height:8px;border-radius:999px;background:#0f766e}.bar{height:10px;border-radius:999px;background:#dbe5e1;overflow:hidden;margin-top:20px}.bar::before{content:'';display:block;width:35%;height:100%;background:linear-gradient(90deg,#0f766e,#115e59);border-radius:999px;animation:move 1.1s ease-in-out infinite}@keyframes move{0%{transform:translateX(-110%)}100%{transform:translateX(320%)}}");
  html += F("</style></head><body><main><div class='chip'><span class='dot'></span><span>Saved, restarting bridge</span></div><h1>Config tersimpan</h1><p>Halaman ini akan balik ke portal utama otomatis begitu device siap lagi.</p><div class='bar'></div></main><script>");
  html += F("try{history.replaceState(null,'','/');}catch(e){}");
  html += F("(async()=>{const start=Date.now();const ping=async()=>{try{const res=await fetch('/status',{cache:'no-store'});if(res.ok){location.reload();return;}}catch(e){}if(Date.now()-start<30000){setTimeout(ping,1000);}else{location.href='/';}};setTimeout(ping,1000);})();");
  html += F("</script></body></html>");
  server.send(200, "text/html", html);
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

void WebPortal::handleLanScanStart() {
  JsonDocument doc;
  if (WiFi.status() != WL_CONNECTED) {
    doc["ok"] = false;
    doc["message"] = "WiFi harus tersambung ke router dulu untuk scan LAN.";
  } else {
    lanDiscovery.startScan();
    doc["ok"] = true;
    doc["message"] = "Scan LAN dimulai.";
    doc["subnet"] = lanDiscovery.subnetLabel();
  }

  String body;
  serializeJson(doc, body);
  server.send(200, "application/json", body);
}

void WebPortal::handleLanScanStatus() {
  JsonDocument doc;
  doc["scanning"] = lanDiscovery.isScanning();
  doc["progress"] = lanDiscovery.progressPercent();
  doc["subnet"] = lanDiscovery.subnetLabel();

  JsonArray results = doc["results"].to<JsonArray>();
  for (size_t i = 0; i < lanDiscovery.resultCount(); i++) {
    const LanHostRecord &host = lanDiscovery.result(i);
    JsonObject item = results.add<JsonObject>();
    item["ipAddress"] = host.ipAddress;
    item["macAddress"] = host.macAddress;
    item["displayName"] = host.displayName;
    item["latencyMs"] = host.latencyMs;
    item["online"] = host.online;
  }

  String body;
  serializeJson(doc, body);
  server.send(200, "application/json", body);
}

void WebPortal::handleLanDevices() {
  JsonDocument doc;
  if (!supabase.fetchDevices(doc)) {
    server.send(200, "application/json", "[]");
    return;
  }

  String body;
  serializeJson(doc, body);
  server.send(200, "application/json", body);
}

void WebPortal::handleRegisterDevice() {
  JsonDocument doc;
  AppConfig &cfg = *configRef;

  const String name = server.arg("name");
  const String macAddress = server.arg("macAddress");
  const String ipAddress = server.arg("ipAddress");
  const String broadcastIp = server.arg("broadcastIp").length() > 0 ? server.arg("broadcastIp") : defaultBroadcastIp();
  const uint16_t port = static_cast<uint16_t>(server.arg("port").toInt() > 0 ? server.arg("port").toInt() : 9);

  if (!supabase.isConfigured()) {
    doc["ok"] = false;
    doc["message"] = "Supabase belum siap.";
  } else if (cfg.bridgeId.isEmpty()) {
    doc["ok"] = false;
    doc["message"] = "Bridge ID belum diisi.";
  } else if (macAddress.isEmpty()) {
    doc["ok"] = false;
    doc["message"] = "MAC address belum ada, jadi PC belum bisa didaftarkan.";
  } else {
    const bool ok = supabase.upsertDeviceByMac(cfg.bridgeId,
                                               name.length() > 0 ? name : ipAddress,
                                               macAddress,
                                               ipAddress,
                                               broadcastIp,
                                               port);
    doc["ok"] = ok;
    doc["message"] = ok ? "PC tersimpan ke Supabase." : "Gagal menyimpan PC ke Supabase.";
  }

  String body;
  serializeJson(doc, body);
  server.send(200, "application/json", body);
}

void WebPortal::handleSupabaseTest() {
  JsonDocument doc;
  AppConfig &cfg = *configRef;

  if (cfg.supabaseUrl.isEmpty() || cfg.supabaseKey.isEmpty()) {
    doc["ok"] = false;
    doc["message"] = "Supabase URL atau key belum diisi.";
    String body;
    serializeJson(doc, body);
    server.send(200, "application/json", body);
    return;
  }

  String baseUrl = cfg.supabaseUrl;
  baseUrl.trim();
  if (baseUrl.endsWith("/")) baseUrl.remove(baseUrl.length() - 1);
  const String url = baseUrl + "/rest/v1/wol_bridges?select=id&limit=1";

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  if (!http.begin(client, url)) {
    doc["ok"] = false;
    doc["message"] = "Gagal membuka koneksi HTTP ke Supabase.";
    String body;
    serializeJson(doc, body);
    server.send(200, "application/json", body);
    return;
  }

  http.addHeader("apikey", cfg.supabaseKey);
  http.addHeader("Authorization", "Bearer " + cfg.supabaseKey);
  const int code = http.GET();
  const String response = http.getString();
  http.end();

  doc["httpCode"] = code;
  if (code >= 200 && code < 300) {
    doc["ok"] = true;
    doc["message"] = "Koneksi Supabase OK. REST endpoint merespons normal.";
  } else {
    doc["ok"] = false;
    doc["message"] = "Supabase merespons error " + String(code) + ". Cek URL, key, atau schema tabel wol_bridges.";
    if (response.length() > 0) {
      doc["response"] = response;
    }
  }

  String body;
  serializeJson(doc, body);
  server.send(200, "application/json", body);
}

void WebPortal::handleRegisterBridge() {
  JsonDocument doc;
  AppConfig &cfg = *configRef;

  if (!supabase.isConfigured()) {
    doc["ok"] = false;
    doc["message"] = "Supabase belum dikonfigurasi penuh. Isi URL, key, dan Bridge ID dulu.";
    String body;
    serializeJson(doc, body);
    server.send(200, "application/json", body);
    return;
  }

  const bool ok = supabase.upsertBridge(cfg.bridgeId,
                                        cfg.deviceName.length() > 0 ? cfg.deviceName : String("WOL Bridge"),
                                        WiFi.localIP().toString(),
                                        WiFi.softAPIP().toString(),
                                        WiFi.status() == WL_CONNECTED);

  doc["ok"] = ok;
  doc["message"] = ok ? "Bridge berhasil didaftarkan ke tabel wol_bridges." : "Gagal mendaftarkan bridge ke Supabase.";
  doc["bridgeId"] = cfg.bridgeId;

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
