//
// Created by switchblade on 2022-03-03.
//

#include <gtest/gtest.h>

#include <string>

#include "sekhmet/utility.hpp"

struct empty_t
{
};
static_assert(sizeof(sek::packed_pair<empty_t, int>) == sizeof(int));
static_assert(sizeof(sek::packed_pair<int, int>) == sizeof(int) * 2);

TEST(utility_tests, version_test)
{
	sek::version v1 = {0, 0, 1};
	sek::version v2 = {0, 0, 2};
	sek::version v3 = {0, 1, 2};

	EXPECT_LT(v1, v2);
	EXPECT_LT(v1, v3);
	EXPECT_LT(v2, v3);

	EXPECT_EQ(v3, sek::version(0, 1, 2));

	EXPECT_EQ(hash(v3), hash(sek::version(0, 1, 2)));

	std::string s;
	v3.to_string<char>(std::back_inserter(s));
	EXPECT_EQ(s, "0.1.2");
}

TEST(utility_tests, uuid_test)
{
	using sek::hash;

	constexpr char uuid_str[] = "e7d751b6-f2f8-4541-8b40-81063d82af28";
	constexpr sek::uuid id = sek::uuid{uuid_str};
	constexpr sek::hash_t id_hash = hash(id);

	EXPECT_NE(id, sek::uuid{});
	EXPECT_EQ(sek::uuid{}, sek::uuid{"00000000-0000-0000-0000-000000000000"});
	EXPECT_EQ(hash(id), id_hash);
	EXPECT_NE(hash(id), hash(sek::uuid{sek::uuid::version4}));

	std::string s;
	id.to_string<char>(std::back_inserter(s));
	EXPECT_EQ(s, "e7d751b6-f2f8-4541-8b40-81063d82af28");
	EXPECT_NE(sek::uuid{sek::uuid::version4}, sek::uuid{sek::uuid::version4});
}

#include "sekhmet/adapter.hpp"

namespace
{
	struct size_proxy : sek::adapter_proxy<std::size_t()>
	{
		template<typename T>
		std::size_t operator()(const T &i) const noexcept
		{
			return i.size();
		}
	};
}	 // namespace

TEST(utility_tests, adapter_test)
{
	struct int_size
	{
		constexpr std::size_t size() const noexcept { return sizeof(int); }
	};
	struct long_size
	{
		constexpr std::size_t size() const noexcept { return sizeof(long); }
	};
	struct size_get
	{
		constexpr std::size_t size() const noexcept { return i; }
		std::size_t i;
	};

	using size_adapter = sek::adapter<size_proxy>;

	const int_size i;
	long_size l;

	auto const_adapter_int = size_adapter{i};
	auto adapter_long = size_adapter{l};

	EXPECT_FALSE(const_adapter_int.empty());
	EXPECT_FALSE(adapter_long.empty());

	auto adapter = const_adapter_int;
	EXPECT_FALSE(adapter.empty());
	EXPECT_EQ(adapter.invoke<size_proxy>(), const_adapter_int.invoke<size_proxy>());
	EXPECT_EQ(adapter.invoke<size_proxy>(), sizeof(int));
	EXPECT_EQ(adapter.delegate<size_proxy>().invoke(), sizeof(int));

	adapter = adapter_long;
	EXPECT_FALSE(adapter.empty());
	EXPECT_EQ(adapter.invoke<size_proxy>(), adapter_long.invoke<size_proxy>());
	EXPECT_EQ(adapter.invoke<size_proxy>(), sizeof(long));
	EXPECT_EQ(adapter.delegate<size_proxy>().invoke(), sizeof(long));

	adapter.reset();
	EXPECT_TRUE(adapter.empty());

	const size_get s = {sizeof(void *)};
	adapter.rebind(s);

	EXPECT_FALSE(adapter.empty());
	EXPECT_EQ(adapter.invoke<size_proxy>(), s.size());
	EXPECT_EQ(adapter.delegate<size_proxy>().invoke(), s.size());
}

#include "sekhmet/thread_pool.hpp"

TEST(utility_tests, thread_pool_test)
{
	using ms_t = std::chrono::milliseconds;
	constexpr auto task = []() { std::this_thread::sleep_for(ms_t(100)); };

	/* 4 threads will need to wait once (4 tasks / 4 threads). */
	sek::thread_pool tp{4};
	auto f1 = tp.schedule(task);
	auto f2 = tp.schedule(task);
	auto f3 = tp.schedule(task);
	auto f4 = tp.schedule(task);
	auto wait_start = std::chrono::steady_clock::now();
	f1.wait();
	f2.wait();
	f3.wait();
	f4.wait();
	EXPECT_GE(std::chrono::duration_cast<ms_t>(std::chrono::steady_clock::now() - wait_start), ms_t{100});

	/* 2 threads will need to wait twice (4 tasks / 2 threads). */
	tp.resize(2);
	f1 = tp.schedule(task);
	f2 = tp.schedule(task);
	f3 = tp.schedule(task);
	f4 = tp.schedule(task);
	wait_start = std::chrono::steady_clock::now();
	f1.wait();
	f2.wait();
	f3.wait();
	f4.wait();
	EXPECT_GE(std::chrono::duration_cast<ms_t>(std::chrono::steady_clock::now() - wait_start), ms_t{200});
}

#include "sekhmet/logger.hpp"

TEST(utility_tests, logger_test)
{
	std::stringstream ss;
	const auto listener = sek::delegate{+[](std::stringstream *ss, std::string_view msg) { *ss << msg; }, &ss};

	{
		constexpr auto log_msg = "Test log info";
		sek::logger::info().log_event += listener;
		sek::logger::info() << log_msg;

		auto output = ss.str();
		EXPECT_NE(output.find(log_msg), std::string::npos);
		EXPECT_NE(output.find("Info"), std::string::npos);

		EXPECT_TRUE(sek::logger::info().log_event -= listener);
	}
	{
		constexpr auto log_msg = "Test log warning";
		sek::logger::warn().log_event += listener;
		sek::logger::warn() << log_msg;

		auto output = ss.str();
		EXPECT_NE(output.find(log_msg), std::string::npos);
		EXPECT_NE(output.find("Warn"), std::string::npos);

		EXPECT_TRUE(sek::logger::warn().log_event -= listener);
	}
	{
		constexpr auto log_msg = "Test log error";
		sek::logger::error().log_event += listener;
		sek::logger::error() << log_msg;

		auto output = ss.str();
		EXPECT_NE(output.find(log_msg), std::string::npos);
		EXPECT_NE(output.find("Error"), std::string::npos);

		EXPECT_TRUE(sek::logger::error().log_event -= listener);
	}
}

#include "sekhmet/event.hpp"

template class sek::delegate<void(int &)>;
template class sek::basic_event<void(int &), std::allocator<sek::delegate<void(int &)>>>;

TEST(utility_tests, event_test)
{
	sek::event<void(int &)> event;

	auto i = 0;
	auto sub1 = event += sek::delegate{+[](int &i) { EXPECT_EQ(i++, 0); }};
	auto sub2 = event += sek::delegate{+[](int &i) { EXPECT_EQ(i++, 1); }};

	event(i);
	EXPECT_EQ(i, 2);

	event -= sub1;
	event -= sub2;
	EXPECT_TRUE(event.empty());
	EXPECT_EQ(event.size(), 0);

	sub1 = event += sek::delegate{+[](int &i) { EXPECT_EQ(i++, 0); }};
	sub2 = event += sek::delegate{+[](int &i) { EXPECT_EQ(i++, 1); }};
	EXPECT_EQ(event.size(), 2);

	i = 0;
	event(i);
	EXPECT_EQ(i, 2);

	auto sub2_pos = event.find(sub2);
	EXPECT_NE(sub2_pos, event.end());

	// clang-format off
	sub2 = event.subscribe(sub2_pos, +[](int i) { EXPECT_EQ(i, 1); });
	event.subscribe_after(sub2, +[](int &i) { EXPECT_EQ(i, 1); });
	EXPECT_EQ(event.size(), 4);
	// clang-format on

	i = 0;
	event(i);
	EXPECT_EQ(i, 2);
}
