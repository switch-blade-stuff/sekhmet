//
// Created by switchblade on 2022-01-31.
//

#pragma once

#include <bit>

#include "../util.hpp"
#include "common.hpp"

namespace sek::math::detail
{
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
}	 // namespace sek::math::detail