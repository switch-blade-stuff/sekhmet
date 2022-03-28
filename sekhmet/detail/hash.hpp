//
// Created by switchblade on 2021-11-09.
//

#pragma once

#include <cstddef>
#include <cstdint>

#include "../math/detail/util.hpp"
#include "meta_util.hpp"

namespace sek::detail
{
	typedef std::size_t hash_t;

	[[nodiscard]] constexpr std::size_t sdbm(const uint8_t *data, std::size_t len, uint32_t seed)
	{
		std::size_t result = seed;
		for (; len > 0; --len, ++data) result = *data + (result << 6) + (result << 16) - result;
		return result;
	}

#if INTPTR_MAX < INT64_MAX
	constexpr const hash_t fnv1a_prime = 0x01000193;
	constexpr const hash_t fnv1a_offset = 0x811c9dc5;
#else
	constexpr const hash_t fnv1a_prime = 0x00000100000001b3;
	constexpr const hash_t fnv1a_offset = 0xcbf29ce484222325;
#endif
	template<std::size_t Size, std::size_t Byte = 1>
	[[nodiscard]] constexpr hash_t fnv1a_iteration(std::size_t value, hash_t result) noexcept
	{
		/* Iterating by std::size_t value in order to allow for compile-time evaluation. */
		if constexpr (Byte <= Size)
		{
			result ^= static_cast<std::uint8_t>(value >> (8 * (Size - Byte)));
			result *= fnv1a_prime;
			return fnv1a_iteration<Size, Byte + 1>(value, result);
		}
		else
			return result;
	}
	template<typename T>
	[[nodiscard]] constexpr hash_t fnv1a(const T *data, std::size_t len, hash_t seed = fnv1a_offset)
	{
		hash_t result = seed;
		while (len--) { result = fnv1a_iteration<sizeof(T)>(static_cast<std::size_t>(data[len]), result); }
		return result;
	}

	[[nodiscard]] constexpr hash_t byte_hash(const void *data, std::size_t len, hash_t seed = fnv1a_offset) noexcept
	{
		return fnv1a(static_cast<const uint8_t *>(data), len, seed);
	}

	template<math::arithmetic T>
	[[nodiscard]] constexpr hash_t hash(T value) noexcept
	{
		return fnv1a(&value, 1);
	}

	template<typename T>
	[[nodiscard]] constexpr hash_t hash(T *value) noexcept
	{
		return byte_hash(&value, sizeof(value));
	}
	template<pointer_like T>
	[[nodiscard]] constexpr hash_t hash(T value) noexcept
	{
		return hash(std::to_address(value));
	}
	[[nodiscard]] constexpr hash_t hash(std::nullptr_t) noexcept { return hash(0); }

	template<typename T>
	concept has_hash = requires(T t)
	{
		hash(t);
	};

	/** Combines hash of the value type with the seed.
	 * @param seed Seed to combine the hash with.
	 * @param value Value to hash.
	 * @return Copy of seed.
	 * @note Value type must have a hash function defined for it. The function is looked up via ADL. */
	template<has_hash T>
	constexpr hash_t hash_combine(hash_t &seed, const T &value) noexcept
	{
		return seed = (seed ^ (hash(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2)));
	}

	template<std::ranges::forward_range R>
	[[nodiscard]] constexpr hash_t hash(const R &r) noexcept requires has_hash<std::ranges::range_value_t<R>>
	{
		hash_t result = {};
		for (const auto &value : r) hash_combine(result, value);
		return result;
	}

	template<std::size_t I, has_hash... Ts>
	constexpr void tuple_hash_unwrap(hash_t &result, const std::tuple<Ts...> &t) noexcept
	{
		hash_combine(result, std::get<I>(t));
		if constexpr (I + 1 < sizeof...(Ts)) unwrap<I + 1>(result, t);
	}
	template<has_hash... Ts>
	[[nodiscard]] constexpr hash_t hash(const std::tuple<Ts...> &t) noexcept
	{
		hash_t result = 0;
		unwrap<0>(result, type_seq<Ts...>, t);
		return result;
	}
	template<has_hash T1, has_hash T2>
	[[nodiscard]] constexpr hash_t hash(const std::pair<T1, T2> &p) noexcept
	{
		auto result = hash(p.first);
		return hash_combine(result, hash(p.second));
	}

	/** @brief Hasher that calls `hash` function on the passed object via ADL.
	 * If such function is not available, uses `std::hash`. */
	struct default_hash
	{
		template<math::arithmetic T>
		[[nodiscard]] constexpr hash_t operator()(T value) const noexcept
		{
			return hash(value);
		}
		template<typename T>
		[[nodiscard]] constexpr hash_t operator()(const T &value) const noexcept
		{
			if constexpr (has_hash<T>)
				return hash(value);
			else
				return std::hash<T>{}(value);
		}
	};
}	 // namespace sek::detail
