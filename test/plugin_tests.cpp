//
// Created by switchblade on 12/05/22.
//

#include <gtest/gtest.h>

#include "sekhmet/plugin.hpp"

#define TEST_PLUGIN_ID "test_plugin"

struct test_plugin
{
	static bool enabled;

	[[nodiscard]] constexpr static std::string_view id() noexcept { return TEST_PLUGIN_ID; }

	static void on_enable() { enabled = true; }
	static void on_disable() { enabled = false; }
};

SEK_PLUGIN_INSTANCE(test_plugin)
bool test_plugin::enabled = false;

TEST(plugin_tests, registration_test)
{
	auto p = sek::plugin::get(TEST_PLUGIN_ID);
	EXPECT_FALSE(p.enabled());
	EXPECT_FALSE(test_plugin::enabled);

	EXPECT_TRUE(p.enable());
	EXPECT_TRUE(p.enabled());
	EXPECT_TRUE(test_plugin::enabled);

	EXPECT_FALSE(p.enable());
	EXPECT_TRUE(p.disable());
	EXPECT_FALSE(p.enabled());
	EXPECT_FALSE(test_plugin::enabled);
}