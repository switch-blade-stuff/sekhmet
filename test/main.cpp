/*
 * Created by switchblade on 2021-12-16
 */

#include <gtest/gtest.h>

void init_tests();

int main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);
	init_tests();
	return RUN_ALL_TESTS();
}

#include "sekhmet/engine/type_info.hpp"

void init_tests()
{
	static std::remove_pointer_t<decltype(sek::engine::type_database::instance())> type_db;
	sek::engine::type_database::instance(&type_db);
}