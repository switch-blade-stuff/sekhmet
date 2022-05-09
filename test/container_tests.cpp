//
// Created by switchblade on 2021-12-01.
//

#include <gtest/gtest.h>

#include <numbers>
#include <string>
#include <vector>

#include "sekhmet/array_list.hpp"

template class sek::array_list<int>;

TEST(container_tests, array_list_test)
{
	sek::array_list<int> l1;
	l1.push_back({0, 1, 2, 3});
	l1.push_back(4);

	sek::array_list l2 = {0, 1, 2, 3, 4};
	sek::array_list l3 = {0, 1, 2, 3};
	sek::array_list<int> l4;
	l4.resize(5, 1);
	auto l5 = l1;

	EXPECT_EQ(l1, l2);
	EXPECT_LT(l3, l2);
	EXPECT_LT(l3, l1);
	EXPECT_GT(l4, l2);
	EXPECT_GT(l4, l3);
	EXPECT_GT(l4, l1);
	EXPECT_EQ(l5, l1);

	auto l6 = std::move(l1);
	EXPECT_EQ(l5, l6);
	swap(l5, l6);
	EXPECT_EQ(l5, l6);

	sek::array_list l7 = {0, 0, 1, 2};
	sek::array_list<int> l8 = {1, 2};
	EXPECT_NE(l8, l7);
	auto node1 = l7.extract(l7.begin());
	EXPECT_FALSE(node1.empty());
	EXPECT_EQ(node1.value(), 0);
	l8.insert(l8.begin(), std::move(node1));
	EXPECT_TRUE(node1.empty());
	EXPECT_EQ(l8, l7);
}

#include "sekhmet/sparse_map.hpp"

template class sek::sparse_map<std::string, float>;

TEST(container_tests, sparse_map_test)
{
	sek::sparse_map<std::string, float> m1 = {
		{"0", 9.9f},
		{"1", 7.6f},
		{"2", std::numbers::pi_v<float>},
		{"3", 0.f},
		{"4", 0.f},
		{"5", 0.f},
		{"6", 0.f},
		{"7", 0.f},
	};

	EXPECT_TRUE(m1.contains("7"));
	EXPECT_FALSE(m1.contains("8"));

	EXPECT_FLOAT_EQ(m1["0"], 9.9f);
	EXPECT_FLOAT_EQ(m1["1"], 7.6f);
	EXPECT_FLOAT_EQ(m1["2"], std::numbers::pi_v<float>);
	EXPECT_EQ(m1.size(), 8);

	m1.erase("0");
	m1.erase("1");
	EXPECT_EQ(m1.size(), 6);

	EXPECT_NE(m1.find("2"), m1.end());
	EXPECT_EQ((*m1.find("2") <=> std::pair<const std::string, float>{"2", std::numbers::pi_v<float>}),
			  std::weak_ordering::equivalent);
	EXPECT_EQ(m1.find("1"), m1.end());

	sek::sparse_map<std::string, int> m2;

	for (auto i = 0; i < 1000; i++) m2.emplace(std::to_string(i), i);
	for (auto i = 0; i < 200; i++) m2.erase(m2.find(std::to_string(i)));
	for (auto i = 500; i < 1000; i++) m2.erase(m2.find(std::to_string(i)));

	m2[std::to_string(500)] = 500;

	for (auto i = 200; i <= 500; i++)
	{
		int val;
		EXPECT_NO_THROW(val = m2.at(std::to_string(i)));
		EXPECT_EQ(val, i);
	}
}

#include "sekhmet/sparse_set.hpp"

template class sek::sparse_set<std::string>;

TEST(container_tests, sparse_set_test)
{
	sek::sparse_set<std::string> s1 = {"1", "2", "3", "4"};

	EXPECT_EQ(s1.size(), 4);
	EXPECT_FALSE(s1.contains("0"));
	EXPECT_TRUE(s1.contains("1"));

	s1.erase("1");

	EXPECT_EQ(s1.size(), 3);
	EXPECT_FALSE(s1.contains("1"));
	EXPECT_EQ(s1.find("1"), s1.end());
}

#include "sekhmet/dense_map.hpp"

template class sek::dense_map<std::string, float>;

TEST(container_tests, dense_map_test)
{
	sek::dense_map<std::string, float> m1 = {
		{"0", 9.9f},
		{"1", 7.6f},
		{"2", std::numbers::pi_v<float>},
		{"3", 0.f},
		{"4", 0.f},
		{"5", 0.f},
		{"6", 0.f},
		{"7", 0.f},
	};

	EXPECT_TRUE(m1.contains("7"));
	EXPECT_FALSE(m1.contains("8"));

	EXPECT_FLOAT_EQ(m1["0"], 9.9f);
	EXPECT_FLOAT_EQ(m1["1"], 7.6f);
	EXPECT_FLOAT_EQ(m1["2"], std::numbers::pi_v<float>);
	EXPECT_EQ(m1.size(), 8);

	m1.erase("0");
	m1.erase("1");
	EXPECT_EQ(m1.size(), 6);

	auto item = m1.find("2");
	EXPECT_NE(item, m1.end());
	EXPECT_EQ((*item <=> std::pair<const std::string, float>{"2", std::numbers::pi_v<float>}),
			  std::weak_ordering::equivalent);
	EXPECT_EQ(m1.find("1"), m1.end());

	sek::dense_map<std::string, int> m2;

	for (auto i = 0; i < 1000; i++) m2.emplace(std::to_string(i), i);
	for (auto i = 0; i < 200; i++) m2.erase(m2.find(std::to_string(i)));
	for (auto i = 500; i < 1000; i++) m2.erase(m2.find(std::to_string(i)));

	m2[std::to_string(500)] = 500;

	for (auto i = 200; i <= 500; i++)
	{
		int val;
		EXPECT_NO_THROW(val = m2.at(std::to_string(i)));
		EXPECT_EQ(val, i);
	}
}

#include "sekhmet/dense_set.hpp"

template class sek::dense_set<std::string>;

TEST(container_tests, dense_set_test)
{
	sek::dense_set<std::string> s1 = {"1", "2", "3", "4"};

	EXPECT_EQ(s1.size(), 4);
	EXPECT_FALSE(s1.contains("0"));
	EXPECT_TRUE(s1.contains("1"));

	s1.erase("1");

	EXPECT_EQ(s1.size(), 3);
	EXPECT_FALSE(s1.contains("1"));
	EXPECT_EQ(s1.find("1"), s1.end());
}

#include "sekhmet/detail/dynarray.hpp"

template class sek::dynarray<int>;

TEST(container_tests, basic_dynarray_test)
{
	std::vector<int> v = {0, 1, 2, 3, 4};
	sek::dynarray<int> da = {0, 1, 2, 3, 4};

	EXPECT_FALSE(da.empty());
	EXPECT_EQ(da.size(), v.size());
	EXPECT_TRUE(std::equal(v.begin(), v.end(), da.begin(), da.end()));

	v = {0, 1, 2, 3, 4, 5, 6, 7};
	EXPECT_FALSE(std::equal(v.begin(), v.end(), da.begin(), da.end()));

	da = v;
	EXPECT_TRUE(std::equal(v.begin(), v.end(), da.begin(), da.end()));
}
