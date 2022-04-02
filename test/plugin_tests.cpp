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

TEST(plugin_tests, plugin_object_test)
{
	sek::basic_object *obj = new sek::test::test_plugin_object{90};

	EXPECT_TRUE(obj->inherits<sek::test::test_toplevel_base>());
	EXPECT_TRUE(obj->inherits<sek::test::test_middle_base>());
	EXPECT_FALSE(obj->inherits<int>());

	EXPECT_TRUE(obj->has_attribute<sek::test::test_attribute>());
	EXPECT_TRUE(obj->get_attribute<sek::test::test_attribute>()->b);

	auto test_base = sek::object_cast<sek::test::test_toplevel_base>(obj);
	EXPECT_NE(test_base, nullptr);
	EXPECT_EQ(test_base->i, 90);

	delete obj;
}