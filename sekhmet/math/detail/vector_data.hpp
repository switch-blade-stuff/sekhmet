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
 * Created by switchblade on 2022-03-06
 */

#pragma once

#include "sekhmet/detail/define.h"
#include "simd.hpp"

namespace sek::math::detail
{
	template<typename T, std::size_t N, bool UseSimd = false>
	struct vector_data
	{
		constexpr vector_data() noexcept = default;

		// clang-format off
		template<typename U, std::size_t M>
		constexpr explicit vector_data(const vector_data<U, M, false> &other) noexcept requires(std::convertible_to<U, T> && M != N)
		{
			if constexpr (M < N)
				std::copy_n(other.data, M, data);
			else
				std::copy_n(other.data, N, data);
		}
		template<typename U, std::size_t M>
		constexpr explicit vector_data(const vector_data<U, M, true> &other) noexcept requires(std::convertible_to<U, T>);
		// clang-format on
		constexpr explicit vector_data(const T (&vals)[N]) noexcept
		{
			std::copy(std::begin(vals), std::end(vals), data);
		}

		constexpr T &operator[](std::size_t i) noexcept { return data[i]; }
		constexpr const T &operator[](std::size_t i) const noexcept { return data[i]; }

		template<std::size_t I>
		constexpr T &get() noexcept
		{
			return data[I];
		}
		template<std::size_t I>
		constexpr const T &get() const noexcept
		{
			return data[I];
		}

		constexpr auto operator<=>(const vector_data &) const noexcept = default;
		constexpr bool operator==(const vector_data &) const noexcept = default;

		constexpr void swap(vector_data &other) noexcept
		{
			using std::swap;
			swap(data, other.data);
		}

		[[nodiscard]] constexpr sek::hash_t hash() const noexcept { return sek::fnv1a(data); }

		T data[N] = {};
	};
	template<typename T, std::size_t N>
	struct vector_data<T, N, true>
	{
		constexpr vector_data() noexcept = default;

		// clang-format off
		template<typename U, std::size_t M>
		constexpr explicit vector_data(const vector_data<U, M, true> &other) noexcept requires(std::convertible_to<U, T> && M != N)
			: vector_data(other.values)
		{
		}
		template<typename U, std::size_t M>
		constexpr explicit vector_data(const vector_data<U, M, false> &other) noexcept requires(std::convertible_to<U, T>)
			: values(other)
		{
		}
		// clang-format on

		constexpr explicit vector_data(const T (&vals)[N]) noexcept : values(vals) {}

		constexpr T &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr const T &operator[](std::size_t i) const noexcept { return values[i]; }

		template<std::size_t I>
		constexpr T &get() noexcept
		{
			return values.template get<I>();
		}
		template<std::size_t I>
		constexpr const T &get() const noexcept
		{
			return values.template get<I>();
		}

		constexpr auto operator<=>(const vector_data &other) const noexcept { return values <=> other.values; }
		constexpr bool operator==(const vector_data &other) const noexcept { return values == other.values; }

		constexpr void swap(vector_data &other) noexcept { values.swap(other.values); }

		[[nodiscard]] constexpr sek::hash_t hash() const noexcept { return values.hash(); }

		union
		{
			vector_data<T, N, false> values = {};
			simd_t<T, N> simd;
		};
	};

	// clang-format off
	template<typename T, std::size_t N, bool UseSimd>
	template<typename U, std::size_t M>
	constexpr vector_data<T, N, UseSimd>::vector_data(const vector_data<U, M, true> &other) noexcept requires(std::convertible_to<U, T>)
		: vector_data(other.values)
	{
	}

	template<typename T, std::size_t N, storage_policy Policy>
	concept use_simd_data = simd_exists<T, N> && (Policy != storage_policy::PACKED || sizeof(vector_data<T, N, true>) == sizeof(vector_data<T, N>));
	// clang-format on

	template<typename T, std::size_t N, storage_policy Policy>
	using vector_data_t = vector_data<T, N, use_simd_data<T, N, Policy>>;

	template<typename T>
	struct is_simd_data : std::false_type
	{
	};
	template<typename T, std::size_t N>
	struct is_simd_data<vector_data<T, N, true>> : std::true_type
	{
	};
}	 // namespace sek::math::detail