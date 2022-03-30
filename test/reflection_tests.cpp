//
// Created by switchblade on 2022-01-22.
//

#include <gtest/gtest.h>

#include "sekhmet/reflection.hpp"
#include "test_plugin.hpp"

using namespace sek::test;
using namespace sek::literals;

TEST(reflection_tests, factory_test)
{
	sek::type_info::type_guard<test_child> g{};

	auto runtime_get_name = sek::type_info::get("test_child"_tid).tid().name().data();
	auto templated_get_name = sek::type_info::get<test_child>().tid().name().data();

	EXPECT_EQ(runtime_get_name, templated_get_name);
	EXPECT_NE(runtime_get_name, sek::type_name<test_child>().data());
	EXPECT_TRUE(test_child::factory_invoked);

	auto type = sek::type_info::get<test_child>();

	EXPECT_TRUE(type.has_parent<test_parent_A>());
	EXPECT_TRUE(type.has_parent<test_parent_B>());
	EXPECT_FALSE(type.has_attribute<int>());
	EXPECT_TRUE(type.has_attribute<test_attribute>());
	EXPECT_EQ(type.get_attribute<test_attribute>()->i, 9);
}

TEST(reflection_tests, type_info_test)
{
	sek::type_info::type_guard<test_child> g{};

	auto type = sek::type_info::get("test_child"_tid);
	EXPECT_TRUE(type.empty());
	EXPECT_FALSE(type.is_const());
	EXPECT_FALSE(type.is_volatile());
	EXPECT_FALSE(type.is_cv());
	EXPECT_TRUE(type.has_const_variant());
	EXPECT_TRUE(type.has_volatile_variant());
	EXPECT_TRUE(type.has_cv_variant());
	EXPECT_EQ(type.get_attribute(sek::type_id::identify<test_attribute>()).as<test_attribute>().i, 9);
	EXPECT_TRUE(type.constructible_with<>());
	EXPECT_TRUE(type.constructible_with<std::reference_wrapper<const test_child>>());
	EXPECT_TRUE(type.constructible_with<double>());

	sek::type_storage<test_child> s1;
	EXPECT_THROW(type.construct<int>(s1.data(), 0), sek::bad_type_exception);
	EXPECT_NO_THROW(type.construct<double>(s1.data(), 9.9));
	EXPECT_EQ(s1.get<test_child>()->d, 9.9);

	test_child s2;
	EXPECT_NO_THROW(type.construct<std::reference_wrapper<const test_child>>(&s2, {*s1.get<test_child>()}));
	EXPECT_EQ(s2.d, 9.9);
}

TEST(reflection_tests, any_test)
{
	int i = 10;
	sek::any_ref ref1 = i;

	EXPECT_FALSE(ref1.empty());
	EXPECT_TRUE(ref1.contains<int>());
	EXPECT_EQ(ref1.as<int>(), 10);
	EXPECT_EQ(&ref1.as<int>(), &i);

	sek::any any = i;
	EXPECT_FALSE(any.empty());
	EXPECT_TRUE(any.contains<int>());
	EXPECT_EQ(any.as<int>(), 10);

	ref1 = any;
	EXPECT_FALSE(ref1.empty());
	EXPECT_TRUE(ref1.contains<int>());
	EXPECT_EQ(ref1.as<int>(), 10);
	EXPECT_NE(&ref1.as<int>(), &i);
	EXPECT_EQ(&ref1.as<int>(), any.data());

	test_child c;
	sek::any_ref ref2 = c;

	EXPECT_NO_THROW(ref2.construct<double>(10));
	EXPECT_EQ(c.d, 10);
}