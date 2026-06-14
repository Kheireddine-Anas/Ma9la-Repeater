# ma9la-repeater

> An ESP32-S3 Wi-Fi repeater with a dark web portal, NAT internet sharing, DNS fix, multi-network management, live Wi-Fi scanning, connected clients list, hotspot editor, and portal authentication — no router access needed.

---

## About This Version

This is **v2.1** — a major continuation of the original repeater project already published on this GitHub.

The original version was a proof of concept: connect to Wi-Fi, share it, done. **v2.1 turns it into a proper tool:**

| | v1 (original) | v2.1 (this) |
|---|---|---|
| Networks saved | 1 (hardcoded) | Up to 10, switchable from portal |
| Web portal | Basic | Dark cyberpunk, fully async |
| Wi-Fi scan | ✗ | ✓ with RSSI bars |
| Connected clients | ✗ | ✓ with MAC + signal |
| Hotspot editor | ✗ | ✓ (name + password, no reflash) |
| Portal auth | ✗ | ✓ HTTP Basic Auth |
| Uptime / reboot counter | ✗ | ✓ |
| Auto-refresh after reboot | ✗ | ✓ |
| LED scan feedback | ✗ | ✓ purple during scan |

Same idea, completely rewritten.

---

## What It Does

Turn any ESP32-S3 into a proper Wi-Fi repeater:

- Connects to an existing Wi-Fi network (your router)
- Broadcasts its own hotspot (`Ma9la-NET` by default)
- Shares the internet to all devices connected to that hotspot via NAT
- Fixes DNS automatically so browsers actually work (not just pings)
- Hosts a dark cyberpunk web portal at `http://192.168.4.1` — no app, no serial monitor needed
- Saves up to **10 Wi-Fi networks** and lets you switch between them from the portal
- **Live Wi-Fi scan** — see nearby networks with signal strength directly in the browser
- Shows **signal strength (RSSI)** for the connected router in real time
- Shows all **devices connected to the hotspot** with their MAC address and signal
- **Editable hotspot name and password** from the portal (no reflash needed)
- **Portal password protection** — optional HTTP Basic Auth to lock the config page
- **Uptime and reboot counter** shown on the status card
- **Auto-refresh after reboot** — the browser reloads automatically when the ESP32 comes back up
- RGB LED signals every state including a **purple flash during Wi-Fi scan**

---

## Hardware

| Part | Details |
|---|---|
| Board | ESP32-S3 (with built-in RGB LED) |
| USB cable | For flashing only |
| Power | USB or any 5V source after flashing |
| Reboot button | Via the web portal (no physical button needed) |

---

## Two-File Sketch — Important

This sketch uses **two files** that must live in the same folder:

```
ma9la-repeater/
├── ma9la-repeater.ino   ← main firmware (open this in Arduino IDE)
└── index_html.h         ← the web portal HTML/CSS/JS
```

**Why two files?**
The Arduino IDE preprocesses `.ino` files before passing them to the C++ compiler. That preprocessor does not understand C++11 raw string literals (`R"EOF(...)EOF"`), so embedding the full HTML portal inside the `.ino` causes the JavaScript to be misread as C++ code and fails to compile.  
`.h` files are handed straight to the real C++ compiler, which handles raw strings correctly.

**How to use:**
1. Download or clone both files into the same folder named `ma9la-repeater`
2. Open `ma9la-repeater.ino` in Arduino IDE — it will automatically detect `index_html.h` because it is in the same folder
3. Flash normally — no extra steps needed

> Do **not** move or rename `index_html.h`. If the file is missing, the sketch will not compile.

---

## Arduino IDE Setup

> **These settings are required — skipping them will break NAT (no internet sharing).**

1. Add the ESP32 board manager URL in **File → Preferences → Additional boards manager URLs**:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
2. Install **esp32 by Espressif Systems** via **Tools → Board → Boards Manager**
3. Select your board and settings under **Tools**:

| Setting | Value |
|---|---|
| Board | ESP32S3 Dev Module |
| Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |
| USB CDC On Boot | Enabled |

4. Open `ma9la-repeater.ino`, select your COM port, and flash.

---

## How It Works

```
[ Your Router ] ──WiFi──► [ ESP32-S3 STA interface ]
                                     │
                                 NAT + DNS fix
                                     │
                          [ ESP32-S3 AP interface ]
                                     │
                           ──WiFi──► [ Your phone / laptop ]
```

1. **Boot** — ESP32 reads the active network credentials from flash memory
2. **Connect** — ESP32 joins your router as a regular client (STA mode)
3. **Broadcast** — ESP32 starts its own hotspot (AP mode) on the same channel
4. **NAT** — Every packet from hotspot clients gets translated and forwarded to the router
5. **DNS** — DHCP server is reconfigured to advertise `8.8.8.8` so domain names resolve correctly
6. **Web Portal** — Config portal runs at `http://192.168.4.1` to manage everything

---

## First-Time Setup

1. Flash the firmware
2. Connect to hotspot **`Ma9la-NET`** (password: `Ma9la1337`)
3. Open **`http://192.168.4.1`** in your browser
4. Use **Scan Networks** to find your router, or type its name manually
5. Enter the password → **Save & Connect**
6. The browser shows a spinner and auto-reloads once the ESP32 is back up
7. Reconnect to `Ma9la-NET` — you should now have full internet access

---

## Speed & Throughput

Real-world throughput through the repeater will be lower than your router's direct speed. This is expected and unavoidable:

- The ESP32 has a **single shared radio** — it cannot receive from the router and transmit to your device at the same time (half-duplex)
- Every byte crosses the Wi-Fi channel **twice** (router → ESP32, then ESP32 → your device)
- The theoretical ceiling is therefore around **40–50% of your base connection**, and software NAT overhead on the ESP32's CPU reduces this further
- Disabling Wi-Fi modem sleep (`WiFi.setSleep(false)`, already set in the firmware) helps reduce per-packet latency and improves sustained throughput slightly

If you need higher speed, this is a hardware limitation of using a single-radio chip as a repeater. A dual-radio AP/router will always outperform it.

---

## LED Status

| Color | Pattern | Meaning |
|---|---|---|
| Blue | Solid | Connecting to router |
| Purple | Solid (2–4 s) | Scanning for Wi-Fi networks |
| Green | Slow blink (3 s) | Connected — sharing internet |
| Red | Fast blink (0.5 s) | No internet / not configured |

---

## Web Portal

Accessible at `http://192.168.4.1` while connected to the hotspot.

### Status Card
- Live connection state with animated indicator dot
- Connected network name and **signal strength** shown as color-coded bars + exact dBm value
- ESP32 IP address and advertised DNS
- **Uptime** (hours : minutes : seconds since last boot)
- **Reboot counter** (total number of times the ESP32 has booted)

### Connected Clients
- Lists every device currently connected to the hotspot
- Shows **MAC address** and **signal strength** (RSSI) for each client
- Refresh button to update the list on demand

### Scan for Networks
- Tap **Scan Networks** — ESP32 actively scans; LED turns purple while scanning
- Results show: lock icon (encrypted), network name, signal bars, RSSI in dBm
- Tap any result to auto-fill the SSID field below

### Credentials
- Type a network name manually, or use auto-fill from scan
- Enter the password (leave blank for open networks)
- **Save & Connect** — saves the network (or updates its password if already saved) and reboots
- Browser auto-refreshes once the ESP32 is back online

### Saved Networks (up to 10)
- Lists all stored networks, with the currently active one highlighted in green
- **Use** — switch to a different saved network (reboots)
- **✕** — delete a single saved network (no reboot)
- **Forget All & Reboot** — wipes all saved networks and reboots into setup mode

### Hotspot Settings
- Change the **hotspot name (SSID)** and **password** without reflashing
- Changes take effect after reboot; you will need to reconnect to the new hotspot name

### Portal Authentication (HTTP Basic Auth)
The config portal can be password-protected so only you can access it:

1. Open the portal → scroll to **Hotspot Settings**
2. Check **"Require password to access portal"**
3. Set a **username** (default: `admin`) and a **portal password**
4. Hit **Save & Reboot Hotspot**

After reboot, any browser opening `http://192.168.4.1` will get a login prompt before seeing the portal.

> **If you forget the portal password**, flash the firmware again — this resets all NVS settings including auth credentials.  
> Authentication is disabled automatically if the portal password field is left blank.

---

## Serial Monitor

Baud rate: **115200**

Expected output when working correctly:
```
=== Ma9la-Repeater v2.1  (boot #3) ===
Arduino ESP32 core v3.x.x
[STA] Connecting to: YourRouterName
..........
[AP] Hotspot: Ma9la-NET  |  http://192.168.4.1
[DNS] DHCP reconfigured — clients will use 8.8.8.8
[STA] Connected. IP: 192.168.1.xx  RSSI: -58 dBm
[NAT] Internet sharing active.
[HTTP] Web server ready.
```

---

## API Endpoints

| Method | Path | Description |
|---|---|---|
| GET | `/` | Serve the web portal |
| GET | `/status` | JSON: connection state, SSID, RSSI, IP, uptime, boot count |
| GET | `/clients` | JSON: hotspot clients with MAC address and RSSI |
| GET | `/scan` | JSON: nearby networks with RSSI and encryption flag |
| GET | `/networks` | JSON: saved networks list with active flag |
| GET | `/ap-settings` | JSON: current hotspot name and auth config (no passwords) |
| POST | `/save` | Save network (`ssid`, `password`) and reboot |
| POST | `/connect` | Switch active network by `index` and reboot |
| POST | `/delete` | Delete saved network by `index` (no reboot) |
| POST | `/clear` | Wipe all saved networks and reboot |
| POST | `/ap-save` | Update hotspot settings (`apSsid`, `apPass`, `authOn`, `authUser`, `authPass`) and reboot |

---

## Key Technical Decisions

- **SoftAP starts after STA connects** — ensures both interfaces share the same Wi-Fi channel, which is required for NAT to forward packets correctly
- **DNS fix via DHCP reconfiguration** — the default ESP32 DHCP server points clients to `192.168.4.1` for DNS, but the ESP32 runs no DNS server there. The fix stops the DHCP server, sets `8.8.8.8` as the advertised DNS, and restarts it before any client connects
- **Version-aware NAT** — detects ESP32 Arduino core v2.x vs v3.x at compile time and calls the correct NAT function (`ip_napt_enable` vs `esp_netif_napt_enable`)
- **`WiFi.setSleep(false)`** — disables Wi-Fi modem sleep at boot, reducing per-packet latency and improving sustained throughput through the repeater
- **Multi-network NVS storage** — up to 10 networks stored as indexed key-value pairs (`s0/p0` … `s9/p9`) with an `active` pointer; deletion shifts remaining entries to keep indices compact
- **Three NVS namespaces** — `wifi-cfg` for the network list, `ap-cfg` for hotspot/auth settings, `sys-cfg` for the boot counter; each can be cleared independently
- **Portal HTML in a separate `.h` file** — the Arduino IDE preprocessor cannot handle C++11 raw string literals in `.ino` files (it tries to parse JavaScript as C++). Moving the HTML to `index_html.h` bypasses this — `.h` files go straight to the real C++ compiler
- **Static portal from PROGMEM** — the full HTML/CSS/JS portal is stored in flash via `PROGMEM` and served with `send_P`, leaving RAM free for Wi-Fi and NAT operations
- **Async-friendly portal** — status, scan results, clients, and saved networks are fetched via separate JSON endpoints so the page updates live without full reloads
- **Auto-refresh on reboot** — reboot responses are full HTML pages with `<meta http-equiv="refresh" content="9;url=/">` so the browser reloads itself once the ESP32 is back up
- **Blocking scan with LED feedback** — `WiFi.scanNetworks()` is synchronous (~2–4 s); the LED is set to solid purple immediately before the call to give the user visual feedback while the page waits
- **HTTP Basic Auth** — uses the WebServer library's built-in `server.authenticate()` / `server.requestAuthentication()` so browsers handle the credential prompt natively

---

## Author

**Anas Kheireddine** — aka *Ma9la*
GitHub: [@Kheireddine-Anas](https://github.com/Kheireddine-Anas)
School: [1337 School](https://1337.ma)
