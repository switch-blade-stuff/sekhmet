/*
 * Created by switchblade on 2021-12-16
 */

#include <gtest/gtest.h>

int main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);

	testing::GTEST_FLAG(death_test_style) = "fast";
	return RUN_ALL_TESTS();
}
