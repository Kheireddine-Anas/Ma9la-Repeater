/*
 * Ma9la-Repeater — ESP32-S3 Wi-Fi Repeater with NAT
 * v2.1: multi-network (×10), scan, signal strength, clients list,
 *       AP editor, portal auth, uptime/reboot counter, auto-refresh on reboot
 *
 * ARDUINO IDE SETUP (required before flashing):
 *   1. Tools > Board              → ESP32S3 Dev Module
 *   2. Tools > Partition Scheme   → Huge APP (3MB No OTA/1MB SPIFFS)
 *   3. Tools > lwIP Variant       → "Large (No SSL)"  ← enables NAT
 *   4. Tools > USB CDC On Boot    → Enabled
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "esp_netif.h"
#include "esp_wifi.h"
#include "dhcpserver/dhcpserver.h"

// Version-aware NAT support
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  #define ENABLE_NAT() do { \
    esp_netif_t *_n = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"); \
    if (_n) esp_netif_napt_enable(_n); \
  } while(0)
#else
  #include "lwip/lwip_napt.h"
  #define ENABLE_NAT() ip_napt_enable(WiFi.softAPIP(), 1)
#endif

// ─── Defaults (overridden at boot from NVS) ───────────────────────────────────
const char* DEFAULT_AP_SSID  = "Ma9la-NET";
const char* DEFAULT_AP_PASS  = "Ma9la1337";
const int   MAX_NETWORKS     = 10;

// ──────────────────────────────────────────────────────────────────────────────

WebServer   server(80);
Preferences preferences;

// Runtime config loaded from NVS on boot
String apSsid;
String apPass;
String authUser;
String authPass;
bool   authEnabled = false;

unsigned long lastBlinkTime = 0;
bool          ledState       = false;
bool          isScanning     = false;

enum SystemStatus { CONNECTING_BLUE, SCANNING_PURPLE, REPEATER_GREEN, NO_INTERNET_RED };
SystemStatus currentStatus = NO_INTERNET_RED;

// ─── Utility ──────────────────────────────────────────────────────────────────

String jsonEsc(const String& s) {
  String o;
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s[i];
    if      (c == '"')  o += "\\\"";
    else if (c == '\\') o += "\\\\";
    else                o += c;
  }
  return o;
}

String formatUptime() {
  unsigned long s = millis() / 1000;
  unsigned long m = s / 60;
  unsigned long h = m / 60;
  char buf[16];
  sprintf(buf, "%luh %02lum %02lus", h, m % 60, s % 60);
  return String(buf);
}

// ─── AP / Auth config (NVS namespace: "ap-cfg") ───────────────────────────────

void loadApConfig() {
  preferences.begin("ap-cfg", true);
  apSsid      = preferences.getString("apSsid",   DEFAULT_AP_SSID);
  apPass      = preferences.getString("apPass",   DEFAULT_AP_PASS);
  authEnabled = preferences.getBool("authOn",     false);
  authUser    = preferences.getString("authUser", "admin");
  authPass    = preferences.getString("authPass", "");
  preferences.end();
}

void saveApConfig(const String& ssid, const String& pass,
                  bool authOn, const String& user, const String& apass) {
  preferences.begin("ap-cfg", false);
  preferences.putString("apSsid",   ssid);
  preferences.putString("apPass",   pass);
  preferences.putBool("authOn",     authOn);
  preferences.putString("authUser", user);
  preferences.putString("authPass", apass);
  preferences.end();
}

// ─── Boot counter (NVS namespace: "sys-cfg") ──────────────────────────────────

uint32_t incrementBootCount() {
  preferences.begin("sys-cfg", false);
  uint32_t c = preferences.getUInt("boots", 0) + 1;
  preferences.putUInt("boots", c);
  preferences.end();
  return c;
}

uint32_t getBootCount() {
  preferences.begin("sys-cfg", true);
  uint32_t c = preferences.getUInt("boots", 0);
  preferences.end();
  return c;
}

// ─── Network list (NVS namespace: "wifi-cfg") ─────────────────────────────────

int getNetworkCount() {
  preferences.begin("wifi-cfg", true);
  int c = preferences.getInt("count", 0);
  preferences.end();
  return c;
}

String getNetworkSSID(int idx) {
  preferences.begin("wifi-cfg", true);
  String v = preferences.getString(("s" + String(idx)).c_str(), "");
  preferences.end();
  return v;
}

String getNetworkPass(int idx) {
  preferences.begin("wifi-cfg", true);
  String v = preferences.getString(("p" + String(idx)).c_str(), "");
  preferences.end();
  return v;
}

int getActiveIndex() {
  preferences.begin("wifi-cfg", true);
  int v = preferences.getInt("active", -1);
  preferences.end();
  return v;
}

// Returns saved index, or -1 if full. Updates password if SSID already exists.
int saveNetwork(const String& ssid, const String& pass) {
  preferences.begin("wifi-cfg", false);
  int count = preferences.getInt("count", 0);
  for (int i = 0; i < count; i++) {
    if (preferences.getString(("s" + String(i)).c_str(), "") == ssid) {
      preferences.putString(("p" + String(i)).c_str(), pass);
      preferences.putInt("active", i);
      preferences.end();
      return i;
    }
  }
  if (count >= MAX_NETWORKS) { preferences.end(); return -1; }
  preferences.putString(("s" + String(count)).c_str(), ssid);
  preferences.putString(("p" + String(count)).c_str(), pass);
  preferences.putInt("count", count + 1);
  preferences.putInt("active", count);
  preferences.end();
  return count;
}

void removeNetwork(int idx) {
  preferences.begin("wifi-cfg", false);
  int count  = preferences.getInt("count", 0);
  int active = preferences.getInt("active", -1);
  if (idx < 0 || idx >= count) { preferences.end(); return; }
  for (int i = idx; i < count - 1; i++) {
    preferences.putString(("s" + String(i)).c_str(),
                          preferences.getString(("s" + String(i+1)).c_str(), ""));
    preferences.putString(("p" + String(i)).c_str(),
                          preferences.getString(("p" + String(i+1)).c_str(), ""));
  }
  preferences.remove(("s" + String(count-1)).c_str());
  preferences.remove(("p" + String(count-1)).c_str());
  count--;
  preferences.putInt("count", count);
  if      (active == idx) preferences.putInt("active", count > 0 ? 0 : -1);
  else if (active > idx)  preferences.putInt("active", active - 1);
  preferences.end();
}

// ─── Auth Gate ────────────────────────────────────────────────────────────────

bool checkAuth() {
  if (!authEnabled || authPass.length() == 0) return true;
  if (!server.authenticate(authUser.c_str(), authPass.c_str())) {
    server.requestAuthentication(BASIC_AUTH, "Ma9la-NET Portal");
    return false;
  }
  return true;
}

// ─── DNS Fix ──────────────────────────────────────────────────────────────────

void fixAPDns() {
  esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
  if (!ap_netif) { Serial.println("[DNS] Could not get AP netif handle."); return; }
  esp_netif_dhcps_stop(ap_netif);
  esp_netif_dns_info_t dns;
  memset(&dns, 0, sizeof(dns));
  dns.ip.u_addr.ip4.addr = ipaddr_addr("8.8.8.8");
  dns.ip.type = IPADDR_TYPE_V4;
  esp_err_t err = esp_netif_set_dns_info(ap_netif, ESP_NETIF_DNS_MAIN, &dns);
  if (err != ESP_OK)
    Serial.printf("[DNS] set_dns_info failed: %s\n", esp_err_to_name(err));
  dhcps_offer_t offer = OFFER_DNS;
  esp_netif_dhcps_option(ap_netif, ESP_NETIF_OP_SET,
                         ESP_NETIF_DOMAIN_NAME_SERVER, &offer, sizeof(offer));
  esp_netif_dhcps_start(ap_netif);
  Serial.println("[DNS] DHCP reconfigured — clients will use 8.8.8.8");
}

// ─── Reboot page with auto-refresh ───────────────────────────────────────────
// Sends a full HTML page. The browser auto-refreshes back to / after 9 s,
// giving the ESP ~3 s to reboot and ~6 s to reconnect and serve again.

void sendRebootPage(const String& msg, const char* color) {
  String html =
    "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<meta http-equiv='refresh' content='9;url=/'>"
    "<title>Ma9la-NET</title>"
    "<style>"
    "body{background:#060a14;margin:0;display:flex;align-items:center;"
    "justify-content:center;min-height:100vh;font-family:system-ui;text-align:center;padding:20px}"
    ".spin{width:40px;height:40px;border:3px solid #1a2a4a;"
    "border-top-color:" + String(color) + ";"
    "border-radius:50%;animation:s .8s linear infinite;margin:0 auto 18px}"
    "@keyframes s{to{transform:rotate(360deg)}}"
    ".msg{color:" + String(color) + ";font-size:1.05rem;font-weight:600;margin-bottom:10px}"
    ".sub{color:#64748b;font-size:.78rem;line-height:1.5}"
    "</style></head><body>"
    "<div><div class='spin'></div>"
    "<div class='msg'>" + msg + "</div>"
    "<div class='sub'>Rebooting ESP32&hellip;<br>This page will reload automatically in a few seconds.</div>"
    "</div></body></html>";
  server.sendHeader("Connection", "close");
  server.send(200, "text/html", html);
  delay(2000);
  ESP.restart();
}

// ─── HTML Portal ─────────────────────────────────────────────────────────────
// Kept in index_html.h — Arduino IDE preprocesses .ino files and cannot handle
// C++11 raw string literals (R"EOF(...)EOF") inside them. .h files are passed
// straight to the C++ compiler and work correctly.
#include "index_html.h"


// ─── Route Helpers ────────────────────────────────────────────────────────────

// Call at the start of every handler.
// 1. TCP_NODELAY — disables Nagle's algorithm so small responses go out
//    immediately instead of being buffered for up to 200 ms.
// 2. Connection: close — releases the socket right after the response so the
//    next request is not blocked waiting on a keep-alive timeout.
// 3. Cache-Control: no-cache — always fresh data.
void prepareResponse() {
  server.client().setNoDelay(true);
  server.sendHeader("Cache-Control", "no-cache");
  server.sendHeader("Connection", "close");
}

// ─── Route Handlers ───────────────────────────────────────────────────────────

void handleRoot() {
  if (!checkAuth()) return;
  prepareResponse();
  server.send_P(200, "text/html", INDEX_HTML);
}

// GET /init → single startup payload: status + saved networks + AP settings.
// Replaces three separate /status /networks /ap-settings calls on page load,
// cutting initial round-trips from 4 down to 2 (init + clients).
void handleInit() {
  if (!checkAuth()) return;
  int  active    = getActiveIndex();
  int  count     = getNetworkCount();
  bool connected = (WiFi.status() == WL_CONNECTED);
  String savedSSID = (active >= 0) ? getNetworkSSID(active) : "";

  String json = "{";
  json += "\"connected\":"  + String(connected ? "true" : "false");
  json += ",\"ssid\":\""    + jsonEsc(connected ? WiFi.SSID() : savedSSID) + "\"";
  json += ",\"rssi\":"      + String(connected ? WiFi.RSSI() : 0);
  json += ",\"ip\":\""      + String(connected ? WiFi.localIP().toString() : "") + "\"";
  json += ",\"uptime\":\""  + formatUptime() + "\"";
  json += ",\"boots\":"     + String(getBootCount());
  json += ",\"networks\":[";
  for (int i = 0; i < count; i++) {
    if (i > 0) json += ",";
    json += "{\"ssid\":\""  + jsonEsc(getNetworkSSID(i)) + "\"";
    json += ",\"index\":"   + String(i);
    json += ",\"active\":"  + String(i == active ? "true" : "false") + "}";
  }
  json += "]";
  json += ",\"apSsid\":\""  + jsonEsc(apSsid) + "\"";
  json += ",\"authOn\":"    + String(authEnabled ? "true" : "false");
  json += ",\"authUser\":\"" + jsonEsc(authUser) + "\"";
  json += "}";

  prepareResponse();
  server.send(200, "application/json", json);
}

// GET /status → JSON (used for 5-second polling after page load)
void handleStatus() {
  if (!checkAuth()) return;
  int active = getActiveIndex();
  String savedSSID = (active >= 0) ? getNetworkSSID(active) : "";
  bool connected = (WiFi.status() == WL_CONNECTED);

  String json = "{";
  json += "\"connected\":" + String(connected ? "true" : "false");
  json += ",\"ssid\":\""   + jsonEsc(connected ? WiFi.SSID() : savedSSID) + "\"";
  json += ",\"rssi\":"     + String(connected ? WiFi.RSSI() : 0);
  json += ",\"ip\":\""     + String(connected ? WiFi.localIP().toString() : "") + "\"";
  json += ",\"uptime\":\"" + formatUptime() + "\"";
  json += ",\"boots\":"    + String(getBootCount());
  json += "}";

  prepareResponse();
  server.send(200, "application/json", json);
}

// GET /clients → JSON array of hotspot clients (MAC + RSSI)
void handleClients() {
  if (!checkAuth()) return;
  wifi_sta_list_t sta_list;
  memset(&sta_list, 0, sizeof(sta_list));
  esp_wifi_ap_get_sta_list(&sta_list);

  String json = "[";
  for (int i = 0; i < sta_list.num; i++) {
    if (i > 0) json += ",";
    uint8_t* mac = sta_list.sta[i].mac;
    char mac_str[18];
    sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    json += "{\"mac\":\"" + String(mac_str) + "\"";
    json += ",\"rssi\":" + String(sta_list.sta[i].rssi) + "}";
  }
  json += "]";

  prepareResponse();
  server.send(200, "application/json", json);
}

// GET /scan → async Wi-Fi scan. First call starts the scan and returns
// {"scanning":true}. Subsequent calls return the same until the radio
// finishes (~2-4 s). Final call returns {"scanning":false,"networks":[...]}.
// The server stays fully responsive during the scan (no blocking).
void handleScan() {
  if (!checkAuth()) return;
  int n = WiFi.scanComplete();

  if (n == WIFI_SCAN_FAILED) {
    // No scan running — start one now (async = true, non-blocking)
    neopixelWrite(RGB_BUILTIN, 80, 0, 150);
    isScanning = true;
    WiFi.scanNetworks(true);
    prepareResponse();
    server.send(200, "application/json", "{\"scanning\":true}");
    return;
  }

  if (n == WIFI_SCAN_RUNNING) {
    prepareResponse();
    server.send(200, "application/json", "{\"scanning\":true}");
    return;
  }

  // n >= 0: scan finished
  isScanning = false;
  lastBlinkTime = 0;

  String json = "{\"scanning\":false,\"networks\":[";
  for (int i = 0; i < n; i++) {
    if (i > 0) json += ",";
    json += "{\"ssid\":\""  + jsonEsc(WiFi.SSID(i)) + "\"";
    json += ",\"rssi\":"    + String(WiFi.RSSI(i));
    json += ",\"enc\":"     + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false") + "}";
  }
  json += "]}";
  WiFi.scanDelete();

  prepareResponse();
  server.send(200, "application/json", json);
}

// GET /networks → JSON array of saved networks
void handleNetworks() {
  if (!checkAuth()) return;
  int count  = getNetworkCount();
  int active = getActiveIndex();

  String json = "[";
  for (int i = 0; i < count; i++) {
    if (i > 0) json += ",";
    json += "{\"ssid\":\""  + jsonEsc(getNetworkSSID(i)) + "\"";
    json += ",\"index\":"   + String(i);
    json += ",\"active\":"  + String(i == active ? "true" : "false") + "}";
  }
  json += "]";

  prepareResponse();
  server.send(200, "application/json", json);
}

// GET /ap-settings → JSON with current AP config (no passwords returned)
void handleApSettings() {
  if (!checkAuth()) return;
  String json = "{";
  json += "\"apSsid\":\""  + jsonEsc(apSsid) + "\"";
  json += ",\"authOn\":"   + String(authEnabled ? "true" : "false");
  json += ",\"authUser\":\"" + jsonEsc(authUser) + "\"";
  json += "}";
  prepareResponse();
  server.send(200, "application/json", json);
}

// POST /save  body: ssid=...&password=...
void handleSave() {
  if (!checkAuth()) return;
  String ssid = server.arg("ssid");
  String pass = server.arg("password");

  if (ssid.length() == 0) {
    server.send(400, "text/plain", "SSID cannot be empty.");
    return;
  }

  int idx = saveNetwork(ssid, pass);
  if (idx < 0) {
    server.send(409, "text/plain", "Network storage full (10/10). Delete one first.");
    return;
  }

  Serial.printf("[SAVE] '%s' at index %d — rebooting.\n", ssid.c_str(), idx);
  sendRebootPage("Saved! Connecting to " + ssid + "…", "#22c55e");
}

// POST /connect  body: index=N
void handleConnect() {
  if (!checkAuth()) return;
  int idx   = server.arg("index").toInt();
  int count = getNetworkCount();
  if (idx < 0 || idx >= count) { server.send(400, "text/plain", "Invalid index."); return; }

  preferences.begin("wifi-cfg", false);
  preferences.putInt("active", idx);
  preferences.end();

  String ssid = getNetworkSSID(idx);
  Serial.printf("[CONNECT] Switching to '%s' — rebooting.\n", ssid.c_str());
  sendRebootPage("Switching to " + ssid + "…", "#22c55e");
}

// POST /delete  body: index=N
void handleDelete() {
  if (!checkAuth()) return;
  int idx = server.arg("index").toInt();
  Serial.printf("[DELETE] Removing network at index %d.\n", idx);
  removeNetwork(idx);
  prepareResponse();
  server.send(200, "text/plain", "ok");
}

// POST /reboot
void handleReboot() {
  if (!checkAuth()) return;
  Serial.println("[REBOOT] Manual reboot via web portal.");
  sendRebootPage("Rebooting&hellip;", "#a855f7");
}

// POST /clear
void handleClearAll() {
  if (!checkAuth()) return;
  preferences.begin("wifi-cfg", false);
  preferences.clear();
  preferences.end();
  Serial.println("[CLEAR] All networks forgotten — rebooting.");
  sendRebootPage("All networks forgotten.", "#ef4444");
}

// POST /ap-save  body: apSsid=...&apPass=...&authOn=0|1&authUser=...&authPass=...
void handleApSave() {
  if (!checkAuth()) return;
  String newSsid  = server.arg("apSsid");
  String newPass  = server.arg("apPass");
  bool   newAuth  = server.arg("authOn") == "1";
  String newUser  = server.arg("authUser");
  String newAPass = server.arg("authPass");

  if (newSsid.length() == 0) {
    server.send(400, "text/plain", "Hotspot name cannot be empty.");
    return;
  }

  // Keep existing passwords if fields left blank
  if (newPass.length() == 0)  newPass  = apPass;
  if (newAPass.length() == 0) newAPass = authPass;
  if (newUser.length() == 0)  newUser  = "admin";

  if (newPass.length() < 8) {
    server.send(400, "text/plain", "Hotspot password must be at least 8 characters.");
    return;
  }

  saveApConfig(newSsid, newPass, newAuth, newUser, newAPass);
  Serial.printf("[AP] Settings updated: SSID='%s' auth=%s — rebooting.\n",
                newSsid.c_str(), newAuth ? "on" : "off");
  sendRebootPage("Hotspot settings saved.<br><small>Reconnect to <b>" + newSsid + "</b></small>", "#f59e0b");
}

// ─── Setup ────────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  delay(500);

  uint32_t boots = incrementBootCount();
  Serial.printf("\n\n=== Ma9la-Repeater v2.1  (boot #%u) ===\n", boots);
  Serial.printf("Arduino ESP32 core v%d.%d.%d\n",
    ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);

  neopixelWrite(RGB_BUILTIN, 0, 0, 0);
  delay(300);

  loadApConfig();

  WiFi.mode(WIFI_AP_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);  // disable modem sleep — reduces latency, improves throughput

  int active = getActiveIndex();
  String ssid = (active >= 0) ? getNetworkSSID(active) : "";
  String pass = (active >= 0) ? getNetworkPass(active) : "";

  if (ssid != "") {
    Serial.printf("[STA] Connecting to: %s\n", ssid.c_str());
    currentStatus = CONNECTING_BLUE;
    neopixelWrite(RGB_BUILTIN, 0, 0, 150);

    WiFi.begin(ssid.c_str(), pass.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    Serial.println();

    WiFi.softAP(apSsid.c_str(), apPass.c_str());
    delay(500);
    Serial.printf("[AP] Hotspot: %s  |  http://%s\n",
                  apSsid.c_str(), WiFi.softAPIP().toString().c_str());

    fixAPDns();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[STA] Connected. IP: %s  RSSI: %d dBm\n",
                    WiFi.localIP().toString().c_str(), WiFi.RSSI());
      ENABLE_NAT();
      Serial.println("[NAT] Internet sharing active.");
      currentStatus = REPEATER_GREEN;
    } else {
      Serial.println("[STA] Could not connect. Hotspot up for reconfiguration.");
      currentStatus = NO_INTERNET_RED;
    }

  } else {
    Serial.println("[AP] No network saved — hotspot up for setup.");
    WiFi.softAP(apSsid.c_str(), apPass.c_str());
    delay(500);
    fixAPDns();
    Serial.printf("[AP] Connect to '%s' then open http://%s\n",
                  apSsid.c_str(), WiFi.softAPIP().toString().c_str());
    currentStatus = NO_INTERNET_RED;
  }

  server.on("/",           HTTP_GET,  handleRoot);
  server.on("/status",     HTTP_GET,  handleStatus);
  server.on("/clients",    HTTP_GET,  handleClients);
  server.on("/scan",       HTTP_GET,  handleScan);
  server.on("/networks",   HTTP_GET,  handleNetworks);
  server.on("/ap-settings",HTTP_GET,  handleApSettings);
  server.on("/save",       HTTP_POST, handleSave);
  server.on("/connect",    HTTP_POST, handleConnect);
  server.on("/delete",     HTTP_POST, handleDelete);
  server.on("/init",       HTTP_GET,  handleInit);
  server.on("/clear",      HTTP_POST, handleClearAll);
  server.on("/ap-save",    HTTP_POST, handleApSave);
  server.on("/reboot",     HTTP_POST, handleReboot);
  server.begin();
  Serial.println("[HTTP] Web server ready.");
}

// ─── Loop ─────────────────────────────────────────────────────────────────────

void loop() {
  server.handleClient();

  // When async scan finishes, clear the scanning flag so LED returns to normal.
  // (WiFi.scanComplete() returns WIFI_SCAN_RUNNING=-1 while in progress)
  if (isScanning && WiFi.scanComplete() != WIFI_SCAN_RUNNING) {
    isScanning = false;
    lastBlinkTime = 0;
  }

  if (isScanning) {
    neopixelWrite(RGB_BUILTIN, 80, 0, 150);  // solid purple while scanning
    return;
  }

  unsigned long now = millis();

  if (currentStatus == REPEATER_GREEN) {
    if (now - lastBlinkTime >= 3000) {
      lastBlinkTime = now;
      ledState = !ledState;
      neopixelWrite(RGB_BUILTIN, 0, ledState ? 100 : 0, 0);
    }
  } else if (currentStatus == NO_INTERNET_RED) {
    if (now - lastBlinkTime >= 500) {
      lastBlinkTime = now;
      ledState = !ledState;
      neopixelWrite(RGB_BUILTIN, ledState ? 100 : 0, 0, 0);
    }
  } else {
    neopixelWrite(RGB_BUILTIN, 0, 0, 100);  // solid blue = connecting
  }
}
