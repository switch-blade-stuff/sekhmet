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
 * Created by switchblade on 2022-03-03
 */

#include <gtest/gtest.h>

#include <string>

#include "sekhmet/utility.hpp"

namespace
{
	struct empty_t
	{
	};
	static_assert(sizeof(sek::packed_pair<empty_t, int>) == sizeof(int));
	static_assert(sizeof(sek::packed_pair<int, int>) == sizeof(int) * 2);
}	 // namespace

TEST(utility_tests, version_test)
{
	using namespace sek::literals;

	EXPECT_NE(sek::version{SEK_ENGINE_VERSION}, "0.0.0"_ver);

	constexpr sek::version v1 = "0.0.1"_ver;
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
	using namespace sek::literals;
	using sek::hash;

	constexpr sek::uuid id = "e7d751b6-f2f8-4541-8b40-81063d82af28"_uuid;
	constexpr sek::hash_t id_hash = hash(id);

	EXPECT_NE(id, sek::uuid{});
	EXPECT_EQ(sek::uuid{}, "00000000-0000-0000-0000-000000000000"_uuid);
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

#include "sekhmet/engine/logger.hpp"

TEST(utility_tests, logger_test)
{
	std::stringstream ss;
	const auto listener = sek::delegate{+[](std::stringstream *ss, std::string_view msg) { *ss << msg; }, &ss};

	{
		constexpr auto log_msg = "Test log info";
		sek::engine::logger::info().on_log() += listener;
		sek::engine::logger::info() << log_msg;

		auto output = ss.str();
		EXPECT_NE(output.find(log_msg), std::string::npos);
		EXPECT_NE(output.find("Info"), std::string::npos);

		EXPECT_TRUE(sek::engine::logger::info().on_log() -= listener);
	}
	{
		constexpr auto log_msg = "Test log warning";
		sek::engine::logger::warn().on_log() += listener;
		sek::engine::logger::warn() << log_msg;

		auto output = ss.str();
		EXPECT_NE(output.find(log_msg), std::string::npos);
		EXPECT_NE(output.find("Warn"), std::string::npos);

		EXPECT_TRUE(sek::engine::logger::warn().on_log() -= listener);
	}
	{
		constexpr auto log_msg = "Test log error";
		sek::engine::logger::error().on_log() += listener;
		sek::engine::logger::error() << log_msg;

		auto output = ss.str();
		EXPECT_NE(output.find(log_msg), std::string::npos);
		EXPECT_NE(output.find("Error"), std::string::npos);

		EXPECT_TRUE(sek::engine::logger::error().on_log() -= listener);
	}
}

#include "sekhmet/access_guard.hpp"

TEST(utility_tests, access_guard_test)
{
	using namespace std::chrono_literals;

	constexpr auto thread_func = [](sek::access_guard<int> *i)
	{
		std::this_thread::sleep_for(100ms);
		EXPECT_FALSE(i->try_access_unique().has_value());
		std::this_thread::sleep_for(100ms);
		auto handle = i->access_unique();
		std::this_thread::sleep_for(100ms);
		EXPECT_EQ((*handle)++, 1);
	};

	sek::access_guard<int> i;
	auto t1 = std::thread{thread_func, &i};
	{
		auto handle = i.access_unique();
		std::this_thread::sleep_for(200ms);
		EXPECT_EQ((*handle)++, 0);
	}
	{
		std::this_thread::sleep_for(100ms);
		auto handle = i.access_unique();
		EXPECT_EQ(*handle, 2);
	}
	t1.join();
}

#include "sekhmet/event.hpp"

template class sek::delegate<void(int &)>;
template class sek::basic_event<void(int &), std::allocator<sek::delegate<void(int &)>>>;

TEST(utility_tests, event_test)
{
	sek::event<void(int &)> event;
	sek::event_proxy proxy = {event};

	auto i = 0;
	auto sub1 = proxy += +[](int &i) { EXPECT_EQ(i++, 0); };
	auto sub2 = proxy += +[](int &i) { EXPECT_EQ(i++, 1); };

	event(i);
	EXPECT_EQ(i, 2);

	proxy -= sub1;
	proxy -= sub2;
	EXPECT_TRUE(proxy.empty());
	EXPECT_EQ(proxy.size(), 0);

	proxy += +[](int &i) { EXPECT_EQ(i++, 0); };
	sub2 = proxy += +[](int &i) { EXPECT_EQ(i++, 1); };
	EXPECT_EQ(proxy.size(), 2);

	i = 0;
	event(i);
	EXPECT_EQ(i, 2);

	auto sub2_pos = proxy.find(sub2);
	EXPECT_NE(sub2_pos, proxy.end());

	// clang-format off
	sub2 = proxy.subscribe_before(sub2, +[](int &i) { EXPECT_EQ(i++, 1); });
	proxy.subscribe_before(sub2, sek::delegate{+[](int j, int &i) { EXPECT_EQ(i, j); }, 1});
	proxy.subscribe_after(sub2, +[](int &i) { EXPECT_EQ(i--, 2); });
	EXPECT_EQ(proxy.size(), 5);
	// clang-format on

	i = 0;
	event(i);
	EXPECT_EQ(i, 2);

	event.clear();
	EXPECT_TRUE(event.empty());

	{
		sek::subscriber_handle<sek::event<void(int &)>> handle;
		EXPECT_TRUE(handle.empty());

		handle.manage(
			event += +[](int) {}, event);
		EXPECT_FALSE(handle.empty());
		EXPECT_FALSE(event.empty());
	}

	EXPECT_TRUE(event.empty());
}

#include "sekhmet/message.hpp"

namespace
{
	struct test_message
	{
		[[nodiscard]] constexpr bool operator==(const test_message &) const noexcept = default;

		int i;
	};
}	 // namespace

template<>
[[nodiscard]] constexpr std::string_view sek::type_name<test_message>() noexcept
{
	return "test_message";
}

TEST(utility_tests, message_test)
{
	using namespace sek::literals;
	using namespace sek::attributes;

	sek::type_info::reflect<test_message>().attribute(make_message_type<test_message>);

	constexpr static auto msg_data = test_message{10};
	constexpr auto filter = [](std::size_t &ctr, const test_message &msg)
	{
		++ctr;
		EXPECT_EQ(msg_data, msg);
		return true;
	};
	constexpr auto receiver = [](std::size_t &ctr, const test_message &msg)
	{
		++ctr;
		EXPECT_EQ(msg_data, msg);
		return true;
	};

	std::size_t filter_ctr = 0, receiver_ctr = 0;

	{
		auto [l, proxy] = sek::message_queue<test_message>::on_send();
		proxy += sek::delegate<bool(const test_message &)>{filter, filter_ctr};
		EXPECT_EQ(proxy.size(), 1);
	}
	{
		auto [l, proxy] = sek::message_queue<test_message>::on_receive();
		proxy += sek::delegate<bool(const test_message &)>{receiver, receiver_ctr};
		EXPECT_EQ(proxy.size(), 1);
	}

	sek::message_queue<test_message>::send(msg_data);
	EXPECT_EQ(filter_ctr, 1);
	EXPECT_EQ(receiver_ctr, 1);

	sek::message_queue<test_message>::queue(msg_data);
	EXPECT_EQ(filter_ctr, 2);
	EXPECT_EQ(receiver_ctr, 1);

	sek::message_queue<test_message>::dispatch();
	EXPECT_EQ(filter_ctr, 2);
	EXPECT_EQ(receiver_ctr, 2);

	filter_ctr = 0;
	receiver_ctr = 0;

	auto attr = "test_message"_type.get_attribute<message_type>().cast<message_type>();

	attr.send(msg_data);
	EXPECT_EQ(filter_ctr, 1);
	EXPECT_EQ(receiver_ctr, 1);

	attr.queue(msg_data);
	EXPECT_EQ(filter_ctr, 2);
	EXPECT_EQ(receiver_ctr, 1);

	attr.dispatch();
	EXPECT_EQ(filter_ctr, 2);
	EXPECT_EQ(receiver_ctr, 2);
}
