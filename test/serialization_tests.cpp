//
// Created by switchblade on 2022-04-15.
//

#include <gtest/gtest.h>

#include "sekhmet/serialization/json_archive.hpp"

namespace ser = sek::serialization;

TEST(serialization_tests, ubjson_test)
{
	const char data[] = "[$S#U\2U\x0dHello, world!U\x10This is a string";
	ser::ubj_input_archive archive(data, sizeof(data));

	std::string document[2];
	archive.read(document);
	EXPECT_EQ(document[0], "Hello, world!");
	EXPECT_EQ(document[1], "This is a string");
}