/*
 * Created by switchblade on 16/07/22
 */

#include <gtest/gtest.h>

#include "sekhmet/engine/ecs.hpp"

using namespace sek::engine;

template class sek::engine::basic_entity_set<>;

TEST(ecs_tests, entity_test)
{
	{
		const auto et1 = entity_t::tombstone();
		const auto et2 = entity_t{et1.generation(), {}};

		EXPECT_EQ(et1, et2);
		EXPECT_NE(et1.index(), et2.index());

		entity_t e1 = {};

		EXPECT_NE(et1, e1);
		EXPECT_NE(et2, e1);
		EXPECT_NE(et1.index(), e1.index());
		EXPECT_EQ(et2.index(), e1.index());
	}
	{
		const entity_t e0 = {entity_t::index_type{0}};
		const entity_t e1 = {entity_t::index_type{1}};
		const entity_t e2 = {entity_t::index_type{2}};

		entity_set set;
		set.insert(e0);
		set.insert(e1);
		set.insert(e2);

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
	struct flag_t
	{
	};
}	 // namespace

template class sek::engine::component_set<int>;
template class sek::engine::component_set<dummy_t>;

TEST(ecs_tests, set_test)
{
	entity_world world;

	const entity_t e0 = {entity_t::index_type{0}};
	const entity_t e1 = {entity_t::index_type{1}};
	const entity_t e2 = {entity_t::index_type{2}};

	{
		auto s = component_set<int>{world};
		s.emplace(e0);
		s.emplace(e1);
		s.emplace(e2);

		EXPECT_EQ(s.size(), 3);

		s.get(e0) = 0;
		s.get(e1) = 1;
		s.get(e2) = 2;

		EXPECT_EQ((s.begin() + 0)->second, 2);
		EXPECT_EQ((s.begin() + 1)->second, 1);
		EXPECT_EQ((s.begin() + 2)->second, 0);

		const auto order = std::array{e1, e0};
		s.sort(order.begin(), order.end());
		EXPECT_EQ((s.begin() + 0)->second, 0);
		EXPECT_EQ((s.begin() + 1)->second, 1);
		EXPECT_EQ((s.begin() + 2)->second, 2);

		s.erase(e2);
		EXPECT_EQ(s.size(), 2);
		EXPECT_EQ(s.find(e0)->second, 0);
		EXPECT_EQ(s.find(e1)->second, 1);
	}
	{
		auto s = component_set<dummy_t>{world};
		s.emplace(e0);
		s.emplace(e1);
		s.emplace(e2);

		EXPECT_EQ(s.size(), 3);
		EXPECT_TRUE(s.contains(e0));
		EXPECT_TRUE(s.contains(e1));
		EXPECT_TRUE(s.contains(e2));

		s.erase(e2);
		EXPECT_EQ(s.size(), 2);
		EXPECT_TRUE(s.contains(e0));
		EXPECT_TRUE(s.contains(e1));
		EXPECT_FALSE(s.contains(e2));
	}
	{
		auto si0 = component_set<int>{world};
		si0.emplace(e0, 0);
		si0.emplace(e1, 1);

		auto sf0 = component_set<float>{world};
		sf0.emplace(e0, 0.0f);
		sf0.emplace(e1, 1.0f);
		sf0.emplace(e2, 2.0f);

		auto iptr = component_ptr{e0, si0};
		auto fptr = component_ptr{e0, sf0};
		EXPECT_TRUE(iptr);
		EXPECT_TRUE(fptr);
		EXPECT_EQ(*iptr, 0);
		EXPECT_EQ(*fptr, 0.0f);

		auto pi1 = component_set<int>{world};
		pi1.emplace(e0, 10);

		EXPECT_EQ(iptr.reset(&pi1), &si0);
		EXPECT_TRUE(iptr);
		EXPECT_EQ(*iptr, 10);
	}
	{
		const auto a0 = forward_any<int>(0);
		const auto a1 = forward_any<int>(1);

		auto s = component_set<int>{world};
		auto &gs = static_cast<generic_component_set &>(s);

		gs.insert(e0, a0);
		gs.insert(e1, a1);

		EXPECT_TRUE(s.contains(e0));
		EXPECT_TRUE(s.contains(e1));

		EXPECT_EQ(gs.get_any(e0).cast<int>(), a0.cast<int>());
		EXPECT_EQ(gs.get_any(e1).cast<int>(), a1.cast<int>());

		EXPECT_EQ(gs.get_any(e0).as_ptr<int>(), &s.get(e0));
		EXPECT_EQ(gs.get_any(e1).as_ptr<int>(), &s.get(e1));
	}
}

template class sek::engine::component_set<float>;

TEST(ecs_tests, world_test)
{
	entity_world world;

	world.reserve<int>();
	world.reserve<float, dummy_t>();

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

	const auto sv = world.storage();
	EXPECT_FALSE(sv.empty());
	EXPECT_EQ(sv.size(), 3);

	auto &gs = sv.front();
	const auto t = gs.type();
	EXPECT_EQ(&gs, world.storage(t));

	const auto e3 = world.generate();
	gs.insert(e3, t.construct());
	EXPECT_TRUE(gs.contains(e3));
	EXPECT_FALSE(world.empty(e3));
}

template class sek::engine::component_view<included_t<int>>;
template class sek::engine::component_view<included_t<int>, excluded_t<dummy_t>>;
template class sek::engine::component_view<included_t<int>, excluded_t<dummy_t>, optional_t<float>>;

TEST(ecs_tests, view_test)
{
	entity_world world;

	const auto total = 1000003u;
	world.reserve<int>(total);

	for (std::size_t i = total - 3; i-- != 0;) world.insert<int>();
	const auto e0 = *world.insert<int>();
	const auto e1 = *world.insert(int{1}, float{1});
	const auto e2 = *world.insert(int{2}, dummy_t{});

	const auto start = std::chrono::system_clock::now();

	const auto view1 = world.query().include<int>().exclude<dummy_t>().optional<float>().view();
	EXPECT_FALSE(view1.empty());
	EXPECT_EQ(view1.size_hint(), total);

	view1.for_each(
		[&](entity_t e, int *i, const float *f)
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

	const auto view2 = world.query().include<int>().optional<float, dummy_t>().view();
	EXPECT_FALSE(view1.empty());
	EXPECT_EQ(view1.size_hint(), total);

	std::size_t iterations = 0;
	view2.for_each([&](auto /*e*/, int *i, auto /*f*/, auto /*d*/) { ++(*i), ++iterations; });

	EXPECT_EQ(iterations, view2.size_hint());
	EXPECT_EQ(world.get<int>(e0), 1);
	EXPECT_EQ(world.get<int>(e1), 3);
	EXPECT_EQ(world.get<int>(e2), 3);

	world.view<int>().for_each([](auto /*e*/, const int *i) { EXPECT_NE(*i, 0); });

	using namespace std::literals;

	const auto end = std::chrono::system_clock::now();
	const auto ms = duration_cast<std::chrono::duration<double, std::milli>>(end - start);
	const auto ns = duration_cast<std::chrono::duration<double, std::nano>>(end - start);
	printf("%.2f fps\n", 1s / ns);
	printf("%.2f ms\n", ms.count());
	printf("%.2f ns\n", ns.count());
}

template class sek::engine::component_collection<collected_t<flag_t>>;
template class sek::engine::component_collection<collected_t<int>, included_t<>, excluded_t<>, optional_t<flag_t>>;
template class sek::engine::component_collection<collected_t<int>, included_t<>, excluded_t<dummy_t>, optional_t<flag_t>>;
template class sek::engine::component_collection<collected_t<int, float>, included_t<>, excluded_t<dummy_t>, optional_t<flag_t>>;
template class sek::engine::component_collection<collected_t<>, included_t<int, flag_t>, excluded_t<>, optional_t<>>;

TEST(ecs_tests, collection_test)
{
	entity_world world;

	const auto e0 = *world.insert<int>(0);
	const auto e1 = *world.insert<int, flag_t>(1, {});
	const auto e2 = *world.insert<int, float, flag_t>(2, 2.0f, {});
	const auto e3 = *world.insert<int, float, flag_t, dummy_t>(3, 3.0f, {}, {});

	const auto c1 = world.collection<flag_t>();
	const auto c2 = world.query().collect<int>().optional<flag_t>().collection();
	const auto c3 = world.query().collect<int>().optional<flag_t>().exclude<dummy_t>().collection();
	const auto c4 = world.query().collect<int, float>().optional<flag_t>().exclude<dummy_t>().collection();
	const auto c5 = world.query().include<int, flag_t>().collection();

	EXPECT_EQ(c1.size(), 3);
	EXPECT_FALSE(c1.contains(e0));
	EXPECT_TRUE(c1.contains(e1));
	EXPECT_TRUE(c1.contains(e2));
	EXPECT_TRUE(c1.contains(e3));

	EXPECT_EQ(c2.size(), 4);
	EXPECT_TRUE(c2.contains(e0));
	EXPECT_TRUE(c2.contains(e1));
	EXPECT_TRUE(c2.contains(e2));
	EXPECT_TRUE(c2.contains(e3));

	EXPECT_EQ(c3.size(), 3);
	EXPECT_TRUE(c3.contains(e0));
	EXPECT_TRUE(c3.contains(e1));
	EXPECT_TRUE(c3.contains(e2));
	EXPECT_FALSE(c3.contains(e3));

	EXPECT_EQ(c4.size(), 1);
	EXPECT_FALSE(c4.contains(e0));
	EXPECT_FALSE(c4.contains(e1));
	EXPECT_TRUE(c4.contains(e2));
	EXPECT_FALSE(c4.contains(e3));

	EXPECT_EQ(c5.size(), 3);
	EXPECT_FALSE(c5.contains(e0));
	EXPECT_TRUE(c5.contains(e1));
	EXPECT_TRUE(c5.contains(e2));
	EXPECT_TRUE(c5.contains(e3));

	c1.for_each([&](entity_t e, auto *) { EXPECT_NE(e, e0); });
	c2.for_each([&](entity_t e, const int *i, auto...) { EXPECT_EQ(e.index().value(), *i); });
	c3.for_each(
		[&](entity_t e, const int *i, auto...)
		{
			EXPECT_NE(e, e3);
			EXPECT_LT(*i, 3);
		});
	c4.for_each(
		[&](entity_t e, const int *i, const float *f, auto...)
		{
			EXPECT_NE(e, e0);
			EXPECT_NE(e, e1);
			EXPECT_NE(e, e3);
			EXPECT_EQ(*i, 2);
			EXPECT_EQ(static_cast<int>(*f), *i);
		});
	c5.for_each(
		[&](entity_t e, const int *i, auto...)
		{
			EXPECT_NE(e, e0);
			EXPECT_NE(*i, 0);
		});

	const auto e4 = *world.insert<int, flag_t>(4, {});

	EXPECT_EQ(c1.size(), 4);
	EXPECT_TRUE(c1.contains(e4));

	EXPECT_EQ(c2.size(), 5);
	EXPECT_TRUE(c2.contains(e4));

	EXPECT_EQ(c3.size(), 4);
	EXPECT_TRUE(c3.contains(e4));

	EXPECT_EQ(c4.size(), 1);
	EXPECT_FALSE(c4.contains(e4));

	EXPECT_EQ(c5.size(), 4);
	EXPECT_TRUE(c5.contains(e4));

	EXPECT_EQ(std::addressof(world.get<flag_t>(e4)), c1.get<flag_t>(e4));
	EXPECT_EQ(std::addressof(world.get<flag_t>(e4)), c5.get<flag_t>(e4));
	EXPECT_EQ(std::addressof(world.get<int>(e4)), c2.get<int>(e4));
	EXPECT_EQ(std::addressof(world.get<int>(e4)), c3.get<int>(e4));
	EXPECT_EQ(std::addressof(world.get<int>(e4)), c5.get<int>(e4));

	EXPECT_TRUE(world.is_collected<int>());
	EXPECT_TRUE(world.is_collected<float>());
	EXPECT_TRUE(world.is_collected<flag_t>());
	EXPECT_FALSE(world.is_collected<dummy_t>());
}