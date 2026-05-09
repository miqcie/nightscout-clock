// Portable mapping from Nightscout/Dexcom direction strings to BG_TREND values.
// Mirrors the production logic in BGSource::parseDirection — keep in sync with
// src/BGSource.cpp parseDirection. The integer return matches the BG_TREND enum
// in src/enums.h (NONE=0 .. RATE_OUT_OF_RANGE=9) so we don't need to drag the
// Arduino-flavored enums.h header into the native test build.

#pragma once

#include <algorithm>
#include <cctype>
#include <string>

// Values match enum class BG_TREND in src/enums.h.
enum BgTrendValue {
    BG_TREND_NONE = 0,
    BG_TREND_DOUBLE_UP = 1,
    BG_TREND_SINGLE_UP = 2,
    BG_TREND_FORTY_FIVE_UP = 3,
    BG_TREND_FLAT = 4,
    BG_TREND_FORTY_FIVE_DOWN = 5,
    BG_TREND_SINGLE_DOWN = 6,
    BG_TREND_DOUBLE_DOWN = 7,
    BG_TREND_NOT_COMPUTABLE = 8,
    BG_TREND_RATE_OUT_OF_RANGE = 9,
};

inline int parseDirectionString(const std::string& input) {
    std::string direction = input;
    std::transform(direction.begin(), direction.end(), direction.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    if (direction == "doubleup")
        return BG_TREND_DOUBLE_UP;
    if (direction == "singleup")
        return BG_TREND_SINGLE_UP;
    if (direction == "fortyfiveup")
        return BG_TREND_FORTY_FIVE_UP;
    if (direction == "flat")
        return BG_TREND_FLAT;
    if (direction == "fortyfivedown")
        return BG_TREND_FORTY_FIVE_DOWN;
    if (direction == "singledown")
        return BG_TREND_SINGLE_DOWN;
    if (direction == "doubledown")
        return BG_TREND_DOUBLE_DOWN;
    if (direction == "not_computable")
        return BG_TREND_NOT_COMPUTABLE;
    if (direction == "rate_out_of_range")
        return BG_TREND_RATE_OUT_OF_RANGE;
    return BG_TREND_NONE;
}
