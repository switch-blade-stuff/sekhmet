/*
 * Created by switchblade on 16/07/22
 */

#include <gtest/gtest.h>

#include "sekhmet/engine/ecs.hpp"

TEST(ecs_tests, entity_test)
{
	{
		const auto et1 = sek::engine::entity_t::tombstone();
		const auto et2 = sek::engine::entity_t{et1.generation(), {}};

		EXPECT_EQ(et1, et2);
		EXPECT_NE(et1.index(), et2.index());

		sek::engine::entity_t e1 = {};

		EXPECT_NE(et1, e1);
		EXPECT_NE(et2, e1);
		EXPECT_NE(et1.index(), e1.index());
		EXPECT_EQ(et2.index(), e1.index());
	}
	{
		const sek::engine::entity_t e0 = {sek::engine::entity_t::index_type{0}};
		const sek::engine::entity_t e1 = {sek::engine::entity_t::index_type{1}};
		const sek::engine::entity_t e2 = {sek::engine::entity_t::index_type{2}};

		sek::engine::entity_set set = {e0, e1, e2};

		EXPECT_EQ(set.size(), 3);
		EXPECT_EQ(*(set.begin() + 0), e2);
		EXPECT_EQ(*(set.begin() + 1), e1);
		EXPECT_EQ(*(set.begin() + 2), e0);

		const auto order = std::array{e0, e1};
		set.sort(order.begin(), order.end());
		EXPECT_EQ(*(set.begin() + 0), e1);
		EXPECT_EQ(*(set.begin() + 1), e0);
		EXPECT_EQ(*(set.begin() + 2), e2);

		set.erase(e2);
		EXPECT_EQ(set.size(), 2);
	}
}

TEST(ecs_tests, pool_test)
{
	const sek::engine::entity_t e0 = {sek::engine::entity_t::index_type{0}};
	const sek::engine::entity_t e1 = {sek::engine::entity_t::index_type{1}};
	const sek::engine::entity_t e2 = {sek::engine::entity_t::index_type{2}};

	{
		sek::engine::component_set<int> p = {e0, e1, e2};

		EXPECT_EQ(p.size(), 3);

		p.get(e0) = 0;
		p.get(e1) = 1;
		p.get(e2) = 2;

		EXPECT_EQ((p.begin() + 0)->second, 2);
		EXPECT_EQ((p.begin() + 1)->second, 1);
		EXPECT_EQ((p.begin() + 2)->second, 0);

		const auto order = std::array{e1, e0};
		p.sort(order.begin(), order.end());
		EXPECT_EQ((p.begin() + 0)->second, 0);
		EXPECT_EQ((p.begin() + 1)->second, 1);
		EXPECT_EQ((p.begin() + 2)->second, 2);

		p.erase(e2);
		EXPECT_EQ(p.size(), 2);
		EXPECT_EQ(p.find(e0)->second, 0);
		EXPECT_EQ(p.find(e1)->second, 1);
	}
	{
		struct dummy
		{
		};

		sek::engine::component_set<dummy> p;
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
	{
		sek::engine::component_set<int> pi0;
		pi0.emplace(e0, 0);
		pi0.emplace(e1, 1);

		sek::engine::component_set<float> pf0;
		pf0.emplace(e0, 0.0f);
		pf0.emplace(e1, 1.0f);
		pf0.emplace(e2, 2.0f);

		auto iptr = sek::engine::component_ptr{e0, pi0};
		auto fptr = sek::engine::component_ptr{e0, pf0};
		EXPECT_TRUE(iptr);
		EXPECT_TRUE(fptr);
		EXPECT_EQ(*iptr, 0);
		EXPECT_EQ(*fptr, 0.0f);

		sek::engine::component_set<int> pi1;
		pi1.emplace(e0, 10);

		EXPECT_EQ(iptr.reset(&pi1), &pi0);
		EXPECT_TRUE(iptr);
		EXPECT_EQ(*iptr, 10);
	}
}

TEST(ecs_tests, world_test)
{
	struct dummy_t
	{
	};

	{
		sek::engine::entity_world world;

		const auto e0 = world.generate();
		const auto e1 = world.generate();
		const auto e2 = world.generate();
		EXPECT_EQ(world.size(), 3);
		EXPECT_TRUE(world.contains(e0));
		EXPECT_TRUE(world.contains(e1));
		EXPECT_TRUE(world.contains(e2));

		world.emplace<int>(e0, 0);
		world.emplace<int>(e1, 1);
		world.emplace<float>(e0);
		world.emplace<dummy_t>(e2);

		EXPECT_TRUE((world.contains_all<int, float>(e0)));
		EXPECT_FALSE((world.contains_all<int, float>(e1)));
		EXPECT_TRUE((world.contains_any<int, float>(e1)));
		EXPECT_TRUE((world.contains_none<int, float>(e2)));
		EXPECT_TRUE((world.contains_all<dummy_t>(e2)));
		EXPECT_TRUE((world.contains_any<dummy_t>(e2)));

		EXPECT_EQ(world.get<int>(e0), 0);
		EXPECT_EQ(world.get<int>(e1), 1);

		EXPECT_FALSE(world.erase_and_release<float>(e0));
		EXPECT_FALSE(world.contains_all<float>(e0));
		EXPECT_EQ(world.size(e0), 1);
		EXPECT_EQ(world.size(e1), 1);

		EXPECT_TRUE(world.erase_and_release<dummy_t>(e2));
		EXPECT_FALSE(world.contains(e2));
	}
}