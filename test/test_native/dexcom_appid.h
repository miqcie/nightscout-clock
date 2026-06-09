// Portable mirror of the Dexcom applicationId override resolution in
// src/BGSourceDexcom.cpp (isUsableApplicationIdOverride / resolveDexcomApplicationId).
// Keep in sync with that file — if the production guard changes, update this
// helper so the tests keep catching regressions.
//
// Background (issue #51): ArduinoJson v7's .as<String>() returns the literal
// 4-char string "null" for a missing or null key, not an empty string. The
// override is only honored when it is a real value, so "null" must be rejected
// just like "" — otherwise it gets sent to Dexcom as the applicationId and the
// auth call fails with 500 InvalidArgument (UUID "null").

#pragma once

#include <string>

// True only when the configured override is a real value worth sending.
inline bool isUsableApplicationIdOverride(const std::string& value) {
    return value.length() > 0 && value != "null";
}

// Returns the override when usable, otherwise the built-in default.
inline std::string resolveDexcomApplicationId(const std::string& override_value,
                                              const std::string& builtin_default) {
    return isUsableApplicationIdOverride(override_value) ? override_value : builtin_default;
}
