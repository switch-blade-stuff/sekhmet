//
// Created by switchblade on 2022-03-29.
//

#define TEST_PLUGIN_EXPORT
#include "test_plugin.hpp"

using namespace sek::test;

TEST_PLUGIN_API int test_plugin_data::ctr = 0;

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