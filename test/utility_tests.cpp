//
// Created by switchblade on 2022-03-03.
//

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
		"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vestibulum sodales lobortis ante, sollicitudin "
		"fringilla neque mollis et. Donec hendrerit dolor quam, consequat tincidunt tellus ultrices a. Integer "
		"interdum eros sed sollicitudin bibendum. Cras at nulla ac neque aliquet viverra. Cras mollis non justo ut "
		"vehicula. Sed id metus sagittis, tincidunt nisl a, condimentum enim. Pellentesque pulvinar nunc eget elit "
		"feugiat mollis in in lectus. Ut vel enim varius, sollicitudin orci rutrum, ultrices enim. Nunc in posuere "
		"quam, a tempus lorem. Ut tincidunt tempus pulvinar. Sed vitae quam magna. In rhoncus tempor imperdiet. Nulla "
		"non ipsum sit amet metus congue egestas a vel risus. Suspendisse cursus erat vel est venenatis sodales.\n"
		"Curabitur mauris nisl, dictum rutrum justo eget, congue auctor nisi. Phasellus quis arcu lobortis, laoreet "
		"erat sed, laoreet lacus. Maecenas quis ex turpis. Quisque diam lorem, ultricies eget placerat eu, suscipit "
		"non lacus. Suspendisse potenti. Cras ac arcu rutrum, bibendum nibh a, blandit felis. Integer interdum a sem "
		"eu maximus. Nunc porttitor faucibus nibh quis sodales.\n"
		"Maecenas malesuada mollis odio ac venenatis. Nulla tincidunt suscipit quam at efficitur. Etiam faucibus, "
		"sapien sed accumsan vulputate, magna mi viverra lorem, nec aliquet ante justo et justo. Vivamus euismod ipsum "
		"vitae risus egestas, quis vestibulum turpis tristique. In vitae ex nibh. Maecenas tincidunt nunc nec semper "
		"semper. Aliquam erat volutpat. Praesent quis dolor vehicula, bibendum lorem ut, tempus diam. Nunc in commodo "
		"felis.\n"
		"Suspendisse malesuada, velit sit amet tempor semper, nulla sapien pharetra eros, nec blandit ex lorem non "
		"ante. Sed tempor aliquet neque at ultricies. Vestibulum tellus leo, porta non lacus a, eleifend iaculis "
		"felis. Nam lobortis lacinia vulputate. Vivamus eu ultricies augue. Nullam ut urna erat. Maecenas dictum "
		"lobortis velit, at vulputate turpis porttitor at. Aenean auctor ante ac sapien scelerisque sagittis. Cras "
		"mollis ullamcorper lectus nec tempus.\n"
		"Nullam aliquam augue leo. Vestibulum eu ligula vitae elit varius varius. Curabitur orci mauris, semper vel "
		"orci eu, malesuada auctor arcu. Mauris gravida scelerisque lacus, non condimentum nulla molestie non. Nullam "
		"pretium turpis orci, mollis pellentesque purus dignissim vel. Pellentesque iaculis consectetur nunc, non "
		"lacinia diam luctus eget. Nulla lacinia nisl tortor, ac tempus ex aliquet quis.\n"
		"Vivamus pellentesque ligula vitae sollicitudin aliquet. Ut ultrices sapien sed felis rhoncus, vitae consequat "
		"eros volutpat. Nunc sollicitudin vel ex vel porttitor. Nunc ullamcorper, leo non maximus pellentesque, dolor "
		"metus dictum quam, at vulputate nibh felis non ex. Ut sollicitudin fermentum turpis, at fringilla nulla "
		"lacinia sit amet. Etiam rhoncus ante sem. Pellentesque molestie lacus sem, vel pharetra lorem pellentesque "
		"quis. Fusce semper commodo elit sed rutrum. Vestibulum vestibulum tellus a risus consectetur ultricies.\n"
		"Mauris dictum at nibh et pellentesque. Proin suscipit odio lacus, at consectetur tortor blandit a. Cras odio "
		"diam, bibendum ac volutpat non, feugiat eu urna. Aenean sed interdum metus, ut dapibus lectus. Maecenas "
		"pharetra sit amet tellus a pretium. Etiam iaculis posuere est, mollis hendrerit ligula ullamcorper ut. Nullam "
		"nec dui nec sapien posuere lobortis. Praesent fringilla ligula vitae neque iaculis porttitor. Nunc maximus "
		"blandit magna et luctus. Aliquam turpis ligula, posuere quis maximus sed, ornare ac sapien. Vivamus tempus "
		"velit lectus, sit amet tincidunt velit vulputate a.\n"
		"Sed rhoncus pellentesque gravida. Mauris nec dolor est. Praesent lobortis consequat dolor sed hendrerit. "
		"Donec diam risus, condimentum sit amet mollis a, ullamcorper nec odio. Nulla sit amet finibus mi. Integer "
		"dictum ac felis eget ultrices. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Suspendisse non quam "
		"eros. In hac habitasse platea dictumst. Proin ut elementum eros, vitae pretium tellus.\n"
		"Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Vivamus hendrerit, "
		"lorem in dapibus sodales, elit augue ullamcorper enim, eget consequat libero dui iaculis tellus. Cras varius "
		"metus ullamcorper quam ullamcorper cursus. Nulla pretium tempor ultricies. Nam ultrices volutpat justo, sit "
		"amet consectetur ante dictum id. Vestibulum quis est sed lorem posuere condimentum quis vitae dolor. Aliquam "
		"erat volutpat. Aliquam ultricies eu sem a dignissim. Nunc aliquam, sapien vel imperdiet tempor, dui leo "
		"pulvinar neque, id mollis tellus quam quis justo. Morbi feugiat nisi at lacus sodales pellentesque. Integer "
		"dui nulla, viverra ultricies vulputate nec, semper et arcu. Aenean nibh nibh, sodales eget ultrices eget, "
		"mattis sed elit.\n"
		"Nam cursus dictum lacus. Mauris non dui eros. Curabitur cursus sed tellus id condimentum. Suspendisse at nibh "
		"felis. Etiam vitae lectus aliquet, placerat sem eget, scelerisque elit. Morbi iaculis nulla eleifend risus "
		"vulputate sodales. Curabitur et orci erat. Curabitur tristique ante non mi tempor ullamcorper.\n"
		"Sed vel eros suscipit, maximus augue non, imperdiet massa. Phasellus ut efficitur nunc, quis elementum nisl. "
		"Nulla facilisi. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Duis efficitur, ex id luctus mollis, "
		"augue tortor aliquet quam, convallis convallis libero urna nec orci. Etiam finibus scelerisque ullamcorper. "
		"Sed facilisis orci et metus rhoncus, in condimentum nisi rhoncus. Proin eu euismod ex.\n"
		"Donec efficitur felis ipsum, ac hendrerit diam laoreet eu. Morbi quis augue eu nibh mollis tincidunt. Donec "
		"ullamcorper augue id gravida sodales. Sed sed sollicitudin elit. Donec a blandit felis. Integer quis "
		"condimentum libero. Suspendisse lobortis diam sed velit porttitor, at imperdiet eros vulputate. Curabitur "
		"venenatis laoreet leo, et feugiat turpis hendrerit eu. Proin egestas pellentesque purus sed tempor. Mauris "
		"pellentesque sem nibh, ut fermentum felis congue sit amet. Curabitur eget ultricies lacus. Etiam consequat "
		"auctor ante sit amet mollis. Ut tristique interdum urna et euismod. Pellentesque vitae neque vitae lacus "
		"volutpat egestas. Nullam laoreet, mauris ac eleifend mollis, eros ante pharetra orci, vel laoreet odio sapien "
		"ut velit. Sed nec arcu dapibus, vehicula ante a, maximus sapien.\n"
		"Donec euismod vitae enim sit amet imperdiet. Duis nec tempus justo. Morbi ultricies dolor mauris. Suspendisse "
		"condimentum vestibulum ex. Aliquam sit amet turpis odio. Ut nisi nibh, pulvinar ut orci a, mollis mattis "
		"nibh. Phasellus vel auctor sem. Proin quis placerat est, sed pretium tellus. Nulla cursus turpis elementum "
		"semper finibus. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia curae; "
		"Pellentesque consectetur massa urna, sed consequat risus blandit sed. Donec varius ante et turpis hendrerit "
		"mattis. Ut laoreet, ante non volutpat dapibus, nulla turpis congue mauris, at auctor nunc velit ut erat. "
		"Proin dolor felis, porttitor eu lectus et, sagittis lobortis enim.\n"
		"Proin fermentum erat ut erat aliquet euismod. Cras blandit magna in pulvinar ullamcorper. Phasellus congue "
		"tincidunt arcu quis mattis. Nunc maximus facilisis vestibulum. Quisque bibendum diam a erat blandit mollis. "
		"Morbi luctus magna convallis, fringilla turpis vel, laoreet risus. Donec ut tincidunt est, sit amet blandit "
		"turpis. Etiam gravida tincidunt tempor. Quisque ultricies euismod rutrum.\n"
		"Cras et felis egestas, volutpat quam sed, aliquet justo. Proin rhoncus sem at velit sollicitudin, sed "
		"venenatis libero pellentesque. Ut ac hendrerit ante. Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
		"Curabitur malesuada luctus ultricies. Vestibulum dapibus nunc nec arcu lobortis scelerisque. Pellentesque "
		"vestibulum ultrices dui, non viverra orci tincidunt nec. Vestibulum ac orci ullamcorper, finibus felis et, "
		"ultrices diam. Sed vestibulum est eu ex fringilla, fermentum rhoncus nibh facilisis. Suspendisse in elit ac "
		"libero accumsan tincidunt. Quisque iaculis enim non metus volutpat, et gravida ligula pharetra. Sed "
		"condimentum lacus id purus suscipit iaculis. Vestibulum a lorem id magna tempus molestie.\n"
		"Duis cursus mollis velit, ac pretium felis euismod eget. Proin luctus nisl quis purus efficitur ultricies. "
		"Suspendisse placerat quam sit.";
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