//
// Created by switch_blade on 2022-03-29.
//

#define TEST_PLUGIN_EXPORT
#include "test_plugin.hpp"

using namespace sek::test;

TEST_PLUGIN_API bool test_child::factory_invoked = false;
TEST_PLUGIN_API int test_plugin_data::ctr = 0;

SEK_TYPE_FACTORY(test_child)
{
	parents<test_parent_A, test_parent_B>();
	parents<test_parent_A>();

	attributes<test_attribute{9}>();

	constructor<std::reference_wrapper<const test_child>>();
	constructor<double>();

	test_child::factory_invoked = true;
}

SEK_DECLARE_PLUGIN("Test Plugin", metadata<test_plugin_data{1}>)
SEK_DECLARE_PLUGIN("Test Plugin 2", metadata<test_plugin_data{2}>)


SEK_ON_PLUGIN_ENABLE("Test Plugin")
{
	test_plugin_data::ctr = 1;
}
SEK_ON_PLUGIN_DISABLE("Test Plugin 2")
{
	test_plugin_data::ctr = 2;
}