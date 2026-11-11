# Snapmaker U1 Headless RFID Spool Manager (Paxx 2.0.x + Spoolman)

An ESP32-based contactless RFID scanner that automatically binds filament spools to the Snapmaker U1 running Paxx 2.0.x firmware and Spoolman. 

Instead of manual UI selection, this device uses an automated **staged binding** workflow: tap a spool tag to arm the scanner, then insert the filament into any extruder lane (E0–E3). The ESP32 detects the triggered filament runout switch via Moonraker and registers the tag directly to that channel.

---

## 1. System Architecture & Workflow

[Tap Spool Tag on RC522]
│
▼
[ESP32 Caches UID & Arms 120s Window]
(NeoPixel: Amber center pulse + outer ring chase)
│
▼
[Insert Filament into Lane E0, E1, E2, or E3]
│
▼
[Switch Transitions: filament_detected: false -> true]
│
▼
[ESP32 Detects Edge via Moonraker API Polling]
│
▼
[POST /printer/filament_detect/set to Paxx Moonraker Port 7125]
│
▼
[Paxx Resolves Spool via Spoolman & Links Lane]
(NeoPixel: 3x Green Flash -> Reverts to Idle Breathing)


---

## 2. Hardware Bill of Materials & Pinout

### Components
* **MCU:** ESP32 D1 Mini (MH-ET LIVE Mini32 / Wemos D1 Mini32 form factor)
* **RFID Reader:** RC522 13.56 MHz RFID module (SPI)
* **Visual Indicator:** 7-LED WS2812B NeoPixel disc (LED 6 center, LEDs 0–5 perimeter)

#### LED Ring Index & Geometry (0° is UP / 12 o'clock)
* **LED 6:** Center
* **LED 1:** +30° (Top-Right)
* **LED 0:** +90° (Right / 3 o'clock)
* **LED 5:** +150° (Bottom-Right)
* **LED 4:** -150° / 210° (Bottom-Left)
* **LED 3:** -90° / 270° (Left / 9 o'clock)
* **LED 2:** -30° / 330° (Top-Left)
*(Clockwise perimeter order: 1 → 0 → 5 → 4 → 3 → 2)*

### Wiring Table

| Peripheral | Module Pin | ESP32 D1 Mini Pin | GPIO | Function / Notes |
| :--- | :--- | :--- | :--- | :--- |
| **RC522 RFID** | **3V3** | `3V3` | — | **3.3V Only** (5V will destroy the IC) |
| | **RST** | `IO4` | GPIO 4 | Hardware Reset |
| | **GND** | `GND` | — | Ground Rail |
| | **MISO** | `IO19` / `D6` | GPIO 19 | VSPI MISO |
| | **MOSI** | `IO23` / `D7` | GPIO 23 | VSPI MOSI |
| | **SCK** | `IO18` / `D5` | GPIO 18 | VSPI Clock |
| | **SDA (SS)** | `IO22` / `D1` | GPIO 22 | VSPI Chip Select (CS) - Moved from GPIO 5 |
| | **IRQ** | *N/C* | — | Unconnected |
| **WS2812B LED** | **5V / VCC** | `5V` / `VBUS` | — | **5V Rail** (Powered directly from USB bus) |
| | **GND** | `GND` | — | Ground Rail |
| | **DI / IN** | `IO26` | GPIO 26 | Data In (3.3V logic compatible) |

---

## 3. Snapmaker U1 / Paxx Firmware Configuration

1. **Enable Advanced Mode:** On the printer touchscreen, go to `Settings` > `Maintenance` > `Advanced Mode`.
2. **Access Web Config:** Navigate to `http://<PRINTER_IP>/firmware-config/`.
3. **Configure RFID Mode:**
   * Set **RFID Detection System** to `External` (disables internal coils and accepts network API scans).
4. **Configure Spoolman:**
   * Set **Spoolman Integration** to `Enabled`.
   * Set **Spoolman URL** to your Spoolman instance (e.g., `http://10.0.2.2:7912`).
   * Set **Spoolman Tag Storage** to `native` (Spoolman v0.27.0+) or `card_uids` (legacy extra field).
5. **Verify Sensor Names:** In Moonraker (`http://10.0.2.30:7125/printer/objects/list`), the filament switches match the array in firmware:
   * `filament_motion_sensor e0_filament`
   * `filament_motion_sensor e1_filament`
   * `filament_motion_sensor e2_filament`
   * `filament_motion_sensor e3_filament`

---

## 4. Moonraker API Endpoints Used

* **Polling Switch State:**
  ```http
GET http://<PRINTER_IP>:7125/printer/objects/query?filament_switch_sensor%20e0_sensor&filament_switch_sensor%20e1_sensor...
```

* **Registering Scanned Spool:**
```http
POST http://<PRINTER_IP>:7125/printer/filament_detect/set
Content-Type: application/json

{
  "channel": 0,
  "info": {
    "CARD_UID": "04A1B2C3",
    "CARD_TYPE": "M1"
  }
}
```

(Channel 0–3 maps to lanes E0–E3. CARD_TYPE is `"M1"` for Mifare Classic 1K or `"NTAG"` for NTAG213/215/216).

---

## 5. PlatformIO Configuration (`platformio.ini`)

```ini
[env:wemos_d1_mini32]
platform = espressif32
board = wemos_d1_mini32
framework = arduino
monitor_speed = 115200
upload_speed = 921600

lib_deps =
    bblanchon/ArduinoJson@^7.0.0
    miguelbalboa/MFRC522@^1.4.11
    adafruit/Adafruit NeoPixel@^1.12.0
```

---

## 6. LED Feedback Reference

* **Idle (Normal Operation):** Center LED 6 breathes slowly in soft white.
* **Armed (Tag Buffered, Awaiting Load):** Center LED 6 pulses solid amber; perimeter LEDs run a circular clockwise chase pattern (`1 → 0 → 5 → 4 → 3 → 2`).
* **Setup Hotspot Mode (Fallback AP):** Center LED 6 pulses in cyan/blue while broadcasting `Snapmaker-RFID-Setup`.
* **Connecting to Wi-Fi:** Center LED 6 pulses purple/magenta.
* **Success (Assigned to Extruder):** Entire 7-LED array flashes green 3 times.
* **Timeout / Error:** Entire array flashes red (2 flashes for 120s timeout expiration, 3 flashes for Moonraker HTTP failure).

---

## 7. Web Dashboard & Configuration (`http://snapmaker_rfid.local`)

The ESP32 broadcasts via mDNS and hosts a built-in web dashboard on port 80:

* **mDNS URL:** [http://snapmaker_rfid.local](http://snapmaker_rfid.local)
* **Direct IP:** Accessible directly via the assigned Wi-Fi IP address.
* **Fallback Setup Hotspot:** If Wi-Fi fails to connect (or is unconfigured), the device automatically spins up a setup Access Point named **`Snapmaker-RFID-Setup`** at **`http://192.168.4.1`**.

### Features
* **NVS Settings Configuration:** Update Wi-Fi SSID, Wi-Fi password, Snapmaker U1 IP, Moonraker port, Spoolman URL, and armed timeout directly from the web interface without re-flashing.
* **Spoolman Integration:** Automatically queries Spoolman for the tapped tag's material, name, and color swatch, with a one-click link to view the spool in Spoolman.
* **Live System & Spool Status:** Real-time state machine display (`IDLE` vs `ARMED` with remaining countdown), staged tag UID, tag type, and Moonraker connectivity.
* **Lane Sensor Indicators:** Live monitoring of all 4 extruder filament switches (`E0`–`E3`) to verify switch closures in real time.
* **7-LED Visual Debugger:**
  * Exact interactive SVG diagram matching the physical layout (Center LED 6, perimeter LEDs 0–5).
  * Color picker & quick palettes for individual or all LEDs.
  * Brightness slider.
  * One-click pattern test triggers (**Clockwise Chase**, **Center Pulse**, **Green Flash**, **Red Flash**).
  * Mode toggle between **Auto (System Visuals)** and **Manual Debug Mode**.
* **Simulate & Test Controls:** Trigger a simulated RFID tag tap or disarm/cancel without needing a physical card.

### REST API Endpoints
* `GET /` — Responsive web dashboard.
* `GET /api/status` — JSON telemetry (system state, sensors, current LED hex values, Spoolman info, Wi-Fi stats, heap).
* `GET /api/config` — Retrieve current stored configuration from NVS.
* `POST /api/config` — Save network and printer settings to NVS and schedule a restart.
* `POST /api/led` — Control manual LED colors, brightness, mode, and test animations.
* `POST /api/action` — Trigger simulation actions (`arm`, `disarm`, `refresh_sensors`, `reboot`).