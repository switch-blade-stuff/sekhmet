//
// Created by switchblade on 2022-02-12.
//

#include <gtest/gtest.h>

#include <numbers>

#include "sekhmet/adt.hpp"
#include "sekhmet/reflection.hpp"

using namespace sek::literals;

TEST(adt_tests, node_test)
{
	sek::adt::node n1 = 1;

	EXPECT_TRUE(n1.is_number());
	EXPECT_TRUE(n1.is_int());
	EXPECT_FALSE(n1.is_float());

	EXPECT_THROW([[maybe_unused]] auto f = n1.as_float32(), sek::adt::node_type_exception);
	EXPECT_NO_THROW([[maybe_unused]] auto i = n1.as_int32());
	EXPECT_EQ(n1.as_int32(), 1);

	auto n2 = n1;

	EXPECT_TRUE(n2.is_number());
	EXPECT_TRUE(n2.is_int32());
	EXPECT_FALSE(n2.is_float());
	EXPECT_EQ(n2.as_number<float>(), 1.0f);

	sek::adt::node n3 = "hello, world";
	EXPECT_TRUE(n3.is_string());
	EXPECT_EQ(n3.as_string(), "hello, world");

	sek::adt::node n4 = sek::adt::sequence{"hello, world", 7};
	EXPECT_TRUE(n4.is_sequence());
	EXPECT_TRUE(n4.as_sequence()[0].is_string());
	EXPECT_TRUE(n4.as_sequence()[1].is_int32());

	sek::adt::node n5 = sek::adt::table{
		{"first", 9},
		{"second", std::numbers::pi_v<float>},
	};
	EXPECT_TRUE(n5.is_table());
	EXPECT_TRUE(n5.as_table()["first"].is_int32());
	EXPECT_TRUE(n5.as_table()["second"].is_float32());

	n5 = sek::adt::bytes{0xff, 0x00};
	EXPECT_TRUE(n5.is_binary());
	EXPECT_EQ(n5.as_binary()[0], std::byte{0xff});
	EXPECT_EQ(n5.as_binary()[1], std::byte{0x00});

	sek::adt::node n6;
	EXPECT_TRUE(n6.empty());

	n6.set(sek::adt::node::string_type{"dummy"});
	EXPECT_TRUE(n6.is_string());
}

TEST(adt_tests, serialization_test)
{
	std::pair<float, float> value = {std::numbers::pi_v<float>, std::numbers::e_v<float>};

	sek::adt::node n1 = value;
	EXPECT_FALSE(n1.empty());
	EXPECT_TRUE(n1.is_table());

	value = n1.get<std::pair<float, float>>();
	EXPECT_EQ(value.first, std::numbers::pi_v<float>);
	EXPECT_EQ(value.second, std::numbers::e_v<float>);

	n1.as_table()["first"] = 2.f;
	n1.as_table()["second"] = 4.f;

	value = n1.get<std::pair<float, float>>();
	EXPECT_EQ(value.first, 2);
	EXPECT_EQ(value.second, 4);

	std::vector<int> vec = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

	sek::adt::node n2 = vec;
	EXPECT_FALSE(n2.empty());
	EXPECT_TRUE(n2.is_sequence());

	vec.clear();
	n2.get(vec);

	EXPECT_EQ(vec.size(), 10);
}

using namespace std::literals;

namespace
{
	struct test_serializable_struct
	{
		int i = 0;
		bool b = false;

		friend SEK_ADT_NODE_CONSTEXPR void serialize(sek::adt::node &node, const test_serializable_struct &s)
		{
			node = sek::adt::table{
				{"i"s, s.i},
				{"b"s, s.b},
			};
		}
		friend SEK_ADT_NODE_CONSTEXPR void deserialize(const sek::adt::node &node, test_serializable_struct &s)
		{
			if (node.is_table()) [[likely]]
			{
				auto &t = node.as_table();
				if (t.contains("i") && t.at("i").is_number()) s.i = t.at("i").as_number<int>();
				if (t.contains("b") && t.at("b").is_bool()) s.b = t.at("b").as_bool();
			}
		}
	};
}	 // namespace

SEK_EXPORT_TYPE(test_serializable_struct, "test_serializable_struct")
SEK_TYPE_FACTORY(test_serializable_struct) { attributes<sek::adt::serializable_as<test_serializable_struct>>(); }

TEST(adt_tests, serializable_as_test)
{
	EXPECT_TRUE(sek::type_info::get<test_serializable_struct>().has_attribute<sek::adt::serializable_as_attribute>());

	sek::adt::node n1 = sek::adt::table{
		{"i", 1000},
		{"b", true},
	};
	test_serializable_struct value = {};

	EXPECT_NO_THROW(n1.get(sek::any_ref{value}));
	EXPECT_EQ(value.i, 1000);
	EXPECT_TRUE(value.b);

	n1.reset();

	EXPECT_NO_THROW(n1.set(sek::any_ref{value}));
	EXPECT_TRUE(n1.is_table());
	EXPECT_TRUE(n1.as_table()["i"].is_int32());
	EXPECT_EQ(n1.as_table()["i"].as_int32(), 1000);
	EXPECT_TRUE(n1.as_table()["b"].is_bool());
	EXPECT_TRUE(n1.as_table()["b"].as_bool());
}