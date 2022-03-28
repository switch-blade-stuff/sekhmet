//
// Created by switchblade on 2021-12-01.
//

#include <gtest/gtest.h>

#include "sekhmet/debug.hpp"

TEST(debug_tests, assert_test)
{
	EXPECT_DEATH(SEK_ASSERT_ALWAYS(1 == 0), ".*");
	EXPECT_DEATH(SEK_ASSERT_ALWAYS(!(0 == 0)), ".*");
}
