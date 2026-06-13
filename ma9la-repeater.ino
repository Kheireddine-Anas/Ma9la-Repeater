/*
 * ESP32-S3 Wi-Fi Repeater with NAT
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

// ─── Configuration ────────────────────────────────────────────────────────────
const char* AP_SSID = "Ma9la-NET";
const char* AP_PASS = "Ma9la1337";
// ──────────────────────────────────────────────────────────────────────────────

WebServer server(80);
Preferences preferences;

unsigned long lastBlinkTime = 0;
bool ledState = false;

enum SystemStatus { CONNECTING_BLUE, REPEATER_GREEN, NO_INTERNET_RED };
SystemStatus currentStatus = NO_INTERNET_RED;

// ─── DNS Fix ──────────────────────────────────────────────────────────────────
// The ESP32 DHCP server by default tells clients to use 192.168.4.1 as DNS.
// But the ESP32 has no DNS server — so clients can't resolve domain names.
// This function stops the DHCP server, sets 8.8.8.8 as the advertised DNS,
// then restarts it so every new client gets Google DNS automatically.

void fixAPDns() {
  esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
  if (!ap_netif) {
    Serial.println("[DNS] Could not get AP netif handle.");
    return;
  }

  // Stop DHCP server before reconfiguring
  esp_netif_dhcps_stop(ap_netif);

  // Set DNS server IP to 8.8.8.8 on the AP interface
  esp_netif_dns_info_t dns;
  memset(&dns, 0, sizeof(dns));
  dns.ip.u_addr.ip4.addr = ipaddr_addr("8.8.8.8");
  dns.ip.type = IPADDR_TYPE_V4;
  esp_err_t err = esp_netif_set_dns_info(ap_netif, ESP_NETIF_DNS_MAIN, &dns);
  if (err != ESP_OK) {
    Serial.printf("[DNS] set_dns_info failed: %s\n", esp_err_to_name(err));
  }

  // Tell the DHCP server to advertise this DNS to clients
  dhcps_offer_t offer = OFFER_DNS;
  esp_netif_dhcps_option(
    ap_netif,
    ESP_NETIF_OP_SET,
    ESP_NETIF_DOMAIN_NAME_SERVER,
    &offer, sizeof(offer)
  );

  // Restart DHCP server
  esp_netif_dhcps_start(ap_netif);

  Serial.println("[DNS] DHCP server reconfigured — clients will use 8.8.8.8");
}

// ─── HTML Page ────────────────────────────────────────────────────────────────

String buildPage() {
  preferences.begin("wifi-cfg", true);
  String saved_ssid = preferences.getString("ssid", "");
  preferences.end();

  String html = F("<!DOCTYPE html><html><head>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<title>ESP32 Repeater</title>"
    "<style>"
    "* { box-sizing:border-box; }"
    "body { font-family:sans-serif; background:#1a1a2e; color:#eee; margin:0; padding:20px; text-align:center; }"
    "h2 { color:#e94560; }"
    ".card { background:#16213e; border-radius:10px; padding:15px; margin:10px 0; }"
    ".ok { color:#4CAF50; } .warn { color:#ff9800; } .err { color:#f44336; }"
    "input { width:100%; padding:10px; margin:6px 0; border:none; border-radius:6px; font-size:15px; }"
    ".btn { display:block; width:100%; padding:12px; border:none; border-radius:6px; "
    "       font-size:16px; cursor:pointer; margin:6px 0; color:#fff; }"
    ".btn-green { background:#4CAF50; } .btn-red { background:#f44336; }"
    "</style></head><body>");

  html += "<h2>ESP32 Wi-Fi Repeater</h2>";
  html += "<div class='card'>";

  if (WiFi.status() == WL_CONNECTED) {
    html += "<p class='ok'><b>&#x2714; Sharing Internet</b></p>";
    html += "<p>Target Wi-Fi: <b>" + saved_ssid + "</b></p>";
    html += "<p>ESP32 IP: <b>" + WiFi.localIP().toString() + "</b></p>";
    html += "<p>DNS for clients: <b>8.8.8.8</b></p>";
  } else if (saved_ssid != "") {
    html += "<p class='warn'><b>&#x23F3; Trying to connect to: " + saved_ssid + "</b></p>";
  } else {
    html += "<p class='err'><b>&#x2718; No target Wi-Fi saved</b></p>";
  }
  html += "</div>";

  html += "<div class='card'>"
          "<form action='/save' method='POST'>"
          "<b>Connect ESP to this Wi-Fi:</b><br>"
          "<input type='text' name='ssid' placeholder='Router Wi-Fi name' required><br>"
          "<input type='password' name='password' placeholder='Router password'><br>"
          "<input type='submit' class='btn btn-green' value='Save & Share Internet'>"
          "</form></div>";

  html += "<div class='card'>"
          "<form action='/clear' method='POST'>"
          "<button type='submit' class='btn btn-red'>Forget Target Wi-Fi & Reboot</button>"
          "</form></div>";

  html += "</body></html>";
  return html;
}

// ─── Route Handlers ───────────────────────────────────────────────────────────

void handleRoot() {
  server.send(200, "text/html", buildPage());
}

void handleSave() {
  String ssid = server.arg("ssid");
  String pass = server.arg("password");

  if (ssid.length() == 0) {
    server.send(400, "text/plain", "SSID cannot be empty.");
    return;
  }

  preferences.begin("wifi-cfg", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", pass);
  preferences.end();

  server.send(200, "text/html",
    "<h3 style='color:#4CAF50;font-family:sans-serif;text-align:center'>"
    "Saved! Rebooting in 2 seconds...</h3>");
  delay(2000);
  ESP.restart();
}

void handleClear() {
  preferences.begin("wifi-cfg", false);
  preferences.clear();
  preferences.end();

  server.send(200, "text/html",
    "<h3 style='color:#f44336;font-family:sans-serif;text-align:center'>"
    "Wi-Fi forgotten! Rebooting...</h3>");
  delay(2000);
  ESP.restart();
}

// ─── Setup ────────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n\n=== ESP32-S3 Wi-Fi Repeater ===");
  Serial.printf("Arduino ESP32 core v%d.%d.%d\n",
    ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);

  neopixelWrite(RGB_BUILTIN, 0, 0, 0);
  delay(300);

  WiFi.mode(WIFI_AP_STA);
  WiFi.setAutoReconnect(true);

  preferences.begin("wifi-cfg", true);
  String ssid = preferences.getString("ssid", "");
  String pass = preferences.getString("password", "");
  preferences.end();

  if (ssid != "") {
    Serial.printf("Connecting to router: %s\n", ssid.c_str());
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

    // Start AP after STA connects to match the router's Wi-Fi channel
    WiFi.softAP(AP_SSID, AP_PASS);
    delay(500);

    Serial.printf("Hotspot started: %s  |  http://%s\n",
                  AP_SSID, WiFi.softAPIP().toString().c_str());

    // Fix DNS BEFORE enabling NAT so all new clients get 8.8.8.8
    fixAPDns();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("Connected to router. ESP32 IP: %s\n", WiFi.localIP().toString().c_str());

      ENABLE_NAT();
      Serial.println("NAT enabled — internet is being shared.");

      currentStatus = REPEATER_GREEN;
    } else {
      Serial.println("Could not connect to router. Hotspot up for reconfiguration.");
      currentStatus = NO_INTERNET_RED;
    }

  } else {
    Serial.println("No router saved. Starting hotspot for setup...");
    WiFi.softAP(AP_SSID, AP_PASS);
    delay(500);
    fixAPDns(); // also fix DNS in setup-only mode
    Serial.printf("Connect to '%s' then open http://%s\n",
                  AP_SSID, WiFi.softAPIP().toString().c_str());
    currentStatus = NO_INTERNET_RED;
  }

  server.on("/",      HTTP_GET,  handleRoot);
  server.on("/save",  HTTP_POST, handleSave);
  server.on("/clear", HTTP_POST, handleClear);
  server.begin();
  Serial.println("Web server ready.");
}

// ─── Loop ─────────────────────────────────────────────────────────────────────

void loop() {
  server.handleClient();

  unsigned long now = millis();

  if (currentStatus == REPEATER_GREEN) {
    // Slow green blink = sharing internet
    if (now - lastBlinkTime >= 3000) {
      lastBlinkTime = now;
      ledState = !ledState;
      neopixelWrite(RGB_BUILTIN, 0, ledState ? 100 : 0, 0);
    }
  } else if (currentStatus == NO_INTERNET_RED) {
    // Fast red blink = no internet
    if (now - lastBlinkTime >= 500) {
      lastBlinkTime = now;
      ledState = !ledState;
      neopixelWrite(RGB_BUILTIN, ledState ? 100 : 0, 0, 0);
    }
  } else {
    // Solid blue = connecting
    neopixelWrite(RGB_BUILTIN, 0, 0, 100);
  }
}

