//
// Created by switchblade on 17/05/22.
//

#include <gtest/gtest.h>

#include "sekhmet/logger.hpp"
#include "sekhmet/plugin.hpp"

static bool plugin_enabled = false;

SEK_PLUGIN(test_plugin)
{
	sek::logger::info() << fmt::format("Initializing plugin \"{}\"", id);

	on_enable += +[]()
	{
		plugin_enabled = true;
		return true;
	};
	on_disable += +[]() { plugin_enabled = false; };
}

TEST(utility_tests, plugin_test)
{
	auto p = sek::plugin::get("test_plugin");
	EXPECT_FALSE(p.enabled());
	EXPECT_FALSE(plugin_enabled);

	EXPECT_TRUE(p.enable());
	EXPECT_TRUE(p.enabled());
	EXPECT_TRUE(plugin_enabled);

	EXPECT_FALSE(p.enable());
	EXPECT_TRUE(p.disable());
	EXPECT_FALSE(p.enabled());
	EXPECT_FALSE(plugin_enabled);
}

#include "sekhmet/detail/type_info.hpp"
#include "sekhmet/type_info.hpp"

namespace
{
	struct test_parent_top
	{
	};
	struct test_parent_middle : test_parent_top
	{
	};
	struct test_child : test_parent_middle
	{
	};
	struct test_attribute
	{
	};
}	 // namespace

template<>
constexpr std::string_view sek::type_name<test_parent_top>() noexcept
{
	return "top_parent";
}
template<>
constexpr std::string_view sek::type_name<test_child>() noexcept
{
	return "test_child";
}

SEK_EXTERN_TYPE(test_child)
SEK_EXPORT_TYPE(test_child)

TEST(utility_tests, type_info_test)
{
	sek::type_info::reflect<test_parent_middle>().parent<test_parent_top>();

	// clang-format off
	sek::type_info::reflect<test_child>()
		.attrib<int>(0xff).attrib<int>(0xfc).attrib<test_attribute>()
		.parent<test_parent_middle>();
	// clang-format on

	auto info = sek::type_info::get<test_child>();

	EXPECT_EQ(info, sek::type_info::get("test_child"));
	EXPECT_TRUE(info.valid());
	EXPECT_EQ(info.name(), "test_child");
	EXPECT_EQ(info.name(), sek::type_name<test_child>());
	EXPECT_EQ(info.size(), sizeof(test_child));
	EXPECT_EQ(info.align(), alignof(test_child));
	EXPECT_EQ(info.extent(), 0);
	EXPECT_EQ(info.remove_cv(), info);
	EXPECT_TRUE(info.is_empty());
	EXPECT_FALSE(info.is_qualified());
	EXPECT_FALSE(info.is_array());
	EXPECT_FALSE(info.is_pointer());
	EXPECT_FALSE(info.remove_extent().valid());
	EXPECT_FALSE(info.remove_pointer().valid());

	EXPECT_TRUE(info.inherits<test_parent_middle>());
	EXPECT_TRUE(info.inherits<test_parent_top>());
	EXPECT_TRUE(info.inherits("top_parent"));

	EXPECT_TRUE(sek::type_info::get<const test_child>().is_qualified());
	EXPECT_TRUE(sek::type_info::get<const test_child>().is_const());
	EXPECT_FALSE(sek::type_info::get<const test_child>().is_volatile());
	EXPECT_EQ(sek::type_info::get<const test_child>().remove_cv(), info);

	EXPECT_TRUE(sek::type_info::get<test_child[2]>().is_array());
	EXPECT_EQ(sek::type_info::get<test_child[2]>().extent(), 2);
	EXPECT_EQ(sek::type_info::get<test_child[2]>().remove_extent(), info);

	EXPECT_TRUE(sek::type_info::get<test_child *>().is_pointer());
	EXPECT_EQ(sek::type_info::get<test_child *>().remove_pointer(), info);
	EXPECT_EQ(sek::type_info::get<const test_child *>().remove_pointer(), sek::type_info::get<const test_child>());

	sek::type_info::reset<test_child>();
	EXPECT_FALSE(sek::type_info::get("test_child"));
}

#include "sekhmet/object.hpp"

namespace
{
	struct test_parent_object : sek::object
	{
		SEK_OBJECT_BODY(test_parent_object)

		constexpr bool operator==(const test_parent_object &other) const noexcept { return d == other.d; }

		int d = 0x4444'4444;
	};
	struct test_child_object_a : test_parent_object
	{
		SEK_OBJECT_BODY(test_child_object_a)

		constexpr bool operator==(const test_child_object_a &) const noexcept = default;

		int a = 0x4141'4141;
	};
	struct test_child_object_b : test_parent_object
	{
		SEK_OBJECT_BODY(test_child_object_b)

		constexpr bool operator==(const test_child_object_b &) const noexcept = default;

		int b = 0x4242'4242;
	};
	struct test_child_object_c : test_child_object_b
	{
		SEK_OBJECT_BODY(test_child_object_c)

		constexpr bool operator==(const test_child_object_c &) const noexcept = default;

		int c = 0x4343'4343;
	};
}	 // namespace

TEST(utility_tests, object_test)
{
	sek::type_info::reflect<test_child_object_c>().parent<test_child_object_b>();

	test_child_object_a child_a{};
	test_child_object_b child_b{};
	test_child_object_c child_c{};

	const auto *parent_a = static_cast<const test_parent_object *>(&child_a);
	const auto *parent_b = static_cast<const test_parent_object *>(&child_b);
	const auto *parent_c = static_cast<const test_parent_object *>(&child_c);

	EXPECT_EQ(parent_a->type_of(), sek::type_info::get<test_child_object_a>());
	EXPECT_EQ(parent_b->type_of(), sek::type_info::get<test_child_object_b>());
	EXPECT_EQ(parent_c->type_of(), sek::type_info::get<test_child_object_c>());

	auto *child_a_ptr = sek::object_cast<test_child_object_a>(parent_a);
	EXPECT_NE(child_a_ptr, nullptr);
	EXPECT_EQ(*child_a_ptr, child_a);

	child_a_ptr = sek::object_cast<test_child_object_a>(parent_b);
	EXPECT_EQ(child_a_ptr, nullptr);

	auto *child_b_ptr = sek::object_cast<test_child_object_b>(parent_b);
	EXPECT_NE(child_b_ptr, nullptr);
	EXPECT_EQ(*child_b_ptr, child_b);

	child_b_ptr = sek::object_cast<test_child_object_b>(parent_c);
	EXPECT_NE(child_b_ptr, nullptr);

	auto *child_c_ptr = sek::object_cast<test_child_object_c>(child_b_ptr);
	EXPECT_NE(child_c_ptr, nullptr);
	EXPECT_EQ(*child_c_ptr, child_c);
}