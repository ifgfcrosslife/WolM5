const storageKey = "wolm5-frontend-settings";

const state = {
  settings: {
    supabaseUrl: "",
    supabaseKey: "",
    bridgeId: "m5-atom-s3",
    bridgeSecret: "",
    bridgeName: "Atom S3 Bridge",
    defaultBroadcast: "",
    defaultPort: 9
  },
  bridges: [],
  devices: [],
  commands: [],
  statuses: []
};

const $ = (id) => document.getElementById(id);

function loadSettings() {
  try {
    const raw = localStorage.getItem(storageKey);
    if (!raw) return;
    const saved = JSON.parse(raw);
    state.settings = { ...state.settings, ...saved };
  } catch (_) {}
}

function saveSettingsToStorage() {
  localStorage.setItem(storageKey, JSON.stringify(state.settings));
}

function setStatus(message, tone = "") {
  const box = $("action-result");
  if (!box) return;
  box.textContent = message;
  box.className = `notice${tone ? ` ${tone}` : ""}`;
}

function encodeQuery(params) {
  const search = new URLSearchParams();
  Object.entries(params).forEach(([key, value]) => {
    if (value !== undefined && value !== null && String(value).length > 0) {
      search.set(key, value);
    }
  });
  return search.toString();
}

function normalizeUrl(url) {
  return String(url || "").trim().replace(/\/$/, "");
}

function apiBase() {
  return normalizeUrl(state.settings.supabaseUrl);
}

function authHeaders(preferReturn = false) {
  const headers = {
    apikey: state.settings.supabaseKey,
    Authorization: `Bearer ${state.settings.supabaseKey}`,
    "x-bridge-secret": state.settings.bridgeSecret,
    "Content-Type": "application/json"
  };
  if (preferReturn) {
    headers.Prefer = "return=representation";
  }
  return headers;
}

async function request(path, options = {}) {
  const base = apiBase();
  if (!base) throw new Error("Supabase URL belum diisi.");
  if (!state.settings.supabaseKey) throw new Error("Supabase anon key belum diisi.");
  if (!state.settings.bridgeSecret) throw new Error("Bridge secret belum diisi.");
  const url = `${base}/rest/v1${path}`;
  const res = await fetch(url, {
    ...options,
    headers: {
      ...authHeaders(options.preferReturn),
      ...(options.headers || {})
    }
  });
  const text = await res.text();
  let data = null;
  try {
    data = text ? JSON.parse(text) : null;
  } catch (_) {
    data = text;
  }
  if (!res.ok) {
    const message = (data && data.message) || data || `HTTP ${res.status}`;
    throw new Error(message);
  }
  return data;
}

async function fetchBridges() {
  const query = encodeQuery({
    select: "id,name,local_ip,ap_ip,wifi_connected,last_seen_at,updated_at",
    order: "updated_at.desc"
  });
  state.bridges = await request(`/wol_bridges?${query}`, { method: "GET" });
}

async function fetchDevices() {
  const query = encodeQuery({
    select: "id,bridge_id,name,mac_address,ip_address,broadcast_ip,port,enabled,created_at,updated_at",
    bridge_id: `eq.${state.settings.bridgeId}`,
    order: "updated_at.desc"
  });
  const devices = await request(`/wol_devices?${query}`, { method: "GET" });
  state.devices = Array.isArray(devices) ? devices : [];
}

async function fetchCommands() {
  const query = encodeQuery({
    select: "id,bridge_id,device_id,mac_address,broadcast_ip,port,status,message,created_at,processed_at",
    bridge_id: `eq.${state.settings.bridgeId}`,
    order: "created_at.desc",
    limit: "20"
  });
  const commands = await request(`/wol_commands?${query}`, { method: "GET" });
  state.commands = Array.isArray(commands) ? commands : [];
}

async function fetchStatuses() {
  const query = encodeQuery({
    select: "device_id,bridge_id,online,ip_address,latency_ms,checked_at",
    bridge_id: `eq.${state.settings.bridgeId}`,
    order: "checked_at.desc"
  });
  const rows = await request(`/wol_device_status?${query}`, { method: "GET" });
  state.statuses = Array.isArray(rows) ? rows : [];
}

function bridgeStatusText() {
  const row = state.bridges.find((item) => item.id === state.settings.bridgeId);
  if (!row) return "-";
  return row.name || row.id;
}

function renderOverview() {
  const box = $("overview-box");
  if (!box) return;
  const bridge = state.bridges.find((item) => item.id === state.settings.bridgeId);
  const deviceCount = state.devices.length;
  const commandCount = state.commands.length;
  const onlineCount = state.statuses.filter((item) => item.online).length;

  const deviceCountEl = $("device-count");
  const commandCountEl = $("command-count");
  if (deviceCountEl) deviceCountEl.textContent = String(deviceCount);
  if (commandCountEl) commandCountEl.textContent = String(commandCount);

  box.innerHTML = `
    <div class="stack">
      <div class="chip">Bridge ID: ${escapeHtml(state.settings.bridgeId || "-")}</div>
      <div class="notice">
        <strong>${escapeHtml(bridge ? bridge.name || bridge.id : "Bridge belum terdaftar")}</strong><br>
        <span class="muted-text">${escapeHtml(bridge ? `Last seen ${bridge.last_seen_at || "-"}` : "Klik Register / Update Bridge untuk membuat row bridge.")}</span>
      </div>
      <div class="status-strip" style="grid-template-columns: repeat(3, minmax(0, 1fr)); margin-top: 0;">
        <div class="status-card"><span class="label">Devices</span><strong>${deviceCount}</strong></div>
        <div class="status-card"><span class="label">Commands</span><strong>${commandCount}</strong></div>
        <div class="status-card"><span class="label">Online</span><strong>${onlineCount}</strong></div>
      </div>
    </div>
  `;
}

function renderTable(containerId, rows, columns, emptyText = "Belum ada data.") {
  const box = $(containerId);
  if (!box) return;
  if (!rows || rows.length === 0) {
    box.innerHTML = `<div class="empty">${escapeHtml(emptyText)}</div>`;
    return;
  }

  const head = columns.map((column) => `<th>${escapeHtml(column.label)}</th>`).join("");
  const body = rows.map((row, index) => {
    const cells = columns.map((column) => {
      if (column.render) return `<td>${column.render(row, index)}</td>`;
      const value = row[column.key];
      return `<td>${escapeHtml(value ?? "")}</td>`;
    }).join("");
    return `<tr>${cells}</tr>`;
  }).join("");

  box.innerHTML = `<table class="data-table"><thead><tr>${head}</tr></thead><tbody>${body}</tbody></table>`;
}

function renderBridges() {
  renderTable("bridges-table", state.bridges, [
    { label: "ID", key: "id" },
    { label: "Name", key: "name" },
    { label: "Local IP", key: "local_ip" },
    { label: "AP IP", key: "ap_ip" },
    { label: "WiFi", render: (row) => row.wifi_connected ? `<span class="chip">connected</span>` : `<span class="muted-text">setup</span>` },
    { label: "Updated", key: "updated_at" }
  ], "Belum ada bridge di Supabase.");
}

function renderDevices() {
  const statusMap = new Map();
  state.statuses.forEach((item) => {
    if (!statusMap.has(item.device_id)) statusMap.set(item.device_id, item);
  });

  renderTable("devices-table", state.devices, [
    { label: "Name", key: "name" },
    { label: "MAC", key: "mac_address" },
    { label: "IP", key: "ip_address" },
    { label: "Broadcast", key: "broadcast_ip" },
    { label: "Port", key: "port" },
    {
      label: "Status",
      render: (row) => {
        const status = statusMap.get(row.id);
        if (!status) return `<span class="muted-text">-</span>`;
        return status.online
          ? `<span class="chip">online ${escapeHtml(status.latency_ms ?? "")}ms</span>`
          : `<span class="muted-text">offline</span>`;
      }
    },
    {
      label: "Actions",
      render: (row) => `
        <div class="row-actions">
          <button class="ghost-btn" data-edit-device="${escapeHtml(row.id)}" type="button">Edit</button>
          <button class="secondary-btn" data-send-wol="${escapeHtml(row.id)}" type="button">WOL</button>
          <button class="ghost-btn" data-disable-device="${escapeHtml(row.id)}" type="button">${row.enabled ? "Disable" : "Enable"}</button>
        </div>
      `
    }
  ], "Belum ada device.");
}

function renderCommands() {
  renderTable("commands-table", state.commands, [
    { label: "Created", key: "created_at" },
    { label: "Status", render: (row) => `<span class="chip">${escapeHtml(row.status || "-")}</span>` },
    { label: "MAC", key: "mac_address" },
    { label: "Port", key: "port" },
    { label: "Message", key: "message" }
  ], "Belum ada command.");
}

function renderWakeDevices() {
  const select = $("wake-device");
  if (!select) return;
  select.innerHTML = state.devices
    .filter((item) => item.enabled !== false)
    .map((row) => `<option value="${escapeHtml(row.id)}">${escapeHtml(row.name || row.mac_address || row.id)}</option>`)
    .join("");

  updateWakeFields();
}

function updateWakeFields() {
  const select = $("wake-device");
  const id = select ? select.value : "";
  const row = state.devices.find((item) => item.id === id);
  if (!row) return;
  $("wake-mac").value = row.mac_address || "";
  $("wake-broadcast").value = row.broadcast_ip || state.settings.defaultBroadcast || "";
  $("wake-port").value = row.port || state.settings.defaultPort || 9;
}

function fillDeviceForm(row) {
  $("device-id").value = row?.id || "";
  $("device-name").value = row?.name || "";
  $("device-mac").value = row?.mac_address || "";
  $("device-ip").value = row?.ip_address || "";
  $("device-broadcast").value = row?.broadcast_ip || state.settings.defaultBroadcast || "";
  $("device-port").value = row?.port || state.settings.defaultPort || 9;
  $("device-enabled").checked = row?.enabled !== false;
}

function clearDeviceForm() {
  fillDeviceForm(null);
}

function escapeHtml(value) {
  return String(value ?? "")
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#39;");
}

async function registerBridge() {
  const result = $("action-result");
  try {
    const payload = {
      id: state.settings.bridgeId,
      name: state.settings.bridgeName,
      bridge_secret: state.settings.bridgeSecret,
      local_ip: "",
      ap_ip: "",
      wifi_connected: false,
      last_seen_at: new Date().toISOString()
    };

    const existing = await request(`/wol_bridges?${encodeQuery({ select: "id", id: `eq.${state.settings.bridgeId}`, limit: "1" })}`, { method: "GET" });
    const exists = Array.isArray(existing) && existing.length > 0;

    const method = exists ? "PATCH" : "POST";
    const path = exists ? `/wol_bridges?id=eq.${encodeURIComponent(state.settings.bridgeId)}` : "/wol_bridges";
    await request(path, {
      method,
      preferReturn: true,
      body: JSON.stringify(payload)
    });

    setStatus(exists ? "Bridge updated." : "Bridge registered.", "ok");
    await refreshAll();
  } catch (error) {
    setStatus(error.message || "Failed to register bridge.", "fail");
  }
}

async function testConnection() {
  try {
    const rows = await request(`/wol_bridges?${encodeQuery({ select: "id", limit: "1" })}`, { method: "GET" });
    setStatus(`Connection OK. ${Array.isArray(rows) ? rows.length : 0} row(s) returned.`, "ok");
  } catch (error) {
    setStatus(error.message || "Connection failed.", "fail");
  }
}

async function saveDevice(event) {
  event.preventDefault();
  const device = {
    bridge_id: state.settings.bridgeId,
    name: $("device-name").value.trim(),
    mac_address: $("device-mac").value.trim(),
    ip_address: $("device-ip").value.trim(),
    broadcast_ip: $("device-broadcast").value.trim() || state.settings.defaultBroadcast || "255.255.255.255",
    port: Number($("device-port").value || state.settings.defaultPort || 9),
    enabled: $("device-enabled").checked
  };
  const deviceId = $("device-id").value.trim();
  try {
    if (!device.name || !device.mac_address) {
      throw new Error("Name dan MAC wajib diisi.");
    }
    if (deviceId) {
      await request(`/wol_devices?id=eq.${encodeURIComponent(deviceId)}`, {
        method: "PATCH",
        preferReturn: true,
        body: JSON.stringify(device)
      });
    } else {
      await request("/wol_devices", {
        method: "POST",
        preferReturn: true,
        body: JSON.stringify(device)
      });
    }
    clearDeviceForm();
    setStatus("Device saved.", "ok");
    await refreshAll();
  } catch (error) {
    setStatus(error.message || "Failed to save device.", "fail");
  }
}

async function disableDevice(deviceId) {
  const row = state.devices.find((item) => item.id === deviceId);
  if (!row) return;
  try {
    await request(`/wol_devices?id=eq.${encodeURIComponent(deviceId)}`, {
      method: "PATCH",
      preferReturn: true,
      body: JSON.stringify({ enabled: !row.enabled })
    });
    await refreshAll();
  } catch (error) {
    setStatus(error.message || "Failed to update device.", "fail");
  }
}

async function sendWake(deviceId) {
  const row = state.devices.find((item) => item.id === deviceId);
  if (!row) return;
  try {
    await request("/wol_commands", {
      method: "POST",
      preferReturn: true,
      body: JSON.stringify({
        bridge_id: state.settings.bridgeId,
        device_id: row.id,
        mac_address: row.mac_address,
        broadcast_ip: row.broadcast_ip || state.settings.defaultBroadcast || "255.255.255.255",
        port: row.port || state.settings.defaultPort || 9,
        status: "pending"
      })
    });
    setStatus(`Queued WOL for ${row.name || row.mac_address}.`, "ok");
    await refreshAll();
  } catch (error) {
    setStatus(error.message || "Failed to queue WOL.", "fail");
  }
}

async function saveSettings(event) {
  if (event) event.preventDefault();
  state.settings.supabaseUrl = $("supabase-url").value.trim();
  state.settings.supabaseKey = $("supabase-key").value.trim();
  state.settings.bridgeId = $("bridge-id").value.trim();
  state.settings.bridgeSecret = $("bridge-secret").value.trim();
  state.settings.bridgeName = $("bridge-name").value.trim();
  state.settings.defaultBroadcast = $("default-broadcast").value.trim();
  state.settings.defaultPort = Number($("default-port").value || 9);
  saveSettingsToStorage();
  setStatus("Settings saved locally.", "ok");
  await refreshAll();
}

function loadSettingsToForm() {
  $("supabase-url").value = state.settings.supabaseUrl || "";
  $("supabase-key").value = state.settings.supabaseKey || "";
  $("bridge-id").value = state.settings.bridgeId || "";
  $("bridge-secret").value = state.settings.bridgeSecret || "";
  $("bridge-name").value = state.settings.bridgeName || "";
  $("default-broadcast").value = state.settings.defaultBroadcast || "";
  $("default-port").value = state.settings.defaultPort || 9;
}

async function refreshAll() {
  const project = $("project-status");
  const bridge = $("bridge-status");

  if (!state.settings.supabaseUrl || !state.settings.supabaseKey || !state.settings.bridgeId || !state.settings.bridgeSecret) {
    if (project) project.textContent = "Missing settings";
    if (bridge) bridge.textContent = state.settings.bridgeId || "-";
    renderOverview();
    renderBridges();
    renderDevices();
    renderCommands();
    renderWakeDevices();
    return;
  }

  try {
    await Promise.all([fetchBridges(), fetchDevices(), fetchCommands(), fetchStatuses()]);
    if (project) project.textContent = "Connected";
    if (bridge) bridge.textContent = bridgeStatusText();
    renderOverview();
    renderBridges();
    renderDevices();
    renderCommands();
    renderWakeDevices();
  } catch (error) {
    if (project) project.textContent = "Error";
    if (bridge) bridge.textContent = state.settings.bridgeId || "-";
    setStatus(error.message || "Refresh failed.", "fail");
    renderOverview();
  }
}

function setTab(name) {
  document.querySelectorAll(".tab-btn").forEach((btn) => {
    btn.classList.toggle("active", btn.dataset.tab === name);
  });
  document.querySelectorAll(".panel").forEach((panel) => {
    panel.classList.toggle("active", panel.dataset.panel === name);
  });
  localStorage.setItem("wolm5-active-tab", name);
}

function setupTabs() {
  document.querySelectorAll(".tab-btn").forEach((btn) => {
    btn.addEventListener("click", () => setTab(btn.dataset.tab));
  });
  setTab(localStorage.getItem("wolm5-active-tab") || "dashboard");
}

function setupEvents() {
  $("settings-form").addEventListener("submit", saveSettings);
  $("save-settings-btn").addEventListener("click", saveSettings);
  $("forget-settings-btn").addEventListener("click", () => {
    localStorage.removeItem(storageKey);
    state.settings = {
      supabaseUrl: "",
      supabaseKey: "",
      bridgeId: "m5-atom-s3",
      bridgeSecret: "",
      bridgeName: "Atom S3 Bridge",
      defaultBroadcast: "",
      defaultPort: 9
    };
    loadSettingsToForm();
    setStatus("Settings cleared.", "warn");
    refreshAll();
  });
  $("refresh-btn").addEventListener("click", refreshAll);
  $("test-conn-btn").addEventListener("click", testConnection);
  $("register-bridge-btn").addEventListener("click", registerBridge);
  $("device-form").addEventListener("submit", saveDevice);
  $("device-clear-btn").addEventListener("click", clearDeviceForm);
  $("wake-device").addEventListener("change", updateWakeFields);
  $("wake-form").addEventListener("submit", async (event) => {
    event.preventDefault();
    const deviceId = $("wake-device").value;
    if (!deviceId) return;
    await sendWake(deviceId);
  });

  document.addEventListener("click", async (event) => {
    const editId = event.target.closest("[data-edit-device]")?.dataset.editDevice;
    const sendId = event.target.closest("[data-send-wol]")?.dataset.sendWol;
    const toggleId = event.target.closest("[data-disable-device]")?.dataset.disableDevice;
    if (editId) {
      const row = state.devices.find((item) => item.id === editId);
      if (row) fillDeviceForm(row);
      setTab("devices");
    }
    if (sendId) {
      await sendWake(sendId);
      setTab("commands");
    }
    if (toggleId) {
      await disableDevice(toggleId);
    }
  });
}

async function bootstrap() {
  loadSettings();
  loadSettingsToForm();
  setupTabs();
  setupEvents();
  renderOverview();
  renderBridges();
  renderDevices();
  renderCommands();
  renderWakeDevices();
  await refreshAll();
  setInterval(refreshAll, 30000);
}

bootstrap();
