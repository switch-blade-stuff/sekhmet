//
// Created by switchblade on 2022-01-31.
//

#pragma once

#include "../util.hpp"
#include "common.hpp"

namespace sek::math::detail
{
	template<typename>
	union ieee574_mask;

	template<>
	union ieee574_mask<float>
	{
		constexpr explicit ieee574_mask(std::uint32_t value) noexcept : i(value) {}

		constexpr operator float() const noexcept { return f; }

	private:
		std::uint32_t i;
		float f;
	};
	template<>
	union ieee574_mask<double>
	{
		constexpr explicit ieee574_mask(std::uint64_t value) noexcept : i(value) {}

		constexpr operator double() const noexcept { return f; }

	private:
		std::uint64_t i;
		double f;
	};

	// clang-format off
	template<typename T, std::size_t N, std::size_t M, typename F>
	constexpr void simd_array_invoke(simd_data<T, N> (&out)[M], const simd_data<T, N> (&l)[M], const simd_data<T, N> (&r)[M], F f) noexcept
	{
		for (auto i = M; i-- > 0;) f(out[i].value, l[i].value, r[i].value);
	}
	template<typename T, std::size_t N, std::size_t M, typename F>
	constexpr void simd_array_invoke(simd_data<T, N> (&out)[M], const simd_data<T, N> (&l)[M], T r, F f) noexcept
	{
		for (auto i = M; i-- > 0;) f(out[i].value, l[i].value, r);
	}
	template<typename T, std::size_t N, std::size_t M, typename F>
	constexpr void simd_array_invoke(simd_data<T, N> (&out)[M], const simd_data<T, N> (&l)[M], F f) noexcept
	{
		for (auto i = M; i-- > 0;) f(out[i].value, l[i].value);
	}
	// clang-format on
}	 // namespace sek::math