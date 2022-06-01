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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * ============================================================================
 *
 * Created by switchblade on 2021-11-09
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <bit>

#include "meta_util.hpp"
#include "sekhmet/math/detail/util.hpp"

namespace sek
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

	namespace detail
	{
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
	}	 // namespace detail

	template<typename T>
	[[nodiscard]] constexpr hash_t fnv1a(const T *data, std::size_t len, hash_t seed = fnv1a_offset)
	{
		hash_t result = seed;
		while (len--) { result = detail::fnv1a_iteration<sizeof(T)>(static_cast<std::size_t>(data[len]), result); }
		return result;
	}
	[[nodiscard]] constexpr hash_t byte_hash(const void *data, std::size_t len, hash_t seed = fnv1a_offset) noexcept
	{
		return fnv1a(static_cast<const uint8_t *>(data), len, seed);
	}

	template<std::integral I>
	[[nodiscard]] constexpr hash_t hash(I value) noexcept
	{
		return static_cast<hash_t>(value);
	}
	template<typename E>
	[[nodiscard]] constexpr hash_t hash(E value) noexcept
		requires std::is_enum_v<E>
	{
		return hash(static_cast<std::underlying_type_t<E>>(value));
	}
	template<typename T>
	[[nodiscard]] constexpr hash_t hash(T *value) noexcept
	{
		return std::bit_cast<hash_t>(value);
	}
	template<pointer_like T>
	[[nodiscard]] constexpr hash_t hash(T value) noexcept
	{
		return hash(std::to_address(value));
	}
	[[nodiscard]] constexpr hash_t hash(std::nullptr_t) noexcept { return 0; }

	template<typename T>
	concept has_hash = requires(T t) { hash(t); };

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
	[[nodiscard]] constexpr hash_t hash(const R &r) noexcept
		requires has_hash<std::ranges::range_value_t<R>>
	{
		hash_t result = {};
		for (const auto &value : r) hash_combine(result, value);
		return result;
	}

	namespace detail
	{
		template<typename T, std::size_t... Is>
		constexpr bool hashable_tuple_element(std::index_sequence<Is...>)
		{
			return std::conjunction_v<std::bool_constant<has_hash<std::tuple_element_t<Is, T>>>...>;
		}
		template<typename T>
		concept tuple_like_hash = requires(const T &t) {
									  typename std::tuple_size<T>::type;
									  std::tuple_size_v<T> != 0;
									  hashable_tuple_element<T>(std::make_index_sequence<std::tuple_size_v<T>>());
								  };
		template<typename P>
		concept pair_like_hash = requires(const P &p) {
									 p.first;
									 p.second;
									 requires has_hash<std::decay_t<decltype(p.first)>>;
									 requires has_hash<std::decay_t<decltype(p.second)>>;
								 };
	}	 // namespace detail

	template<detail::tuple_like_hash T>
	[[nodiscard]] constexpr hash_t hash(const T &value) noexcept
	{
		hash_t result = hash(std::get<0>(value));
		if constexpr (std::tuple_size_v<T> != 1)
		{
			constexpr auto unwrap = []<std::size_t... Is>(hash_t & seed, const T &t, std::index_sequence<Is...>)
			{
				(hash_combine(seed, hash(std::get<Is + 1>(t))), ...);
			};
			unwrap(result, value, std::make_index_sequence<std::tuple_size_v<T> - 1>());
		}
		return result;
	}
	template<detail::pair_like_hash P>
	[[nodiscard]] constexpr hash_t hash(const P &p) noexcept
	{
		auto result = hash(p.first);
		return hash_combine(result, hash(p.second));
	}

	template<typename T>
	[[nodiscard]] constexpr hash_t hash(const T &v) noexcept
		requires(!std::integral<T> && !pointer_like<T> && !std::is_pointer_v<T> && !std::same_as<T, std::nullptr_t> &&
				 !std::ranges::forward_range<T> && requires { std::hash<T>{}(v); })
	{
		return std::hash<T>{}(v);
	}

	/** @brief Hasher that calls `hash` function on the passed object via ADL.
	 * If such function is not available, uses `std::hash`. */
	struct default_hash
	{
		template<typename T>
		[[nodiscard]] constexpr hash_t operator()(const T &value) const noexcept
		{
			if constexpr (has_hash<T>)
				return hash(value);
			else
				return std::hash<T>{}(value);
		}
	};
}	 // namespace sek
