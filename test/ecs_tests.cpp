/*
 * Created by switchblade on 16/07/22
 */

#include <gtest/gtest.h>

#include "sekhmet/engine/ecs.hpp"

template class sek::engine::basic_entity_set<>;

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

		sek::engine::entity_set set;
		set.emplace(e0);
		set.emplace(e1);
		set.emplace(e2);

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

namespace
{
	struct dummy_t
	{
	};
}	 // namespace

template class sek::engine::basic_entity_set<int>;
template class sek::engine::basic_entity_set<dummy_t>;

TEST(ecs_tests, pool_test)
{
	sek::engine::entity_world world;

	const sek::engine::entity_t e0 = {sek::engine::entity_t::index_type{0}};
	const sek::engine::entity_t e1 = {sek::engine::entity_t::index_type{1}};
	const sek::engine::entity_t e2 = {sek::engine::entity_t::index_type{2}};

	{
		auto p = sek::engine::component_set<int>{world};
		p.emplace(e0);
		p.emplace(e1);
		p.emplace(e2);

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
		auto p = sek::engine::component_set<dummy_t>{world};
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
		auto pi0 = sek::engine::component_set<int>{world};
		pi0.emplace(e0, 0);
		pi0.emplace(e1, 1);

		auto pf0 = sek::engine::component_set<float>{world};
		pf0.emplace(e0, 0.0f);
		pf0.emplace(e1, 1.0f);
		pf0.emplace(e2, 2.0f);

		auto iptr = sek::engine::component_ptr{e0, pi0};
		auto fptr = sek::engine::component_ptr{e0, pf0};
		EXPECT_TRUE(iptr);
		EXPECT_TRUE(fptr);
		EXPECT_EQ(*iptr, 0);
		EXPECT_EQ(*fptr, 0.0f);

		auto pi1 = sek::engine::component_set<int>{world};
		pi1.emplace(e0, 10);

		EXPECT_EQ(iptr.reset(&pi1), &pi0);
		EXPECT_TRUE(iptr);
		EXPECT_EQ(*iptr, 10);
	}
}

template class sek::engine::component_set<int>;
template class sek::engine::component_set<float>;
template class sek::engine::component_set<dummy_t>;

TEST(ecs_tests, world_test)
{
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
		EXPECT_TRUE(world.contains_all<dummy_t>(e2));
		EXPECT_TRUE(world.contains_any<dummy_t>(e2));

		EXPECT_EQ(world.get<int>(e0), 0);
		EXPECT_EQ(world.get<int>(e1), 1);

		EXPECT_FALSE(world.erase_and_release<float>(e0));
		EXPECT_FALSE(world.contains_all<float>(e0));
		EXPECT_EQ(world.size(e0), 1);
		EXPECT_EQ(world.size(e1), 1);

		EXPECT_TRUE(world.erase_and_release<dummy_t>(e2));
		EXPECT_FALSE(world.contains(e2));
	}
	{
		using namespace sek::engine;

		entity_world world;

		for (std::size_t i = 1000000; i-- != 0;) world.insert<int>();
		const auto e0 = *world.insert<int>();
		const auto e1 = *world.insert(int{1}, float{1});
		const auto e2 = *world.insert(int{2}, dummy_t{});

		const auto view = world.view<int, const float>(excluded_t<dummy_t>{}, optional_t<float>{});
		EXPECT_FALSE(view.empty());
		EXPECT_EQ(view.size_hint(), 1000003);

		view.for_each(
			[&](sek::engine::entity_t e, int *i, const float *f)
			{
				EXPECT_NE(e, e2);
				EXPECT_NE(i, nullptr);
				if (e == e0)
				{
					EXPECT_EQ(f, nullptr);
					EXPECT_EQ(*i, 0);
					return false;
				}
				else if (e == e1)
				{
					EXPECT_NE(f, nullptr);
					EXPECT_EQ(*i, 1);
					EXPECT_EQ(*f, 1.0f);
				}
				(*i)++;
				return true;
			});

		EXPECT_EQ(world.get<int>(e0), 0);
		EXPECT_EQ(world.get<int>(e1), 2);
		EXPECT_EQ(world.get<int>(e2), 2);
	}
}