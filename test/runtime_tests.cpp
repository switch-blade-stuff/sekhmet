/*
 * Created by switchblade on 17/05/22
 */

#include <gtest/gtest.h>

#include "sekhmet/engine/type_info.hpp"

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
constexpr std::string_view sek::engine::type_name<test_parent_top>() noexcept
{
	return "top_parent";
}
template<>
constexpr std::string_view sek::engine::type_name<test_child>() noexcept
{
	return "test_child";
}

SEK_EXTERN_TYPE(test_child)
SEK_EXPORT_TYPE(test_child)

#include "sekhmet/engine/logger.hpp"
#include "sekhmet/engine/plugin.hpp"

static bool plugin_enabled = false;

SEK_PLUGIN("test_plugin", "0.0.0")
{
	sek::engine::logger::info() << fmt::format("Initializing plugin \"{}\"", info.id);

	on_enable += +[]()
	{
		sek::engine::type_info::reflect<test_parent_middle>().parent<test_parent_top>();
		plugin_enabled = true;
		return true;
	};
	on_disable += +[]()
	{
		sek::engine::type_info::reset<test_parent_middle>();
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
	using namespace sek::engine::literals;

	// clang-format off
	sek::engine::type_info::reflect<test_child>()
		.attribute<int>(0xff).attribute<0xfc>().attribute<test_attribute>()
		.parent<test_parent_middle>().submit();
	// clang-format on

	auto info = sek::engine::type_info::get<test_child>();

	EXPECT_EQ(info, "test_child"_type);
	EXPECT_TRUE(info.valid());
	EXPECT_EQ(info.name(), "test_child");
	EXPECT_EQ(info.name(), sek::engine::type_name<test_child>());
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

	constexpr auto pred = [](auto p) { return p.type() == sek::engine::type_info::get<test_parent_middle>(); };
	EXPECT_TRUE(std::ranges::any_of(info.parents(), pred));

	EXPECT_TRUE(sek::engine::type_info::get<test_child[2]>().has_extent());
	EXPECT_TRUE(sek::engine::type_info::get<test_child[2]>().is_range());
	EXPECT_EQ(sek::engine::type_info::get<test_child[2]>().extent(), 2);
	EXPECT_EQ(sek::engine::type_info::get<test_child[2]>().value_type(), info);

	EXPECT_FALSE(sek::engine::type_info::get<test_child[]>().has_extent());
	EXPECT_TRUE(sek::engine::type_info::get<test_child[]>().is_array());
	EXPECT_FALSE(sek::engine::type_info::get<test_child[]>().is_range());
	EXPECT_FALSE(sek::engine::type_info::get<test_child[]>().is_pointer());
	EXPECT_EQ(sek::engine::type_info::get<test_child[]>().extent(), 0);
	EXPECT_NE(sek::engine::type_info::get<test_child[]>().value_type(), info);

	EXPECT_TRUE(sek::engine::type_info::get<test_child *>().is_pointer());
	EXPECT_EQ(sek::engine::type_info::get<test_child *>().value_type(), info);
	EXPECT_EQ(sek::engine::type_info::get<const test_child *>().value_type(), sek::engine::type_info::get<test_child>());

	{
		const auto db = sek::engine::type_database::instance()->access_unique();
		auto query = db->query().with_attribute<test_attribute>();

		EXPECT_EQ(query.size(), 1);
		EXPECT_EQ(*query.begin(), info);

		db->reset<test_child>();
	}
	EXPECT_FALSE("test_child"_type);

	const auto attribs = info.attributes();
	// clang-format off
	EXPECT_TRUE(info.has_attribute<int>());
	EXPECT_TRUE(info.has_attribute<test_attribute>());
	EXPECT_TRUE(std::any_of(attribs.begin(), attribs.end(), [](auto n) { return n.value() == sek::engine::make_any<int>(0xff); }));
	EXPECT_TRUE(std::any_of(attribs.begin(), attribs.end(), [](auto n) { return n.value() == sek::engine::make_any<int>(0xfc); }));
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
		auto a1 = sek::engine::make_any<data_t>(data);

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

		a1 = sek::engine::forward_any(data);
		EXPECT_FALSE(a1.is_local());
		EXPECT_FALSE(a1.is_const());
		EXPECT_TRUE(a1.is_ref());
		EXPECT_NE(a1.as_cptr<data_t>(), nullptr);
		EXPECT_EQ(*a1.as_cptr<data_t>(), data);
		EXPECT_EQ(a1.data(), &data);

		a1 = sek::engine::forward_any(std::as_const(data));
		EXPECT_FALSE(a1.is_local());
		EXPECT_TRUE(a1.is_const());
		EXPECT_TRUE(a1.is_ref());
		EXPECT_EQ(a1.as_ptr<data_t>(), nullptr);
		EXPECT_NE(a1.as_cptr<data_t>(), nullptr);
		EXPECT_EQ(*a1.as_cptr<data_t>(), data);
		EXPECT_EQ(a1.cdata(), &data);
	}
	{
		sek::engine::type_info::reflect<int>().convertible<float>().submit();

		const auto info = sek::engine::type_info::get<int>();
		const auto data = 10;
		const auto a1 = sek::engine::make_any<int>(data);

		const auto convs = info.conversions();
		EXPECT_FALSE(convs.empty());
		EXPECT_TRUE(info.convertible_to<float>());

		auto a2 = convs.front().convert(a1.ref());
		EXPECT_FALSE(a2.empty());
		EXPECT_EQ(a2, sek::engine::make_any<float>(static_cast<float>(data)));

		auto a3 = a1.convert(sek::engine::type_info::get<float>());
		EXPECT_FALSE(a3.empty());
		EXPECT_EQ(a3, sek::engine::make_any<float>(static_cast<float>(data)));
		EXPECT_EQ(a3, a2);
	}
	{
		// clang-format off
		sek::engine::type_info::reflect<test_child_if>()
			.constructor<int, float>().constructor<const test_child_if &>()
			.parent<test_parent_i>().parent<test_parent_f>()
			.function<&test_child_if::get_i>("get_i").function<&test_child_if::set_i>("set_i")
			.function<&test_child_if::get_i_const>("get_i_const")
			.submit();
		// clang-format on
		const auto info = sek::engine::type_info::get<test_child_if>();
		const auto data = test_child_if{10, std::numbers::pi_v<float>};
		auto a1 = sek::engine::make_any<test_child_if>(data);

		EXPECT_FALSE(a1.empty());
		EXPECT_NE(a1.as_ptr<test_child_if>(), nullptr);
		EXPECT_EQ(*a1.as_ptr<test_child_if>(), data);

		const auto parents = info.parents();
		EXPECT_FALSE(parents.empty());

		constexpr auto pred_i = [](auto p) { return p.type() == sek::engine::type_info::get<test_parent_i>(); };
		auto parent_i = std::find_if(parents.begin(), parents.end(), pred_i);
		EXPECT_NE(parent_i, parents.end());

		auto ar1 = parent_i->cast(a1.ref());
		EXPECT_NE(ar1.as_ptr<test_parent_i>(), nullptr);
		EXPECT_EQ(ar1.as_ptr<test_parent_i>(), a1.as_ptr<test_child_if>());
		EXPECT_EQ(*ar1.as_ptr<test_parent_i>(), data);

		constexpr auto pred_f = [](auto p) { return p.type() == sek::engine::type_info::get<test_parent_f>(); };
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

		auto a2 = std::as_const(a1).convert(sek::engine::type_info::get<test_parent_f>());
		EXPECT_FALSE(a2.empty());
		EXPECT_EQ(a2.as_cptr<test_parent_f>(), a1.as_ptr<test_child_if>());
		EXPECT_EQ(a2.as_cptr<test_parent_f>(), cpf);
		EXPECT_EQ(a2.as_cptr<test_parent_f>(), pf);

		a1 = sek::engine::forward_any(data);
		a2 = info.construct(a1.ref());
		auto a3 = info.construct(sek::engine::make_any<int>(data.i), sek::engine::make_any<float>(data.f));
		EXPECT_EQ(a1, a2);
		EXPECT_EQ(a2, a3);
		EXPECT_EQ(info.construct(), sek::engine::make_any<test_child_if>());

		const auto funcs = info.functions();
		EXPECT_FALSE(funcs.empty());

		EXPECT_THROW(a1.invoke("get_i", sek::engine::make_any<int>()), sek::engine::any_type_error);
		EXPECT_THROW(a1.invoke("get_i"), sek::engine::any_const_error);
		EXPECT_THROW(a1.invoke(""), sek::engine::invalid_member_error);

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

#include "sekhmet/engine/config.hpp"

namespace
{
	using namespace sek::serialization;

	struct test_config
	{
		constexpr test_config() noexcept = default;
		constexpr explicit test_config(int i) noexcept : silent_i(i) {}

		void serialize(auto &archive) const
		{
			archive << keyed_entry("some_int", some_int);
			archive << keyed_entry("flag", flag);
		}
		void deserialize(auto &archive)
		{
			archive >> some_int;
			archive >> flag;
		}

		int silent_i = 0;
		int some_int = 0;
		bool flag = false;
	};
}	 // namespace

template<>
[[nodiscard]] constexpr std::string_view sek::engine::type_name<test_config>() noexcept
{
	return "test_config";
}

TEST(runtime_tests, config_test)
{
	using namespace sek::engine::attributes;
	using namespace sek::serialization;
	using namespace std::literals;

	using reg_guard_t = std::remove_pointer_t<decltype(sek::engine::config_registry::instance())>;
	reg_guard_t reg_guard;

	{
		const auto data = R"({ "nodes": { "test_entry": { "test_config": { "some_int": 1, "flag": true }}}})"sv;

		sek::serialization::json::input_archive archive(data.data(), data.size());
		EXPECT_NO_THROW(reg_guard.access_unique()->load("test_category", std::move(*archive.tree)));
	}
	{
		auto ptr = reg_guard.access_shared()->find("test_category");
		EXPECT_TRUE(ptr);
		EXPECT_TRUE(ptr->value().empty());
		EXPECT_EQ(ptr->path(), "test_category");
	}
	sek::engine::type_info::reflect<test_config>().attribute(make_config_type<test_config>()).submit();
	{
		constexpr auto silent_i = 10;
		EXPECT_NO_THROW(reg_guard.access_unique()->insert("test_category/test_entry", test_config{silent_i}));

		auto ptr = reg_guard.access_shared()->find("test_category/test_entry");
		EXPECT_TRUE(ptr);
		EXPECT_FALSE(ptr->value().empty());
		EXPECT_EQ(ptr->path(), "test_category/test_entry");

		auto *cfg = ptr->value().try_cast<const test_config>();
		EXPECT_NE(cfg, nullptr);
		EXPECT_EQ(cfg->silent_i, silent_i);
		EXPECT_EQ(cfg->some_int, 1);
		EXPECT_TRUE(cfg->flag);
	}
	sek::serialization::json_tree data_tree;
	{
		EXPECT_NO_THROW(reg_guard.access_shared()->save("test_category/test_entry", data_tree));
		sek::serialization::json::input_archive archive(data_tree);
		EXPECT_NO_THROW(reg_guard.access_unique()->load("test_category/test_entry_2", std::move(data_tree)));
	}
	{
		EXPECT_NO_THROW(reg_guard.access_unique()->try_insert<test_config>("test_category/test_entry_2"));

		auto e1 = reg_guard.access_shared()->find("test_category/test_entry");
		auto e2 = reg_guard.access_shared()->find("test_category/test_entry_2");
		EXPECT_TRUE(e1);
		EXPECT_TRUE(e2);
		EXPECT_FALSE(e1->value().empty());
		EXPECT_FALSE(e2->value().empty());
		EXPECT_EQ(e1->path(), "test_category/test_entry");
		EXPECT_EQ(e2->path(), "test_category/test_entry_2");

		auto *cfg1 = e1->value().try_cast<const test_config>();
		auto *cfg2 = e2->value().try_cast<const test_config>();
		EXPECT_NE(cfg2, nullptr);
		EXPECT_EQ(cfg2->silent_i, 0);
		EXPECT_EQ(cfg2->some_int, 1);
		EXPECT_TRUE(cfg2->flag);

		EXPECT_NE(cfg2->silent_i, cfg1->silent_i);
		EXPECT_EQ(cfg2->some_int, cfg1->some_int);
		EXPECT_EQ(cfg2->flag, cfg1->flag);
	}
	sek::engine::type_info::reset<test_config>();
}

#include "sekhmet/engine/assets.hpp"

TEST(runtime_tests, asset_test)
{
	using namespace sek::literals;

	using db_guard_t = std::remove_pointer_t<decltype(sek::engine::asset_database::instance())>;
	db_guard_t db_guard;

	auto archive_path = std::filesystem::path(TEST_DIR) / "test_archive.sekpak";
	auto archive_pkg = sek::engine::asset_package::load(archive_path);
	EXPECT_EQ(archive_pkg.path(), archive_path);
	auto loose_path = std::filesystem::path(TEST_DIR) / "test_package";
	auto loose_pkg = sek::engine::asset_package::load(loose_path);
	EXPECT_EQ(loose_pkg.path(), loose_path);

	{
		auto db = db_guard.access_unique();
		db->packages().push_back(archive_pkg);
		db->packages().push_back(loose_pkg);

		EXPECT_FALSE(db->packages().empty());
		EXPECT_EQ(db->packages().size(), 2);
		EXPECT_EQ(db->packages()[0], archive_pkg);
		EXPECT_EQ(db->packages()[1], loose_pkg);
	}
	{
		auto asset = archive_pkg.find("3fa20589-5e11-4249-bdfe-4d3e8038a5b3"_uuid);
		EXPECT_NE(asset, archive_pkg.end());
		EXPECT_EQ(asset->name(), "test_archive_asset");
		EXPECT_EQ(asset, archive_pkg.find("test_archive_asset"));
		EXPECT_TRUE(asset->tags().contains("test"));
		EXPECT_EQ(asset, archive_pkg.match([](auto a) { return a.tags().contains("test"); }));

		auto asset_file = asset->open();
		EXPECT_TRUE(asset_file.has_file() && asset_file.file().is_open());
		auto asset_map = asset_file.map();
		EXPECT_TRUE(asset_map.is_mapped());
		auto data_str = std::string_view{static_cast<char *>(asset_map.data()), static_cast<std::size_t>(asset_map.size())};
		EXPECT_EQ(data_str, "test_archive_asset");

		EXPECT_TRUE(asset->has_metadata());
		auto metadata = asset->metadata();
		EXPECT_FALSE(metadata.empty());
		auto metadata_str = std::string_view{std::bit_cast<const char *>(metadata.data()), metadata.size()};
		EXPECT_EQ(metadata_str, "test_metadata");
	}
	{
		auto asset = loose_pkg.find("c0b16fc9-e969-4dac-97ed-eb8640a144ac"_uuid);
		EXPECT_NE(asset, loose_pkg.end());
		EXPECT_EQ(asset->name(), "test_asset");
		EXPECT_EQ(asset, loose_pkg.find("test_asset"));
		EXPECT_TRUE(asset->tags().contains("test"));

		auto asset_file = asset->open();
		EXPECT_TRUE(asset_file.has_file() && asset_file.file().is_open());
		auto asset_map = asset_file.map();
		EXPECT_TRUE(asset_map.is_mapped());
		auto data_str = std::string_view{static_cast<char *>(asset_map.data()), static_cast<std::size_t>(asset_map.size())};
		EXPECT_EQ(data_str, "test_asset");
	}
	{
		auto asset = loose_pkg.find("3fa20589-5e11-4249-bdfe-4d3e8038a5b3"_uuid);
		EXPECT_NE(asset, loose_pkg.end());
		EXPECT_EQ(asset->name(), "test_asset2");
		EXPECT_EQ(asset, loose_pkg.find("test_asset2"));
		EXPECT_TRUE(asset->tags().contains("test"));

		auto asset_file = asset->open();
		EXPECT_TRUE(asset_file.has_file() && asset_file.file().is_open());
		auto asset_map = asset_file.map();
		EXPECT_TRUE(asset_map.is_mapped());
		auto data_str = std::string_view{static_cast<char *>(asset_map.data()), static_cast<std::size_t>(asset_map.size())};
		EXPECT_EQ(data_str, "test_asset2");
	}
	{
		auto db = db_guard.access_shared();
		auto asset = db->find("c0b16fc9-e969-4dac-97ed-eb8640a144ac"_uuid);
		EXPECT_NE(asset, db->end());
		EXPECT_EQ(asset->name(), "test_asset");
		asset = db->find("3fa20589-5e11-4249-bdfe-4d3e8038a5b3"_uuid);
		EXPECT_NE(asset, db->end());
		EXPECT_EQ(asset->name(), "test_asset2");
	}
	{
		auto db = db_guard.access_unique();
		auto proxy = db->packages();
		proxy.erase(proxy.begin() + 1);

		EXPECT_FALSE(proxy.empty());
		EXPECT_EQ(proxy.size(), 1);
	}
	{
		auto db = db_guard.access_shared();
		EXPECT_EQ(db->find("c0b16fc9-e969-4dac-97ed-eb8640a144ac"_uuid), db->end());
		auto asset = db->find("3fa20589-5e11-4249-bdfe-4d3e8038a5b3"_uuid);
		EXPECT_NE(asset, db->end());
		EXPECT_EQ(asset->name(), "test_archive_asset");
	}
}

#include "sekhmet/engine/resources.hpp"

namespace ser = sek::serialization;

namespace
{
	struct test_resource
	{
		void deserialize(auto &archive)
		{
			archive >> ser::keyed_entry("s", s);
			archive >> ser::keyed_entry("b", b);
			archive >> ser::keyed_entry("i", i);
		}
		void serialize(auto &archive) const
		{
			archive << ser::container_size(3);
			archive << ser::keyed_entry("s", s);
			archive << ser::keyed_entry("b", b);
			archive << ser::keyed_entry("i", i);
		}

		std::string s;
		bool b;
		int i;
	};
}	 // namespace

template<>
constexpr std::string_view sek::engine::type_name<test_resource>() noexcept
{
	return "test_resource";
}

TEST(runtime_tests, resource_test)
{
	using namespace sek::engine::attributes;
	using namespace sek::engine;

	using db_guard_t = std::remove_pointer_t<decltype(asset_database::instance())>;
	db_guard_t db_guard;
	asset_database::instance(&db_guard);

	using cache_guard_t = std::remove_pointer_t<decltype(resource_cache::instance())>;
	cache_guard_t cache_guard;

	{
		auto pkg = asset_package::load(std::filesystem::path(TEST_DIR) / "test_package");
		db_guard.access_unique()->packages().push_back(pkg);
	}
	type_info::reflect<test_resource>().attribute(make_resource_type<test_resource>()).submit();
	{
		std::shared_ptr<test_resource> res;
		EXPECT_FALSE(res = cache_guard.access_unique()->load<test_resource>("invalid_resource"));
		EXPECT_THROW(res = cache_guard.access_unique()->load<test_resource>("test_asset"), resource_error);
		EXPECT_NO_THROW(res = cache_guard.access_unique()->load<test_resource>("test_resource"));
		EXPECT_TRUE(res);

		EXPECT_EQ(res->s, "Hello, World");
		EXPECT_EQ(res->i, -128);
		EXPECT_TRUE(res->b);
	}
	type_info::reset("test_resource");
}
