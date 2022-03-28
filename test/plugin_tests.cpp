//
// Created by switchblade on 2022-01-26.
//

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "sekhmet/plugin.hpp"
#include "sekhmet/reflection.hpp"

struct test_plugin_data
{
	int i;
};

SEK_DECLARE_PLUGIN("Test Plugin", metadata<test_plugin_data{1}>)
SEK_DECLARE_PLUGIN("Test Plugin 2", metadata<test_plugin_data{2}>)

static std::string_view exec_name;
static int ctr = 0;

SEK_ON_PLUGIN_ENABLE("Test Plugin")
{
	std::string s;

	exec_name = sek::type_name<std::decay_t<decltype(*this)>>();
	ctr = 1;
}
SEK_ON_PLUGIN_DISABLE("Test Plugin 2") { ctr = 2; }

TEST(plugin_tests, plugin_db_test)
{
	auto handle2 = sek::plugin::get("Test Plugin 2");
	EXPECT_FALSE(handle2.empty());

	auto handle = sek::plugin::get("Test Plugin");
	EXPECT_FALSE(handle.empty());

	EXPECT_EQ(handle.status(), sek::plugin::status_t::INITIAL);
	EXPECT_EQ(ctr, 0);

	sek::plugin::enable(handle2);
	EXPECT_EQ(handle2.status(), sek::plugin::status_t::ENABLED);
	EXPECT_EQ(handle.status(), sek::plugin::status_t::DISABLED);
	EXPECT_EQ(ctr, 0);

	sek::plugin::enable(handle);
	EXPECT_EQ(handle.status(), sek::plugin::status_t::ENABLED);
	EXPECT_EQ(ctr, 1);

	sek::plugin::disable(handle);
	EXPECT_EQ(handle.status(), sek::plugin::status_t::DISABLED);
	EXPECT_EQ(handle2.status(), sek::plugin::status_t::ENABLED);
	EXPECT_EQ(ctr, 1);

	sek::plugin::disable(handle2);
	EXPECT_EQ(handle2.status(), sek::plugin::status_t::DISABLED);
	EXPECT_EQ(ctr, 2);

	EXPECT_NE(handle.metadata<test_plugin_data>(), nullptr);
	EXPECT_EQ(handle.metadata<int>(), nullptr);
	EXPECT_EQ(handle.metadata<test_plugin_data>()->i, 1);

	EXPECT_NE(handle2.metadata<test_plugin_data>(), nullptr);
	EXPECT_EQ(handle2.metadata<int>(), nullptr);
	EXPECT_EQ(handle2.metadata<test_plugin_data>()->i, 2);

	auto loaded_plugins = sek::plugin::all();
	EXPECT_GE(loaded_plugins.size(), 2);
	EXPECT_NE(
		std::ranges::find_if(loaded_plugins, [](const sek::plugin::handle &h) { return h.name() == "Test Plugin"; }),
		loaded_plugins.end());
	EXPECT_NE(
		std::ranges::find_if(loaded_plugins, [](const sek::plugin::handle &h) { return h.name() == "Test Plugin 2"; }),
		loaded_plugins.end());
}
