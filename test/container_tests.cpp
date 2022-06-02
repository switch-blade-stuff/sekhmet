/*
 * ============================================================================
 * Sekhmet - C++20 game engine & editor
 * Copyright (C) 2022 switchblade
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 * ============================================================================
 *
 * Created by switchblade on 2021-12-01
 */

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
	EXPECT_EQ((*item <=> std::pair<const std::string, float>{"2", std::numbers::pi_v<float>}), std::weak_ordering::equivalent);
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

#include "sekhmet/intern.hpp"

TEST(container_tests, intern_test)
{
	constexpr auto literal = "String to intern";

	sek::intern_pool pool;

	sek::interned_string is1 = {pool, literal};
	sek::interned_string is2 = pool.intern(literal);
	sek::interned_string is3 = {literal};

	EXPECT_EQ(is1, is2);
	EXPECT_EQ(is1, is3);
	EXPECT_EQ(is2, is3);
	EXPECT_EQ(is1.data(), is2.data());
	EXPECT_NE(is1.data(), is3.data());
	EXPECT_NE(is2.data(), is3.data());

	std::string copy = is1;
	EXPECT_EQ(is1, copy);
	EXPECT_NE(is1.data(), copy.data());

	is2 = sek::interned_string{pool, copy};
	EXPECT_EQ(is2, copy);
	EXPECT_NE(is2.data(), copy.data());
	EXPECT_EQ(is2, is1);
	EXPECT_EQ(is2.data(), is1.data());
}

#include "sekhmet/mkmap.hpp"

using multikey_t = sek::multikey<sek::key_t<std::string>, sek::key_t<int>>;
template class sek::detail::mkmap_impl<multikey_t, float, std::allocator<sek::mkmap_value_t<multikey_t, float>>>;

TEST(container_tests, mkmap_test)
{
	sek::mkmap<multikey_t, float> m1 = {
		{multikey_t{"0", 0}, 9.9f},
		{multikey_t{"1", 1}, 7.6f},
		{multikey_t{"2", 2}, std::numbers::pi_v<float>},
		{multikey_t{"3", 3}, 0.f},
		{multikey_t{"4", 4}, 0.f},
		{multikey_t{"5", 5}, 0.f},
		{multikey_t{"6", 6}, 0.f},
		{multikey_t{"7", 7}, 0.f},
	};

	EXPECT_TRUE(m1.contains("7"));
	EXPECT_TRUE(m1.contains<0>("7"));
	EXPECT_TRUE(m1.contains<1>(7));
	EXPECT_FALSE(m1.contains("8"));
	EXPECT_FALSE(m1.contains<0>("8"));
	EXPECT_FALSE(m1.contains<1>(8));

	EXPECT_EQ(m1.find<0>("7"), m1.find<1>(7));
	EXPECT_EQ(m1.find("7"), m1.find<1>(7));
	EXPECT_NE(m1.find<0>("6"), m1.find<1>(7));
	EXPECT_NE(m1.find("6"), m1.find<1>(7));
	EXPECT_EQ(m1.size(), 8);

	EXPECT_TRUE(m1.erase<0>("0"));
	EXPECT_TRUE(m1.erase<1>(1));
	EXPECT_EQ(m1.size(), 6);

	auto item = m1.find("2");
	EXPECT_NE(item, m1.end());
	EXPECT_EQ((*item <=> sek::mkmap_value_t<multikey_t, float>{multikey_t{"2", 2}, std::numbers::pi_v<float>}),
			  std::weak_ordering::equivalent);
	EXPECT_EQ(m1.find("1"), m1.end());

	EXPECT_EQ(m1.emplace(multikey_t{"2", 3}, 9999.f).second, 2);
	EXPECT_TRUE(m1.contains<0>("2"));
	EXPECT_EQ(m1.at<0>("2"), 9999.f);
	EXPECT_TRUE(m1.contains<1>(3));
	EXPECT_EQ(m1.at<1>(3), 9999.f);
	EXPECT_FALSE(m1.contains<1>(2));
	EXPECT_FALSE(m1.contains<0>("3"));

	sek::mkmap<multikey_t, int> m2;

	for (auto i = 0; i < 1000; i++) m2.emplace(multikey_t{std::to_string(i), i}, ~i);
	for (auto i = 0; i < 200; i++) m2.erase(m2.find<1>(i));
	for (auto i = 500; i < 1000; i++) m2.erase(m2.find<0>(std::to_string(i)));

	m2.emplace(multikey_t{"500", 500}, ~500);

	for (auto i = 200; i <= 500; i++)
	{
		int val;
		EXPECT_NO_THROW(val = m2.at<1>(i));
		EXPECT_EQ(val, ~i);
	}
}
