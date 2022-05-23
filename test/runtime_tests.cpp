//
// Created by switchblade on 17/05/22.
//

#include <gtest/gtest.h>

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

#include "sekhmet/logger.hpp"
#include "sekhmet/plugin.hpp"

static bool plugin_enabled = false;

SEK_PLUGIN("test_plugin")
{
	sek::logger::info() << fmt::format("Initializing plugin \"{}\"", info.id);

	on_enable += +[]()
	{
		sek::type_info::reflect<test_parent_middle>().parent<test_parent_top>();
		plugin_enabled = true;
		return true;
	};
	on_disable += +[]()
	{
		sek::type_info::reset<test_parent_middle>();
		plugin_enabled = false;
	};
}

TEST(utility_tests, plugin_test)
{
	auto p = sek::plugin::get("test_plugin");
	EXPECT_FALSE(p.enabled());
	EXPECT_FALSE(plugin_enabled);

	/* Enable from loaded. */
	EXPECT_TRUE(p.enable());
	EXPECT_TRUE(p.enabled());
	EXPECT_TRUE(plugin_enabled);

	/* Double-enable & disable from enabled. */
	EXPECT_FALSE(p.enable());
	EXPECT_TRUE(p.disable());
	EXPECT_FALSE(p.enabled());
	EXPECT_FALSE(plugin_enabled);

	/* Enable from disabled. */
	EXPECT_TRUE(p.enable());
	EXPECT_TRUE(p.enabled());
	EXPECT_TRUE(plugin_enabled);
}

TEST(utility_tests, type_info_test)
{
	// clang-format off
	sek::type_info::reflect<test_child>()
		.attrib<int>(0xff).attrib<0xfc>().attrib<test_attribute>()
		.parent<test_parent_middle>();
	// clang-format on

	auto info = sek::type_info::get<test_child>();

	EXPECT_EQ(info, sek::type_info::get("test_child"));
	EXPECT_TRUE(info.valid());
	EXPECT_EQ(info.name(), "test_child");
	EXPECT_EQ(info.name(), sek::type_name<test_child>());
	EXPECT_TRUE(info.is_empty());
	EXPECT_FALSE(info.has_extent());
	EXPECT_EQ(info.extent(), 0);
	EXPECT_FALSE(info.is_range());
	EXPECT_FALSE(info.is_pointer());
	EXPECT_EQ(info.value_type(), info);

	EXPECT_TRUE(info.inherits<test_parent_middle>());
	EXPECT_TRUE(info.inherits<test_parent_top>());
	EXPECT_TRUE(info.inherits("top_parent"));
	EXPECT_FALSE(info.parents().empty());

	constexpr auto pred = [](auto p) { return p.type() == sek::type_info::get<test_parent_middle>(); };
	EXPECT_TRUE(std::ranges::any_of(info.parents(), pred));

	EXPECT_TRUE(sek::type_info::get<test_child[2]>().has_extent());
	EXPECT_TRUE(sek::type_info::get<test_child[2]>().is_range());
	EXPECT_EQ(sek::type_info::get<test_child[2]>().extent(), 2);
	EXPECT_EQ(sek::type_info::get<test_child[2]>().value_type(), info);

	EXPECT_FALSE(sek::type_info::get<test_child[]>().has_extent());
	EXPECT_TRUE(sek::type_info::get<test_child[]>().is_array());
	EXPECT_FALSE(sek::type_info::get<test_child[]>().is_range());
	EXPECT_FALSE(sek::type_info::get<test_child[]>().is_pointer());
	EXPECT_EQ(sek::type_info::get<test_child[]>().extent(), 0);
	EXPECT_NE(sek::type_info::get<test_child[]>().value_type(), info);

	EXPECT_TRUE(sek::type_info::get<test_child *>().is_pointer());
	EXPECT_EQ(sek::type_info::get<test_child *>().value_type(), info);
	EXPECT_EQ(sek::type_info::get<const test_child *>().value_type(), sek::type_info::get<const test_child>());

	sek::type_info::reset<test_child>();
	EXPECT_FALSE(sek::type_info::get("test_child"));
}

namespace
{
	struct test_parent_i
	{
		constexpr test_parent_i() noexcept = default;
		constexpr test_parent_i(int i) noexcept : i(i) {}

		constexpr bool operator==(const test_parent_i &) const noexcept = default;

		int i;
	};
	struct test_parent_f
	{
		constexpr test_parent_f() noexcept = default;
		constexpr test_parent_f(float f) noexcept : f(f) {}

		constexpr bool operator==(const test_parent_f &) const noexcept = default;

		float f;
	};
	struct test_child_if : test_parent_i, test_parent_f
	{
		constexpr test_child_if() noexcept = default;
		constexpr test_child_if(int i, float f) noexcept : test_parent_i(i), test_parent_f(f) {}

		constexpr bool operator==(const test_child_if &) const noexcept = default;
	};
}	 // namespace

TEST(utility_tests, any_test)
{
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
	{
		sek::type_info::reflect<test_child_if>().parent<test_parent_i>().parent<test_parent_f>();
		const auto info = sek::type_info::get<test_child_if>();
		const auto data = test_child_if{10, std::numbers::pi_v<float>};
		auto a1 = sek::make_any<test_child_if>(data);

		EXPECT_FALSE(a1.empty());
		EXPECT_NE(a1.as_ptr<test_child_if>(), nullptr);
		EXPECT_EQ(*a1.as_ptr<test_child_if>(), data);

		const auto parents = info.parents();
		EXPECT_FALSE(parents.empty());

		constexpr auto pred_i = [](auto p) { return p.type() == sek::type_info::get<test_parent_i>(); };
		auto parent_i = std::find_if(parents.begin(), parents.end(), pred_i);
		EXPECT_NE(parent_i, parents.end());

		auto a2 = parent_i->cast(a1.ref());
		EXPECT_TRUE(a2.is_ref());
		EXPECT_NE(a2.as_ptr<test_parent_i>(), nullptr);
		EXPECT_EQ(a2.as_ptr<test_parent_i>(), a1.as_ptr<test_child_if>());
		EXPECT_EQ(*a2.as_ptr<test_parent_i>(), data);

		constexpr auto pred_f = [](auto p) { return p.type() == sek::type_info::get<test_parent_f>(); };
		auto parent_f = std::find_if(parents.begin(), parents.end(), pred_f);
		EXPECT_NE(parent_f, parents.end());

		a2 = parent_f->cast(a1.ref());
		EXPECT_TRUE(a2.is_ref());
		EXPECT_NE(a2.as_ptr<test_parent_f>(), nullptr);
		EXPECT_EQ(a2.as_ptr<test_parent_f>(), a1.as_ptr<test_child_if>());
		EXPECT_EQ(*a2.as_ptr<test_parent_f>(), data);

#ifndef NDEBUG
		/* Parent cast cannot be by-value. */
		EXPECT_DEATH(a2 = parent_f->cast(std::as_const(a1)), ".*");
#endif
	}
}
