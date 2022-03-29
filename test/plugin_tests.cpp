//
// Created by switchblade on 2022-01-26.
//

#include <gtest/gtest.h>

#include <algorithm>
#include <string>

#include "sekhmet/plugin.hpp"
#include "test_plugin.hpp"

using namespace sek::test;

TEST(plugin_tests, plugin_db_test)
{
	auto handle2 = sek::plugin::get("Test Plugin 2");
	EXPECT_FALSE(handle2.empty());

	auto handle = sek::plugin::get("Test Plugin");
	EXPECT_FALSE(handle.empty());

	EXPECT_EQ(handle.status(), sek::plugin::status_t::INITIAL);
	EXPECT_EQ(test_plugin_data::ctr, 0);

	sek::plugin::enable(handle2);
	EXPECT_EQ(handle2.status(), sek::plugin::status_t::ENABLED);
	EXPECT_EQ(handle.status(), sek::plugin::status_t::DISABLED);
	EXPECT_EQ(test_plugin_data::ctr, 0);

	sek::plugin::enable(handle);
	EXPECT_EQ(handle.status(), sek::plugin::status_t::ENABLED);
	EXPECT_EQ(test_plugin_data::ctr, 1);

	sek::plugin::disable(handle);
	EXPECT_EQ(handle.status(), sek::plugin::status_t::DISABLED);
	EXPECT_EQ(handle2.status(), sek::plugin::status_t::ENABLED);
	EXPECT_EQ(test_plugin_data::ctr, 1);

	sek::plugin::disable(handle2);
	EXPECT_EQ(handle2.status(), sek::plugin::status_t::DISABLED);
	EXPECT_EQ(test_plugin_data::ctr, 2);

	EXPECT_NE(handle.metadata<test_plugin_data>(), nullptr);
	EXPECT_EQ(handle.metadata<int>(), nullptr);
	EXPECT_EQ(handle.metadata<test_plugin_data>()->i, 1);

	EXPECT_NE(handle2.metadata<test_plugin_data>(), nullptr);
	EXPECT_EQ(handle2.metadata<int>(), nullptr);
	EXPECT_EQ(handle2.metadata<test_plugin_data>()->i, 2);

	auto loaded_plugins = sek::plugin::all();
	EXPECT_GE(loaded_plugins.size(), 2);
	EXPECT_NE(std::ranges::find_if(loaded_plugins, [](const sek::plugin::handle &h) { return h.name() == "Test Plugin"; }),
			  loaded_plugins.end());
	EXPECT_NE(
		std::ranges::find_if(loaded_plugins, [](const sek::plugin::handle &h) { return h.name() == "Test Plugin 2"; }),
		loaded_plugins.end());
}
