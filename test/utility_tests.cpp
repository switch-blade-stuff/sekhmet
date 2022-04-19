//
// Created by switchblade on 2022-03-03.
//

#include <gtest/gtest.h>

#include <string>

#include "sekhmet/hash.hpp"
#include "sekhmet/utility.hpp"

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

	adapter = adapter_long;
	EXPECT_FALSE(adapter.empty());
	EXPECT_EQ(adapter.invoke<size_proxy>(), adapter_long.invoke<size_proxy>());
	EXPECT_EQ(adapter.invoke<size_proxy>(), sizeof(long));

	adapter.reset();
	EXPECT_TRUE(adapter.empty());

	const size_get s = {sizeof(void *)};
	adapter.rebind(s);

	EXPECT_FALSE(adapter.empty());
	EXPECT_EQ(adapter.invoke<size_proxy>(), s.size());
}