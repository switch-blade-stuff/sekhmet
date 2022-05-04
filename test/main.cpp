//
// Created by switchblade on 2021-12-16.
//

#include <gtest/gtest.h>

#include "sekhmet/logger.hpp"

void init_test_suite()
{
	sek::logger::msg() += std::cout;
	sek::logger::warn() += std::clog;
	sek::logger::error() += std::cerr;
}

int main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);
	init_test_suite();

	testing::GTEST_FLAG(death_test_style) = "fast";
	return RUN_ALL_TESTS();
}
