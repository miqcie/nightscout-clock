#include "ServerManager.h"

#include <AsyncJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <esp_system.h>

#include "BGDisplayManager.h"
#include "BGSourceManager.h"
#include "DisplayManager.h"
#include "PeripheryManager.h"
#include "SettingsManager.h"
#include "globals.h"
#include "time.h"

// The getter for the instantiated singleton instance
ServerManager_& ServerManager_::getInstance() {
    static ServerManager_ instance;
    return instance;
}

static String htmlEscape(const String& s) {
    String out;
    out.reserve(s.length() + 8);
    for (unsigned int i = 0; i < s.length(); i++) {
        char c = s.charAt(i);
        switch (c) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&#39;";  break;
            default:   out += c;        break;
        }
    }
    return out;
}

static String resolveAlarmMelody(const String& alarmType) {
    if (alarmType == "high") {
        if (SettingsManager.settings.alarm_high_melody.length() > 0) {
            return SettingsManager.settings.alarm_high_melody;
        }
        return sound_high;
    }
    if (alarmType == "low") {
        if (SettingsManager.settings.alarm_low_melody.length() > 0) {
            return SettingsManager.settings.alarm_low_melody;
        }
        return sound_low;
    }
    if (alarmType == "urgent_low") {
        if (SettingsManager.settings.alarm_urgent_low_melody.length() > 0) {
            return SettingsManager.settings.alarm_urgent_low_melody;
        }
        return sound_urgent_low;
    }

    return "";
}

// Web authentication constants, cookie set for 10 minutes
static constexpr unsigned long WEB_AUTH_TOKEN_TTL_MS = 10UL * 60UL * 1000UL;
static constexpr int WEB_AUTH_COOKIE_MAX_AGE_SEC = 10 * 60;
static const char* WEB_AUTH_COOKIE_NAME = "auth_token";

static String getCookieValue(const String& cookieHeader, const String& name) {
    int pos = 0;
    while (pos < cookieHeader.length()) {
        int end = cookieHeader.indexOf(';', pos);
        if (end < 0) {
            end = cookieHeader.length();
        }
        String pair = cookieHeader.substring(pos, end);
        pair.trim();
        int eq = pair.indexOf('=');
        if (eq > 0) {
            String key = pair.substring(0, eq);
            key.trim();
            if (key == name) {
                return pair.substring(eq + 1);
            }
        }
        pos = end + 1;
    }
    return "";
}

static String buildAuthCookie(const String& token, int maxAgeSeconds) {
    String cookie = String(WEB_AUTH_COOKIE_NAME) + "=" + token;
    if (maxAgeSeconds >= 0) {
        cookie += "; Max-Age=" + String(maxAgeSeconds);
    }
    cookie += "; Path=/; SameSite=Strict; HttpOnly";
    return cookie;
}

// Initialize the global shared instance
ServerManager_& ServerManager = ServerManager.getInstance();

String ServerManager_::getHostname() {
    if (SettingsManager.settings.hostname != "") {
        return SettingsManager.settings.hostname;
    }

    String hostname = HOSTNAME_PREFIX;

    if (SettingsManager.settings.custom_hostname_enable) {
        hostname = SettingsManager.settings.custom_hostname;
    }

    SettingsManager.settings.hostname = hostname;

    DEBUG_PRINTF("Hostname: %s\n", hostname.c_str());

    return hostname;
}

bool ServerManager_::isWebAuthEnabled() const {
    return SettingsManager.settings.web_auth_enable &&
           SettingsManager.settings.web_auth_password.length() > 0;
}

bool ServerManager_::isRequestAuthenticated(AsyncWebServerRequest* request) const {
    if (!isWebAuthEnabled()) {
        return false;
    }

    if (webAuthToken.length() == 0) {
        return false;
    }

    if (webAuthTokenIssuedMs == 0 || (millis() - webAuthTokenIssuedMs) > WEB_AUTH_TOKEN_TTL_MS) {
        const_cast<ServerManager_*>(this)->webAuthToken = "";
        const_cast<ServerManager_*>(this)->webAuthTokenIssuedMs = 0;
        return false;
    }

    String requestToken = "";
    if (request->hasHeader("Cookie")) {
        requestToken = getCookieValue(request->getHeader("Cookie")->value(), WEB_AUTH_COOKIE_NAME);
    }

    return requestToken.length() > 0 && requestToken == webAuthToken;
}

String ServerManager_::generateAuthToken() {
    uint32_t part1 = esp_random();
    uint32_t part2 = esp_random();
    uint32_t part3 = millis();
    return String(part1, HEX) + String(part2, HEX) + String(part3, HEX);
}

bool ServerManager_::enforceAuthentication(AsyncWebServerRequest* request) {
    if (!isWebAuthEnabled()) {
        return true;
    }

    if (isRequestAuthenticated(request)) {
        return true;
    }

    request->send(401, "application/json", "{\"status\": \"unauthorized\"}");
    return false;
}

IPAddress ServerManager_::setAPmode(String ssid, String psk) {
    auto ipAP = IPAddress();
    auto netmaskAP = IPAddress();
    auto gatewayAP = IPAddress();
    ipAP.fromString(AP_IP);
    netmaskAP.fromString(AP_NETMASK);
    gatewayAP.fromString(AP_GATEWAY);

    apMode = true;
    WiFi.mode(WIFI_AP_STA);
    WiFi.persistent(false);
    WiFi.softAPConfig(ipAP, gatewayAP, netmaskAP);
    WiFi.softAP(ssid, psk);
    /* Setup the DNS server redirecting all the domains to the apIP */
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", WiFi.softAPIP());
    return WiFi.softAPIP();
}

void initiateWiFiConnection(String wifi_type, String ssid, String username, String password) {
    if (wifi_type == "wpa_eap") {
        WiFi.begin(ssid, WPA2_AUTH_PEAP, username, username, password);
    } else if (password == "") {
        WiFi.begin(ssid);
    } else {
        WiFi.begin(ssid, password);
    }
}

bool tryConnectToWiFi(String wifi_type, String ssid, String username, String password) {
    if (ssid == "") {
        DEBUG_PRINTLN("SSID is empty, cannot connect to WiFi");
        return false;
    }
    int timeout = WIFI_CONNECT_TIMEOUT;

    WiFi.mode(WIFI_STA);

    DEBUG_PRINTF("Connecting to %s (%s)\n", ssid.c_str(), wifi_type.c_str());

    initiateWiFiConnection(wifi_type, ssid, username, password);

    auto startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(300);
        Serial.print(".");
        if (WiFi.status() == WL_CONNECTED) {
            return true;
        }
        // If no connection after a while go in Access Point mode
        if (millis() - startTime > timeout)
            break;
    }

    Serial.println();

    return false;
}

IPAddress ServerManager_::startWifi() {
    this->isInAPMode = false;

    IPAddress ip;

    bool connected = tryConnectToWiFi(
        "wpa_psk", SettingsManager.settings.ssid, "", SettingsManager.settings.wifi_password);
    if (!connected && SettingsManager.settings.additional_wifi_enable) {
        connected = tryConnectToWiFi(
            SettingsManager.settings.additional_wifi_type, SettingsManager.settings.additional_wifi_ssid,
            SettingsManager.settings.additional_wifi_username,
            SettingsManager.settings.additional_wifi_password);
    }

    if (connected) {
        WiFi.setAutoReconnect(true);
        WiFi.persistent(true);
        ip = WiFi.localIP();
        DEBUG_PRINTLN("Connected");
        failedAttempts = 0;  // Reset failed API call attempts counter
        return ip;
    }

    DEBUG_PRINTLN("Failed to connect to WiFi, starting AP mode");

    ip = setAPmode(getHostname(), AP_MODE_PASSWORD);
    this->isInAPMode = true;
    WiFi.begin();
    return ip;
}

// When we cannot get BG for some time, we want to reconnect to wifi
void ServerManager_::reconnectWifi() {
    DEBUG_PRINTLN("Reconnecting to WiFi...");
    WiFi.disconnect();
    delay(1000);
    myIP = startWifi();
    failedAttempts = 0;
}

AsyncWebHandler ServerManager_::addHandler(AsyncWebHandler* handler) {
    if (ws != nullptr) {
        ws->addHandler(handler);
    }
    return *handler;
}

bool canReachInternet() {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
        return false;
    }

    WiFiClient client;
    const char* host = "www.google.com";
    const uint16_t port = 80;
    if (!client.connect(host, port, 2000)) {
        DEBUG_PRINTLN("Internet not reachable (connect failed)");
        client.stop();
        return false;
    }

    // Send a simple HTTP GET request
    client.print("GET /generate_204 HTTP/1.1\r\nHost: www.google.com\r\nConnection: close\r\n\r\n");

    unsigned long start = millis();
    while (client.connected() && !client.available() && millis() - start < 2000) {
        delay(10);
    }

    if (!client.available()) {
        DEBUG_PRINTLN("Internet not reachable (no response)");
        client.stop();
        return false;
    }

    // Optionally, check for HTTP/1.1 204 No Content response
    String line = client.readStringUntil('\n');
    if (line.indexOf("204") > 0 || line.indexOf("200") > 0) {
        // DEBUG_PRINTLN("Internet reachable");
        client.stop();
        return true;
    }

    DEBUG_PRINTLN("Internet not reachable (bad response)");
    client.stop();
    return false;
}

void ServerManager_::setupWebServer(IPAddress ip) {
#ifdef DEBUG_BG_SOURCE
    DEBUG_PRINTLN("ServerManager::setupWebServer");
#endif
    ws = new AsyncWebServer(80);

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

    ws->on("/api/auth/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        String jsonResponse = "{\"enabled\": ";
        jsonResponse += isWebAuthEnabled() ? "true" : "false";
        jsonResponse += ", \"authenticated\": ";
        jsonResponse += isRequestAuthenticated(request) ? "true" : "false";
        jsonResponse += ", \"hasPassword\": ";
        jsonResponse += SettingsManager.settings.web_auth_password.length() > 0 ? "true" : "false";
        jsonResponse += "}";
        request->send(200, "application/json", jsonResponse);
    });

    ws->addHandler(new AsyncCallbackJsonWebHandler(
        "/api/auth/login", [this](AsyncWebServerRequest* request, JsonVariant& json) {
            if (not json.is<JsonObject>()) {
                request->send(400, "application/json", "{\"status\": \"json parsing error\"}");
                return;
            }
            if (!isWebAuthEnabled()) {
                request->send(200, "application/json", "{\"status\": \"disabled\"}");
                return;
            }

            auto&& data = json.as<JsonObject>();
            String password = data["password"].as<String>();
            if (password != SettingsManager.settings.web_auth_password) {
                request->send(401, "application/json", "{\"status\": \"invalid\"}");
                return;
            }

            webAuthToken = generateAuthToken();
            webAuthTokenIssuedMs = millis();
            String jsonResponse = "{\"status\": \"ok\"}";
            auto response = request->beginResponse(200, "application/json", jsonResponse);
            response->addHeader(
                "Set-Cookie", buildAuthCookie(webAuthToken, WEB_AUTH_COOKIE_MAX_AGE_SEC));
            request->send(response);
        }));

    ws->on("/api/auth/logout", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (!isWebAuthEnabled()) {
            request->send(200, "application/json", "{\"status\": \"disabled\"}");
            return;
        }
        if (!isRequestAuthenticated(request)) {
            request->send(401, "application/json", "{\"status\": \"unauthorized\"}");
            return;
        }
        webAuthToken = "";
        webAuthTokenIssuedMs = 0;
        auto response = request->beginResponse(200, "application/json", "{\"status\": \"ok\"}");
        response->addHeader("Set-Cookie", buildAuthCookie("", 0));
        request->send(response);
    });

    ws->addHandler(new AsyncCallbackJsonWebHandler(
        "/api/save", [this](AsyncWebServerRequest* request, JsonVariant& json) {
            if (!enforceAuthentication(request)) {
                return;
            }
            if (not json.is<JsonObject>()) {
                request->send(200, "application/json", "{\"status\": \"json parsing error\"}");
                return;
            }
            auto&& data = json.as<JsonObject>();
            if (SettingsManager.trySaveJsonAsSettings(data)) {
                bgDisplayManager.refreshCachedEnabledFaces();
                request->send(200, "application/json", "{\"status\": \"ok\"}");
            } else {
                request->send(200, "application/json", "{\"status\": \"Settings save error\"}");
            }
        }));

    ws->addHandler(new AsyncCallbackJsonWebHandler(
        "/api/alarm/custom", [](AsyncWebServerRequest* request, JsonVariant& json) {
            if (!ServerManager.enforceAuthentication(request)) {
                return;
            }
            if (not json.is<JsonObject>()) {
                request->send(400, "application/json", "{\"status\": \"json parsing error\"}");
                return;
            }

            auto&& data = json.as<JsonObject>();
            if (!data["rtttl"].is<String>()) {
                request->send(400, "application/json", "{\"status\": \"rtttl key not found\"}");
                return;
            }

            auto melody = data["rtttl"].as<String>();
            melody.trim();

            if (melody.length() == 0 || melody.indexOf(':') < 0) {
                request->send(400, "application/json", "{\"status\": \"invalid melody\"}");
                return;
            }

            PeripheryManager.playRTTTLString(melody);
            request->send(200, "application/json", "{\"status\": \"ok\"}");
        }));

    ws->addHandler(new AsyncCallbackJsonWebHandler(
        "/api/alarm", [this](AsyncWebServerRequest* request, JsonVariant& json) {
            if (!enforceAuthentication(request)) {
                return;
            }
            if (not json.is<JsonObject>()) {
                request->send(400, "application/json", "{\"status\": \"json parsing error\"}");
                return;
            }
            auto&& data = json.as<JsonObject>();
            if (data["alarmType"].is<String>()) {
                auto alarmType = data["alarmType"].as<String>();
                auto melody = resolveAlarmMelody(alarmType);
                if (melody == "") {
                    request->send(400, "application/json", "{\"status\": \"alarm type not found\"}");
                    return;
                }

                PeripheryManager.playRTTTLString(melody);

                request->send(200, "application/json", "{\"status\": \"ok\"}");
            } else {
                request->send(400, "application/json", "{\"status\": \"alarm key not found\"}");
            }
        }));

    ws->on("/api/reset", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (!enforceAuthentication(request)) {
            return;
        }
        request->send(200, "application/json", "{\"status\": \"ok\"}");
        pendingRestart = true;
        pendingRestartMs = millis();
    });

    ws->addHandler(new AsyncCallbackJsonWebHandler(
        "/api/displaypower", [this](AsyncWebServerRequest* request, JsonVariant& json) {
            if (!enforceAuthentication(request)) {
                return;
            }
            if (not json.is<JsonObject>()) {
                request->send(400, "application/json", "{\"status\": \"json parsing error\"}");
                return;
            }
            auto&& data = json.as<JsonObject>();
            if (data["power"].is<String>()) {
                auto powerState = data["power"].as<String>();
                if (powerState == "on") {
                    DisplayManager.setPower(true);
                } else if (powerState == "off") {
                    DisplayManager.setPower(false);
                } else {
                    request->send(400, "application/json", "{\"status\": \"Invalid power state\"}");
                    return;
                }

                request->send(200, "application/json", "{\"status\": \"ok\"}");
            } else {
                request->send(400, "application/json", "{\"status\": \"power key not found\"}");
            }
        }));

    ws->on("/api/factory-reset", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (!enforceAuthentication(request)) {
            return;
        }
        request->send(200, "application/json", "{\"status\": \"ok\"}");
        pendingFactoryReset = true;
        pendingRestartMs = millis();
    });

    // api call which returns status (isConnected, internet is reacheable, is in AP mode, bg source type
    // and status)
    ws->on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        String jsonResponse = "{\"isConnected\": ";
        jsonResponse += this->isConnected ? "true" : "false";
        jsonResponse += ", \"hasInternet\": ";
        jsonResponse += canReachInternet() ? "true" : "false";
        jsonResponse += ", \"isInAPMode\": ";
        jsonResponse += this->isInAPMode ? "true" : "false";
        jsonResponse += ", \"bgSource\": \"";
        jsonResponse += toString(bgSourceManager.getCurrentSourceType());
        jsonResponse += "\", \"bgSourceStatus\": \"";
        jsonResponse += bgSourceManager.getSourceStatus();
        jsonResponse += "\", \"sgv\": ";
        auto glucoseData = bgSourceManager.getGlucoseData();
        if (!glucoseData.empty()) {
            jsonResponse += String(glucoseData.back().sgv);
        } else {
            jsonResponse += "0";
        }
        jsonResponse += "}";
        request->send(200, "application/json", jsonResponse);
    });

    ws->on("/config.json", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (!enforceAuthentication(request)) {
            return;
        }
        request->send(LittleFS, CONFIG_JSON, "application/json");
    });

    // Captive portal detection — serve setup page directly (no redirects).
    // Redirects fail in iOS CaptiveNetworkSupport WebView.
    // All captive portal detection paths serve the same dynamic WiFi setup page.
    // Scan WiFi networks ONCE at startup, cache results.
    // WiFi.scanNetworks(true) starts an async scan. Even async scans briefly
    // disrupt the AP radio, causing captive portal clients to disconnect if
    // scanned per-request — so we scan once and cache.
    WiFi.scanNetworks(true);  // async scan, results retrieved later
    delay(3000);              // wait for scan to complete
    int cachedNetworkCount = WiFi.scanComplete();
    std::vector<std::pair<String, int>> cachedNetworks;
    if (cachedNetworkCount > 0) {
        for (int i = 0; i < cachedNetworkCount; i++) {
            String ssid = WiFi.SSID(i);
            if (ssid.length() == 0) continue;
            bool dup = false;
            for (auto& net : cachedNetworks) {
                if (net.first == ssid) {
                    if (WiFi.RSSI(i) > net.second) net.second = WiFi.RSSI(i);
                    dup = true;
                    break;
                }
            }
            if (!dup) cachedNetworks.push_back({ssid, WiFi.RSSI(i)});
        }
        std::sort(cachedNetworks.begin(), cachedNetworks.end(),
            [](const std::pair<String, int>& a, const std::pair<String, int>& b) {
                return a.second > b.second;
            });
    }
    WiFi.scanDelete();

    // Combined single-page setup: WiFi + CGM source in one form.
    // No JavaScript — CSS :checked selectors handle show/hide.
    // Must complete in <60 seconds (iOS captive portal timeout).
    auto serveCaptivePortal = [cachedNetworks](AsyncWebServerRequest* request) {
        int n = cachedNetworks.size();

        String html;
        html.reserve(6144);
        html +=
            "<!DOCTYPE html><html lang=\"en\"><head>"
            "<meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
            "<title>Nightscout Clock</title><style>"
            "*{margin:0;padding:0;box-sizing:border-box}"
            "body{background:#1a1a1a;color:#e0e0e0;font-family:-apple-system,system-ui,sans-serif;"
            "padding:24px 16px 40px;min-height:100vh}"
            "h1{font-size:20px;font-weight:600;margin-bottom:4px}"
            ".sub{font-size:13px;color:#888;margin-bottom:24px}"
            ".section{margin-top:28px;padding-top:20px;border-top:1px solid #333}"
            ".sl{font-size:12px;color:#666;letter-spacing:.04em;text-transform:uppercase;margin-bottom:12px}"
            ".nets{max-height:240px;overflow-y:auto;margin-bottom:8px}"
            ".net{display:block;padding:12px 0 12px 12px;border-bottom:1px solid #222;cursor:pointer;"
            "-webkit-tap-highlight-color:transparent}"
            ".net input[type=radio]{display:none}"
            ".net .name{font-size:16px;color:#888}"
            ".net .sig{font-size:11px;color:#444;float:right;margin-top:3px;margin-right:4px}"
            ".net input:checked~.name{color:#fff;font-weight:600}"
            ".net input:checked~.sig{color:#888}"
            ".f{margin-bottom:16px}"
            ".fl{display:block;font-size:12px;color:#888;letter-spacing:.04em;text-transform:uppercase;margin-bottom:6px}"
            "input[type=text],input[type=password],input[type=email]{"
            "width:100%;background:rgba(255,255,255,0.05);border:none;"
            "border-bottom:2px solid #555;color:#fff;font-size:16px;padding:10px 8px;outline:none;"
            "-webkit-appearance:none;border-radius:0}"
            "input:focus{border-bottom-color:#e0e0e0;background:rgba(255,255,255,0.08)}"
            "select{width:100%;background:rgba(255,255,255,0.05);border:none;"
            "border-bottom:2px solid #555;color:#e0e0e0;font-size:16px;padding:10px 8px;"
            "outline:none;-webkit-appearance:none;border-radius:0}"
            ".cgm>input[type=radio]{position:absolute;opacity:0;width:0;height:0;pointer-events:none}"
            ".cs{display:block;padding:14px 0 14px 12px;border-bottom:1px solid #222;cursor:pointer;"
            "-webkit-tap-highlight-color:transparent}"
            ".cs .cn{font-size:16px;color:#888}"
            ".cs .ch{font-size:12px;color:#555;margin-top:1px}"
            ".cf{display:none;padding:16px 0 8px 12px;border-left:3px solid #333}"
            "#sd:checked~#fd,#sl:checked~#fl2,#sn:checked~#fn{display:block}"
            "#sd:checked~label[for=sd],#sl:checked~label[for=sl],"
            "#sn:checked~label[for=sn]{border-left:3px solid #e0e0e0;background:rgba(255,255,255,0.03)}"
            "#sd:checked~label[for=sd] .cn,#sl:checked~label[for=sl] .cn,"
            "#sn:checked~label[for=sn] .cn{color:#fff;font-weight:600}"
            ".rg{display:flex;flex-wrap:wrap}"
            ".rg label{font-size:14px;color:#888;padding:8px 14px;cursor:pointer;"
            "border-bottom:2px solid #333;text-transform:none;letter-spacing:normal}"
            ".rg input[type=radio]{display:none}"
            ".rg input:checked+span{color:#fff;font-weight:600}"
            ".rg label:has(input:checked){border-bottom-color:#e0e0e0}"
            ".fh{font-size:13px;color:#888;margin-bottom:14px;line-height:1.4}"
            ".fh strong{color:#e0e0e0}"
            "button[type=submit]{display:block;width:100%;background:#e0e0e0;color:#1a1a1a;border:none;"
            "font-size:16px;font-weight:600;padding:14px;cursor:pointer;letter-spacing:.02em;margin-top:28px}"
            "button[type=submit]:active{background:#fff}"
            "</style></head><body>"
            "<h1>Nightscout Clock</h1>"
            "<p class=\"sub\">One-time setup</p>"
            "<form method=\"POST\" action=\"/api/setup\">"
            "<p class=\"sl\">WiFi network</p>";

        // WiFi network list (from cached scan at boot, not per-request)
        if (n > 0) {
            html += "<div class=\"nets\">";
            for (const auto& net : cachedNetworks) {
                int rssi = net.second;
                String bars;
                if (rssi > -50) bars = "&#9608;&#9608;&#9608;&#9608;";
                else if (rssi > -60) bars = "&#9608;&#9608;&#9608;";
                else if (rssi > -70) bars = "&#9608;&#9608;";
                else bars = "&#9608;";

                String escaped = htmlEscape(net.first);
                html += "<label class=\"net\"><input type=\"radio\" name=\"ssid\" value=\"";
                html += escaped;
                html += "\"><span class=\"sig\">";
                html += bars;
                html += "</span><span class=\"name\">";
                html += escaped;
                html += "</span></label>";
            }
            html += "</div>";
        } else {
            html += "<p style=\"font-size:13px;color:#888;margin-bottom:12px\">"
                    "Nearby networks could not be detected. Enter your network name manually.</p>"
                    "<div class=\"f\">"
                    "<label class=\"fl\" for=\"ssid\">Network name</label>"
                    "<input type=\"text\" id=\"ssid\" name=\"ssid\" required>"
                    "</div>";
        }

        // WiFi password — autocomplete helps iOS offer saved passwords without switching apps
        html += "<div class=\"f\">"
                "<label class=\"fl\" for=\"password\">WiFi password</label>"
                "<input type=\"password\" id=\"password\" name=\"password\" autocomplete=\"current-password\">"
                "<p style=\"font-size:11px;color:#555;margin-top:6px\">"
                "Tap the key icon above your keyboard for saved passwords</p>"
                "</div>";

        // Zip code for timezone + weather location
        html += "<div class=\"f\">"
                "<label class=\"fl\" for=\"zip\">Zip code</label>"
                "<input type=\"text\" id=\"zip\" name=\"zip\" placeholder=\"e.g. 23220\" "
                "inputmode=\"numeric\" maxlength=\"10\">"
                "<p style=\"font-size:11px;color:#555;margin-top:6px\">"
                "For local time and weather (optional)</p>"
                "</div>";

        // CGM source section
        html += "<div class=\"section\">"
                "<p class=\"sl\">Glucose data source</p>"
                "<div class=\"cgm\">"
                "<input type=\"radio\" name=\"data_source\" value=\"dexcom\" id=\"sd\">"
                "<label class=\"cs\" for=\"sd\"><span class=\"cn\">Dexcom</span>"
                "<span class=\"ch\">G6, G7, ONE+</span></label>"
                "<input type=\"radio\" name=\"data_source\" value=\"librelinkup\" id=\"sl\">"
                "<label class=\"cs\" for=\"sl\"><span class=\"cn\">Libre</span>"
                "<span class=\"ch\">LibreLinkUp</span></label>"
                "<input type=\"radio\" name=\"data_source\" value=\"nightscout\" id=\"sn\">"
                "<label class=\"cs\" for=\"sn\"><span class=\"cn\">Nightscout</span>"
                "<span class=\"ch\">Self-hosted or managed</span></label>"
                // Dexcom fields
                "<div class=\"cf\" id=\"fd\">"
                "<div class=\"fh\">Use the <strong>sensor wearer's</strong> account, not a follower.</div>"
                "<div class=\"f\"><label class=\"fl\">Username</label>"
                "<input type=\"text\" name=\"dexcom_username\" autocomplete=\"username\"></div>"
                "<div class=\"f\"><label class=\"fl\">Password</label>"
                "<input type=\"password\" name=\"dexcom_password\" autocomplete=\"current-password\"></div>"
                "<div class=\"f\"><label class=\"fl\">Region</label>"
                "<div class=\"rg\">"
                "<label><input type=\"radio\" name=\"dexcom_server\" value=\"us\"><span>US</span></label>"
                "<label><input type=\"radio\" name=\"dexcom_server\" value=\"ous\"><span>Outside US</span></label>"
                "<label><input type=\"radio\" name=\"dexcom_server\" value=\"jp\"><span>Japan</span></label>"
                "</div></div></div>"
                // Libre fields
                "<div class=\"cf\" id=\"fl2\">"
                "<div class=\"f\"><label class=\"fl\">Email</label>"
                "<input type=\"email\" name=\"librelinkup_email\" autocomplete=\"email\"></div>"
                "<div class=\"f\"><label class=\"fl\">Password</label>"
                "<input type=\"password\" name=\"librelinkup_password\" autocomplete=\"current-password\"></div>"
                "<div class=\"f\"><label class=\"fl\">Region</label>"
                "<select name=\"librelinkup_region\">"
                "<option value=\"\">Choose...</option>"
                "<option value=\"US\">United States</option>"
                "<option value=\"EU\">Europe</option>"
                "<option value=\"EU2\">Europe 2</option>"
                "<option value=\"DE\">Germany</option>"
                "<option value=\"FR\">France</option>"
                "<option value=\"AU\">Australia</option>"
                "<option value=\"CA\">Canada</option>"
                "<option value=\"JP\">Japan</option>"
                "</select></div></div>"
                // Nightscout fields
                "<div class=\"cf\" id=\"fn\">"
                "<div class=\"f\"><label class=\"fl\">Nightscout URL</label>"
                "<input type=\"text\" name=\"nightscout_url\" placeholder=\"https://yoursite.herokuapp.com\"></div>"
                "<div class=\"f\"><label class=\"fl\">API secret <span style=\"text-transform:none;color:#555\">"
                "(if required)</span></label>"
                "<input type=\"password\" name=\"api_secret\"></div></div>"
                "</div></div>";

        html += "<button type=\"submit\">Connect &amp; start</button>"
                "</form></body></html>";

        request->send(200, "text/html", html);
    };

    // iOS/macOS captive portal detection
    ws->on("/hotspot-detect.html", HTTP_GET, serveCaptivePortal);
    // Android/ChromeOS captive portal detection
    ws->on("/generate_204", HTTP_GET, serveCaptivePortal);
    // Windows captive portal detection
    ws->on("/connecttest.txt", HTTP_GET, serveCaptivePortal);
    // Direct access
    ws->on("/captive.html", HTTP_GET, serveCaptivePortal);

    // Combined setup endpoint — WiFi + CGM source in one form POST from captive portal
    ws->on("/api/setup", HTTP_POST, [this](AsyncWebServerRequest* request) {
        // Validate required field
        if (!request->hasParam("ssid", true) ||
            request->getParam("ssid", true)->value().length() == 0) {
            request->send(400, "text/html",
                "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
                "<style>body{background:#1a1a1a;color:#F04848;font-family:sans-serif;padding:24px}"
                "a{color:#58a6ff}</style></head><body>"
                "<p>Network name is required.</p>"
                "<a href=\"/captive.html\">Try again</a></body></html>");
            return;
        }

        // WiFi
        if (request->hasParam("ssid", true)) {
            SettingsManager.settings.ssid = request->getParam("ssid", true)->value();
        }
        if (request->hasParam("password", true)) {
            SettingsManager.settings.wifi_password = request->getParam("password", true)->value();
        }

        // CGM data source
        if (request->hasParam("data_source", true)) {
            String source = request->getParam("data_source", true)->value();
            if (source == "nightscout") SettingsManager.settings.bg_source = BG_SOURCE::NIGHTSCOUT;
            else if (source == "dexcom") SettingsManager.settings.bg_source = BG_SOURCE::DEXCOM;
            else if (source == "librelinkup") SettingsManager.settings.bg_source = BG_SOURCE::LIBRELINKUP;
        }

        // Dexcom credentials
        if (request->hasParam("dexcom_username", true)) {
            SettingsManager.settings.dexcom_username = request->getParam("dexcom_username", true)->value();
        }
        if (request->hasParam("dexcom_password", true)) {
            SettingsManager.settings.dexcom_password = request->getParam("dexcom_password", true)->value();
        }
        if (request->hasParam("dexcom_server", true)) {
            String server = request->getParam("dexcom_server", true)->value();
            if (server == "us") SettingsManager.settings.dexcom_server = DEXCOM_SERVER::US;
            else if (server == "ous") SettingsManager.settings.dexcom_server = DEXCOM_SERVER::NON_US;
            else if (server == "jp") SettingsManager.settings.dexcom_server = DEXCOM_SERVER::JAPAN;
        }

        // LibreLinkUp credentials
        if (request->hasParam("librelinkup_email", true)) {
            SettingsManager.settings.librelinkup_email = request->getParam("librelinkup_email", true)->value();
        }
        if (request->hasParam("librelinkup_password", true)) {
            SettingsManager.settings.librelinkup_password = request->getParam("librelinkup_password", true)->value();
        }
        if (request->hasParam("librelinkup_region", true)) {
            SettingsManager.settings.librelinkup_region = request->getParam("librelinkup_region", true)->value();
        }

        // Nightscout credentials
        if (request->hasParam("nightscout_url", true)) {
            SettingsManager.settings.nightscout_url = request->getParam("nightscout_url", true)->value();
        }
        if (request->hasParam("api_secret", true)) {
            SettingsManager.settings.nightscout_api_key = request->getParam("api_secret", true)->value();
        }

        // Zip code (triggers timezone + weather location geocoding after WiFi connects)
        if (request->hasParam("zip", true)) {
            SettingsManager.settings.setup_zip = request->getParam("zip", true)->value();
        }

        if (!SettingsManager.saveSettingsToFile()) {
            request->send(500, "text/html",
                "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
                "<style>body{background:#1a1a1a;color:#F04848;font-family:sans-serif;padding:24px}</style>"
                "</head><body><h1>Save failed</h1>"
                "<p>Could not save settings to flash. Please try again.</p>"
                "<a href=\"/captive.html\" style=\"color:#58a6ff\">Retry setup</a></body></html>");
            return;
        }

        String ssid = htmlEscape(SettingsManager.settings.ssid);
        String responseHtml =
            "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" "
            "content=\"width=device-width,initial-scale=1\"><style>body{background:#1a1a1a;"
            "color:#e0e0e0;font-family:sans-serif;padding:24px}h1{font-size:20px;margin-bottom:8px}"
            ".ok{color:#07E0A0}</style></head><body>"
            "<h1>Setup complete</h1>"
            "<p class=\"ok\">Connecting to <strong>" + ssid + "</strong>...</p>"
            "<p style=\"margin-top:12px;color:#888\">The clock will show your glucose data within 2 minutes.</p>"
            "</body></html>";
        request->send(200, "text/html", responseHtml);

        pendingRestart = true;
        pendingRestartMs = millis();
    });

    // Legacy WiFi-only endpoint (kept for backwards compatibility)
    ws->on("/api/wifi", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (!request->hasParam("ssid", true)) {
            request->send(400, "text/html",
                "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" "
                "content=\"width=device-width,initial-scale=1\"><style>body{background:#1a1a1a;"
                "color:#F04848;font-family:sans-serif;padding:24px}</style></head><body>"
                "<p>Network name is required.</p><a href=\"/captive.html\">Try again</a></body></html>");
            return;
        }

        String ssid = request->getParam("ssid", true)->value();
        String password = request->hasParam("password", true)
            ? request->getParam("password", true)->value() : "";

        SettingsManager.settings.ssid = ssid;
        SettingsManager.settings.wifi_password = password;

        if (!SettingsManager.saveSettingsToFile()) {
            request->send(500, "text/html",
                "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
                "<style>body{background:#1a1a1a;color:#F04848;font-family:sans-serif;padding:24px}</style>"
                "</head><body><h1>Save failed</h1>"
                "<p>Could not save settings to flash. Please try again.</p>"
                "<a href=\"/captive.html\" style=\"color:#58a6ff\">Retry setup</a></body></html>");
            return;
        }

        String escapedSsid = htmlEscape(ssid);
        String responseHtml =
            "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" "
            "content=\"width=device-width,initial-scale=1\"><style>body{background:#1a1a1a;"
            "color:#e0e0e0;font-family:sans-serif;padding:24px}h1{font-size:20px;margin-bottom:8px}"
            ".ok{color:#07E0A0}a{color:#58a6ff}</style></head><body>"
            "<h1>WiFi saved</h1>"
            "<p class=\"ok\">The clock will now restart and connect to <strong>" + escapedSsid + "</strong>.</p>"
            "<p style=\"margin-top:16px;color:#888\">After the clock restarts, open your browser and go to "
            "the IP address shown on the clock display to finish setup.</p>"
            "</body></html>";
        request->send(200, "text/html", responseHtml);

        pendingRestart = true;
        pendingRestartMs = millis();
    });

    addStaticFileHandler();

    ws->onNotFound([](AsyncWebServerRequest* request) {
        if (request->method() == HTTP_OPTIONS) {
            request->send(200);
        } else {
            request->send(404);
        }
    });

    ws->begin();
}

void ServerManager_::removeStaticFileHandler() {
    if (staticFilesHandler != nullptr) {
        ws->removeHandler(staticFilesHandler);
        staticFilesHandler = nullptr;
    } else {
        DEBUG_PRINTLN("removeStaticFileHandler: staticFilesHandler is null");
    }
}

void ServerManager_::addStaticFileHandler() {
    if (staticFilesHandler != nullptr) {
        removeStaticFileHandler();
    }
    staticFilesHandler = new AsyncStaticWebHandler("/", LittleFS, "/", NULL);
    staticFilesHandler->setDefaultFile("index.html");
    ws->addHandler(staticFilesHandler);
}

bool ServerManager_::initTimeIfNeeded() {
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo) || getUtcEpoch() - lastTimeSync > TIME_SYNC_INTERVAL) {
        configTime(0, 0, "pool.ntp.org");  // Connect to NTP server, with 0 TZ offset
        if (!getLocalTime(&timeinfo)) {
            DEBUG_PRINTLN("Failed to obtain time");
            return false;
        }

        lastTimeSync = getUtcEpoch();

        setTimezone();

        timeinfo = getTimezonedTime();

        DEBUG_PRINTF(
            "Timezone is: %s, local time is: %02d.%02d.%d %02d:%02d:%02d\n",
            SettingsManager.settings.tz_libc_value.c_str(), timeinfo.tm_mday, timeinfo.tm_mon + 1,
            timeinfo.tm_year + 1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        Serial.printf(
            "Local time is: %02d.%02d.%d %02d:%02d:%02d\n", timeinfo.tm_mday, timeinfo.tm_mon + 1,
            timeinfo.tm_year + 1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }

    return true;
}

void ServerManager_::setTimezone() {
    setenv("TZ", SettingsManager.settings.tz_libc_value.c_str(), 1);
    tzset();
}

unsigned long ServerManager_::getUtcEpoch() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        DEBUG_PRINTLN("Failed to obtain time");
        return 0;
    } else {
        auto utc = time(nullptr);
        return utc;
    }
}

tm ServerManager_::getTimezonedTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        DEBUG_PRINTLN("Failed to obtain time");
    }
    return timeinfo;
}

void ServerManager_::stop() {
    ws->end();
    delete ws;
    WiFi.disconnect();
}

void ServerManager_::setup() {
    WiFi.setHostname(getHostname().c_str());  // define hostname

    myIP = startWifi();

    auto ipAP = IPAddress();
    ipAP.fromString(AP_IP);
    DEBUG_PRINTF("My IP: %d.%d.%d.%d", myIP[0], myIP[1], myIP[2], myIP[3]);

    setupWebServer(myIP);
    setTimezone();

    isConnected = myIP != ipAP;
}

void ServerManager_::tick() {
    // Deferred restart: allow async response to flush before rebooting
    if ((pendingRestart || pendingFactoryReset) && (millis() - pendingRestartMs > 2000)) {
        if (pendingFactoryReset) {
            SettingsManager.factoryReset();  // copies factory config, then restarts
        }
        LittleFS.end();
        ESP.restart();
    }

    if (apMode) {
        dnsServer.processNextRequest();
    } else {
        initTimeIfNeeded();
    }
}
