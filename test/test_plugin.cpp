//
// Created by switchblade on 2022-03-29.
//

#define TEST_PLUGIN_EXPORT
#include "test_plugin.hpp"

using namespace sek::test;

TEST_PLUGIN_API int test_plugin_data::ctr = 0;

SEK_DECLARE_PLUGIN("Test Plugin", SEK_VERSION(0, 0, 1))
SEK_DECLARE_PLUGIN("Test Plugin 2", SEK_VERSION(0, 0, 1))

#include "sekhmet/assets.hpp"

SEK_ON_PLUGIN_ENABLE("Test Plugin")
{
	sek::asset_repository::global(new sek::asset_repository{});

	test_plugin_data::ctr = 1;
}
SEK_ON_PLUGIN_DISABLE("Test Plugin 2")
{
	delete sek::asset_repository::global(nullptr);

	test_plugin_data::ctr = 2;
}