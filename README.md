# ma9la-repeater

> An ESP32-S3 Wi-Fi repeater with a built-in web config panel, NAT internet sharing, and DNS fix — no router access needed.


---

## What It Does

Turn any ESP32-S3 into a proper Wi-Fi repeater:

- Connects to an existing Wi-Fi network (your router)
- Broadcasts its own hotspot (`Ma9la-NET`)
- Shares the internet to all devices connected to that hotspot via NAT
- Fixes DNS automatically so browsers actually work (not just pings)
- Hosts a web config page at `http://192.168.4.1` — no app, no serial monitor needed

---

## Hardware

| Part | Details |
|---|---|
| Board | ESP32-S3 (with built-in RGB LED) |
| USB cable | For flashing only |
| Power | USB or any 5V source after flashing |

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
| **lwIP Variant** | **Large (No SSL)** ← critical for NAT |
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

1. **Boot** — ESP32 reads saved credentials from flash memory
2. **Connect** — ESP32 joins your router as a regular client (STA mode)
3. **Broadcast** — ESP32 starts its own hotspot (AP mode) on the same channel
4. **NAT** — Every packet from hotspot clients gets translated and forwarded to the router
5. **DNS** — DHCP server is reconfigured to advertise `8.8.8.8` so domain names resolve correctly
6. **Web UI** — A config page runs at `http://192.168.4.1` to save/change/forget the target Wi-Fi

---

## First-Time Setup

1. Flash the firmware
2. Connect to hotspot **`Ma9la-NET`** (password: `1337ma9la`)
3. Open **`http://192.168.4.1`** in your browser
4. Enter your router's Wi-Fi name and password → **Save & Share Internet**
5. ESP32 reboots, connects, and starts sharing internet
6. Reconnect to `Ma9la-NET` — you should now have full internet access

---

## LED Status

| Color | Pattern | Meaning |
|---|---|---|
| Blue | Solid | Connecting to router |
| Green | Slow blink (3s) | Connected — sharing internet |
| Red | Fast blink (0.5s) | No internet / not configured |

---

## Web Config Page

Accessible at `http://192.168.4.1` while connected to the hotspot.

- **Save & Share Internet** — enter a new router SSID/password and reboot
- **Forget Target Wi-Fi** — clears saved credentials and reboots into setup mode

---

## Serial Monitor

Baud rate: **115200**

Expected output when working correctly:
```
=== ESP32-S3 Wi-Fi Repeater ===
Connecting to router: YourRouterName
..........
Hotspot started: Ma9la-NET  |  http://192.168.4.1
[DNS] DHCP server reconfigured — clients will use 8.8.8.8
Connected to router. ESP32 IP: 192.168.1.xx
NAT enabled — internet is being shared.
Web server ready.
```

---

## Key Technical Decisions

- **SoftAP starts after STA connects** — this ensures both interfaces share the same Wi-Fi channel, which is required for NAT to forward packets correctly
- **DNS fix via DHCP reconfiguration** — the default ESP32 DHCP server points clients to `192.168.4.1` for DNS, but the ESP32 runs no DNS server there. The fix stops the DHCP server, sets `8.8.8.8` as the advertised DNS, and restarts it before any client connects
- **Version-aware NAT** — the code detects ESP32 Arduino core v2.x vs v3.x at compile time and calls the correct NAT function (`ip_napt_enable` vs `esp_netif_napt_enable`)
- **Credentials stored in NVS flash** — survives power cycles; cleared cleanly via the web UI

---

## Author

**Anas Kheireddine** — aka *Ma9la*
GitHub: [@Kheireddine-Anas](https://github.com/Kheireddine-Anas)
School: [1337 School](https://1337.ma)
