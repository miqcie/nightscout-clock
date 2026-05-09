// Tests for the BG trend direction-string mapping. Mirrors the production
// BGSource::parseDirection in src/BGSource.cpp — if these diverge, fix the
// helper in bg_trend.h to match production.

#include <unity.h>

#include "bg_trend.h"

static void test_bg_trend_lowercase_canonical(void) {
    TEST_ASSERT_EQUAL_INT(BG_TREND_DOUBLE_UP, parseDirectionString("doubleup"));
    TEST_ASSERT_EQUAL_INT(BG_TREND_SINGLE_UP, parseDirectionString("singleup"));
    TEST_ASSERT_EQUAL_INT(BG_TREND_FORTY_FIVE_UP, parseDirectionString("fortyfiveup"));
    TEST_ASSERT_EQUAL_INT(BG_TREND_FLAT, parseDirectionString("flat"));
    TEST_ASSERT_EQUAL_INT(BG_TREND_FORTY_FIVE_DOWN, parseDirectionString("fortyfivedown"));
    TEST_ASSERT_EQUAL_INT(BG_TREND_SINGLE_DOWN, parseDirectionString("singledown"));
    TEST_ASSERT_EQUAL_INT(BG_TREND_DOUBLE_DOWN, parseDirectionString("doubledown"));
    TEST_ASSERT_EQUAL_INT(BG_TREND_NOT_COMPUTABLE, parseDirectionString("not_computable"));
    TEST_ASSERT_EQUAL_INT(BG_TREND_RATE_OUT_OF_RANGE, parseDirectionString("rate_out_of_range"));
}

static void test_bg_trend_case_insensitive(void) {
    // Nightscout sends "DoubleUp" / "Flat" with mixed case; the production
    // parser lowercases first, so we must match that behavior exactly.
    TEST_ASSERT_EQUAL_INT(BG_TREND_DOUBLE_UP, parseDirectionString("DoubleUp"));
    TEST_ASSERT_EQUAL_INT(BG_TREND_DOUBLE_UP, parseDirectionString("DOUBLEUP"));
    TEST_ASSERT_EQUAL_INT(BG_TREND_FLAT, parseDirectionString("Flat"));
    TEST_ASSERT_EQUAL_INT(BG_TREND_FORTY_FIVE_DOWN, parseDirectionString("FortyFiveDown"));
    TEST_ASSERT_EQUAL_INT(BG_TREND_SINGLE_DOWN, parseDirectionString("SingleDown"));
    TEST_ASSERT_EQUAL_INT(BG_TREND_NOT_COMPUTABLE, parseDirectionString("NOT_COMPUTABLE"));
    TEST_ASSERT_EQUAL_INT(BG_TREND_RATE_OUT_OF_RANGE, parseDirectionString("Rate_Out_Of_Range"));
}

static void test_bg_trend_unknown_falls_back_to_none(void) {
    TEST_ASSERT_EQUAL_INT(BG_TREND_NONE, parseDirectionString("sideways"));
    TEST_ASSERT_EQUAL_INT(BG_TREND_NONE, parseDirectionString("unknown"));
    TEST_ASSERT_EQUAL_INT(BG_TREND_NONE, parseDirectionString("???"));
}

static void test_bg_trend_empty_string_is_none(void) {
    TEST_ASSERT_EQUAL_INT(BG_TREND_NONE, parseDirectionString(""));
}

void register_bg_trend_tests(void) {
    RUN_TEST(test_bg_trend_lowercase_canonical);
    RUN_TEST(test_bg_trend_case_insensitive);
    RUN_TEST(test_bg_trend_unknown_falls_back_to_none);
    RUN_TEST(test_bg_trend_empty_string_is_none);
}
