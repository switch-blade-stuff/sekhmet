//
// Created by switchblade on 2022-02-12.
//

#include <gtest/gtest.h>

#include <numbers>

#include "sekhmet/adt.hpp"
#include "sekhmet/type_id.hpp"

using namespace sek::literals;

TEST(adt_tests, node_test)
{
	sek::adt::node n1 = 1;

	EXPECT_TRUE(n1.is_number());
	EXPECT_TRUE(n1.is_int());
	EXPECT_FALSE(n1.is_float());

	EXPECT_THROW([[maybe_unused]] auto f = n1.as_float(), sek::adt::node_error);
	EXPECT_NO_THROW([[maybe_unused]] auto i = n1.as_int());
	EXPECT_EQ(n1.as_int(), 1);

	auto n2 = n1;

	EXPECT_TRUE(n2.is_number());
	EXPECT_TRUE(n2.is_int());
	EXPECT_FALSE(n2.is_float());
	EXPECT_EQ(n2.as_number<float>(), 1.0f);

	sek::adt::node n3 = "hello, world";
	EXPECT_TRUE(n3.is_string());
	EXPECT_EQ(n3.as_string(), "hello, world");

	sek::adt::node n4 = sek::adt::sequence{"hello, world", 7};
	EXPECT_TRUE(n4.is_sequence());
	EXPECT_TRUE(n4.as_sequence()[0].is_string());
	EXPECT_TRUE(n4.as_sequence()[1].is_int());

	sek::adt::node n5 = sek::adt::table{
		{"first", 9},
		{"second", std::numbers::pi_v<float>},
	};
	EXPECT_TRUE(n5.is_table());
	EXPECT_TRUE(n5.as_table()["first"].is_int());
	EXPECT_TRUE(n5.as_table()["second"].is_float());

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

TEST(adt_tests, ubjson_test)
{
	sek::adt::node node;

	{
		static const char data[] = "SU\x0chello, world";

		EXPECT_NO_THROW((node = sek::adt::ubj_input_archive{data, sizeof(data)}.read()));
		EXPECT_TRUE(node.is_string());
		EXPECT_EQ(node.as_string(), "hello, world");
	}

	{
		static const char data[] = "[$T#U\5";
		EXPECT_NO_THROW((node = sek::adt::ubj_input_archive{data, sizeof(data)}.read()));
		EXPECT_TRUE(node.is_sequence());

		auto &seq = node.as_sequence();
		EXPECT_EQ(seq.size(), 5);
		EXPECT_TRUE(std::all_of(seq.begin(), seq.end(), [](auto &n) { return n.is_bool() && n.as_bool(); }));
	}

	{
		static const char data[] = "{$S#U\3"
								   "U\2_0U\5item0"
								   "U\2_1U\5item1"
								   "U\2_2U\5item2";
		EXPECT_NO_THROW((node = sek::adt::ubj_input_archive{data, sizeof(data)}.read()));
		EXPECT_TRUE(node.is_table());

		auto &table = node.as_table();
		EXPECT_TRUE(table["_0"].is_string());
		EXPECT_EQ(table.at("_0").as_string(), "item0");
		EXPECT_TRUE(table["_1"].is_string());
		EXPECT_EQ(table.at("_1").as_string(), "item1");
		EXPECT_TRUE(table["_2"].is_string());
		EXPECT_EQ(table.at("_2").as_string(), "item2");
	}

	{
		static const char data[] = "{#U\2"
								   "U\4flagT"
								   "U\5child[Z]";
		EXPECT_NO_THROW((node = sek::adt::ubj_input_archive{data, sizeof(data)}.read()));
		EXPECT_TRUE(node.is_table());

		auto &table = node.as_table();
		EXPECT_TRUE(table["flag"].is_bool());
		EXPECT_TRUE(table.at("flag").as_bool());
		EXPECT_TRUE(table["child"].is_sequence());

		auto &seq = table.at("child").as_sequence();
		EXPECT_EQ(seq.size(), 1);
		EXPECT_TRUE(seq[0].empty());
	}

	{
		static const char data[] = "HU\0";
		EXPECT_THROW((node = sek::adt::ubj_input_archive{data, sizeof(data)}.read()), sek::adt::archive_error);
		EXPECT_NO_THROW(
			(node = sek::adt::ubj_input_archive{data, sizeof(data), sek::adt::ubj_input_archive::highp_skip}.read()));
		EXPECT_TRUE(node.empty());
		EXPECT_NO_THROW(
			(node = sek::adt::ubj_input_archive{data, sizeof(data), sek::adt::ubj_input_archive::highp_string}.read()));
		EXPECT_TRUE(node.is_string());
	}

	{
		static const char data[] = "[$U#U\1\1";
		EXPECT_NO_THROW((node = sek::adt::ubj_input_archive{data, sizeof(data)}.read()));
		EXPECT_TRUE(node.is_binary());
		EXPECT_EQ(node.as_binary()[0], std::byte{1});
	}

	{
		std::string buffer(20, '\0');
		sek::adt::node data = sek::adt::sequence{
			"text0",
			"text1",
		};

		EXPECT_NO_THROW((sek::adt::ubj_output_archive(buffer.data(), buffer.size()).write(data)));
		EXPECT_EQ(buffer, "[#i\x02Si\x05text0Si\x05text1");

		auto flags = sek::adt::ubj_output_archive::fixed_type | sek::adt::ubj_output_archive::best_fit;
		EXPECT_NO_THROW((sek::adt::ubj_output_archive(buffer.data(), buffer.size(), flags).write(data)));
		EXPECT_EQ(buffer, "[$S#i\x02i\x05text0i\x05text1");

		EXPECT_NO_THROW((sek::adt::ubj_input_archive(buffer.data(), buffer.size()).read(node)));
		EXPECT_TRUE(node.is_sequence());

		auto &seq = node.as_sequence();
		EXPECT_EQ(seq.size(), 2);
		EXPECT_TRUE(seq[0].is_string());
		EXPECT_EQ(seq[0].as_string(), "text0");
		EXPECT_TRUE(seq[1].is_string());
		EXPECT_EQ(seq[1].as_string(), "text1");
	}
}
