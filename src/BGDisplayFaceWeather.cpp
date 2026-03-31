#include "BGDisplayFaceWeather.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>

#include "BGDisplayManager.h"
#include "DisplayManager.h"
#include "SettingsManager.h"
#include "globals.h"

// Static member initialization
WeatherData BGDisplayFaceWeather::cachedWeather = {0, 0, -1, 0, false};

// Cache duration: 30 minutes
static const unsigned long WEATHER_CACHE_MS = 30UL * 60UL * 1000UL;

// Default location (Richmond, VA) — will be overridden by settings if available
static const float DEFAULT_LAT = 37.5407f;
static const float DEFAULT_LON = -77.4360f;

void BGDisplayFaceWeather::fetchWeather() {
    // Don't fetch if cache is still fresh
    if (cachedWeather.valid && (millis() - cachedWeather.fetchedAtMs) < WEATHER_CACHE_MS) {
        return;
    }

    float lat = DEFAULT_LAT;
    float lon = DEFAULT_LON;
    // TODO: Use settings for lat/lon when available
    // if (SettingsManager.settings.weather_lat != 0) lat = SettingsManager.settings.weather_lat;
    // if (SettingsManager.settings.weather_lon != 0) lon = SettingsManager.settings.weather_lon;

    HTTPClient http;
    String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(lat, 4) +
                 "&longitude=" + String(lon, 4) +
                 "&current=temperature_2m,relative_humidity_2m,weather_code"
                 "&temperature_unit=fahrenheit";

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

    // Draw a weather indicator pixel pattern at bottom-left (3x1)
    // Simple: sun=yellow dot, cloud=gray dot, rain=blue dot, snow=cyan dot
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
