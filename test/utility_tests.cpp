//
// Created by switchblade on 2022-03-03.
//

#include <gtest/gtest.h>

#include <string>

#include "sekhmet/hash.hpp"
#include "sekhmet/utility.hpp"

TEST(utility_tests, version_test)
{
	sek::version v1 = {0, 0, 1};
	sek::version v2 = {0, 0, 2};
	sek::version v3 = {0, 1, 2};

	EXPECT_LT(v1, v2);
	EXPECT_LT(v1, v3);
	EXPECT_LT(v2, v3);

	EXPECT_EQ(v3, sek::version(0, 1, 2));

	EXPECT_EQ(hash(v3), hash(sek::version(0, 1, 2)));

	std::string s;
	v3.to_string<char>(std::back_inserter(s));
	EXPECT_EQ(s, "0.1.2");
}

TEST(utility_tests, uuid_test)
{
	using sek::hash;

	constexpr char uuid_str[] = "e7d751b6-f2f8-4541-8b40-81063d82af28";
	constexpr sek::uuid id = sek::uuid{uuid_str};
	constexpr sek::hash_t id_hash = hash(id);

	EXPECT_NE(id, sek::uuid{});
	EXPECT_EQ(sek::uuid{}, sek::uuid{"00000000-0000-0000-0000-000000000000"});
	EXPECT_EQ(hash(id), id_hash);
	EXPECT_NE(hash(id), hash(sek::uuid{sek::uuid::version4}));

	std::string s;
	id.to_string<char>(std::back_inserter(s));
	EXPECT_EQ(s, "e7d751b6-f2f8-4541-8b40-81063d82af28");
	EXPECT_NE(sek::uuid{sek::uuid::version4}, sek::uuid{sek::uuid::version4});
}