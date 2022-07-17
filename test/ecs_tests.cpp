/*
 * Created by switchblade on 16/07/22
 */

#include <gtest/gtest.h>

#include "sekhmet/engine/ecs.hpp"

TEST(ecs_tests, entity_test)
{
	{
		const auto et1 = sek::engine::entity::tombstone();
		const auto et2 = sek::engine::entity{et1.generation(), {}};

		EXPECT_EQ(et1, et2);
		EXPECT_NE(et1.index(), et2.index());

		sek::engine::entity e1 = {};

		EXPECT_NE(et1, e1);
		EXPECT_NE(et2, e1);
		EXPECT_NE(et1.index(), e1.index());
		EXPECT_EQ(et2.index(), e1.index());
	}
	{
		const sek::engine::entity e0 = {sek::engine::entity::index_type{0}};
		const sek::engine::entity e1 = {sek::engine::entity::index_type{1}};
		const sek::engine::entity e2 = {sek::engine::entity::index_type{2}};

		sek::engine::entity_set set;
		set.insert(e0);
		set.insert(e1);
		set.insert(e2);

		EXPECT_EQ(set.size(), 3);
		EXPECT_EQ(*(set.begin() + 0), e2);
		EXPECT_EQ(*(set.begin() + 1), e1);
		EXPECT_EQ(*(set.begin() + 2), e0);

		const auto order = std::array{e1, e0};
		set.sort(order.begin(), order.end());
		EXPECT_EQ(*(set.begin() + 0), e0);
		EXPECT_EQ(*(set.begin() + 1), e1);
		EXPECT_EQ(*(set.begin() + 2), e2);

		set.erase(e2);
		EXPECT_EQ(set.size(), 2);
	}
	{
		const sek::engine::entity e0 = {sek::engine::entity::index_type{0}};
		const sek::engine::entity e1 = {sek::engine::entity::index_type{1}};
		const sek::engine::entity e2 = {sek::engine::entity::index_type{2}};

		sek::engine::basic_entity_set<std::allocator<sek::engine::entity>, true> set;
		set.insert(e0);
		set.insert(e1);
		set.insert(e2);

		EXPECT_EQ(set.size(), 3);
		EXPECT_EQ(*(set.begin() + 0), e2);
		EXPECT_EQ(*(set.begin() + 1), e1);
		EXPECT_EQ(*(set.begin() + 2), e0);

		set.erase(e1);
		EXPECT_EQ(set.size(), 3);
		EXPECT_TRUE((set.begin() + 1)->is_tombstone());

		set.pack();
		EXPECT_EQ(set.size(), 2);
	}
}

TEST(ecs_tests, pool_test)
{
	const sek::engine::entity e0 = {sek::engine::entity::index_type{0}};
	const sek::engine::entity e1 = {sek::engine::entity::index_type{1}};
	const sek::engine::entity e2 = {sek::engine::entity::index_type{2}};

	{
		sek::engine::basic_component_pool<int> p;
		p.emplace(e0, 0);
		p.emplace(e1, 1);
		p.emplace(e2, 2);

		EXPECT_EQ(p.size(), 3);
		EXPECT_EQ(*(p.begin() + 0), 2);
		EXPECT_EQ(*(p.begin() + 1), 1);
		EXPECT_EQ(*(p.begin() + 2), 0);

		const auto order = std::array{e1, e0};
		p.entities().sort(order.begin(), order.end());
		EXPECT_EQ(*(p.begin() + 0), 0);
		EXPECT_EQ(*(p.begin() + 1), 1);
		EXPECT_EQ(*(p.begin() + 2), 2);

		p.erase(e2);
		EXPECT_EQ(p.size(), 2);
		EXPECT_EQ(*p.find(e0), 0);
		EXPECT_EQ(*p.find(e1), 1);
	}
	{
		struct dummy
		{
		};

		sek::engine::basic_component_pool<dummy> p;
		p.emplace(e0);
		p.emplace(e1);
		p.emplace(e2);

		EXPECT_EQ(p.size(), 3);
		EXPECT_TRUE(p.contains(e0));
		EXPECT_TRUE(p.contains(e1));
		EXPECT_TRUE(p.contains(e2));

		p.erase(e2);
		EXPECT_EQ(p.size(), 2);
		EXPECT_TRUE(p.contains(e0));
		EXPECT_TRUE(p.contains(e1));
		EXPECT_FALSE(p.contains(e2));
	}
}