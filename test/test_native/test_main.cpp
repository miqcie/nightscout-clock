// Host-side unit tests. These run on the build machine via `pio test -e native`
// and must not depend on any Arduino / ESP32 headers — anything that pulls in
// HardwareSerial, FastLED, LittleFS, etc. needs to be mocked or refactored into
// a pure-logic helper before it can be tested here.
//
// This file owns the single main() entry point. Each additional test_*.cpp
// file exposes a register_<suite>_tests() function that calls RUN_TEST for its
// own test functions; main() invokes them in order. This avoids multiple
// definitions of main() when PlatformIO compiles every .cpp in test/test_native/.

#include <unity.h>

#include "bg_units.h"

void setUp(void) {}
void tearDown(void) {}

// Forward declarations for suites defined in sibling .cpp files.
void register_bg_trend_tests(void);
void register_nightscout_parser_tests(void);

static void test_mgdl_to_mmol_basic(void) {
    // Standard conversion factor is 18.0 mg/dL per mmol/L.
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.55f, mgdl_to_mmol(100));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, mgdl_to_mmol(180));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, mgdl_to_mmol(0));
}

static void test_mmol_to_mgdl_basic(void) {
    TEST_ASSERT_EQUAL_INT(100, mmol_to_mgdl(5.55f));
    TEST_ASSERT_EQUAL_INT(180, mmol_to_mgdl(10.0f));
    TEST_ASSERT_EQUAL_INT(0, mmol_to_mgdl(0.0f));
}

static void test_round_trip_is_stable(void) {
    // Round-tripping a typical clinical value should land within 1 mg/dL.
    static const int kClinicalValues[] = {55, 70, 100, 140, 180, 240};
    for (int mgdl : kClinicalValues) {
        int round_tripped = mmol_to_mgdl(mgdl_to_mmol(mgdl));
        TEST_ASSERT_INT_WITHIN(1, mgdl, round_tripped);
    }
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_mgdl_to_mmol_basic);
    RUN_TEST(test_mmol_to_mgdl_basic);
    RUN_TEST(test_round_trip_is_stable);
    register_bg_trend_tests();
    register_nightscout_parser_tests();
    return UNITY_END();
}
