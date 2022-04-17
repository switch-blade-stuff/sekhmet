//
// Created by switchblade on 2022-04-15.
//

#include <gtest/gtest.h>

#include "sekhmet/serialization/json_archive.hpp"

namespace ser = sek::serialization;

struct test_serializable
{
	template<typename A>
	void serialize(A &archive)
	{
		archive << ser::named_entry{"s", s};
		archive << ser::named_entry{"i", i};
		archive << ser::named_entry{"b", b};
		archive << v << p;
	}
	template<typename A>
	void deserialize(A &archive)
	{
		archive >> ser::named_entry{"s", s};
		archive >> ser::named_entry{"i", i};
		archive >> ser::named_entry{"b", b};
		archive >> v >> p;
	}

	std::string s;
	int i;
	bool b;
	std::vector<int> v;
	std::pair<int, float> p;
};

TEST(serialization_tests, base64_test)
{
	struct data_t
	{
		constexpr bool operator==(const data_t &) const noexcept = default;

		int i;
		float f;
	} data = {1234, std::numbers::pi_v<float>}, decoded;

	auto len = ser::detail::base64_encode<char16_t>(&data, sizeof(data), nullptr);
	auto buff = new char16_t[len];
	ser::detail::base64_encode(&data, sizeof(data), buff);

	EXPECT_TRUE(ser::detail::base64_decode(&decoded, sizeof(decoded), buff, len));
	EXPECT_EQ(decoded, data);

	delete[] buff;
}

TEST(serialization_tests, ubjson_test)
{
	const char data[] = "{#i\x05i\x01sSi\x0dHello, world!U\x01iI\x04\x20i\01bTi\01v[$i#i\04\x00\x01\x02\x03"
						"i\x01p[i\105d\x43\xd2\x00\x00]";
	ser::ubj_input_archive archive(data, sizeof(data) - 1);

	test_serializable serializable;
	archive >> serializable;
	EXPECT_EQ(serializable.s, "Hello, world!");
	EXPECT_EQ(serializable.i, 0x420);
	EXPECT_EQ(serializable.b, true);
	EXPECT_EQ(serializable.v, (std::vector{0, 1, 2, 3}));
	EXPECT_EQ(serializable.p, (std::pair<int, float>{69, 420.0}));
}