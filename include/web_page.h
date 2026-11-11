#pragma once

const char INDEX_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Snapmaker U1 - RFID & LED Manager</title>
  <style>
    :root {
      --bg: #0f172a;
      --card-bg: #1e293b;
      --card-border: #334155;
      --text: #f8fafc;
      --text-muted: #94a3b8;
      --primary: #38bdf8;
      --primary-hover: #0ea5e9;
      --success: #22c55e;
      --warning: #f59e0b;
      --danger: #ef4444;
      --radius: 12px;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      background: var(--bg);
      color: var(--text);
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      padding: 16px;
      line-height: 1.5;
    }
    .container {
      max-width: 960px;
      margin: 0 auto;
      display: flex;
      flex-direction: column;
      gap: 16px;
    }
    header {
      display: flex;
      flex-wrap: wrap;
      justify-content: space-between;
      align-items: center;
      padding: 16px 20px;
      background: var(--card-bg);
      border: 1px solid var(--card-border);
      border-radius: var(--radius);
      gap: 12px;
    }
    .header-left h1 {
      font-size: 1.35rem;
      font-weight: 700;
      color: var(--primary);
      display: flex;
      align-items: center;
      gap: 8px;
    }
    .header-left p {
      font-size: 0.85rem;
      color: var(--text-muted);
    }
    .badge {
      display: inline-block;
      padding: 4px 10px;
      border-radius: 9999px;
      font-size: 0.8rem;
      font-weight: 600;
      text-transform: uppercase;
      letter-spacing: 0.05em;
    }
    .badge-idle { background: #334155; color: #cbd5e1; }
    .badge-armed { background: #78350f; color: #fde68a; animation: pulse 1.5s infinite; }
    .badge-ap { background: #3b0764; color: #e9d5ff; border: 1px solid #7e22ce; animation: pulse 1.5s infinite; }
    .badge-live { background: rgba(34, 197, 94, 0.15); color: #4ade80; border: 1px solid rgba(34, 197, 94, 0.3); }
    @keyframes pulse {
      0%, 100% { opacity: 1; }
      50% { opacity: 0.6; }
    }
    .alert-banner {
      background: rgba(245, 158, 11, 0.15);
      border: 1px solid var(--warning);
      color: #fef08a;
      padding: 14px 18px;
      border-radius: var(--radius);
      font-size: 0.92rem;
      display: flex;
      align-items: center;
      gap: 12px;
    }

    /* Tab Bar */
    .tab-bar {
      display: flex;
      gap: 6px;
      background: rgba(15, 23, 42, 0.8);
      padding: 6px;
      border-radius: var(--radius);
      border: 1px solid var(--card-border);
      overflow-x: auto;
    }
    .tab-btn {
      flex: 1;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
      padding: 10px 16px;
      border-radius: 8px;
      background: transparent;
      border: none;
      color: var(--text-muted);
      font-weight: 600;
      font-size: 0.9rem;
      cursor: pointer;
      white-space: nowrap;
      transition: all 0.2s;
    }
    .tab-btn:hover {
      color: var(--text);
      background: rgba(255, 255, 255, 0.05);
    }
    .tab-btn.active {
      color: var(--primary);
      background: var(--card-bg);
      box-shadow: 0 2px 4px rgba(0, 0, 0, 0.2);
      border: 1px solid var(--card-border);
    }
    .tab-content {
      display: none;
      flex-direction: column;
      gap: 16px;
    }
    .tab-content.active {
      display: flex;
    }

    .grid {
      display: grid;
      grid-template-columns: 1fr;
      gap: 16px;
    }
    @media (min-width: 768px) {
      .grid-2 { grid-template-columns: 1fr 1fr; }
    }
    .card {
      background: var(--card-bg);
      border: 1px solid var(--card-border);
      border-radius: var(--radius);
      padding: 20px;
      display: flex;
      flex-direction: column;
      gap: 16px;
    }
    .card-title {
      font-size: 1.1rem;
      font-weight: 600;
      color: #e2e8f0;
      display: flex;
      justify-content: space-between;
      align-items: center;
      border-bottom: 1px solid var(--card-border);
      padding-bottom: 10px;
    }
    .stat-row {
      display: flex;
      justify-content: space-between;
      align-items: center;
      font-size: 0.9rem;
      padding: 5px 0;
      border-bottom: 1px dashed rgba(255,255,255,0.06);
    }
    .stat-row:last-child { border-bottom: none; }
    .stat-label { color: var(--text-muted); }
    .stat-value { font-weight: 600; font-family: monospace; }

    /* Sensors Grid */
    .sensor-grid {
      display: grid;
      grid-template-columns: repeat(2, 1fr);
      gap: 10px;
    }
    .sensor-box {
      background: rgba(15, 23, 42, 0.6);
      border: 1px solid var(--card-border);
      border-radius: 8px;
      padding: 10px;
      text-align: center;
      transition: all 0.2s;
    }
    .sensor-box.detected {
      border-color: var(--success);
      background: rgba(34, 197, 94, 0.1);
    }
    .sensor-box .name { font-size: 0.85rem; color: var(--text-muted); }
    .sensor-box .status { font-size: 0.95rem; font-weight: bold; margin-top: 4px; }
    .sensor-box.detected .status { color: var(--success); }
    .sensor-box.empty .status { color: var(--text-muted); }

    /* LED Visualizer */
    .led-visual-container {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 12px;
    }
    .led-svg-wrap {
      width: 240px;
      height: 240px;
    }
    .led-ring-svg {
      width: 100%;
      height: 100%;
      overflow: visible;
    }
    .led-node {
      cursor: pointer;
      transition: transform 0.15s ease-out;
    }
    .led-node:hover circle {
      stroke: var(--primary) !important;
      stroke-width: 3px !important;
    }
    .led-node.selected circle {
      stroke: #38bdf8 !important;
      stroke-width: 3px !important;
    }

    /* Controls & Buttons */
    .control-group {
      display: flex;
      flex-direction: column;
      gap: 8px;
    }
    .control-label {
      font-size: 0.85rem;
      font-weight: 600;
      color: var(--text-muted);
      text-transform: uppercase;
      letter-spacing: 0.05em;
    }
    .btn-row {
      display: flex;
      flex-wrap: wrap;
      gap: 8px;
    }
    button {
      background: #334155;
      color: var(--text);
      border: 1px solid var(--card-border);
      padding: 8px 14px;
      border-radius: 8px;
      font-size: 0.88rem;
      font-weight: 500;
      cursor: pointer;
      transition: all 0.15s;
    }
    button:hover { background: #475569; }
    .btn-primary { background: var(--primary); color: #0f172a; border-color: var(--primary); font-weight: 600; }
    .btn-primary:hover { background: var(--primary-hover); }
    .btn-danger { background: #dc2626; border-color: #ef4444; }
    .btn-danger:hover { background: #b91c1c; }
    .btn-amber { background: #d97706; border-color: #f59e0b; }
    .btn-amber:hover { background: #b45309; }
    .btn-success { background: #16a34a; border-color: #22c55e; }
    .btn-success:hover { background: #15803d; }
    .active-mode {
      background: var(--primary) !important;
      color: #0f172a !important;
      border-color: var(--primary) !important;
      font-weight: bold;
    }

    /* Color picker & palette */
    .palette-row {
      display: flex;
      flex-wrap: wrap;
      gap: 8px;
      align-items: center;
    }
    .color-btn {
      width: 32px;
      height: 32px;
      border-radius: 8px;
      border: 2px solid #475569;
      cursor: pointer;
      padding: 0;
    }
    .color-btn:hover { transform: scale(1.1); border-color: #fff; }
    input[type="color"] {
      width: 38px;
      height: 34px;
      border-radius: 8px;
      border: 1px solid var(--card-border);
      background: none;
      cursor: pointer;
      padding: 0;
    }
    input[type="range"] {
      width: 100%;
      accent-color: var(--primary);
      cursor: pointer;
    }
    select, input[type="text"], input[type="password"], input[type="number"] {
      background: #0f172a;
      border: 1px solid var(--card-border);
      border-radius: 8px;
      padding: 9px 12px;
      color: var(--text);
      font-size: 0.9rem;
      width: 100%;
      outline: none;
    }
    select:focus, input:focus {
      border-color: var(--primary);
    }
    .form-grid {
      display: grid;
      grid-template-columns: 1fr;
      gap: 14px;
    }
    @media (min-width: 600px) {
      .form-grid { grid-template-columns: 1fr 1fr; }
    }

    /* Spoolman Registration Card & Callouts */
    .spool-box {
      background: rgba(15, 23, 42, 0.7);
      border: 1px solid var(--primary);
      border-radius: 8px;
      padding: 12px 14px;
      display: flex;
      align-items: center;
      gap: 12px;
    }
    .reg-prompt-card {
      background: linear-gradient(135deg, rgba(245, 158, 11, 0.12), rgba(15, 23, 42, 0.9));
      border: 1px solid var(--warning);
      border-radius: var(--radius);
      padding: 16px;
      display: flex;
      flex-direction: column;
      gap: 12px;
    }
    .reg-header {
      display: flex;
      align-items: center;
      gap: 10px;
      color: #fef08a;
      font-weight: 600;
      font-size: 1rem;
    }
    .color-swatch {
      width: 18px;
      height: 18px;
      border-radius: 4px;
      border: 1px solid #fff;
      display: inline-block;
      flex-shrink: 0;
    }
    .toast {
      position: fixed;
      bottom: 20px;
      right: 20px;
      background: #0284c7;
      color: #fff;
      padding: 10px 18px;
      border-radius: 8px;
      box-shadow: 0 4px 12px rgba(0,0,0,0.5);
      font-size: 0.88rem;
      z-index: 9999;
      opacity: 0;
      transition: opacity 0.3s;
      pointer-events: none;
    }
    .toast.show { opacity: 1; }
  </style>
</head>
<body>
  <div class="container">
    <!-- Header -->
    <header>
      <div class="header-left">
        <h1><span>⚡ Snapmaker U1 RFID</span></h1>
        <p>mDNS: <strong>http://snapmaker_rfid.local</strong> | Host: <span id="hdr-ip">---</span></p>
      </div>
      <div>
        <span id="badge-state" class="badge badge-idle">IDLE</span>
      </div>
    </header>

    <!-- AP Mode Warning Banner (if in SoftAP mode) -->
    <div id="ap-banner" class="alert-banner" style="display:none;">
      <span>⚠️ <strong>Setup Hotspot Active:</strong> Connected to Access Point <code>Snapmaker-RFID-Setup</code>. Configure your home Wi-Fi and Snapmaker U1 IP in the <strong>Settings</strong> tab, then click <strong>Save & Reboot</strong>.</span>
    </div>

    <!-- Tab Navigation Bar -->
    <div class="tab-bar">
      <button class="tab-btn active" id="tab-btn-status" onclick="switchTab('status')">
        <span>📡 RFID & Status</span>
      </button>
      <button class="tab-btn" id="tab-btn-settings" onclick="switchTab('settings')">
        <span>⚙️ Settings</span>
      </button>
      <button class="tab-btn" id="tab-btn-led" onclick="switchTab('led')">
        <span>💡 LED Controls</span>
      </button>
      <button class="tab-btn" id="tab-btn-info" onclick="switchTab('info')">
        <span>ℹ️ Device Info</span>
      </button>
    </div>

    <!-- ========================================== -->
    <!-- TAB 1: RFID & SYSTEM STATUS (DEFAULT)       -->
    <!-- ========================================== -->
    <div id="tab-content-status" class="tab-content active">
      <!-- Unregistered Spool Quick-Action Card -->
      <div id="card-unregistered-prompt" class="reg-prompt-card" style="display:none;">
        <div class="reg-header">
          <span style="font-size:1.3rem;">🏷️</span>
          <div>
            <div><strong>New Spool Tag Detected:</strong> <code id="prompt-uid" style="color:#38bdf8; font-size:1.05rem;">--</code></div>
            <div style="font-size:0.85rem; color:var(--text-muted); font-weight:normal;">This RFID tag is not yet registered to a spool in Spoolman.</div>
          </div>
        </div>
        <div class="btn-row" style="margin-top:4px;">
          <a id="btn-open-spoolman" href="#" target="_blank" style="text-decoration:none;">
            <button class="btn-primary" type="button">🚀 Register in Spoolman ↗</button>
          </a>
          <button onclick="copyTagUid()" class="btn-amber">📋 Copy Tag UID</button>
          <button onclick="dismissUnregisteredPrompt()" style="background:transparent; border-color:var(--card-border);">Dismiss</button>
        </div>
      </div>

      <div class="grid grid-2">
        <!-- Status Card -->
        <div class="card">
          <div class="card-title">
            <span>RFID & System Status</span>
            <span class="badge badge-live">Live</span>
          </div>

          <div class="stat-row">
            <span class="stat-label">RC522 RFID Hardware</span>
            <span class="stat-value" id="val-rfid-hw">Checking...</span>
          </div>
          <div class="stat-row">
            <span class="stat-label">Current State</span>
            <span class="stat-value" id="val-state">IDLE</span>
          </div>
          <div class="stat-row">
            <span class="stat-label">Staged Tag UID</span>
            <span class="stat-value" id="val-uid">None</span>
          </div>
          <div class="stat-row">
            <span class="stat-label">Tag Card Type</span>
            <span class="stat-value" id="val-type">--</span>
          </div>
          <div class="stat-row">
            <span class="stat-label">Total Tags Scanned</span>
            <span class="stat-value" id="val-scans">0</span>
          </div>
          <div class="stat-row">
            <span class="stat-label">Armed Timeout</span>
            <span class="stat-value" id="val-timeout">--</span>
          </div>
          <div class="stat-row">
            <span class="stat-label">Moonraker Host</span>
            <span class="stat-value" id="val-moonraker">Checking...</span>
          </div>

          <!-- Spoolman Live Info (if identified) -->
          <div id="spoolman-box" class="spool-box" style="display:none;">
            <span id="spool-swatch" class="color-swatch"></span>
            <div style="flex:1;">
              <div><strong>Spool:</strong> <span id="spool-name">--</span></div>
              <a id="spool-link" href="#" target="_blank" style="font-size:0.8rem; color:var(--primary);">Open Spool in Spoolman ↗</a>
            </div>
          </div>
        </div>

        <!-- Filament Sensors & Quick Actions Card -->
        <div class="card">
          <div class="card-title">
            <span>Extruder Filament Switches</span>
            <span style="font-size:0.8rem; font-weight:normal; color:var(--text-muted);">Moonraker Sensors</span>
          </div>

          <!-- Filament Extruder Lanes E0-E3 -->
          <div class="sensor-grid">
            <div id="sensor-0" class="sensor-box empty">
              <div class="name">Extruder E0</div>
              <div class="status" id="sensor-0-text">Empty</div>
            </div>
            <div id="sensor-1" class="sensor-box empty">
              <div class="name">Extruder E1</div>
              <div class="status" id="sensor-1-text">Empty</div>
            </div>
            <div id="sensor-2" class="sensor-box empty">
              <div class="name">Extruder E2</div>
              <div class="status" id="sensor-2-text">Empty</div>
            </div>
            <div id="sensor-3" class="sensor-box empty">
              <div class="name">Extruder E3</div>
              <div class="status" id="sensor-3-text">Empty</div>
            </div>
          </div>

          <!-- Staging & State Simulation Buttons -->
          <div class="control-group" style="margin-top: 8px;">
            <span class="control-label">Debug & Test Actions:</span>
            <div class="btn-row">
              <button class="btn-amber" onclick="simulateArm()">Simulate Tag Scan</button>
              <button onclick="disarm()">Cancel / Disarm</button>
              <button onclick="refreshSensors()">Poll Sensors Now</button>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- ========================================== -->
    <!-- TAB 2: SETTINGS & CONFIGURATION            -->
    <!-- ========================================== -->
    <div id="tab-content-settings" class="tab-content">
      <div class="card">
        <div class="card-title">
          <span>Settings & Network Configuration</span>
          <span style="font-size:0.8rem; font-weight:normal; color:var(--text-muted);">Stored in Flash NVS</span>
        </div>

        <div class="form-grid">
          <div class="control-group">
            <label class="control-label" for="cfg-ssid">Wi-Fi Network Name (SSID):</label>
            <input type="text" id="cfg-ssid" placeholder="Home Wi-Fi Name">
          </div>
          <div class="control-group">
            <label class="control-label" for="cfg-pass">Wi-Fi Password:</label>
            <input type="password" id="cfg-pass" placeholder="•••••••• (Leave blank to keep unchanged)">
          </div>
          <div class="control-group">
            <label class="control-label" for="cfg-printer">Snapmaker U1 / Moonraker IP:</label>
            <input type="text" id="cfg-printer" placeholder="10.0.2.30">
          </div>
          <div class="control-group">
            <label class="control-label" for="cfg-port">Moonraker Port:</label>
            <input type="number" id="cfg-port" placeholder="7125" value="7125">
          </div>
          <div class="control-group">
            <label class="control-label" for="cfg-timeout">Staged Binding Timeout (seconds):</label>
            <input type="number" id="cfg-timeout" placeholder="120" value="120">
          </div>
          <div class="control-group">
            <label class="control-label" for="cfg-spoolman">Spoolman URL (optional):</label>
            <input type="text" id="cfg-spoolman" placeholder="http://10.0.2.2:7912">
          </div>
        </div>

        <div class="btn-row" style="margin-top: 12px;">
          <button class="btn-primary" onclick="saveConfig()">💾 Save Settings & Reboot</button>
          <button class="btn-danger" onclick="rebootDevice()">🔄 Restart ESP32</button>
        </div>
      </div>
    </div>

    <!-- ========================================== -->
    <!-- TAB 3: LED CONTROLS & VISUALIZER           -->
    <!-- ========================================== -->
    <div id="tab-content-led" class="tab-content">
      <div class="grid grid-2">
        <!-- 7-LED Visualizer (Angles: 30°, 90°, 150°, no text) -->
        <div class="card">
          <div class="card-title">
            <span>7-LED Visualizer</span>
            <span id="led-mode-indicator" class="badge badge-idle">AUTO VISUALS</span>
          </div>

          <div class="led-visual-container">
            <div class="led-svg-wrap">
              <svg viewBox="0 0 240 240" class="led-ring-svg">
                <!-- Outer guide ring -->
                <circle cx="120" cy="120" r="70" stroke="#334155" stroke-dasharray="4 4" fill="none" stroke-width="2"/>

                <!-- LED 1: +30° (Top-Right, cx: 155, cy: 59.4) -->
                <g class="led-node" id="node-1" onclick="selectLed(1)" title="LED 1 (+30°)">
                  <circle id="led-1" cx="155" cy="59.4" r="18" fill="#1e293b" stroke="#64748b" stroke-width="2"/>
                </g>

                <!-- LED 0: +90° (Right / 3 o'clock, cx: 190, cy: 120) -->
                <g class="led-node" id="node-0" onclick="selectLed(0)" title="LED 0 (+90°)">
                  <circle id="led-0" cx="190" cy="120" r="18" fill="#1e293b" stroke="#64748b" stroke-width="2"/>
                </g>

                <!-- LED 5: +150° (Bottom-Right, cx: 155, cy: 180.6) -->
                <g class="led-node" id="node-5" onclick="selectLed(5)" title="LED 5 (+150°)">
                  <circle id="led-5" cx="155" cy="180.6" r="18" fill="#1e293b" stroke="#64748b" stroke-width="2"/>
                </g>

                <!-- LED 4: -150° / 210° (Bottom-Left, cx: 85, cy: 180.6) -->
                <g class="led-node" id="node-4" onclick="selectLed(4)" title="LED 4 (-150°)">
                  <circle id="led-4" cx="85" cy="180.6" r="18" fill="#1e293b" stroke="#64748b" stroke-width="2"/>
                </g>

                <!-- LED 3: -90° / 270° (Left / 9 o'clock, cx: 50, cy: 120) -->
                <g class="led-node" id="node-3" onclick="selectLed(3)" title="LED 3 (-90°)">
                  <circle id="led-3" cx="50" cy="120" r="18" fill="#1e293b" stroke="#64748b" stroke-width="2"/>
                </g>

                <!-- LED 2: -30° / 330° (Top-Left, cx: 85, cy: 59.4) -->
                <g class="led-node" id="node-2" onclick="selectLed(2)" title="LED 2 (-30°)">
                  <circle id="led-2" cx="85" cy="59.4" r="18" fill="#1e293b" stroke="#64748b" stroke-width="2"/>
                </g>

                <!-- LED 6: Center (120, 120) -->
                <g class="led-node selected" id="node-6" onclick="selectLed(6)" title="LED 6 (Center)">
                  <circle id="led-6" cx="120" cy="120" r="23" fill="#1e293b" stroke="#38bdf8" stroke-width="3"/>
                </g>
              </svg>
            </div>
            <div style="font-size:0.8rem; color: var(--text-muted); text-align:center;">
              Positions: 30°, 90°, 150° | Click any circle to select
            </div>
          </div>
        </div>

        <!-- LED Settings & Color Palette -->
        <div class="card">
          <div class="card-title">
            <span>Color & Effect Controls</span>
          </div>

          <!-- Mode Toggle -->
          <div class="btn-row">
            <button id="btn-mode-auto" class="active-mode" onclick="setLedMode('auto')">Auto (System Visuals)</button>
            <button id="btn-mode-manual" onclick="setLedMode('manual')">Manual Debug</button>
          </div>

          <!-- LED Target & Color Controls -->
          <div class="control-group">
            <div style="display:flex; justify-content:space-between; align-items:center;">
              <span class="control-label">Target LED:</span>
              <select id="sel-led" onchange="onSelectChange(this.value)">
                <option value="-1">All 7 LEDs</option>
                <option value="6" selected>LED 6: Center</option>
                <option value="1">LED 1: 30° (Top-Right)</option>
                <option value="0">LED 0: 90° (Right)</option>
                <option value="5">LED 5: 150° (Bottom-Right)</option>
                <option value="4">LED 4: -150° (Bottom-Left)</option>
                <option value="3">LED 3: -90° (Left)</option>
                <option value="2">LED 2: -30° (Top-Left)</option>
              </select>
            </div>

            <div class="palette-row" style="margin-top: 6px;">
              <button class="color-btn" style="background:#000000;" title="Off" onclick="applyColor('#000000')"></button>
              <button class="color-btn" style="background:#ffffff;" title="White" onclick="applyColor('#ffffff')"></button>
              <button class="color-btn" style="background:#ef4444;" title="Red" onclick="applyColor('#ef4444')"></button>
              <button class="color-btn" style="background:#22c55e;" title="Green" onclick="applyColor('#22c55e')"></button>
              <button class="color-btn" style="background:#3b82f6;" title="Blue" onclick="applyColor('#3b82f6')"></button>
              <button class="color-btn" style="background:#f59e0b;" title="Amber" onclick="applyColor('#f59e0b')"></button>
              <button class="color-btn" style="background:#eab308;" title="Yellow" onclick="applyColor('#eab308')"></button>
              <button class="color-btn" style="background:#a855f7;" title="Purple" onclick="applyColor('#a855f7')"></button>
              <button class="color-btn" style="background:#06b6d4;" title="Cyan" onclick="applyColor('#06b6d4')"></button>
              <input type="color" id="custom-color" value="#38bdf8" onchange="applyColor(this.value)">
            </div>
          </div>

          <!-- Brightness Slider -->
          <div class="control-group">
            <div style="display:flex; justify-content:space-between;">
              <span class="control-label">Brightness Cap</span>
              <span class="stat-value" id="val-brightness">80 / 255</span>
            </div>
            <input type="range" id="rng-brightness" min="5" max="255" value="80" oninput="onBrightnessInput(this.value)" onchange="applyBrightness(this.value)">
          </div>

          <!-- Test Effects -->
          <div class="control-group">
            <span class="control-label">Test FX Animations:</span>
            <div class="btn-row">
              <button onclick="triggerFx('flash_green')">Triple Green Flash</button>
              <button onclick="triggerFx('flash_red')">Triple Red Flash</button>
              <button onclick="triggerFx('chase')">Ring Chase</button>
              <button onclick="triggerFx('breathe')">Center Pulse</button>
              <button onclick="applyColor('#000000')">All Off</button>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- ========================================== -->
    <!-- TAB 4: DEVICE INFO & DIAGNOSTICS          -->
    <!-- ========================================== -->
    <div id="tab-content-info" class="tab-content">
      <div class="card">
        <div class="card-title">Device & Network Information</div>
        <div class="grid grid-2">
          <div>
            <div class="stat-row"><span class="stat-label">WiFi SSID</span><span class="stat-value" id="val-ssid">--</span></div>
            <div class="stat-row"><span class="stat-label">IP Address</span><span class="stat-value" id="val-ip">--</span></div>
            <div class="stat-row"><span class="stat-label">mDNS Hostname</span><span class="stat-value"><a href="http://snapmaker_rfid.local" target="_blank" style="color:var(--primary);">http://snapmaker_rfid.local</a></span></div>
            <div class="stat-row"><span class="stat-label">Signal Strength (RSSI)</span><span class="stat-value" id="val-rssi">--</span></div>
            <div class="stat-row"><span class="stat-label">Free Heap Memory</span><span class="stat-value" id="val-heap">--</span></div>
          </div>
          <div>
            <div class="stat-row"><span class="stat-label">System Uptime</span><span class="stat-value" id="val-uptime">--</span></div>
            <div class="stat-row"><span class="stat-label">Microcontroller</span><span class="stat-value">ESP32 (WEMOS D1 Mini32)</span></div>
            <div class="stat-row"><span class="stat-label">RFID Reader Chip</span><span class="stat-value" id="val-rfid-chip">RC522 (Checking...)</span></div>
            <div class="stat-row"><span class="stat-label">LED Ring Geometry</span><span class="stat-value">6 Outer (30°, 90°, 150°), Center</span></div>
            <div class="stat-row"><span class="stat-label">Firmware</span><span class="stat-value">v1.2.0 (Tabbed UI)</span></div>
          </div>
        </div>

        <div class="btn-row" style="margin-top: 12px;">
          <button class="btn-danger" onclick="rebootDevice()">🔄 Restart ESP32</button>
        </div>
      </div>
    </div>
  </div>

  <div id="toast" class="toast">Tag UID copied to clipboard!</div>

  <script>
    let selectedLedIndex = 6;
    let isManualMode = false;
    let cachedSpoolmanUrl = "";
    let currentStagedUid = "";
    let promptDismissedForUid = "";

    function switchTab(tabId) {
      const tabs = ['status', 'settings', 'led', 'info'];
      tabs.forEach(t => {
        const btn = document.getElementById('tab-btn-' + t);
        const content = document.getElementById('tab-content-' + t);
        if (t === tabId) {
          if (btn) btn.classList.add('active');
          if (content) content.classList.add('active');
        } else {
          if (btn) btn.classList.remove('active');
          if (content) content.classList.remove('active');
        }
      });
    }

    function selectLed(idx) {
      selectedLedIndex = idx;
      document.getElementById('sel-led').value = idx;
      updateSvgSelection();
    }

    function onSelectChange(val) {
      selectedLedIndex = parseInt(val);
      updateSvgSelection();
    }

    function updateSvgSelection() {
      for (let i = 0; i <= 6; i++) {
        const node = document.getElementById('node-' + i);
        if (node) {
          if (i === selectedLedIndex || selectedLedIndex === -1) {
            node.classList.add('selected');
          } else {
            node.classList.remove('selected');
          }
        }
      }
    }

    async function setLedMode(mode) {
      await fetch('/api/led', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: new URLSearchParams({ mode: mode })
      });
      fetchStatus();
    }

    async function applyColor(hex) {
      await fetch('/api/led', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: new URLSearchParams({ color: hex, index: selectedLedIndex })
      });
      fetchStatus();
    }

    function onBrightnessInput(val) {
      document.getElementById('val-brightness').innerText = val + ' / 255';
    }

    async function applyBrightness(val) {
      await fetch('/api/led', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: new URLSearchParams({ brightness: val })
      });
      fetchStatus();
    }

    async function triggerFx(fxName) {
      await fetch('/api/led', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: new URLSearchParams({ action: fxName })
      });
      fetchStatus();
    }

    async function simulateArm() {
      await fetch('/api/action', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: new URLSearchParams({ action: 'arm' })
      });
      fetchStatus();
    }

    async function disarm() {
      await fetch('/api/action', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: new URLSearchParams({ action: 'disarm' })
      });
      fetchStatus();
    }

    async function refreshSensors() {
      await fetch('/api/action', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: new URLSearchParams({ action: 'refresh_sensors' })
      });
      fetchStatus();
    }

    function copyTagUid() {
      if (!currentStagedUid) return;
      navigator.clipboard.writeText(currentStagedUid).then(() => {
        showToast("UID " + currentStagedUid + " copied to clipboard!");
      }).catch(() => {
        prompt("Copy UID:", currentStagedUid);
      });
    }

    function showToast(msg) {
      const toast = document.getElementById('toast');
      toast.innerText = msg;
      toast.classList.add('show');
      setTimeout(() => toast.classList.remove('show'), 2500);
    }

    function dismissUnregisteredPrompt() {
      promptDismissedForUid = currentStagedUid;
      document.getElementById('card-unregistered-prompt').style.display = 'none';
    }

    async function saveConfig() {
      const ssid = document.getElementById('cfg-ssid').value.trim();
      const pass = document.getElementById('cfg-pass').value;
      const printer = document.getElementById('cfg-printer').value.trim();
      const port = document.getElementById('cfg-port').value.trim();
      const timeout = document.getElementById('cfg-timeout').value.trim();
      const spoolman = document.getElementById('cfg-spoolman').value.trim();

      const params = new URLSearchParams();
      if (ssid) params.append('ssid', ssid);
      if (pass) params.append('pass', pass);
      if (printer) params.append('printer_ip', printer);
      if (port) params.append('port', port);
      if (timeout) params.append('timeout', timeout);
      if (spoolman !== undefined) params.append('spoolman_url', spoolman);

      if (!confirm("Save settings to NVS and reboot the ESP32?")) return;

      try {
        const res = await fetch('/api/config', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: params
        });
        if (res.ok) {
          alert("Settings saved! ESP32 is restarting... Reconnect to Wi-Fi if changed.");
        }
      } catch (e) {
        alert("Reboot initiated. Reloading page in 8 seconds...");
      }
      setTimeout(() => location.reload(), 8000);
    }

    async function rebootDevice() {
      if (!confirm("Restart the ESP32?")) return;
      await fetch('/api/action', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: new URLSearchParams({ action: 'reboot' })
      });
      alert("Restarting... Reloading page in 6 seconds.");
      setTimeout(() => location.reload(), 6000);
    }

    async function loadConfig() {
      try {
        const res = await fetch('/api/config');
        if (!res.ok) return;
        const cfg = await res.json();
        if (cfg.ssid) document.getElementById('cfg-ssid').value = cfg.ssid;
        if (cfg.printer_ip) document.getElementById('cfg-printer').value = cfg.printer_ip;
        if (cfg.port) document.getElementById('cfg-port').value = cfg.port;
        if (cfg.timeout) document.getElementById('cfg-timeout').value = cfg.timeout;
        if (cfg.spoolman_url) {
          document.getElementById('cfg-spoolman').value = cfg.spoolman_url;
          cachedSpoolmanUrl = cfg.spoolman_url;
        }
      } catch (e) {}
    }

    function formatUptime(sec) {
      const d = Math.floor(sec / 86400);
      const h = Math.floor((sec % 86400) / 3600);
      const m = Math.floor((sec % 3600) / 60);
      const s = sec % 60;
      if (d > 0) return `${d}d ${h}h ${m}m`;
      if (h > 0) return `${h}h ${m}m ${s}s`;
      return `${m}m ${s}s`;
    }

    async function fetchStatus() {
      try {
        const res = await fetch('/api/status');
        if (!res.ok) return;
        const data = await res.json();

        // Header and badges
        document.getElementById('hdr-ip').innerText = data.wifi_ip || '---';
        const badgeState = document.getElementById('badge-state');
        badgeState.innerText = data.state;
        if (data.is_ap_mode) {
          badgeState.className = 'badge badge-ap';
          badgeState.innerText = 'SETUP HOTSPOT';
          document.getElementById('ap-banner').style.display = 'flex';
        } else {
          badgeState.className = 'badge ' + (data.state === 'ARMED' ? 'badge-armed' : 'badge-idle');
          document.getElementById('ap-banner').style.display = 'none';
        }

        // Stats
        const rfidEl = document.getElementById('val-rfid-hw');
        const rfidChipEl = document.getElementById('val-rfid-chip');
        if (rfidEl && data.rfid_version) {
          if (data.rfid_connected) {
            rfidEl.innerText = 'Connected (' + data.rfid_version + ')';
            rfidEl.style.color = '#22c55e';
            if (rfidChipEl) rfidChipEl.innerText = 'NXP MFRC522 (' + data.rfid_version + ')';
          } else {
            rfidEl.innerText = 'COMM ERROR (' + data.rfid_version + ')';
            rfidEl.style.color = '#ef4444';
            if (rfidChipEl) rfidChipEl.innerText = 'Error / Offline';
          }
        }

        currentStagedUid = data.staged_uid || '';
        document.getElementById('val-state').innerText = data.state;
        document.getElementById('val-uid').innerText = data.staged_uid || 'None';
        document.getElementById('val-type').innerText = data.staged_type || '--';
        document.getElementById('val-scans').innerText = data.rfid_scans !== undefined ? data.rfid_scans : '--';
        document.getElementById('val-timeout').innerText = data.state === 'ARMED' ? (data.armed_remaining_s + 's') : '--';
        document.getElementById('val-moonraker').innerText = data.moonraker_ok ? 'Connected (OK)' : 'Unreachable / Error';
        document.getElementById('val-moonraker').style.color = data.moonraker_ok ? '#22c55e' : '#ef4444';

        if (data.spoolman_url) cachedSpoolmanUrl = data.spoolman_url;

        // Unregistered Tag Registration Prompt
        const promptCard = document.getElementById('card-unregistered-prompt');
        const hasActiveTag = (data.state === 'ARMED' || (data.staged_uid && data.staged_uid.length > 0));
        const notFoundInSpoolman = !data.spool_info || data.spool_info === '';

        if (hasActiveTag && notFoundInSpoolman && currentStagedUid !== promptDismissedForUid) {
          promptCard.style.display = 'flex';
          document.getElementById('prompt-uid').innerText = currentStagedUid;

          const baseSpoolUrl = (cachedSpoolmanUrl || 'http://10.0.2.2:7912').replace(/\/+$/, '');
          // Spoolman direct deep links: search or add spool with tag prefill
          const regLink = document.getElementById('btn-open-spoolman');
          if (regLink) {
            regLink.href = `${baseSpoolUrl}/?search=${encodeURIComponent(currentStagedUid)}`;
          }
        } else {
          promptCard.style.display = 'none';
        }

        // Spoolman preview (known spool)
        const spoolBox = document.getElementById('spoolman-box');
        if (data.spool_info) {
          spoolBox.style.display = 'flex';
          document.getElementById('spool-name').innerText = data.spool_info;
          const swatch = document.getElementById('spool-swatch');
          if (data.spool_color) {
            swatch.style.background = data.spool_color;
            swatch.style.display = 'inline-block';
          } else {
            swatch.style.display = 'none';
          }
          const link = document.getElementById('spool-link');
          const baseSpoolUrl = (cachedSpoolmanUrl || 'http://10.0.2.2:7912').replace(/\/+$/, '');
          if (data.spool_id) {
            link.href = `${baseSpoolUrl}/spool/show/${data.spool_id}`;
          } else {
            link.href = `${baseSpoolUrl}/?search=${encodeURIComponent(data.staged_uid || '')}`;
          }
          link.style.display = 'inline-block';
        } else {
          spoolBox.style.display = 'none';
        }

        // Sensors
        if (data.sensors) {
          for (let i = 0; i < 4; i++) {
            const sBox = document.getElementById('sensor-' + i);
            const sTxt = document.getElementById('sensor-' + i + '-text');
            if (sBox && sTxt && data.sensors[i] !== undefined) {
              const detected = data.sensors[i].detected;
              if (detected) {
                sBox.className = 'sensor-box detected';
                sTxt.innerText = 'Filament Present';
              } else {
                sBox.className = 'sensor-box empty';
                sTxt.innerText = 'Empty';
              }
            }
          }
        }

        // LED Visuals update
        isManualMode = data.manual_led_mode;
        const modeBadge = document.getElementById('led-mode-indicator');
        const btnAuto = document.getElementById('btn-mode-auto');
        const btnMan = document.getElementById('btn-mode-manual');
        if (isManualMode) {
          modeBadge.innerText = 'MANUAL DEBUG';
          modeBadge.className = 'badge badge-armed';
          btnAuto.classList.remove('active-mode');
          btnMan.classList.add('active-mode');
        } else {
          modeBadge.innerText = 'AUTO VISUALS';
          modeBadge.className = 'badge badge-idle';
          btnAuto.classList.add('active-mode');
          btnMan.classList.remove('active-mode');
        }

        if (data.led_brightness !== undefined) {
          document.getElementById('rng-brightness').value = data.led_brightness;
          document.getElementById('val-brightness').innerText = data.led_brightness + ' / 255';
        }

        if (data.leds && data.leds.length === 7) {
          for (let i = 0; i < 7; i++) {
            const circle = document.getElementById('led-' + i);
            if (circle) {
              const color = data.leds[i];
              circle.setAttribute('fill', color);
              if (color !== '#000000') {
                circle.style.filter = `drop-shadow(0 0 8px ${color})`;
              } else {
                circle.style.filter = 'none';
              }
            }
          }
        }

        // Network stats
        document.getElementById('val-ssid').innerText = data.wifi_ssid || '--';
        document.getElementById('val-ip').innerText = data.wifi_ip || '--';
        document.getElementById('val-rssi').innerText = (data.wifi_rssi || '--') + ' dBm';
        document.getElementById('val-heap').innerText = Math.round((data.free_heap || 0) / 1024) + ' KB';
        document.getElementById('val-uptime').innerText = formatUptime(data.uptime_s || 0);

      } catch (err) {
        console.error("Status fetch error:", err);
      }
    }

    // Initial fetch and loop
    fetchStatus();
    loadConfig();
    setInterval(fetchStatus, 1000);
  </script>
</body>
</html>
)rawliteral";
