//
// Created by switchblade on 2022-03-06.
//

#pragma once

#include "sekhmet/detail/define.h"

namespace sek::math::detail
{
	template<typename T, std::size_t N, bool = false>
	struct vector_data
	{
		constexpr vector_data() noexcept = default;
		constexpr explicit vector_data(const T (&vals)[N]) noexcept
		{
			std::copy(std::begin(vals), std::end(vals), data);
		}

		constexpr T &operator[](std::size_t i) noexcept { return data[i]; }
		constexpr const T &operator[](std::size_t i) const noexcept { return data[i]; }

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
		constexpr explicit vector_data(const T (&vals)[N]) noexcept
		{
			std::copy(std::begin(vals), std::end(vals), array.data);
		}

		constexpr T &operator[](std::size_t i) noexcept { return array[i]; }
		constexpr const T &operator[](std::size_t i) const noexcept { return array[i]; }

		constexpr auto operator<=>(const vector_data &other) const noexcept { return array <=> other.array; }
		constexpr bool operator==(const vector_data &other) const noexcept { return array == other.array; }

		constexpr void swap(vector_data &other) noexcept { array.swap(other.array); }

		[[nodiscard]] constexpr sek::hash_t hash() const noexcept { return array.hash(); }

		union
		{
			/* Need to have the data array be potentially bigger than N in order to 0-initialize every element of SIMD vector. */
			vector_data<T, sizeof(simd_storage<T, N>) / sizeof(T), false> array = {};
			simd_storage<T, N> data;
		};
	};

	template<typename T, std::size_t N>
	using vector_data_t = vector_data<T, N, has_simd_data_v<T, N>>;
}	 // namespace sek::math::detail