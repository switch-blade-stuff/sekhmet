//
// Created by switchblade on 2022-03-06.
//

#pragma once

#include "sekhmet/detail/define.h"
#include "simd.hpp"

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

		constexpr operator simd_t<T, N> &() noexcept { return simd; }
		constexpr operator const simd_t<T, N> &() const noexcept { return simd; }

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

	template<typename T, std::size_t N>
	using vector_data_t = vector_data<T, N, simd_exists<T, N>>;
}	 // namespace sek::math::detail