//
// Created by switchblade on 17/05/22.
//

#include <gtest/gtest.h>

#include "sekhmet/logger.hpp"
#include "sekhmet/plugin.hpp"

static bool plugin_enabled = false;

SEK_PLUGIN("test_plugin")
{
	sek::logger::info() << fmt::format("Initializing plugin \"{}\"", info.id);

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
	//	sek::type_info::reflect<test_parent_middle>().parent<test_parent_top>();

	// clang-format off
	sek::type_info::reflect<test_child>();
//		.attrib<int>(0xff).attrib<int>(0xfc).attrib<test_attribute>()
//		.parent<test_parent_middle>();
	// clang-format on

	auto info = sek::type_info::get<test_child>();

	EXPECT_EQ(info, sek::type_info::get("test_child"));
	EXPECT_TRUE(info.valid());
	EXPECT_EQ(info.name(), "test_child");
	EXPECT_EQ(info.name(), sek::type_name<test_child>());
	EXPECT_EQ(info.size(), sizeof(test_child));
	EXPECT_EQ(info.align(), alignof(test_child));
	EXPECT_TRUE(info.is_empty());
	EXPECT_FALSE(info.has_extent());
	EXPECT_EQ(info.extent(), 0);
	EXPECT_FALSE(info.is_range());
	EXPECT_FALSE(info.is_pointer());
	EXPECT_EQ(info.value_type(), info);

	//	EXPECT_TRUE(info.inherits<test_parent_middle>());
	//	EXPECT_TRUE(info.inherits<test_parent_top>());
	//	EXPECT_TRUE(info.inherits("top_parent"));

	EXPECT_TRUE(sek::type_info::get<test_child[2]>().has_extent());
	EXPECT_TRUE(sek::type_info::get<test_child[2]>().is_range());
	EXPECT_EQ(sek::type_info::get<test_child[2]>().extent(), 2);
	EXPECT_EQ(sek::type_info::get<test_child[2]>().value_type(), info);

	EXPECT_TRUE(sek::type_info::get<test_child *>().is_pointer());
	EXPECT_EQ(sek::type_info::get<test_child *>().value_type(), info);
	EXPECT_EQ(sek::type_info::get<const test_child *>().value_type(), sek::type_info::get<const test_child>());

	sek::type_info::reset<test_child>();
	EXPECT_FALSE(sek::type_info::get("test_child"));
}

TEST(utility_tests, any_test)
{
	using data_t = std::array<int, 4>;
	data_t data = {0, 1, 2, 3};
	auto a1 = sek::make_any<data_t>(data);

	EXPECT_FALSE(a1.is_local());
	EXPECT_FALSE(a1.is_const());
	EXPECT_FALSE(a1.is_ref());
	EXPECT_NE(a1.as_ptr<data_t>(), nullptr);
	EXPECT_EQ(*a1.as_ptr<data_t>(), data);
	EXPECT_NE(a1.data(), &data);

	auto a2 = a1.ref();
	EXPECT_FALSE(a2.is_local());
	EXPECT_FALSE(a2.is_const());
	EXPECT_TRUE(a2.is_ref());
	EXPECT_NE(a2.as_ptr<data_t>(), nullptr);
	EXPECT_EQ(*a2.as_ptr<data_t>(), data);
	EXPECT_EQ(a2.data(), a1.data());

	auto a3 = std::as_const(a2);
	EXPECT_FALSE(a3.is_local());
	EXPECT_FALSE(a3.is_const());
	EXPECT_FALSE(a3.is_ref());
	EXPECT_NE(a3.as_ptr<data_t>(), nullptr);
	EXPECT_EQ(*a3.as_ptr<data_t>(), data);
	EXPECT_NE(a3.data(), a1.data());

	a1 = sek::forward_any(data);
	EXPECT_FALSE(a1.is_local());
	EXPECT_FALSE(a1.is_const());
	EXPECT_TRUE(a1.is_ref());
	EXPECT_NE(a1.as_cptr<data_t>(), nullptr);
	EXPECT_EQ(*a1.as_cptr<data_t>(), data);
	EXPECT_EQ(a1.data(), &data);

	a1 = sek::forward_any(std::as_const(data));
	EXPECT_FALSE(a1.is_local());
	EXPECT_TRUE(a1.is_const());
	EXPECT_TRUE(a1.is_ref());
	EXPECT_EQ(a1.as_ptr<data_t>(), nullptr);
	EXPECT_NE(a1.as_cptr<data_t>(), nullptr);
	EXPECT_EQ(*a1.as_cptr<data_t>(), data);
	EXPECT_EQ(a1.cdata(), &data);
}
