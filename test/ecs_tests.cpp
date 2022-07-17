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