// Tests for the Nightscout JSON response parser. Mirrors the production
// deserialization in src/BGSourceNightscout.cpp::retrieveReadings.
//
// The fixture is also stored at test/fixtures/nightscout_basic.json so that
// humans can eyeball the realistic shape of the data; the inline copy below
// keeps the test self-contained regardless of cwd at runtime.

#include <unity.h>

#include <string>

#include "nightscout_parser.h"

// Mirror of test/fixtures/nightscout_basic.json. Three readings exercising:
//   - numeric trend (reading 0)
//   - missing trend, string direction (reading 1)
//   - missing trend AND missing direction => NONE (reading 2)
static const char* kBasicFixture = R"JSON(
[
  {
    "_id": "anonymized-001",
    "sgv": 142,
    "date": 1714500000000,
    "dateString": "2024-04-30T18:00:00.000Z",
    "trend": 4,
    "direction": "Flat",
    "device": "test-cgm",
    "type": "sgv"
  },
  {
    "_id": "anonymized-002",
    "sgv": 158,
    "date": 1714500300000,
    "dateString": "2024-04-30T18:05:00.000Z",
    "direction": "FortyFiveUp",
    "device": "test-cgm",
    "type": "sgv"
  },
  {
    "_id": "anonymized-003",
    "sgv": 175,
    "date": 1714500600000,
    "dateString": "2024-04-30T18:10:00.000Z",
    "device": "test-cgm",
    "type": "sgv"
  }
]
)JSON";

static void test_nightscout_parses_three_readings(void) {
    auto readings = parseNightscoutResponse(kBasicFixture);
    TEST_ASSERT_EQUAL_size_t(3, readings.size());
}

static void test_nightscout_extracts_sgv(void) {
    auto readings = parseNightscoutResponse(kBasicFixture);
    TEST_ASSERT_EQUAL_INT(142, readings[0].sgv);
    TEST_ASSERT_EQUAL_INT(158, readings[1].sgv);
    TEST_ASSERT_EQUAL_INT(175, readings[2].sgv);
}

static void test_nightscout_converts_ms_to_seconds(void) {
    // 1714500000000 ms / 1000 = 1714500000 s
    auto readings = parseNightscoutResponse(kBasicFixture);
    TEST_ASSERT_EQUAL_UINT64(1714500000ULL, readings[0].epoch_seconds);
    TEST_ASSERT_EQUAL_UINT64(1714500300ULL, readings[1].epoch_seconds);
    TEST_ASSERT_EQUAL_UINT64(1714500600ULL, readings[2].epoch_seconds);
}

static void test_nightscout_prefers_numeric_trend(void) {
    auto readings = parseNightscoutResponse(kBasicFixture);
    // Reading 0 has trend=4 (FLAT) — numeric trend wins regardless of direction.
    TEST_ASSERT_EQUAL_INT(BG_TREND_FLAT, readings[0].trend);
}

static void test_nightscout_falls_back_to_direction_string(void) {
    auto readings = parseNightscoutResponse(kBasicFixture);
    // Reading 1 has no trend, only direction="FortyFiveUp".
    TEST_ASSERT_EQUAL_INT(BG_TREND_FORTY_FIVE_UP, readings[1].trend);
}

static void test_nightscout_missing_trend_is_none(void) {
    auto readings = parseNightscoutResponse(kBasicFixture);
    // Reading 2 has neither trend nor direction.
    TEST_ASSERT_EQUAL_INT(BG_TREND_NONE, readings[2].trend);
}

static void test_nightscout_malformed_json_returns_empty(void) {
    auto readings = parseNightscoutResponse("{ not valid json");
    TEST_ASSERT_EQUAL_size_t(0, readings.size());
}

static void test_nightscout_empty_array_returns_empty(void) {
    auto readings = parseNightscoutResponse("[]");
    TEST_ASSERT_EQUAL_size_t(0, readings.size());
}

void register_nightscout_parser_tests(void) {
    RUN_TEST(test_nightscout_parses_three_readings);
    RUN_TEST(test_nightscout_extracts_sgv);
    RUN_TEST(test_nightscout_converts_ms_to_seconds);
    RUN_TEST(test_nightscout_prefers_numeric_trend);
    RUN_TEST(test_nightscout_falls_back_to_direction_string);
    RUN_TEST(test_nightscout_missing_trend_is_none);
    RUN_TEST(test_nightscout_malformed_json_returns_empty);
    RUN_TEST(test_nightscout_empty_array_returns_empty);
}
