#include "BGDisplayFaceWeather.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>

#include "BGDisplayManager.h"
#include "DisplayManager.h"
#include "SettingsManager.h"
#include "globals.h"

// Static member initialization
WeatherData BGDisplayFaceWeather::cachedWeather = {0, 0, -1, 0, false};

static const unsigned long WEATHER_CACHE_MS = 30UL * 60UL * 1000UL;         // refresh interval
static const unsigned long WEATHER_MAX_STALE_MS = 2UL * 60UL * 60UL * 1000UL;  // 2hr max age

// No fallback location — if no zip/coordinates configured, weather displays "---"

// IANA timezone → POSIX TZ string for common US timezones
static String ianaToPosix(const String& iana) {
    if (iana.startsWith("America/New_York") || iana.startsWith("America/Detroit") ||
        iana.startsWith("America/Indiana"))
        return "EST5EDT,M3.2.0,M11.1.0";
    if (iana.startsWith("America/Chicago") || iana.startsWith("America/Menominee"))
        return "CST6CDT,M3.2.0,M11.1.0";
    if (iana == "America/Phoenix")
        return "MST7";  // Arizona, no DST
    if (iana.startsWith("America/Denver") || iana.startsWith("America/Boise"))
        return "MST7MDT,M3.2.0,M11.1.0";
    if (iana.startsWith("America/Los_Angeles"))
        return "PST8PDT,M3.2.0,M11.1.0";
    if (iana.startsWith("America/Anchorage") || iana.startsWith("America/Juneau"))
        return "AKST9AKDT,M3.2.0,M11.1.0";
    if (iana.startsWith("Pacific/Honolulu"))
        return "HST10";
    return "";  // unknown — caller should leave timezone unchanged
}

// Geocoding: resolve zip code to lat/lon + timezone after WiFi connects.
// Retries up to 3 times with 30s backoff — WiFi may still be stabilizing at first boot.
static bool zipResolved = false;
static int zipResolveAttempts = 0;
static const int MAX_ZIP_ATTEMPTS = 3;
static unsigned long lastZipAttemptMs = 0;

void BGDisplayFaceWeather::resolveZipLocation() {
    if (zipResolved) return;
    if (SettingsManager.settings.setup_zip.length() == 0) return;
    if (SettingsManager.settings.weather_lat != 0) {
        zipResolved = true;  // already resolved in a previous boot
        return;
    }
    // Backoff: wait 30s between attempts
    if (zipResolveAttempts > 0 && (millis() - lastZipAttemptMs) < 30000UL) return;
    if (zipResolveAttempts >= MAX_ZIP_ATTEMPTS) {
        DEBUG_PRINTF("Zip geocoding gave up after %d attempts, using fallback location", MAX_ZIP_ATTEMPTS);
        zipResolved = true;
        return;
    }

    lastZipAttemptMs = millis();
    zipResolveAttempts++;

    HTTPClient http;
    String url = "https://geocoding-api.open-meteo.com/v1/search?name=" +
                 SettingsManager.settings.setup_zip + "&count=1&language=en&format=json";
    http.begin(url);
    http.setTimeout(5000);
    int httpCode = http.GET();

    if (httpCode == 200) {
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error && doc["results"].size() > 0) {
            SettingsManager.settings.weather_lat = doc["results"][0]["latitude"].as<float>();
            SettingsManager.settings.weather_lon = doc["results"][0]["longitude"].as<float>();

            String iana = doc["results"][0]["timezone"].as<String>();
            String posix = ianaToPosix(iana);
            if (posix.length() > 0 && SettingsManager.settings.tz_libc_value.length() == 0) {
                SettingsManager.settings.tz_libc_value = posix;
                setenv("TZ", posix.c_str(), 1);
                tzset();
            } else if (posix.length() == 0) {
                DEBUG_PRINTF("Unrecognized timezone '%s' from geocoding, clock time may be wrong", iana.c_str());
            }

            if (!SettingsManager.saveSettingsToFile()) {
                DEBUG_PRINTLN("WARNING: Resolved zip location but failed to save to flash");
            }
            DEBUG_PRINTF("Zip resolved: %.4f, %.4f, tz=%s",
                SettingsManager.settings.weather_lat,
                SettingsManager.settings.weather_lon, iana.c_str());
            zipResolved = true;
        }
    } else {
        DEBUG_PRINTF("Zip geocoding attempt %d/%d failed, HTTP %d",
            zipResolveAttempts, MAX_ZIP_ATTEMPTS, httpCode);
    }

    http.end();
}

void BGDisplayFaceWeather::fetchWeather() {
    // One-time: resolve zip code to coordinates + timezone
    resolveZipLocation();

    // Don't fetch if cache is still fresh
    if (cachedWeather.valid && (millis() - cachedWeather.fetchedAtMs) < WEATHER_CACHE_MS) {
        return;
    }

    float lat = SettingsManager.settings.weather_lat;
    float lon = SettingsManager.settings.weather_lon;

    // No location configured — skip fetch, display will show "---"
    if (lat == 0 && lon == 0) {
        return;
    }

    String tempUnit = IS_CELSIUS ? "celsius" : "fahrenheit";
    HTTPClient http;
    String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(lat, 4) +
                 "&longitude=" + String(lon, 4) +
                 "&current=temperature_2m,relative_humidity_2m,weather_code"
                 "&temperature_unit=" + tempUnit;

    http.begin(url);
    http.setTimeout(5000);
    int httpCode = http.GET();

    if (httpCode == 200) {
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            cachedWeather.temperature = doc["current"]["temperature_2m"].as<float>();
            cachedWeather.humidity = doc["current"]["relative_humidity_2m"].as<int>();
            cachedWeather.weatherCode = doc["current"]["weather_code"].as<int>();
            cachedWeather.fetchedAtMs = millis();
            cachedWeather.valid = true;

            DEBUG_PRINTF("Weather fetched: %.1fF, %d%% humidity, code %d",
                cachedWeather.temperature, cachedWeather.humidity, cachedWeather.weatherCode);
        } else {
            DEBUG_PRINTF("Weather JSON parse error: %s", error.c_str());
        }
    } else {
        DEBUG_PRINTF("Weather fetch failed, HTTP %d", httpCode);
    }

    http.end();

    // Expire cache after 2 hours of failed refreshes so display shows "---" instead of stale data
    if (cachedWeather.valid && (millis() - cachedWeather.fetchedAtMs) > WEATHER_MAX_STALE_MS) {
        cachedWeather.valid = false;
    }
}

uint16_t BGDisplayFaceWeather::weatherCodeToColor(int code) {
    // WMO weather codes → display color
    if (code == 0) return COLOR_YELLOW;              // Clear sky → sunny yellow
    if (code <= 3) return COLOR_WHITE;               // Partly cloudy → white
    if (code <= 48) return COLOR_GRAY;               // Fog → gray
    if (code <= 67 || (code >= 80 && code <= 82)) return COLOR_BLUE;  // Rain → blue
    if (code <= 77 || (code >= 85 && code <= 86)) return COLOR_CYAN;  // Snow → cyan
    if (code >= 95) return COLOR_RED;                // Thunderstorm → red
    return COLOR_WHITE;
}

void BGDisplayFaceWeather::showWeather() const {
    if (!cachedWeather.valid) {
        DisplayManager.setTextColor(COLOR_GRAY);
        DisplayManager.printText(0, 6, "---", TEXT_ALIGNMENT::LEFT, 2);
        return;
    }

    int temp = (int)cachedWeather.temperature;
    uint16_t color = weatherCodeToColor(cachedWeather.weatherCode);

    char tempStr[5];
    snprintf(tempStr, sizeof(tempStr), "%d", temp);

    DisplayManager.setTextColor(color);
    DisplayManager.printText(0, 6, tempStr, TEXT_ALIGNMENT::LEFT, 2);

    // Draw 3-pixel color indicator at bottom-left; color encodes weather condition
    uint16_t indicatorColor = weatherCodeToColor(cachedWeather.weatherCode);
    for (int i = 0; i < 3; i++) {
        DisplayManager.drawPixel(i, 7, indicatorColor);
    }
}

void BGDisplayFaceWeather::showReadings(
    const std::list<GlucoseReading>& readings, bool dataIsOld) const {
    DisplayManager.clearMatrix();

    // BG reading on the right
    showReading(readings.back(), 31, 6, TEXT_ALIGNMENT::RIGHT, FONT_TYPE::MEDIUM, dataIsOld);
    showTrendVerticalLine(31, readings.back().trend, dataIsOld);

    // Outdoor weather on the left
    showWeather();

    BGDisplayManager_::drawTimerBlocks(readings.back(), MATRIX_WIDTH - 17, 18, 7);
}

void BGDisplayFaceWeather::showNoData() const {
    DisplayManager.clearMatrix();
    showWeather();

    String noData = "---";
    if (SettingsManager.settings.bg_units == BG_UNIT::MMOLL) {
        noData = "--.-";
    }
    DisplayManager.setTextColor(BG_COLOR_OLD);
    DisplayManager.printText(31, 6, noData.c_str(), TEXT_ALIGNMENT::RIGHT, 2);
}
