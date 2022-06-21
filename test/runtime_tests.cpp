/*
 * ============================================================================
 * Sekhmet - C++20 game engine & editor
 * Copyright (C) 2022 switchblade
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 * ============================================================================
 *
 * Created by switchblade on 17/05/22
 */

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

#include "sekhmet/engine/logger.hpp"
#include "sekhmet/engine/plugin.hpp"

static bool plugin_enabled = false;

SEK_PLUGIN("test_plugin")
{
	sek::engine::logger::info() << fmt::format("Initializing plugin \"{}\"", info.id);

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

TEST(runtime_tests, plugin_test)
{
	auto p = sek::engine::plugin::get("test_plugin");
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

TEST(runtime_tests, type_info_test)
{
	using namespace sek::literals;

	// clang-format off
	sek::type_info::reflect<test_child>()
		.attribute<int>(0xff).attribute<0xfc>().attribute<test_attribute>()
		.parent<test_parent_middle>();
	// clang-format on

	auto info = sek::type_info::get<test_child>();

	EXPECT_EQ(info, "test_child"_type);
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
	EXPECT_EQ(sek::type_info::get<const test_child *>().value_type(), sek::type_info::get<test_child>());

	sek::type_info::reset<test_child>();
	EXPECT_FALSE("test_child"_type);

	const auto attribs = info.attributes();
	// clang-format off
	EXPECT_TRUE(info.has_attribute<int>());
	EXPECT_TRUE(info.has_attribute<test_attribute>());
	EXPECT_TRUE(std::any_of(attribs.begin(), attribs.end(), [](auto n) { return n.value() == sek::make_any<int>(0xff); }));
	EXPECT_TRUE(std::any_of(attribs.begin(), attribs.end(), [](auto n) { return n.value() == sek::make_any<int>(0xfc); }));
	// clang-format on

	auto a1 = info.construct();
	EXPECT_FALSE(a1.empty());
	auto a1c = std::as_const(a1).ref();
	auto a2 = std::as_const(a1c).convert("top_parent");
	EXPECT_FALSE(a2.empty());
	EXPECT_EQ(a2.as_cptr<test_parent_top>(), a1.as_ptr<test_child>());
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

		constexpr void set_i(int v) noexcept { i = v; }
		[[nodiscard]] constexpr int &get_i() noexcept { return i; }
		[[nodiscard]] constexpr const int &get_i_const() const noexcept { return i; }

		constexpr bool operator==(const test_child_if &) const noexcept = default;
	};
}	 // namespace

TEST(runtime_tests, any_test)
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
		sek::type_info::reflect<int>().convertible<float>();

		const auto info = sek::type_info::get<int>();
		const auto data = 10;
		const auto a1 = sek::make_any<int>(data);

		const auto convs = info.conversions();
		EXPECT_FALSE(convs.empty());
		EXPECT_TRUE(info.convertible_to<float>());

		auto a2 = convs.front().convert(a1.ref());
		EXPECT_FALSE(a2.empty());
		EXPECT_EQ(a2, sek::make_any<float>(static_cast<float>(data)));

		auto a3 = a1.convert(sek::type_info::get<float>());
		EXPECT_FALSE(a3.empty());
		EXPECT_EQ(a3, sek::make_any<float>(static_cast<float>(data)));
		EXPECT_EQ(a3, a2);
	}
	{
		// clang-format off
		sek::type_info::reflect<test_child_if>()
			.constructor<int, float>().constructor<const test_child_if &>()
			.parent<test_parent_i>().parent<test_parent_f>()
			.function<&test_child_if::get_i>("get_i").function<&test_child_if::set_i>("set_i")
			.function<&test_child_if::get_i_const>("get_i_const");
		// clang-format on
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

		auto ar1 = parent_i->cast(a1.ref());
		EXPECT_NE(ar1.as_ptr<test_parent_i>(), nullptr);
		EXPECT_EQ(ar1.as_ptr<test_parent_i>(), a1.as_ptr<test_child_if>());
		EXPECT_EQ(*ar1.as_ptr<test_parent_i>(), data);

		constexpr auto pred_f = [](auto p) { return p.type() == sek::type_info::get<test_parent_f>(); };
		auto parent_f = std::find_if(parents.begin(), parents.end(), pred_f);
		EXPECT_NE(parent_f, parents.end());

		ar1 = parent_f->cast(a1.ref());
		EXPECT_NE(ar1.as_ptr<test_parent_f>(), nullptr);
		EXPECT_EQ(ar1.as_ptr<test_parent_f>(), a1.as_ptr<test_child_if>());
		EXPECT_EQ(*ar1.as_ptr<test_parent_f>(), data);

		auto *pf = a1.try_cast<test_parent_f>();
		EXPECT_NE(pf, nullptr);
		EXPECT_EQ(pf, a1.as_ptr<test_child_if>());
		EXPECT_EQ(pf, ar1.as_ptr<test_parent_f>());
		EXPECT_EQ(*pf, data);

		auto *cpf = a1.try_cast<const test_parent_f>();
		EXPECT_NE(cpf, nullptr);
		EXPECT_EQ(cpf, a1.as_ptr<test_child_if>());
		EXPECT_EQ(cpf, ar1.as_ptr<test_parent_f>());
		EXPECT_EQ(cpf, pf);
		EXPECT_EQ(*cpf, data);

		auto a2 = std::as_const(a1).convert(sek::type_info::get<test_parent_f>());
		EXPECT_FALSE(a2.empty());
		EXPECT_EQ(a2.as_cptr<test_parent_f>(), a1.as_ptr<test_child_if>());
		EXPECT_EQ(a2.as_cptr<test_parent_f>(), cpf);
		EXPECT_EQ(a2.as_cptr<test_parent_f>(), pf);

		a1 = sek::forward_any(data);
		a2 = info.construct(a1.ref());
		auto a3 = info.construct(sek::make_any<int>(data.i), sek::make_any<float>(data.f));
		EXPECT_EQ(a1, a2);
		EXPECT_EQ(a2, a3);
		EXPECT_EQ(info.construct(), sek::make_any<test_child_if>());

		const auto funcs = info.functions();
		EXPECT_FALSE(funcs.empty());

		EXPECT_THROW(a1.invoke("get_i", sek::make_any<int>()), sek::any_type_error);
		EXPECT_THROW(a1.invoke("get_i"), sek::any_const_error);
		EXPECT_THROW(a1.invoke(""), sek::invalid_member_error);

		a1 = a3.invoke("get_i");
		EXPECT_TRUE(a1.is_ref());
		EXPECT_FALSE(a1.is_const());
		EXPECT_EQ(a1.cast<int>(), data.i);
		a2 = a3.cref().invoke("get_i_const");
		EXPECT_TRUE(a2.is_ref());
		EXPECT_TRUE(a2.is_const());
		EXPECT_EQ(a2.cast<int>(), data.i);
		EXPECT_EQ(a1.cdata(), a2.cdata());
	}
}

#include "sekhmet/engine/assets.hpp"

TEST(runtime_tests, asset_test)
{
	using namespace sek::literals;

	{
		auto pkg_path = std::filesystem::path(TEST_DIR) / "test_package";
		auto pkg = sek::engine::asset_package::load(pkg_path);
		EXPECT_EQ(pkg.path(), pkg_path);
		EXPECT_FALSE(pkg.empty());

		auto asset = pkg.find("c0b16fc9-e969-4dac-97ed-eb8640a144ac"_uuid);
		EXPECT_NE(asset, pkg.end());
		EXPECT_EQ(asset->name(), "test_asset");
		EXPECT_EQ(asset, pkg.find("test_asset"));
		EXPECT_TRUE(asset->tags().contains("test"));
		EXPECT_EQ(asset, pkg.match([](auto a) { return a.tags().contains("test"); }));

		auto asset_file = asset->open();
		EXPECT_TRUE(asset_file.has_file() && asset_file.file().is_open());
		std::string data(64, '\0');
		asset_file.read(data.data(), data.size());
		EXPECT_EQ(data.erase(data.find_first_of('\0')), "test_asset");
	}
	{
		auto pkg_path = std::filesystem::path(TEST_DIR) / "test_archive.sekpak";
		auto pkg = sek::engine::asset_package::load(pkg_path);
		EXPECT_EQ(pkg.path(), pkg_path);

		auto asset = pkg.find("3fa20589-5e11-4249-bdfe-4d3e8038a5b3"_uuid);
		EXPECT_NE(asset, pkg.end());
		EXPECT_EQ(asset->name(), "test_archive_asset");
		EXPECT_EQ(asset, pkg.find("test_archive_asset"));
		EXPECT_TRUE(asset->tags().contains("test"));
		EXPECT_EQ(asset, pkg.match([](auto a) { return a.tags().contains("test"); }));

		auto asset_file = asset->open();
		EXPECT_TRUE(asset_file.has_file() && asset_file.file().is_open());
		std::string data(64, '\0');
		asset_file.read(data.data(), data.size());
		EXPECT_EQ(data.erase(data.find_first_of('\0')), "test_archive_asset");

		EXPECT_TRUE(asset->has_metadata());
		auto metadata = asset->metadata();
		auto metadata_str = std::string_view{std::bit_cast<const char *>(metadata.data()), metadata.size()};
		EXPECT_EQ(metadata_str, "test_metadata");
	}
}
