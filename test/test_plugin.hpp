//
// Created by switch_blade on 2022-03-29.
//

#pragma once

#include "sekhmet/plugin.hpp"
#include "sekhmet/reflection.hpp"

#ifdef TEST_PLUGIN_EXPORT
#define TEST_PLUGIN_API SEK_API_EXPORT
#else
#define TEST_PLUGIN_API SEK_API_IMPORT
#endif

namespace sek::test
{
	struct TEST_PLUGIN_API test_plugin_data
	{
		static int ctr;

		int i;
	};

	struct test_parent_A
	{
	};
	struct test_parent_B : test_parent_A
	{
	};
	struct TEST_PLUGIN_API test_child : test_parent_B
	{
		static bool factory_invoked;

		constexpr test_child() noexcept = default;
		constexpr explicit test_child(double d) noexcept : d(d) {}

		double d = 0;
	};

	struct test_attribute
	{
		int i;
	};
}	 // namespace sek::test

SEK_DECLARE_TYPE(sek::test::test_child, "test_child")