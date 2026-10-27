#pragma once

const char DASHBOARD_HTML[] PROGMEM = R"HTML(

<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>NETWATCH Sentinel</title>

<style>
:root {
  color-scheme: dark;
  --bg: #090d14;
  --panel: #101722;
  --panel2: #151e2b;
  --border: #263244;
  --text: #e7edf5;
  --muted: #8998ac;
  --accent: #58b8ff;
  --green: #42d6a0;
  --red: #ff657a;
  --amber: #ffc36a;
}

* { box-sizing: border-box; }

body {
  margin: 0;
  background: var(--bg);
  color: var(--text);
  font-family: Inter, -apple-system, BlinkMacSystemFont, "Segoe UI",
               sans-serif;
  font-size: 14px;
}

button, input, select { font: inherit; }

button {
  cursor: pointer;
  border: 1px solid var(--border);
  background: var(--panel2);
  color: var(--text);
  border-radius: 8px;
  padding: 9px 12px;
}

button:hover { border-color: var(--accent); }
button:disabled { opacity: 0.6; cursor: wait; }

input, select {
  width: 100%;
  background: #0b111a;
  color: var(--text);
  border: 1px solid var(--border);
  border-radius: 8px;
  padding: 11px 12px;
  outline: none;
}

input:focus, select:focus {
  border-color: var(--accent);
}

.app { min-height: 100vh; }

header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 20px;
  padding: 20px 28px;
  border-bottom: 1px solid var(--border);
  background: #0d131d;
}

.brand {
  display: flex;
  align-items: center;
  gap: 13px;
}

.logo {
  display: grid;
  place-items: center;
  width: 42px;
  height: 42px;
  background: #122a40;
  border: 1px solid #24577d;
  border-radius: 12px;
  color: var(--accent);
  font-weight: 800;
  font-size: 20px;
}

h1 {
  font-size: 20px;
  letter-spacing: 1.5px;
  margin: 0;
}

.subtitle {
  color: var(--muted);
  margin-top: 5px;
  font-size: 12px;
}

.header-right {
  display: flex;
  align-items: center;
  gap: 12px;
  flex-wrap: wrap;
}

.status {
  display: flex;
  align-items: center;
  gap: 8px;
  color: var(--muted);
  font-size: 12px;
}

.dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: var(--amber);
}

.dot.connected { background: var(--green); }
.dot.error { background: var(--red); }

main {
  max-width: 1600px;
  margin: auto;
  padding: 26px 28px 40px;
}

.page-heading {
  display: flex;
  justify-content: space-between;
  align-items: flex-end;
  gap: 16px;
  margin-bottom: 20px;
}

h2 {
  font-size: 23px;
  margin: 0 0 7px;
}

h3 {
  font-size: 15px;
  margin: 0;
}

.muted { color: var(--muted); }
.small { font-size: 12px; }

.metrics {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 14px;
  margin-bottom: 20px;
}

.metric, .panel {
  background: var(--panel);
  border: 1px solid var(--border);
  border-radius: 12px;
}

.metric { padding: 19px; }

.metric-label {
  color: var(--muted);
  font-size: 12px;
  margin-bottom: 13px;
}

.metric-value {
  font-size: 30px;
  font-weight: 700;
  letter-spacing: -1px;
}

.metric-foot {
  margin-top: 8px;
  font-size: 11px;
  color: var(--muted);
}

.panel { overflow: hidden; }

.panel-heading {
  padding: 18px 20px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 14px;
  border-bottom: 1px solid var(--border);
}

.toolbar {
  display: grid;
  grid-template-columns: minmax(220px, 2fr)
                         minmax(140px, 1fr)
                         minmax(150px, 1fr);
  gap: 12px;
  padding: 16px 20px;
  border-bottom: 1px solid var(--border);
}

.table-wrap { overflow-x: auto; }

table {
  width: 100%;
  border-collapse: collapse;
  white-space: nowrap;
}

th {
  background: #111a26;
  color: var(--muted);
  font-size: 11px;
  text-align: left;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: .7px;
}

th, td {
  padding: 14px 16px;
  border-bottom: 1px solid #202b3a;
}

tbody tr { cursor: pointer; }
tbody tr:hover { background: #172231; }
tbody tr:last-child td { border-bottom: 0; }

.ip {
  color: var(--accent);
  font-weight: 600;
  font-variant-numeric: tabular-nums;
}

.mac {
  font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
  font-size: 12px;
  color: #b6c3d3;
}

.badge {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  border-radius: 6px;
  padding: 5px 8px;
  font-size: 11px;
  font-weight: 600;
  background: #202b39;
  color: #c6d1df;
}

.badge.online {
  color: var(--green);
  background: #12332b;
}

.badge.offline {
  color: #ff8a99;
  background: #351b25;
}

.badge.warning {
  color: var(--amber);
  background: #382b18;
}

.port-list {
  display: flex;
  flex-wrap: wrap;
  gap: 5px;
}

.port {
  border: 1px solid #31435a;
  background: #172638;
  border-radius: 5px;
  padding: 3px 6px;
  font-size: 11px;
  color: #b8dcff;
}

.score {
  display: flex;
  align-items: center;
  gap: 8px;
}

.score-track {
  width: 54px;
  height: 5px;
  border-radius: 10px;
  background: #263244;
  overflow: hidden;
}

.score-fill {
  height: 100%;
  background: var(--accent);
}

.empty {
  padding: 50px 20px;
  text-align: center;
  color: var(--muted);
}

.empty strong {
  display: block;
  margin-bottom: 8px;
  color: var(--text);
  font-size: 15px;
}

.footer {
  display: flex;
  justify-content: space-between;
  gap: 12px;
  flex-wrap: wrap;
  padding: 13px 20px;
  border-top: 1px solid var(--border);
  color: var(--muted);
  font-size: 11px;
}

.notice {
  display: none;
  margin-bottom: 18px;
  padding: 14px 16px;
  border: 1px solid #76552c;
  background: #241d13;
  color: #ffd59a;
  border-radius: 9px;
  line-height: 1.6;
}

.notice.visible { display: block; }

.notice.error {
  border-color: #71303b;
  background: #26151b;
  color: #ffadb8;
}

.detail-overlay {
  display: none;
  position: fixed;
  inset: 0;
  z-index: 10;
  background: #000a;
  justify-content: flex-end;
}

.detail-overlay.open { display: flex; }

.detail {
  width: min(520px, 100%);
  height: 100%;
  overflow-y: auto;
  background: var(--panel);
  border-left: 1px solid var(--border);
  padding: 24px;
}

.detail-top {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 12px;
  margin-bottom: 25px;
}

.detail-ip {
  font-size: 23px;
  font-weight: 700;
  margin: 8px 0;
  overflow-wrap: anywhere;
}

.detail-section {
  border-top: 1px solid var(--border);
  padding: 19px 0;
}

.detail-section h3 { margin-bottom: 15px; }

.field-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 18px 12px;
}

.field-label {
  color: var(--muted);
  font-size: 11px;
  margin-bottom: 6px;
}

.field-value {
  overflow-wrap: anywhere;
  font-size: 13px;
}

.detail-ports {
  display: flex;
  flex-wrap: wrap;
  gap: 7px;
}

@media (max-width: 900px) {
  main { padding: 20px 14px; }
  header { padding: 16px; }
  .metrics { grid-template-columns: repeat(2, minmax(0, 1fr)); }
  .toolbar { grid-template-columns: 1fr 1fr; }
  .toolbar input { grid-column: 1 / -1; }
}

@media (max-width: 520px) {
  .page-heading { align-items: flex-start; flex-direction: column; }
  .metrics { gap: 9px; }
  .metric { padding: 14px; }
  .metric-value { font-size: 25px; }
  .toolbar { grid-template-columns: 1fr; }
  .toolbar input { grid-column: auto; }
  h1 { font-size: 17px; }
}
</style>
</head>

<body>
<div class="app">
  <header>
    <div class="brand">
      <div class="logo">N</div>
      <div>
        <h1>NETWATCH <span style="color:#58b8ff">SENTINEL</span></h1>
        <div class="subtitle">Network asset discovery and monitoring</div>
      </div>
    </div>

    <div class="header-right">
      <div class="status">
        <span id="statusDot" class="dot"></span>
        <span id="connectionStatus">Connecting to API…</span>
      </div>
      <button id="refreshButton" type="button">Refresh now</button>
    </div>
  </header>

  <main>
    <div class="page-heading">
      <div>
        <h2>Asset inventory</h2>
        <div class="muted">
          Discovered network devices, observed ports and device fingerprints.
        </div>
      </div>
      <div class="muted small" id="lastUpdated">Waiting for API response</div>
    </div>

    <div id="notice" class="notice" role="status"></div>

    <section class="metrics" aria-label="Inventory summary">
      <div class="metric">
        <div class="metric-label">Devices discovered</div>
        <div class="metric-value" id="totalCount">—</div>
        <div class="metric-foot">Records returned by the API</div>
      </div>

      <div class="metric">
        <div class="metric-label">Online devices</div>
        <div class="metric-value" id="onlineCount">—</div>
        <div class="metric-foot">Reported online by the scanner</div>
      </div>

      <div class="metric">
        <div class="metric-label">Offline devices</div>
        <div class="metric-value" id="offlineCount">—</div>
        <div class="metric-foot">Reported offline by the scanner</div>
      </div>

      <div class="metric">
        <div class="metric-label">Observed open ports</div>
        <div class="metric-value" id="portCount">—</div>
        <div class="metric-foot">Sum of reported port records</div>
      </div>
    </section>

    <section class="panel">
      <div class="panel-heading">
        <div>
          <h3>Discovered devices</h3>
          <div class="muted small" style="margin-top:6px">
            Select any row to inspect its full record.
          </div>
        </div>
        <span class="badge" id="resultCount">Waiting for data</span>
      </div>

      <div class="toolbar">
        <input
          id="searchInput"
          type="search"
          placeholder="Search IP, MAC, hostname, vendor, type, or port…"
          aria-label="Search devices"
        >

        <select id="statusFilter" aria-label="Filter by status">
          <option value="all">All statuses</option>
          <option value="online">Online</option>
          <option value="offline">Offline</option>
        </select>

        <select id="sortSelect" aria-label="Sort devices">
          <option value="ip">IP address</option>
          <option value="type">Device type</option>
          <option value="vendor">Vendor</option>
          <option value="ports">Open port count</option>
          <option value="lastSeen">Last seen</option>
          <option value="score">Fingerprint score</option>
        </select>
      </div>

      <div class="table-wrap">
        <table>
          <thead>
            <tr>
              <th>Status</th>
              <th>IP address</th>
              <th>Hostname</th>
              <th>Vendor</th>
              <th>Device type</th>
              <th>Open ports</th>
              <th>Fingerprint</th>
              <th>Last seen</th>
            </tr>
          </thead>
          <tbody id="deviceRows">
            <tr>
              <td colspan="8">
                <div class="empty">
                  <strong>Waiting for inventory</strong>
                  The dashboard has not received device data yet.
                </div>
              </td>
            </tr>
          </tbody>
        </table>
      </div>

      <div class="footer">
        <span id="inventorySummary">No inventory loaded</span>
        <span>Automatic refresh: 10 seconds</span>
      </div>
    </section>
  </main>
</div>

<div
  id="detailOverlay"
  class="detail-overlay"
  role="dialog"
  aria-modal="true"
  aria-label="Device details"
>
  <aside class="detail">
    <div class="detail-top">
      <div>
        <div class="muted small">DEVICE RECORD</div>
        <div id="detailIP" class="detail-ip">—</div>
        <div id="detailStatus"></div>
      </div>
      <button id="closeDetail" type="button" aria-label="Close details">
        Close
      </button>
    </div>

    <div class="detail-section">
      <h3>Identification</h3>
      <div class="field-grid" id="identityFields"></div>
    </div>

    <div class="detail-section">
      <h3>Network ports</h3>
      <div id="detailPorts" class="detail-ports"></div>
    </div>

    <div class="detail-section">
      <h3>Fingerprint</h3>
      <div id="detailFingerprint"></div>
    </div>

    <div class="detail-section">
      <h3>Observation history</h3>
      <div class="field-grid" id="timeFields"></div>
    </div>
  </aside>
</div>

<script>
"use strict";

/*
 * API routes are relative to the ESP32 serving this page.
 * No hard-coded server address is needed.
 */
const API_URL = "/api/data";
const STATUS_URL = "/api/status";
const REFRESH_INTERVAL_MS = 10000;

let devices = [];
let securityEvents = [];
let networkInfo = {};
let serverStatus = null;
let lastSuccessfulFetch = null;
let requestInProgress = false;

const $ = (id) => document.getElementById(id);

const elements = {
  statusDot: $("statusDot"),
  connectionStatus: $("connectionStatus"),
  lastUpdated: $("lastUpdated"),
  notice: $("notice"),
  totalCount: $("totalCount"),
  onlineCount: $("onlineCount"),
  offlineCount: $("offlineCount"),
  portCount: $("portCount"),
  resultCount: $("resultCount"),
  deviceRows: $("deviceRows"),
  inventorySummary: $("inventorySummary"),
  searchInput: $("searchInput"),
  statusFilter: $("statusFilter"),
  sortSelect: $("sortSelect"),
  detailOverlay: $("detailOverlay")
};

function safeText(value, fallback = "Unknown") {
  if (value === null || value === undefined || value === "") {
    return fallback;
  }

  return String(value);
}

function escapeHTML(value) {
  return String(value)
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#039;");
}

/*
 * Normalize port data without changing the display.
 *
 * Supported:
 *   [22, 80, 443]
 *   ["22", "80", "443"]
 *   "22,80,443"
 *   "[22,80,443]"
 *   "[\"22\",\"80\",\"443\"]"
 *   [{ "port": 80, "service": "http" }]
 */
function normalizePorts(device) {
  const raw = device.ports ?? device.openPorts ?? [];

  if (Array.isArray(raw)) {
    return raw.map((port) => {
      if (typeof port === "object" && port !== null) {
        return {
          number: String(
            port.port ?? port.number ?? port.id ?? ""
          ).trim(),
          service: String(port.service ?? port.name ?? "")
        };
      }

      return {
        number: String(port).trim(),
        service: ""
      };
    }).filter(port => /^\d+$/.test(port.number) &&
                      Number(port.number) >= 1 &&
                      Number(port.number) <= 65535);
  }

  if (typeof raw === "number") {
    if (!Number.isInteger(raw) || raw < 1 || raw > 65535) {
      return [];
    }

    return [{ number: String(raw), service: "" }];
  }

  if (typeof raw === "string") {
    const value = raw.trim();

    if (!value ||
        value.toUpperCase() === "NONE" ||
        value.toUpperCase() === "UNKNOWN") {
      return [];
    }

    // Parse JSON-encoded arrays before trying string delimiters.
    if (value.startsWith("[") && value.endsWith("]")) {
      try {
        const parsed = JSON.parse(value);

        if (Array.isArray(parsed)) {
          return normalizePorts({ ports: parsed });
        }
      } catch (_) {
        // Fall through to support legacy comma-separated strings.
      }
    }

    return value.split(/[,\s;]+/)
      .map(port => port.trim().replace(/^["']|["']$/g, ""))
      .filter(port => /^\d+$/.test(port) &&
                      Number(port) >= 1 &&
                      Number(port) <= 65535)
      .map(number => ({ number, service: "" }));
  }

  return [];
}

function normalizeDevice(device) {
  if (!device || typeof device !== "object" ||
      Array.isArray(device)) {
    return null;
  }

  const ports = normalizePorts(device);
  const rawScore = device.fingerprintScore ?? device.fingerprint;

  const score =
    rawScore === null ||
    rawScore === undefined ||
    rawScore === ""
      ? null
      : Number(rawScore);

  return {
    ...device,
    ip: safeText(device.ip, ""),
    hostname: safeText(device.hostname),
    mac: safeText(device.mac),
    vendor: safeText(device.vendor),

    deviceType: safeText(
      device.deviceType ??
      device.device_type ??
      device.type
    ),

    online:
      device.online === true ||
      device.online === 1 ||
      String(device.online).toLowerCase() === "true",

    openPorts: ports,
    portCount: ports.length,

    fingerprintScore:
      score !== null && Number.isFinite(score)
        ? score
        : null,

    firstSeen: device.firstSeen ?? device.first_seen ?? null,
    lastSeen: device.lastSeen ?? device.last_seen ?? null
  };
}

function extractDevices(payload) {
  if (Array.isArray(payload)) {
    return payload;
  }

  if (payload && typeof payload === "object") {
    if (Array.isArray(payload.devices)) {
      return payload.devices;
    }

    if (Array.isArray(payload.currentDevices)) {
      return payload.currentDevices;
    }

    if (payload.data && Array.isArray(payload.data.devices)) {
      return payload.data.devices;
    }
  }

  throw new Error(
    "The API response does not contain a device array. " +
    "Expected an array or an object containing devices."
  );
}

function showNotice(message, isError = false) {
  elements.notice.textContent = message;
  elements.notice.className =
    "notice visible" + (isError ? " error" : "");
}

function hideNotice() {
  elements.notice.textContent = "";
  elements.notice.className = "notice";
}

function setConnection(connected, message) {
  elements.statusDot.className =
    "dot" + (connected ? " connected" : " error");

  elements.connectionStatus.textContent = message;
}

function renderMetrics() {
  const online = devices.filter(device => device.online).length;
  const offline = devices.length - online;

  const openPorts = devices.reduce(
    (sum, device) => sum + device.portCount,
    0
  );

  elements.totalCount.textContent = devices.length;
  elements.onlineCount.textContent = online;
  elements.offlineCount.textContent = offline;
  elements.portCount.textContent = openPorts;

  elements.inventorySummary.textContent =
    `${devices.length} device record${devices.length === 1 ? "" : "s"} loaded`;
}

/*
 * firstSeen and lastSeen from the current server are generated
 * using millis(), so these are milliseconds since ESP32 boot,
 * not Unix epoch timestamps.
 */
function formatTimestamp(value) {
  if (value === null || value === undefined || value === "") {
    return "Not recorded";
  }

  // Numeric values from the current server are millis() uptime.
  if (typeof value === "number" ||
      (typeof value === "string" &&
       /^\d+(?:\.\d+)?$/.test(value.trim()))) {

    const numeric = Number(value);

    if (!Number.isFinite(numeric) || numeric < 0) {
      return "Not recorded";
    }

    const totalSeconds = Math.floor(numeric / 1000);
    const hours = Math.floor(totalSeconds / 3600);
    const minutes = Math.floor((totalSeconds % 3600) / 60);
    const seconds = totalSeconds % 60;

    if (hours > 0) {
      return `${hours}h ${minutes}m ${seconds}s after boot`;
    }

    if (minutes > 0) {
      return `${minutes}m ${seconds}s after boot`;
    }

    return `${seconds}s after boot`;
  }

  // Preserve support for timestamp strings supplied by other firmware.
  const date = new Date(value);

  return Number.isNaN(date.getTime())
    ? String(value)
    : date.toLocaleString();
}

function deviceSearchText(device) {
  const ports = device.openPorts
    .map(port => `${port.number} ${port.service}`)
    .join(" ");

  return [
    device.ip,
    device.hostname,
    device.mac,
    device.vendor,
    device.deviceType,
    ports
  ].join(" ").toLowerCase();
}

function compareIP(a, b) {
  const aa = String(a).split(".").map(Number);
  const bb = String(b).split(".").map(Number);

  if (
    aa.length === 4 &&
    bb.length === 4 &&
    aa.every(Number.isFinite) &&
    bb.every(Number.isFinite)
  ) {
    for (let i = 0; i < 4; i++) {
      if (aa[i] !== bb[i]) {
        return aa[i] - bb[i];
      }
    }

    return 0;
  }

  return String(a).localeCompare(String(b));
}

function timestampNumber(value) {
  if (value === null || value === undefined || value === "") {
    return 0;
  }

  const numeric = Number(value);

  if (Number.isFinite(numeric) && String(value).trim() !== "") {
    return numeric;
  }

  const parsed = Date.parse(value);
  return Number.isNaN(parsed) ? 0 : parsed;
}

function sortDevices(list) {
  const sortBy = elements.sortSelect.value;
  const sorted = [...list];

  sorted.sort((a, b) => {
    switch (sortBy) {
      case "type":
        return a.deviceType.localeCompare(b.deviceType);

      case "vendor":
        return a.vendor.localeCompare(b.vendor);

      case "ports":
        return b.portCount - a.portCount;

      case "lastSeen":
        return timestampNumber(b.lastSeen) -
               timestampNumber(a.lastSeen);

      case "score":
        return (b.fingerprintScore ?? -1) -
               (a.fingerprintScore ?? -1);

      case "ip":
      default:
        return compareIP(a.ip, b.ip);
    }
  });

  return sorted;
}

function renderPorts(ports) {
  if (!ports.length) {
    return '<span class="muted">None reported</span>';
  }

  return '<div class="port-list">' +
    ports.map(port =>
      `<span class="port">${escapeHTML(port.number)}</span>`
    ).join("") +
    "</div>";
}

function renderDevices() {
  const search =
    elements.searchInput.value.trim().toLowerCase();

  const filter = elements.statusFilter.value;

  const filtered = devices.filter(device => {
    const matchesSearch =
      deviceSearchText(device).includes(search);

    const matchesStatus =
      filter === "all" ||
      (filter === "online" && device.online) ||
      (filter === "offline" && !device.online);

    return matchesSearch && matchesStatus;
  });

  const sorted = sortDevices(filtered);

  elements.resultCount.textContent = `${sorted.length} shown`;

  if (!sorted.length) {
    const message = devices.length
      ? "No devices match your filters."
      : "No device records were returned by the API.";

    elements.deviceRows.innerHTML = `
      <tr>
        <td colspan="8">
          <div class="empty">
            <strong>${escapeHTML(message)}</strong>
            ${devices.length
              ? "Try changing the search or status filter."
              : "Check the API response and scanner connection."}
          </div>
        </td>
      </tr>`;

    return;
  }

  elements.deviceRows.innerHTML = sorted.map(device => {
    const originalIndex = devices.indexOf(device);
    const score = device.fingerprintScore;

    const scoreDisplay =
      score === null ? "N/A" : String(score);

    const scoreWidth = score === null
      ? 0
      : Math.max(0, Math.min(100, score));

    return `
      <tr data-device-index="${originalIndex}" tabindex="0"
          aria-label="Inspect ${escapeHTML(device.ip)}">
        <td>
          <span class="badge ${device.online ? "online" : "offline"}">
            ${device.online ? "● Online" : "● Offline"}
          </span>
        </td>
        <td class="ip">${escapeHTML(device.ip)}</td>
        <td>${escapeHTML(device.hostname)}</td>
        <td>${escapeHTML(device.vendor)}</td>
        <td>${escapeHTML(device.deviceType)}</td>
        <td>${renderPorts(device.openPorts)}</td>
        <td>
          <div class="score">
            <span>${escapeHTML(scoreDisplay)}</span>
            <div class="score-track">
              <div class="score-fill"
                   style="width:${scoreWidth}%"></div>
            </div>
          </div>
        </td>
        <td>${escapeHTML(formatTimestamp(device.lastSeen))}</td>
      </tr>`;
  }).join("");

  elements.deviceRows
    .querySelectorAll("tr[data-device-index]")
    .forEach(row => {
      const open = () => {
        const index = Number(row.dataset.deviceIndex);

        if (Number.isInteger(index) && devices[index]) {
          openDetails(devices[index]);
        }
      };

      row.addEventListener("click", open);

      row.addEventListener("keydown", event => {
        if (event.key === "Enter" || event.key === " ") {
          event.preventDefault();
          open();
        }
      });
    });
}

function renderField(label, value) {
  return `
    <div>
      <div class="field-label">${escapeHTML(label)}</div>
      <div class="field-value">${escapeHTML(safeText(value))}</div>
    </div>`;
}

function openDetails(device) {
  elements.detailOverlay.classList.add("open");

  $("detailIP").textContent = device.ip;

  $("detailStatus").innerHTML = `
    <span class="badge ${device.online ? "online" : "offline"}">
      ${device.online ? "● Online" : "● Offline"}
    </span>`;

  $("identityFields").innerHTML = [
    renderField("Hostname", device.hostname),
    renderField("MAC address", device.mac),
    renderField("Vendor", device.vendor),
    renderField("Device type", device.deviceType),
    renderField("Reported online", device.online ? "Yes" : "No"),
    renderField("Open port count", device.portCount)
  ].join("");

  $("detailPorts").innerHTML = device.openPorts.length
    ? device.openPorts.map(port => `
        <span class="port">
          ${escapeHTML(port.number)}
          ${port.service ? " · " + escapeHTML(port.service) : ""}
        </span>
      `).join("")
    : '<span class="muted">No open ports reported.</span>';

  const score = device.fingerprintScore;

  $("detailFingerprint").innerHTML = `
    <div class="metric-value" style="font-size:26px">
      ${score === null ? "N/A" : escapeHTML(score)}
    </div>
    <div class="muted small" style="margin-top:7px">
      Scanner-reported fingerprint score. Interpret according to
      the scoring rules in your firmware.
    </div>`;

  $("timeFields").innerHTML = [
    renderField("First seen", formatTimestamp(device.firstSeen)),
    renderField("Last seen", formatTimestamp(device.lastSeen))
  ].join("");
}

function closeDetails() {
  elements.detailOverlay.classList.remove("open");
}

function normalizeEvent(event) {
  if (!event || typeof event !== "object" ||
      Array.isArray(event)) {
    return null;
  }

  return {
    source: safeText(event.source, "SCANNER"),
    type: safeText(event.type, "UNKNOWN"),
    sourceIP: safeText(event.sourceIP ?? event.ip, ""),
    severity: safeText(event.severity, "INFO"),
    port: Number(event.port) || 0,
    service: safeText(event.service, ""),
    details: safeText(event.details ?? event.description, ""),
    timestamp: event.timestamp ?? null
  };
}

async function fetchJSON(url) {
  const response = await fetch(url, {
    method: "GET",
    headers: { "Accept": "application/json" },
    cache: "no-store"
  });

  if (!response.ok) {
    throw new Error(
      `HTTP ${response.status} ${response.statusText} from ${url}`
    );
  }

  const contentType =
    response.headers.get("content-type") || "";

  if (!contentType.toLowerCase().includes("application/json")) {
    throw new Error(`${url} did not return JSON.`);
  }

  return response.json();
}

async function refreshInventory() {
  if (requestInProgress) {
    return;
  }

  requestInProgress = true;
  $("refreshButton").disabled = true;

  try {
    const payload = await fetchJSON(API_URL);
    const rawDevices = extractDevices(payload);
    const normalized = rawDevices.map(normalizeDevice);

    if (normalized.some(device => device === null)) {
      throw new Error(
        "The inventory contains an invalid device record."
      );
    }

    devices = normalized;

    networkInfo = {
      scannerIP: payload.scannerIP ?? "",
      network: payload.network ?? "",
      honeypotIP: payload.honeypotIP ?? ""
    };

    securityEvents = Array.isArray(payload.events)
      ? payload.events.map(normalizeEvent).filter(Boolean)
      : [];

    lastSuccessfulFetch = new Date();

    renderMetrics();
    renderDevices();

    setConnection(true, "API connected");

    elements.lastUpdated.textContent =
      "Updated " + lastSuccessfulFetch.toLocaleTimeString();

    // Fetch scanner health independently from inventory.
    // A status failure must not invalidate valid device data.
    try {
      serverStatus = await fetchJSON(STATUS_URL);

      const age = Number(serverStatus.scannerAge);

      // Prefer the timeout configured by the ESP32 server.
      // Fall back to 300 seconds to match the revised server code.
      const configuredTimeoutMs =
        Number(serverStatus.onlineTimeoutMs) ||
        Number(payload.onlineTimeoutMs) ||
        300000;

      const staleThresholdSeconds =
        configuredTimeoutMs / 1000;

      if (
        Number.isFinite(age) &&
        age !== 999999 &&
        age > staleThresholdSeconds
      ) {
        showNotice(
          `The dashboard API is responding, but the scanner's last update ` +
          `was ${age} seconds ago. Check the scanner ESP32 and its Wi-Fi connection.`
        );

        setConnection(
          true,
          `API connected · Scanner stale (${age}s)`
        );
      } else if (age === 999999) {
        showNotice(
          "The dashboard API is responding, but no scanner update has " +
          "been recorded since the server started."
        );

        setConnection(true, "API connected · Awaiting scanner");
      } else {
        hideNotice();
      }
    } catch (statusError) {
      console.warn(
        "NETWATCH status request failed:",
        statusError
      );

      // The inventory API worked, so retain the connected state.
    }
  } catch (error) {
    console.error(
      "NETWATCH inventory request failed:",
      error
    );

    setConnection(false, "API unavailable");

    showNotice(
      `Could not load live inventory from ${API_URL}. ` +
      `${error.message} Confirm that the ESP32 server is online and ` +
      "GET /api/data returns a JSON object containing devices.",
      true
    );

    elements.lastUpdated.textContent = lastSuccessfulFetch
      ? "Last successful update: " +
        lastSuccessfulFetch.toLocaleTimeString()
      : "No successful API response";

    // Preserve the last successful inventory during API failures.
  } finally {
    requestInProgress = false;
    $("refreshButton").disabled = false;
  }
}

elements.searchInput.addEventListener("input", renderDevices);
elements.statusFilter.addEventListener("change", renderDevices);
elements.sortSelect.addEventListener("change", renderDevices);

$("refreshButton").addEventListener(
  "click",
  refreshInventory
);

$("closeDetail").addEventListener(
  "click",
  closeDetails
);

elements.detailOverlay.addEventListener("click", event => {
  if (event.target === elements.detailOverlay) {
    closeDetails();
  }
});

document.addEventListener("keydown", event => {
  if (event.key === "Escape") {
    closeDetails();
  }
});

renderMetrics();
renderDevices();
refreshInventory();

setInterval(refreshInventory, REFRESH_INTERVAL_MS);
</script>
</body>
</html>
)HTML";
