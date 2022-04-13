//
// Created by switchblade on 2022-03-29.
//

#pragma once

#include "sekhmet/object.hpp"
#include "sekhmet/plugin.hpp"
#include "sekhmet/type_info.hpp"

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

	struct test_attribute
	{
		bool b;
	};

	struct test_toplevel_base : basic_object
	{
		constexpr test_toplevel_base() noexcept = default;
		constexpr explicit test_toplevel_base(int i) noexcept : i(i) {}

		int i = 0;
	};
	struct test_middle_base : test_toplevel_base
	{
		SEK_OBJECT_BODY

	public:
		constexpr test_middle_base() noexcept = default;
		constexpr explicit test_middle_base(int i) noexcept : test_toplevel_base(i) {}
	};

	struct test_plugin_object : test_middle_base
	{
		SEK_OBJECT_BODY

	public:
		constexpr test_plugin_object() noexcept = default;
		constexpr explicit test_plugin_object(int i) noexcept : test_middle_base(i) {}
	};
}	 // namespace sek::test

SEK_REFLECT_TYPE(sek::test::test_middle_base) { parents<sek::test::test_toplevel_base>(); }

SEK_REFLECT_TYPE(sek::test::test_plugin_object)
{
	parents<sek::test::test_middle_base>();
	attributes<sek::test::test_attribute{true}>();
}