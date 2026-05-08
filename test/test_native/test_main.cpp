// Host-side unit tests. These run on the build machine via `pio test -e native`
// and must not depend on any Arduino / ESP32 headers — anything that pulls in
// HardwareSerial, FastLED, LittleFS, etc. needs to be mocked or refactored into
// a pure-logic helper before it can be tested here.
//
// Add new tests to this file (or split into more files in test/test_native/).
// Every test function must be registered in setup() with RUN_TEST.

#include <unity.h>

#include "bg_units.h"

void setUp(void) {}
void tearDown(void) {}

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
    for (int mgdl : {55, 70, 100, 140, 180, 240}) {
        int round_tripped = mmol_to_mgdl(mgdl_to_mmol(mgdl));
        TEST_ASSERT_INT_WITHIN(1, mgdl, round_tripped);
    }
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_mgdl_to_mmol_basic);
    RUN_TEST(test_mmol_to_mgdl_basic);
    RUN_TEST(test_round_trip_is_stable);
    return UNITY_END();
}
