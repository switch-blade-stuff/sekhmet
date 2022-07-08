/*
 * Created by switchblade on 2022-04-15
 */

#include <gtest/gtest.h>

#include <array>
#include <numbers>

#include "sekhmet/serialization/archive.hpp"

namespace ser = sek::serialization;

TEST(serialization_tests, base64_test)
{
	struct data_t
	{
		constexpr bool operator==(const data_t &) const noexcept = default;

		int i;
		float f;
	} data = {1234, std::numbers::pi_v<float>}, decoded;

	auto len = ser::base64_encode<char16_t>(&data, sizeof(data), nullptr);
	auto buff = new char16_t[len];
	ser::base64_encode(&data, sizeof(data), buff);

	EXPECT_TRUE(ser::base64_decode(&decoded, sizeof(decoded), buff, len));
	EXPECT_EQ(decoded, data);

	delete[] buff;
}

namespace
{
	struct serializable_t
	{
		void serialize(auto &archive) const
		{
			archive << ser::keyed_entry("m", m);
			archive << ser::keyed_entry("n", nullptr);
			archive << ser::keyed_entry("s", s);
			archive << ser::keyed_entry("i", i);
			archive << ser::keyed_entry("b", b);
			archive << v << p << a;
		}
		void deserialize(auto &archive)
		{
			archive >> ser::keyed_entry("n", nullptr);
			archive >> ser::keyed_entry("s", s);
			archive >> ser::keyed_entry("i", i);
			archive >> ser::keyed_entry("m", m);
			archive >> ser::keyed_entry("b", b);
			archive >> v >> p >> a;
		}

		bool operator==(const serializable_t &) const noexcept = default;

		std::string s;
		int i;
		bool b;
		std::vector<int> v;
		std::pair<int, float> p;
		std::map<std::string, int> m;
		std::array<std::uint8_t, SEK_KB(1)> a;
	};

	serializable_t deserialize(std::in_place_type_t<serializable_t>, auto &archive, bool value)
	{
		EXPECT_TRUE(value);
		serializable_t result{};
		result.deserialize(archive);
		return result;
	}

	const serializable_t data = {
		.s = "Hello, world!",
		.i = 0x420,
		.b = true,
		.v = {0xff, 0xfff, 0, 1, 2, 3},
		.p = {69, 420.0f},
		.m = {{"i1", 1}, {"i2", 2}},
		.a = {},
	};
}	 // namespace

#include "sekhmet/serialization/json.hpp"

namespace json = ser::json;

template class json::basic_input_archive<json::pretty_print | json::inline_arrays>;
template class json::basic_output_archive<json::pretty_print | json::inline_arrays>;

TEST(serialization_tests, json_test)
{
	std::string json_string;
	{
		json::output_archive archive{json_string};
		archive << data;
		archive.flush();
	}
	json_string = "// Test comment\n" + json_string;
	serializable_t deserialized = {};
	{
		json::input_archive archive{json_string.data(), json_string.size()};
		EXPECT_TRUE(archive.try_read(deserialized));
	}
	EXPECT_EQ(data, deserialized);
	{
		json::input_archive archive{json_string.data(), json_string.size()};
		EXPECT_NO_THROW(deserialized = archive.read(std::in_place_type<serializable_t>, true));
	}
	EXPECT_EQ(data, deserialized);
}

#include "sekhmet/serialization/ubjson.hpp"

namespace ubj = ser::ubj;

template class ubj::basic_input_archive<ubj::highp_error>;
template class ubj::basic_output_archive<ubj::fixed_type>;

TEST(serialization_tests, ubjson_test)
{
	std::string ubj_string;
	{
		std::stringstream ss;
		ubj::basic_output_archive<ubj::fixed_type> archive{ss};
		archive << data;

		archive.flush();
		ubj_string = ss.str();
	}
	serializable_t deserialized = {};
	{
		ubj::input_archive archive{ubj_string.data(), ubj_string.size()};
		EXPECT_TRUE(archive.try_read(deserialized));
	}
	EXPECT_EQ(data, deserialized);
	{
		ubj::input_archive archive{ubj_string.data(), ubj_string.size()};
		EXPECT_NO_THROW(deserialized = archive.read(std::in_place_type<serializable_t>, true));
	}
	EXPECT_EQ(data, deserialized);
}

TEST(serialization_tests, json_tree_test)
{
	ser::json_tree tree;
	{
		std::stringstream ss;
		ubj::basic_output_archive<ubj::fixed_type> archive{ss};
		archive << data;
		tree = archive.release_tree();
	}
	serializable_t deserialized = {};
	{
		ubj::input_archive archive{tree};
		EXPECT_TRUE(archive.try_read(deserialized));
	}
	EXPECT_EQ(data, deserialized);
}

namespace
{
	struct serializable_a
	{
		constexpr serializable_a() noexcept = default;
		constexpr serializable_a(int i) noexcept : i(i) {}

		constexpr void serialize(auto &archive) const { archive << ser::keyed_entry("a", i); }
		constexpr void deserialize(auto &archive) { archive >> ser::keyed_entry("a", i); }

		constexpr bool operator==(const serializable_a &) const noexcept = default;

		int i = 0;
	};
	struct serializable_b
	{
		constexpr serializable_b() noexcept = default;
		constexpr serializable_b(float f) noexcept : f(f) {}

		constexpr void serialize(auto &archive) const { archive << ser::keyed_entry("b", f); }
		constexpr void deserialize(auto &archive) { archive >> ser::keyed_entry("b", f); }

		constexpr bool operator==(const serializable_b &) const noexcept = default;

		float f = 0;
	};
	struct serializable_ab : serializable_a, serializable_b
	{
		constexpr serializable_ab() noexcept = default;
		constexpr serializable_ab(int i, float f) noexcept : serializable_a(i), serializable_b(f) {}

		constexpr void serialize(auto &archive) const
		{
			serializable_a::serialize(archive);
			serializable_b::serialize(archive);
		}
		constexpr void deserialize(auto &archive)
		{
			serializable_a::deserialize(archive);
			serializable_b::deserialize(archive);
		}

		constexpr bool operator==(const serializable_ab &) const noexcept = default;
	};
}	 // namespace

TEST(serialization_tests, reuse_test)
{
	namespace json = ser::json;

	const auto ab_data = serializable_ab{1, std::numbers::pi_v<float>};
	std::string json_string;
	{
		std::stringstream ss;
		json::output_archive archive{ss};
		EXPECT_NO_THROW(archive << ab_data);
		EXPECT_NO_THROW(archive.flush());

		json_string = ss.str();
		EXPECT_FALSE(json_string.empty());
	}
	{
		json::input_archive archive{json_string.data(), json_string.size()};

		serializable_a a;
		EXPECT_NO_THROW(archive >> a);
		EXPECT_EQ(a, ab_data);

		serializable_b b;
		EXPECT_NO_THROW(archive >> b);
		EXPECT_EQ(b, ab_data);

		serializable_ab ab;
		EXPECT_NO_THROW(archive >> ab);
		EXPECT_EQ(ab, ab_data);
	}
}

#include "sekhmet/math.hpp"

TEST(serialization_tests, math_test)
{
	namespace math = sek::math;
	namespace json = ser::json;

	std::string json_string;

	{
		const auto v = math::fvec4{1, 2, 3, std::numeric_limits<float>::infinity()};
		{
			std::stringstream ss;
			json::basic_output_archive<json::pretty_print | json::inline_arrays | json::extended_fp> archive_ex{ss};
			archive_ex << v;

			archive_ex.flush();
			json_string = ss.str();
		}
		math::fvec4 deserialized = {};
		{
			auto f = [&]() -> void { json::input_archive archive{json_string.data(), json_string.size()}; };
			EXPECT_THROW(f(), ser::archive_error);

			json::basic_input_archive<json::extended_fp> archive{json_string.data(), json_string.size()};
			EXPECT_TRUE(archive.try_read(deserialized));
		}
		EXPECT_TRUE(all(v == deserialized));
	}

	{
		const auto m = math::fmat4{2};
		{
			std::stringstream ss;
			json::basic_output_archive<json::pretty_print | json::inline_arrays> archive_ex{ss};
			archive_ex << m;

			archive_ex.flush();
			json_string = ss.str();
		}
		math::fmat4 deserialized = {};
		{
			json::input_archive archive{json_string.data(), json_string.size()};
			EXPECT_TRUE(archive.try_read(deserialized));
		}
		EXPECT_EQ(m, deserialized);
	}
}

#include "sekhmet/dense_map.hpp"

TEST(serialization_tests, dense_map_test)
{
	namespace json = ser::json;

	std::string json_string;
	const auto m = sek::dense_map<std::string, float>{
		{"pi", std::numbers::pi_v<float>},
		{"0.0", 0.0f},
		{"2.0", 2.0f},
	};

	{
		std::stringstream ss;
		json::basic_output_archive<json::pretty_print | json::inline_arrays | json::extended_fp> archive_ex{ss};
		archive_ex << m;

		archive_ex.flush();
		json_string = ss.str();
	}
	sek::dense_map<std::string, float> deserialized = {};
	{
		json::basic_input_archive<json::extended_fp> archive{json_string.data(), json_string.size()};
		EXPECT_TRUE(archive.try_read(deserialized));
	}
	EXPECT_EQ(m, deserialized);
}

#include "sekhmet/utility.hpp"

TEST(serialization_tests, version_test)
{
	using namespace sek::literals;
	namespace json = ser::json;

	const auto ver_data = "0.1.2"_ver;
	std::string json_string;
	{
		std::stringstream ss;
		json::output_archive archive{ss};
		EXPECT_NO_THROW(archive << ver_data);
		EXPECT_NO_THROW(archive.flush());

		json_string = ss.str();
		EXPECT_FALSE(json_string.empty());
	}
	{
		json::input_archive archive{json_string.data(), json_string.size()};

		sek::version ver;
		EXPECT_NO_THROW(archive >> ver);
		EXPECT_EQ(ver, ver_data);
	}
}

namespace
{
	struct uuid_container
	{
		constexpr uuid_container(sek::uuid id) noexcept : id(id) {}
		constexpr uuid_container(auto &archive) { id = sek::uuid{archive.read(std::in_place_type<std::string_view>)}; }

		void serialize(auto &archive) const { archive << id.to_string(); }

		constexpr bool operator==(const uuid_container &) const noexcept = default;

		sek::uuid id;
	};
}	 // namespace

TEST(serialization_tests, uuid_test)
{
	using namespace sek::literals;
	namespace json = ser::json;

	const uuid_container id_data = "a7d71296-f456-4541-8b40-810678812d28"_uuid;
	std::string json_string;
	{
		std::stringstream ss;
		json::output_archive archive{ss};
		EXPECT_NO_THROW(archive << id_data);
		EXPECT_NO_THROW(archive.flush());

		json_string = ss.str();
		EXPECT_FALSE(json_string.empty());
	}
	{
		json::input_archive archive{json_string.data(), json_string.size()};
		EXPECT_NO_THROW(EXPECT_EQ(archive.read(std::in_place_type<uuid_container>, 0), id_data));
	}
}
