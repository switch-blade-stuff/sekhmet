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

#include "sekhmet/logger.hpp"

TEST(utility_tests, logger_test)
{
	std::stringstream ss;
	const auto listener = sek::delegate{+[](std::stringstream *ss, std::string_view msg) { *ss << msg; }, &ss};

	{
		constexpr auto log_msg = "Test log info";
		sek::logger::info().on_log() += listener;
		sek::logger::info() << log_msg;

		auto output = ss.str();
		EXPECT_NE(output.find(log_msg), std::string::npos);
		EXPECT_NE(output.find("Info"), std::string::npos);

		EXPECT_TRUE(sek::logger::info().on_log() -= listener);
	}
	{
		constexpr auto log_msg = "Test log warning";
		sek::logger::warn().on_log() += listener;
		sek::logger::warn() << log_msg;

		auto output = ss.str();
		EXPECT_NE(output.find(log_msg), std::string::npos);
		EXPECT_NE(output.find("Warn"), std::string::npos);

		EXPECT_TRUE(sek::logger::warn().on_log() -= listener);
	}
	{
		constexpr auto log_msg = "Test log error";
		sek::logger::error().on_log() += listener;
		sek::logger::error() << log_msg;

		auto output = ss.str();
		EXPECT_NE(output.find(log_msg), std::string::npos);
		EXPECT_NE(output.find("Error"), std::string::npos);

		EXPECT_TRUE(sek::logger::error().on_log() -= listener);
	}
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
	proxy.subscribe_before(sub2, +[](int i) { EXPECT_EQ(i, 1); });
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

		handle.manage(event += +[](int) {}, event);
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

	sek::type_info::reflect<test_message>().attribute(make_message_source<test_message>);

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

	auto attr = "test_message"_type.get_attribute<message_source>().cast<message_source>();

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

#include "sekhmet/detail/zstd_util.hpp"

TEST(utility_tests, zstd_test)
{
	constexpr char src_data[] =
		"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aliquam ullamcorper velit leo, a vehicula sapien "
		"vehicula sed. Maecenas volutpat, velit vitae tincidunt maximus, turpis enim dignissim ligula, a auctor urna "
		"turpis ac tellus. Sed in sem felis. Duis accumsan lorem quis lacus vehicula, vel iaculis dolor luctus. Nulla "
		"at ante massa. Donec laoreet dapibus enim quis consectetur. Donec feugiat eros nulla, id condimentum risus "
		"facilisis nec.\n"
		"Proin a tincidunt erat. Pellentesque posuere, tellus nec consequat malesuada, ligula eros fringilla ante, et "
		"efficitur purus lorem quis justo. Aenean efficitur eu ipsum quis suscipit. Phasellus libero purus, tristique "
		"ut semper non, blandit non risus. Curabitur tincidunt non tortor eu pretium. Donec et lacinia massa. Quisque "
		"vulputate efficitur sagittis.\n"
		"Suspendisse mauris eros, ullamcorper vitae gravida ut, hendrerit ut odio. Integer non faucibus mauris. "
		"Maecenas in aliquam massa. Cras a iaculis risus. Fusce nec quam condimentum dui tincidunt finibus. Fusce "
		"faucibus, erat vitae scelerisque auctor, sem nisi faucibus magna, vel pellentesque eros urna nec est. Nunc "
		"nec justo ac nunc eleifend volutpat vitae in velit. Aenean accumsan ac tortor et molestie. Vestibulum ante "
		"turpis, ultricies ac interdum ut, accumsan nec metus. Morbi et urna luctus, congue quam porttitor, porttitor "
		"urna. Vestibulum convallis posuere neque, sit amet iaculis eros scelerisque ac.\n"
		"Nullam ac nulla hendrerit, pharetra eros eu, suscipit nisl. Proin aliquam nisl sit amet enim tristique "
		"malesuada. Proin dictum aliquam leo, et interdum augue lobortis viverra. Interdum et malesuada fames ac ante "
		"ipsum primis in faucibus. Etiam sodales ipsum vel turpis fermentum tristique. Pellentesque elementum nulla "
		"non magna consectetur ullamcorper. Donec a velit efficitur, sodales tellus vel, condimentum lorem. Sed "
		"pretium lacus non massa efficitur, vitae egestas elit ullamcorper. Sed imperdiet nisi suscipit erat lobortis "
		"venenatis. Maecenas erat arcu, consectetur in velit id, tincidunt suscipit leo. Donec sit amet risus nec "
		"ipsum venenatis venenatis. Ut lorem nunc, ultrices et sem sit amet, egestas fringilla sapien.\n"
		"Interdum et malesuada fames ac ante ipsum primis in faucibus. Morbi ultricies blandit condimentum. Ut porta "
		"condimentum eros a tincidunt. Proin fringilla sollicitudin tortor vitae lobortis. Aliquam ornare dictum nisl "
		"non hendrerit. Mauris in mattis felis. Nunc sit amet urna eget magna cursus varius ac eu magna. Sed egestas, "
		"nisi non cursus auctor, dolor sapien tempor ipsum, in ullamcorper orci mi varius odio. Maecenas at elit "
		"vulputate, laoreet purus aliquam, placerat tortor. Mauris nec nisl sit amet ligula ultrices porttitor ac quis "
		"neque. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas.\n"
		"Nulla facilisi. Vivamus in urna non diam faucibus mollis ut in lectus. Suspendisse id bibendum enim, vel "
		"tempor sem. Integer volutpat purus ac ante elementum ornare. Suspendisse consectetur ante magna, vitae congue "
		"enim fringilla ac. In vitae diam sit amet neque pulvinar ullamcorper. Morbi ultrices mi turpis, eget mattis "
		"ante maximus sed. Suspendisse pellentesque nibh tellus, eu lobortis urna ullamcorper eu.\n"
		"Pellentesque laoreet lacinia felis laoreet vestibulum. Sed enim arcu, posuere quis accumsan quis, auctor ac "
		"lorem. Phasellus malesuada vestibulum lorem, porta aliquet urna pellentesque semper. Sed sit amet interdum "
		"mi, et fringilla neque. Vivamus dolor sapien, ornare non lobortis at, pharetra sit amet elit. Proin egestas "
		"diam nec libero varius consequat. Phasellus pharetra sem nisi, quis vestibulum libero eleifend in. Cras "
		"auctor ligula in augue lacinia, nec interdum nibh ultrices. Sed posuere sagittis libero, imperdiet porta nisl "
		"imperdiet quis. Morbi quis efficitur dolor. Donec lectus mauris, mollis nec sem ac, imperdiet blandit lectus. "
		"Mauris vestibulum arcu leo, at pellentesque nulla tincidunt ut. Cras dignissim tempor aliquet.\n"
		"In est ante, laoreet consectetur diam non, iaculis aliquam tortor. Ut aliquam sem a sapien pharetra "
		"vestibulum. Sed sed auctor ex. Pellentesque ut nibh erat. Morbi feugiat rhoncus ultricies. Praesent molestie "
		"ac turpis sollicitudin auctor. Phasellus scelerisque, urna nec laoreet ultricies, odio ligula aliquam urna, "
		"ut consequat elit nulla sed nibh. In eleifend dignissim risus a iaculis. Proin sit amet velit eu purus "
		"suscipit dictum. Proin pulvinar magna vel erat gravida, vitae venenatis purus varius. Sed egestas purus sem, "
		"sed molestie mi sollicitudin a. Etiam purus nunc, pulvinar vitae sapien sollicitudin, scelerisque "
		"pellentesque felis.\n"
		"In mollis metus a tortor aliquet, ut lobortis sapien venenatis. Integer ultrices congue nunc, nec venenatis "
		"nisi mattis in. Aliquam rutrum tellus enim, sodales commodo enim luctus nec. Vivamus pharetra vel tellus "
		"cursus blandit. Suspendisse eu lobortis odio, blandit luctus risus. Proin suscipit id dui eu laoreet. Aliquam "
		"placerat metus sit amet elementum imperdiet.\n"
		"Nam ac commodo ante. Vivamus viverra porttitor mi in gravida. Sed scelerisque ante eu velit viverra, "
		"porttitor ultrices odio iaculis. Donec laoreet mauris tellus, vitae auctor tellus tincidunt non. Etiam vitae "
		"felis id dolor eleifend ornare. Vivamus eleifend, nulla nec egestas interdum, justo neque placerat arcu, "
		"dapibus fermentum neque magna a odio. Pellentesque lobortis et tellus eleifend tristique. Pellentesque "
		"egestas turpis sed libero scelerisque consectetur. Curabitur leo ante, ullamcorper malesuada laoreet et, "
		"volutpat eu urna. Donec vel diam congue, sodales lectus ut, mollis justo. Praesent enim tortor, dapibus vitae "
		"metus at, laoreet convallis ipsum. Curabitur pellentesque sit amet magna vel fermentum. Donec ultrices est "
		"nec elit commodo eleifend. Cras dapibus eget sapien a commodo. Sed nisi risus, sagittis quis convallis vel, "
		"laoreet sed nulla. Sed id orci convallis, ullamcorper metus quis, finibus massa.\n"
		"Nam at lacus in odio venenatis dignissim at a enim. Nulla felis massa, congue nec hendrerit et, faucibus sit "
		"amet leo. Vestibulum bibendum leo massa, rhoncus placerat ligula lacinia vitae. Aliquam nec enim quis odio "
		"interdum volutpat et in sapien. Aenean vitae consectetur neque. Nullam at massa in turpis efficitur placerat. "
		"Aenean in scelerisque nibh. Maecenas faucibus mollis nibh a aliquam. Vivamus faucibus vestibulum ultrices. "
		"Vestibulum ullamcorper semper elit, at iaculis lacus volutpat sed. Vivamus porttitor tortor eget ultrices "
		"placerat.\n"
		"Nulla et tempor nibh, in porta ligula. Suspendisse tincidunt leo at quam laoreet fringilla. Praesent vitae "
		"convallis lorem, ac rhoncus dui. Phasellus vel semper tortor, nec scelerisque metus. Nam nec ex ipsum. Duis "
		"convallis justo eu sapien feugiat pellentesque. Maecenas tincidunt sapien lorem, eu tempus velit tristique "
		"ut. Nulla molestie urna sed diam dignissim, a pretium sem accumsan.\n"
		"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin fermentum ligula libero, sed dapibus mauris "
		"tempus consequat. Nam ultricies fringilla nisl ac consequat. Ut consequat at magna sed eleifend. Proin eget "
		"laoreet diam. Praesent consectetur, diam id varius pharetra, lorem augue fermentum metus, vel congue nisi "
		"urna et nisl. Quisque ante libero, venenatis nec efficitur vel, porttitor non mauris. Cras laoreet euismod "
		"dolor et rutrum. Donec eu ante sed velit consectetur vehicula.\n"
		"Sed a velit sollicitudin, ultricies enim eu, hendrerit mauris. Mauris vitae consectetur tellus, in accumsan "
		"enim. Duis dui lorem, viverra at pulvinar scelerisque, condimentum id magna. Donec vulputate dignissim odio "
		"tincidunt varius. Mauris dictum id dui ac aliquet. Maecenas sed sapien sit amet ex elementum lobortis. Proin "
		"ac mauris risus. Aenean tincidunt, velit vitae hendrerit condimentum, metus dolor placerat massa, eu mattis "
		"metus quam ac velit. Suspendisse mollis enim enim, in dignissim mi ultrices nec. Phasellus ipsum diam, "
		"ullamcorper non semper pharetra, sollicitudin eget nunc. Morbi diam velit, rutrum et posuere eu, aliquam sit "
		"amet mi. Donec elementum commodo porta.\n"
		"Morbi euismod libero nibh, ut tempus purus dapibus euismod. Donec aliquam, dolor a viverra viverra, urna "
		"justo vestibulum ex, non elementum odio risus nec orci. Quisque posuere dapibus consequat. Donec feugiat "
		"consectetur risus vel efficitur. Donec et nunc tincidunt, aliquam dolor quis, egestas risus. Donec "
		"pellentesque massa diam, sit amet ornare nisi bibendum quis. Vivamus in diam in odio pellentesque pulvinar. "
		"Quisque tincidunt ante quis eleifend semper. Duis magna urna, laoreet in eros id, posuere aliquam lorem. "
		"Morbi non ante accumsan neque sagittis placerat ultrices quis odio.\n"
		"Suspendisse imperdiet enim vitae nulla dapibus, ultricies congue metus posuere. Etiam maximus, neque in "
		"viverra finibus, arcu quam imperdiet erat, non pretium dolor massa nec neque. Maecenas a feugiat nulla. Sed "
		"fringilla vitae arcu non feugiat. Vestibulum elementum interdum ante in egestas. Donec ultricies est erat. "
		"Praesent lobortis commodo mi tempus lobortis. Nullam id sem felis. Quisque rhoncus eros sed dolor fringilla "
		"viverra. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia curae; Aliquam "
		"congue eget ipsum ut posuere. Sed elit sem, posuere ut nunc sit amet, convallis venenatis quam. Nulla "
		"convallis lectus vel massa tincidunt, in finibus lorem ultricies. Aenean sed enim in lectus iaculis tincidunt "
		"et sit amet ante. Ut fermentum, orci vel mollis bibendum, risus dolor tincidunt quam, ac interdum dui justo "
		"eget lectus.\n"
		"Cras tristique ut magna sit amet interdum. Curabitur tincidunt auctor nulla, id eleifend metus congue vitae. "
		"Etiam semper enim id massa tincidunt ullamcorper. Fusce vitae facilisis leo. Sed erat libero, rutrum ac "
		"pretium quis, efficitur non nibh. Fusce enim quam, facilisis sed commodo at, congue a felis. Sed ipsum quam, "
		"facilisis a congue non, aliquam sit amet ipsum. Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
		"Nulla elementum id orci eget aliquet. Donec erat justo, auctor quis gravida tincidunt, finibus a turpis.\n"
		"Duis non justo dapibus, scelerisque augue at, lacinia libero. Fusce aliquet quam mi, volutpat volutpat enim "
		"porttitor quis. Donec eu posuere elit, ac hendrerit metus. Proin aliquam bibendum augue eu congue. Donec "
		"ornare, erat sed bibendum volutpat, elit sem blandit nulla, et elementum enim ex a justo. Nunc at tincidunt "
		"ex. Ut viverra diam gravida justo egestas volutpat. Etiam id orci eros. Nulla eu nibh ut orci mattis varius. "
		"Suspendisse nulla tortor, imperdiet in libero non, rutrum ultrices nisi. Vestibulum blandit quis leo non "
		"lobortis. Sed ut fringilla quam. Curabitur fringilla eleifend nisi, in feugiat mauris aliquet sit amet.\n"
		"Nunc vel dolor vel mauris imperdiet eleifend. Vivamus nec laoreet metus. Aliquam ac aliquam felis, ut rutrum "
		"quam. Maecenas venenatis tellus vel porttitor tempus. Aliquam tincidunt tincidunt nunc, nec imperdiet urna "
		"lobortis a. Curabitur at sagittis tortor. Curabitur mollis sodales porttitor. Etiam in lacinia est, eget "
		"semper massa. Aliquam erat volutpat. Nulla dapibus turpis eget leo rhoncus laoreet.\n"
		"Vivamus ligula ligula, consectetur non fermentum sed, tristique a ligula. Ut sollicitudin erat id nisi "
		"vestibulum mollis. Phasellus quis commodo lectus. Donec eu facilisis purus. Sed eget enim nulla. Pellentesque "
		"eget risus ex. Morbi sem lorem, congue sit amet est id, venenatis feugiat ipsum. Quisque nec mauris mi. "
		"Praesent sed massa turpis. Mauris cursus risus at tristique tincidunt. Nulla erat nisi, faucibus a gravida "
		"et, tempor a sapien. Nunc facilisis lacinia scelerisque. Pellentesque sit amet lacus pharetra, tristique "
		"augue at, sodales arcu. In consequat libero sed tristique gravida. Aliquam quis porta leo, porttitor ultrices "
		"enim. Praesent et odio fringilla, varius lacus a, luctus urna.\n"
		"Duis sit amet turpis ac lacus molestie cursus. Duis quam erat, porttitor vitae scelerisque non, ultrices "
		"vitae sem. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia curae; Nunc id "
		"luctus augue. Maecenas mollis suscipit odio ut maximus. Duis auctor commodo nisl nec venenatis. Aenean luctus "
		"scelerisque mi, non posuere turpis tempus in. Aenean in nulla quam. Phasellus egestas ultrices diam quis "
		"venenatis. Duis ac faucibus lorem. Nullam hendrerit erat eget sem posuere, non molestie lectus auctor. Ut "
		"posuere, orci et accumsan scelerisque, sapien magna pretium nisi, quis consectetur lorem felis eu elit.\n"
		"Fusce vehicula lobortis viverra. Donec id nisl neque. Morbi id suscipit tellus. Morbi mollis nisi a pretium "
		"pharetra. Nunc blandit bibendum pharetra. Proin aliquet mi eget ipsum rutrum iaculis. Aenean vitae ligula "
		"fermentum, tristique ipsum in, dapibus nisi. Quisque odio lectus, tristique eget ligula ac, aliquet feugiat "
		"magna. Vestibulum dictum eget justo in gravida. Suspendisse nibh enim, aliquet commodo hendrerit nec, "
		"porttitor non odio. Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. In "
		"ullamcorper sapien est, eget auctor metus convallis ut.\n"
		"Praesent non efficitur arcu. Mauris quis interdum est. Morbi vestibulum, risus sed pellentesque dignissim, "
		"nunc metus semper dolor, nec tristique odio ex sit amet est. Donec ac libero dui. Donec dictum nulla eu leo "
		"faucibus pellentesque. Etiam at semper velit, ut tempor augue. Fusce fermentum ante eget lectus aliquam, sit "
		"amet imperdiet magna elementum. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per "
		"inceptos himenaeos. Aliquam erat volutpat. Nam tellus dui, sodales id hendrerit id, aliquam sed lorem.\n"
		"Aenean efficitur vitae mi sit amet dignissim. Donec id nisi nulla. Aenean pharetra risus vitae tellus posuere "
		"consectetur. Curabitur sit amet efficitur turpis, id imperdiet arcu. Vestibulum in tellus ipsum. Nam sodales "
		"libero vitae porttitor blandit. Quisque non mollis tortor. Nullam varius justo a auctor efficitur.\n"
		"In blandit pharetra porttitor. Mauris egestas, dolor quis scelerisque suscipit, arcu sapien consequat ante, "
		"nec ornare ipsum lectus sed massa. Etiam quis malesuada lacus, at molestie velit. Nulla eget augue neque. Sed "
		"vel purus ipsum. Interdum et malesuada fames ac ante ipsum primis in faucibus. Nullam nec interdum elit.\n"
		"Mauris feugiat condimentum nibh, vitae euismod tellus posuere eu. Vivamus imperdiet turpis fringilla, varius "
		"justo ut, accumsan orci. Integer porta urna sapien, at fermentum erat molestie in. Quisque suscipit molestie "
		"mi ac tristique. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. "
		"Integer tincidunt risus in ante placerat venenatis. Vestibulum rutrum blandit magna dictum lobortis. Praesent "
		"enim ligula, commodo sed nunc non, blandit varius mi. Ut eu ipsum ac velit rhoncus maximus ac vel leo. Fusce "
		"tincidunt lobortis metus, eget laoreet justo sollicitudin et. Integer eget mauris sed odio tincidunt "
		"consequat vitae non risus. Aliquam tincidunt facilisis tellus, ut suscipit massa efficitur eget. Vestibulum "
		"sit amet elit dui.\n"
		"Integer consequat, ipsum in rhoncus aliquet, libero ante consectetur mi, nec gravida lectus justo a mauris. "
		"Aliquam scelerisque elit magna, ac faucibus magna cursus sit amet. Duis ullamcorper erat semper placerat "
		"egestas. Donec ultricies viverra vestibulum. Phasellus at gravida justo. Suspendisse potenti. Ut ultrices id "
		"est eget vestibulum. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos "
		"himenaeos. Sed laoreet lectus tellus, id placerat lorem lobortis eu. Etiam elementum ex vitae erat mollis, in "
		"consequat eros consectetur. Cras ultrices magna eu eros viverra viverra quis nec odio.\n"
		"Sed rutrum risus a justo tempus, et lacinia augue porttitor. Nulla et mollis quam. Nunc vehicula convallis "
		"sapien. In euismod ultrices magna, a ullamcorper purus tincidunt vitae. Aenean in sem enim. In id velit mi. "
		"Nam eu urna et turpis venenatis eleifend. Donec varius, ex id mollis rhoncus, leo mauris dictum ante, vitae "
		"efficitur tellus velit non ipsum. Praesent hendrerit mauris nec sem accumsan, et finibus velit placerat. "
		"Curabitur id enim luctus, vehicula leo vel, vehicula magna. Aliquam facilisis ornare enim, id gravida ipsum "
		"tempor eget. Ut pharetra ut nisl a maximus. Pellentesque bibendum malesuada leo. Aliquam rutrum vulputate "
		"odio, in cursus ligula consequat id. Mauris a lorem eu ipsum convallis facilisis. Duis feugiat est at "
		"suscipit ultricies.\n"
		"Integer commodo turpis vel velit tristique tempus a nec quam. Donec venenatis lacus consectetur libero "
		"congue, sit amet est.";
	constexpr auto src_size = SEK_ARRAY_SIZE(src_data);

	auto &ctx = sek::zstd_thread_ctx::instance();
	auto zstd_pool = sek::thread_pool{2};
	sek::dynarray<std::byte> compressed;
	sek::dynarray<char> decompressed;

	auto c_writer = sek::delegate{+[](sek::dynarray<std::byte> &dst, const void *src, std::size_t n) -> std::size_t
								  {
									  auto bytes = static_cast<const std::byte *>(src);
									  dst.insert(dst.end(), bytes, bytes + n);
									  return n;
								  },
								  compressed};
	auto d_writer = sek::delegate{+[](sek::dynarray<char> &dst, const void *src, std::size_t n) -> std::size_t
								  {
									  auto bytes = static_cast<const char *>(src);
									  dst.insert(dst.end(), bytes, bytes + n);
									  return n;
								  },
								  decompressed};

	{
		auto c_reader = sek::zstd_thread_ctx::buffer_reader{static_cast<const void *>(src_data), src_size};

		EXPECT_NO_THROW(ctx.compress(zstd_pool, c_reader, c_writer, 0));
		EXPECT_LE(compressed.size(), src_size);

		auto d_reader = sek::zstd_thread_ctx::buffer_reader{static_cast<const void *>(compressed.data()), compressed.size()};

		EXPECT_NO_THROW(ctx.decompress(zstd_pool, d_reader, d_writer));
		EXPECT_TRUE(std::ranges::equal(decompressed, src_data));
	}

	const auto l0_size = compressed.size();
	compressed.clear();
	decompressed.clear();

	{
		auto c_reader = sek::zstd_thread_ctx::buffer_reader{static_cast<const void *>(src_data), src_size};

		EXPECT_NO_THROW(ctx.compress(zstd_pool, c_reader, c_writer, 10));
		EXPECT_LE(compressed.size(), src_size);
		EXPECT_LE(compressed.size(), l0_size);

		auto d_reader = sek::zstd_thread_ctx::buffer_reader{static_cast<const void *>(compressed.data()), compressed.size()};

		EXPECT_NO_THROW(ctx.decompress(zstd_pool, d_reader, d_writer));
		EXPECT_TRUE(std::ranges::equal(decompressed, src_data));
	}

	const auto l10_size = compressed.size();
	compressed.clear();
	decompressed.clear();

	{
		auto c_reader = sek::zstd_thread_ctx::buffer_reader{static_cast<const void *>(src_data), src_size};

		EXPECT_NO_THROW(ctx.compress(zstd_pool, c_reader, c_writer, 20));
		EXPECT_LE(compressed.size(), src_size);
		EXPECT_LE(compressed.size(), l10_size);
		EXPECT_LE(compressed.size(), l0_size);

		auto d_reader = sek::zstd_thread_ctx::buffer_reader{static_cast<const void *>(compressed.data()), compressed.size()};

		EXPECT_NO_THROW(ctx.decompress(zstd_pool, d_reader, d_writer));
		EXPECT_TRUE(std::ranges::equal(decompressed, src_data));
	}
}