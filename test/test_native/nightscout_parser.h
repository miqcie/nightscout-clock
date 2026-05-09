// Portable Nightscout response parser used in host-side tests.
// Mirrors the production deserialization in
// src/BGSourceNightscout.cpp BGSourceNightscout::retrieveReadings (lines
// 117-165) — keep in sync. Logic intentionally duplicated here because the
// production code path pulls in HTTPClient/DisplayManager/etc. that we can't
// link against on the host.

#pragma once

#include <ArduinoJson.h>

#include <cstdint>
#include <string>
#include <vector>

#include "bg_trend.h"

struct ParsedReading {
    int sgv;
    int trend;  // matches BG_TREND values; see bg_trend.h
    uint64_t epoch_seconds;
};

// Returns the parsed readings in the order they appear in the JSON. On any
// deserialization error returns an empty vector — matching the production
// behavior of bailing out without surfacing partial readings.
inline std::vector<ParsedReading> parseNightscoutResponse(const std::string& json) {
    std::vector<ParsedReading> readings;

    JsonDocument doc;
    JsonDocument filter;
    filter[0]["sgv"] = true;
    filter[0]["date"] = true;
    filter[0]["trend"] = true;
    filter[0]["direction"] = true;

    DeserializationError error = deserializeJson(doc, json, DeserializationOption::Filter(filter));
    if (error)
        return readings;
    if (!doc.is<JsonArray>())
        return readings;

    JsonArray arr = doc.as<JsonArray>();
    for (JsonVariant v : arr) {
        ParsedReading r;
        r.sgv = v["sgv"].as<int>();
        r.epoch_seconds = v["date"].as<uint64_t>() / 1000;  // ms -> s, matches production
        if (v["trend"].is<int>()) {
            r.trend = v["trend"].as<int>();
        } else if (v["direction"].is<const char*>()) {
            r.trend = parseDirectionString(std::string(v["direction"].as<const char*>()));
        } else {
            r.trend = BG_TREND_NONE;
        }
        readings.push_back(r);
    }
    return readings;
}
