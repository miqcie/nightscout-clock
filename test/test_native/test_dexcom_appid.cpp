// Tests for the Dexcom applicationId override resolution. Mirrors the production
// guard in src/BGSourceDexcom.cpp — if these diverge, fix the helper in
// dexcom_appid.h to match production. Regression coverage for issue #51.

#include <unity.h>

#include "dexcom_appid.h"

// Stand-in for the hardcoded Follow-app ids in src/globals.h. The exact value
// doesn't matter for these tests — only that the resolver falls back to it.
static const std::string kBuiltinDefault = "d89443d2-327c-4a6f-89e5-496bbb0317db";

static void test_appid_empty_override_uses_default(void) {
    // Fresh device: no override configured.
    TEST_ASSERT_FALSE(isUsableApplicationIdOverride(""));
    TEST_ASSERT_EQUAL_STRING(kBuiltinDefault.c_str(),
                             resolveDexcomApplicationId("", kBuiltinDefault).c_str());
}

static void test_appid_literal_null_uses_default(void) {
    // The crux of issue #51: ArduinoJson v7 yields "null" for a missing key.
    // It must be treated as "no override", not sent verbatim to Dexcom.
    TEST_ASSERT_FALSE(isUsableApplicationIdOverride("null"));
    TEST_ASSERT_EQUAL_STRING(kBuiltinDefault.c_str(),
                             resolveDexcomApplicationId("null", kBuiltinDefault).c_str());
}

static void test_appid_real_override_is_honored(void) {
    // A genuine replacement id (e.g. Dexcom rotated the Follow-app id) wins.
    const std::string custom = "11111111-2222-3333-4444-555555555555";
    TEST_ASSERT_TRUE(isUsableApplicationIdOverride(custom));
    TEST_ASSERT_EQUAL_STRING(custom.c_str(),
                             resolveDexcomApplicationId(custom, kBuiltinDefault).c_str());
}

static void test_appid_null_substring_is_still_honored(void) {
    // Only the exact 4-char "null" is rejected; a UUID that merely contains the
    // letters "null" is a valid override and must not be discarded.
    const std::string custom = "null0000-1111-2222-3333-444455556666";
    TEST_ASSERT_TRUE(isUsableApplicationIdOverride(custom));
    TEST_ASSERT_EQUAL_STRING(custom.c_str(),
                             resolveDexcomApplicationId(custom, kBuiltinDefault).c_str());
}

void register_dexcom_appid_tests(void) {
    RUN_TEST(test_appid_empty_override_uses_default);
    RUN_TEST(test_appid_literal_null_uses_default);
    RUN_TEST(test_appid_real_override_is_honored);
    RUN_TEST(test_appid_null_substring_is_still_honored);
}
