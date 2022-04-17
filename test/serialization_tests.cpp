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
	}
	template<typename A>
	void deserialize(A &archive)
	{
		archive >> ser::named_entry{"s", s};
		archive >> ser::named_entry{"i", i};
		archive >> ser::named_entry{"b", b};
		archive >> v;
	}

	std::string s;
	int i;
	bool b;
	std::vector<int> v;
};

TEST(serialization_tests, ubjson_test)
{
	const char data[] = "{#i\x04i\x01sSi\x0dHello, world!U\x01iI\x04\x20i\01bTi\01v[$i#i\04\x00\x01\x02\x03";
	ser::ubj_input_archive archive(data, sizeof(data) - 1);

	test_serializable serializable;
	archive >> serializable;
	EXPECT_EQ(serializable.s, "Hello, world!");
	EXPECT_EQ(serializable.i, 0x420);
	EXPECT_EQ(serializable.b, true);
	EXPECT_EQ(serializable.v, (std::vector{0, 1, 2, 3}));
}