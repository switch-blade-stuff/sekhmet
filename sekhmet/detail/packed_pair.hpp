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
 * Created by switchblade on 07/05/22
 */

#pragma once

#include <tuple>
#include <utility>

#include "ebo_base_helper.hpp"
#include "hash.hpp"

namespace sek
{
	namespace detail
	{
		template<typename T, std::size_t>
		struct packed_pair_base : ebo_base_helper<T>
		{
			using ebo_base = ebo_base_helper<T>;

			// clang-format off
			constexpr packed_pair_base() noexcept(std::is_nothrow_default_constructible_v<ebo_base>) = default;
			constexpr packed_pair_base(const packed_pair_base &) noexcept(std::is_nothrow_copy_constructible_v<ebo_base>) = default;
			constexpr packed_pair_base &operator=(const packed_pair_base &) noexcept(std::is_nothrow_copy_assignable_v<ebo_base>) = default;
			constexpr packed_pair_base(packed_pair_base &&) noexcept(std::is_nothrow_move_constructible_v<ebo_base>) = default;
			constexpr packed_pair_base &operator=(packed_pair_base &&) noexcept(std::is_nothrow_move_assignable_v<ebo_base>) = default;

			template<typename... Args>
			constexpr explicit packed_pair_base(Args &&...args)
				noexcept(std::is_nothrow_constructible_v<ebo_base, Args...>)
				requires std::is_constructible_v<ebo_base, Args...>
				: ebo_base(std::forward<Args>(args)...)
			{
			}
			// clang-format on

			constexpr void swap(packed_pair_base &other) noexcept(std::is_nothrow_swappable_v<ebo_base>)
			{
				ebo_base::swap(other);
			}

			using ebo_base::get;
		};
	}	 // namespace detail

	/** @brief Simple EBO-enabled pair. */
	template<typename T0, typename T1>
	class packed_pair : detail::packed_pair_base<T0, 0>, detail::packed_pair_base<T1, 1>
	{
	private:
		using base_first = detail::packed_pair_base<T0, 0>;
		using base_second = detail::packed_pair_base<T1, 1>;

		template<typename... Args>
		constexpr static bool first_nothrow_construct = std::is_nothrow_constructible_v<T0, Args...>;
		template<typename... Args>
		constexpr static bool second_nothrow_construct = std::is_nothrow_constructible_v<T1, Args...>;

		template<typename... Args0, std::size_t... Is0, typename... Args1, std::size_t... Is1>
		constexpr packed_pair(std::tuple<Args0...> &&args0,
							  std::index_sequence<Is0...>,
							  std::tuple<Args1...> &&args1,
							  std::index_sequence<Is1...>)
			: base_first(std::get<Is0>(std::forward<std::tuple<Args0...>>(args0))...),
			  base_second(std::get<Is1>(std::forward<std::tuple<Args1...>>(args1))...)
		{
		}

	public:
		// clang-format off
		constexpr packed_pair() noexcept(first_nothrow_construct<T0> && second_nothrow_construct<T1>)
			requires(std::is_default_constructible_v<T0> && std::is_default_constructible_v<T1>) = default;

		constexpr packed_pair(const packed_pair &) = default;
		constexpr packed_pair &operator=(const packed_pair &) = default;
		constexpr packed_pair(packed_pair &&) = default;
		constexpr packed_pair &operator=(packed_pair &&) noexcept(std::is_nothrow_move_assignable_v<T0> && std::is_nothrow_move_assignable_v<T1>) = default;
		constexpr ~packed_pair() = default;

		constexpr packed_pair(const T0 &f, const T1 &s)
			noexcept(first_nothrow_construct<const T0 &> && second_nothrow_construct<const T1 &>)
			requires std::is_copy_constructible_v<T0> && std::is_copy_constructible_v<T1> &&
					 std::is_convertible_v<const T0 &, T0> && std::is_convertible_v<const T1 &, T1>
			: base_first(f), base_second(s)
		{
		}
		constexpr explicit packed_pair(const T0 &f, const T1 &s)
			noexcept(first_nothrow_construct<const T0 &> && second_nothrow_construct<const T1 &>)
			requires(std::is_copy_constructible_v<T0> && std::is_copy_constructible_v<T1> &&
					 (!std::is_convertible_v<const T0 &, T0> || !std::is_convertible_v<const T1 &, T1>))
			: base_first(f), base_second(s)
		{
		}

		template<typename U0, typename U1>
		constexpr packed_pair(U0 &&x, U1 &&y)
			noexcept(first_nothrow_construct<U0> && second_nothrow_construct<U1>)
			requires std::is_constructible_v<T0, U0> && std::is_constructible_v<T1, U1> &&
					 std::is_convertible_v<U0, T0> && std::is_convertible_v<U1, T1>
			: base_first(std::forward<U0>(x)), base_second(std::forward<U1>(y))
		{
		}
		template<typename U0, typename U1>
		constexpr explicit packed_pair(U0 &&x, U1 &&y)
			noexcept(first_nothrow_construct<U0> && second_nothrow_construct<U1>)
			requires(std::is_constructible_v<T0, U0> && std::is_constructible_v<T1, U1> &&
					 (!std::is_convertible_v<U0, T0> || !std::is_convertible_v<U1, T1>))
			: base_first(std::forward<U0>(x)), base_second(std::forward<U1>(y))
		{
		}

		template<typename U0, typename U1>
		constexpr packed_pair(const packed_pair<U0, U1> &p)
			noexcept(first_nothrow_construct<const U0 &> && second_nothrow_construct<const U1 &>)
			requires std::is_constructible_v<T0, const U0 &> && std::is_constructible_v<T1, const U1 &> &&
					 std::is_convertible_v<const U0 &, T0> && std::is_convertible_v<const U1 &, T1>
			: base_first(p.first()), base_second(p.second())
		{
		}
		template<typename U0, typename U1>
		constexpr explicit packed_pair(const packed_pair<U0, U1> &p)
			noexcept(first_nothrow_construct<const U0 &> && second_nothrow_construct<const U1 &>)
			requires(std::is_constructible_v<T0, const U0 &> && std::is_constructible_v<T1, const U1 &> &&
					 (!std::is_convertible_v<const U0 &, T0> || !std::is_convertible_v<const U1 &, T1>))
			: base_first(p.first()), base_second(p.second())
		{
		}

		template<typename U0, typename U1>
		constexpr packed_pair(packed_pair<U0, U1> &&p)
			noexcept(first_nothrow_construct<U0> && second_nothrow_construct<U1>)
			requires std::is_constructible_v<T0, U0> && std::is_constructible_v<T1, U1> &&
					 std::is_convertible_v<U0, T0> && std::is_convertible_v<U1, T1>
			: base_first(std::move(p.first())), base_second(std::move(p.second()))
		{
		}
		template<typename U0, typename U1>
		constexpr explicit packed_pair(packed_pair<U0, U1> &&p)
			noexcept(first_nothrow_construct<U0> && second_nothrow_construct<U1>)
			requires(std::is_constructible_v<T0, U0> && std::is_constructible_v<T1, U1> &&
					 (!std::is_convertible_v<U0, T0> || !std::is_convertible_v<U1, T1>))
			: base_first(std::move(p.first())), base_second(std::move(p.second()))
		{
		}

		template<typename... Args0, typename... Args1>
		constexpr packed_pair(std::piecewise_construct_t,
							  std::tuple<Args0...> args0,
							  std::tuple<Args1...> args1)
			noexcept(first_nothrow_construct<Args0...> && second_nothrow_construct<Args1...>)
			requires std::is_constructible_v<T0, Args0...> && std::is_constructible_v<T1, Args1...>
			: packed_pair(std::forward<std::tuple<Args0...>>(args0),
						  std::make_index_sequence<std::tuple_size<std::tuple<Args0...>>::value>{},
						  std::forward<std::tuple<Args1...>>(args1),
						  std::make_index_sequence<std::tuple_size<std::tuple<Args1...>>::value>{})
		{
		}

		template<typename U0, typename U1>
		constexpr packed_pair &operator=(const packed_pair<U0, U1> &other)
			requires std::is_assignable_v<T0 &, const U0 &> && std::is_assignable_v<T1 &, const U1 &>
		{
			first() = other.first();
			second() = other.second();
			return *this;
		}
		template<typename U0, typename U1>
		constexpr packed_pair &operator=(packed_pair<U0, U1> &&other)
			noexcept(std::is_nothrow_assignable_v<T0 &, U0 &&> && std::is_nothrow_assignable_v<T1 &, U1 &&>)
			requires std::is_assignable_v<T0 &, U0 &&> && std::is_assignable_v<T1 &, U1 &&>
		{
			first() = std::move(other.first());
			second() = std::move(other.second());
			return *this;
		}
		// clang-format on

		/** Returns reference to the first element of the pair. */
		[[nodiscard]] constexpr T0 &first() noexcept { return *base_first::get(); }
		/** @copydoc first */
		[[nodiscard]] constexpr const T0 &first() const noexcept { return *base_first::get(); }
		/** Returns reference to the second element of the pair. */
		[[nodiscard]] constexpr T1 &second() noexcept { return *base_second::get(); }
		/** @copydoc second */
		[[nodiscard]] constexpr const T1 &second() const noexcept { return *base_second::get(); }

		// clang-format off
		constexpr void swap(packed_pair &other) noexcept(std::is_nothrow_swappable_v<base_first> && std::is_nothrow_swappable_v<base_second>)
		{
			base_first::swap(other);
			base_second::swap(other);
		}

		friend constexpr void swap(packed_pair &a, packed_pair &b)
			noexcept(std::is_nothrow_swappable_v<base_first> && std::is_nothrow_swappable_v<base_second>) { a.swap(b); }
		// clang-format on

		[[nodiscard]] constexpr hash_t hash() const
			requires has_hash<T0> && has_hash<T1>
		{
			auto s = hash(first());
			return hash_combine(s, second());
		}
		[[nodiscard]] friend constexpr hash_t hash(const packed_pair &p)
			requires has_hash<T0> && has_hash<T1>
		{
			return p.hash();
		}
	};
}	 // namespace sek

template<typename T0, typename T1>
struct std::hash<sek::packed_pair<T0, T1>>
{
	constexpr std::size_t operator()(const sek::packed_pair<T0, T1> &p) noexcept { return p.hash(); }
};