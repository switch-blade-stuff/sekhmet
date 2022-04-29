//
// Created by switchblade on 2022-04-15.
//

#include <gtest/gtest.h>

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

TEST(serialization_tests, json_test)
{
	namespace json = sek::serialization::json;

	std::string json_string;
	{
		std::stringstream ss;
		json::output_archive archive{ss};
		archive << data;

		archive.flush();
		json_string = ss.str();
	}
	json_string = "// Test comment\n" + json_string;
	serializable_t deserialized = {};
	{
		json::input_archive archive{json_string.data(), json_string.size()};
		EXPECT_TRUE(archive.try_read(deserialized));
	}
	EXPECT_EQ(data, deserialized);
}

#include "sekhmet/serialization/ubjson.hpp"

TEST(serialization_tests, ubjson_test)
{
	namespace ubj = sek::serialization::ubj;

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
}

#include "sekhmet/type_id.hpp"

template<typename T>
struct printer_base
{
	struct invoker
	{
		invoker() noexcept { printf("%s\n", sek::type_name<T>().data()); }
	};

	static const invoker instance;
};

template<typename T>
const typename printer_base<T>::invoker printer_base<T>::instance = {};

namespace
{
	struct test_struct : printer_base<test_struct>
	{
	};
}	 // namespace

// template<>
// std::string_view sek::type_name<test_struct>() noexcept
//{
//	return "test_struct";
// }

namespace sek
{
	template<typename T>
	constexpr void resource_factory(T &)
	{
	}
}	 // namespace sek

#define SEK_DETAIL_PREFAB_1(type)
#define SEK_DETAIL_PREFAB_2(type, name) SEK_DETAIL_PREFAB_1(type)

#define SEK_PREFAB_FACTORY(...) SEK_GET_MACRO_2(__VA_ARGS__, SEK_DETAIL_PREFAB_2, SEK_DETAIL_PREFAB_1)(__VA_ARGS__)
