/*
 * Created by switchblade on 2021-12-16
 */

#pragma once

#include <bit>

#include "sekhmet/detail/define.h"

#include "../storage.hpp"

#ifdef SEK_ARCH_x86

#include <emmintrin.h>
#include <immintrin.h>
#include <mmintrin.h>
#include <nmmintrin.h>
#include <pmmintrin.h>
#include <smmintrin.h>
#include <tmmintrin.h>
#include <xmmintrin.h>

#ifdef SEK_NO_SIMD
#ifdef SEK_USE_SSE
#undef SEK_USE_SSE
#endif
#ifdef SEK_USE_AVX
#undef SEK_USE_AVX
#endif
#endif

#if defined(SEK_USE_SSE) && (!defined(__SSE__))
#undef SEK_USE_SSE
#endif
#if defined(SEK_USE_SSE2) && (!defined(__SSE2__) || !defined(SEK_USE_SSE))
#undef SEK_USE_SSE2
#endif
#if defined(SEK_USE_SSE3) && (!defined(__SSE3__) || !defined(SEK_USE_SSE))
#undef SEK_USE_SSE3
#endif
#if defined(SEK_USE_SSSE3) && (!defined(__SSSE3__) || !defined(SEK_USE_SSE))
#undef SEK_USE_SSSE3
#endif
#if defined(SEK_USE_SSE4) && !defined(SEK_USE_SSE)
#undef SEK_USE_SSE4
#endif
#if defined(SEK_USE_SSE4_1) && (!defined(__SSE4_1__) || !defined(SEK_USE_SSE4))
#undef SEK_USE_SSE4_1
#endif
#if defined(SEK_USE_SSE4_2) && (!defined(__SSE4_2__) || !defined(SEK_USE_SSE4))
#undef SEK_USE_SSE4_2
#endif

#if defined(SEK_USE_AVX) && !defined(__AVX__)
#undef SEK_USE_AVX
#endif
#if defined(SEK_USE_AVX2) && (!defined(__AVX2__) || !defined(SEK_USE_AVX))
#undef SEK_USE_AVX2
#endif

// clang-format off
#define SEK_DETAIL_IS_SIMD_1(a) (requires{ (a).simd; })
#define SEK_DETAIL_IS_SIMD_2(a, b) (requires{ (a).simd; } && requires{ (b).simd; })
#define SEK_DETAIL_IS_SIMD_3(a, b, c) (requires{ (a).simd; } && requires{ (b).simd; } && requires{ (c).simd; })
// clang-format on

#define SEK_DETAIL_IS_SIMD(...)                                                                                        \
	SEK_GET_MACRO_3(__VA_ARGS__, SEK_DETAIL_IS_SIMD_3, SEK_DETAIL_IS_SIMD_2, SEK_DETAIL_IS_SIMD_1)(__VA_ARGS__)

namespace sek::math::detail
{
	template<typename T, std::size_t N>
	using simd_vector = vector_data<T, N, storage_policy::OPTIMAL>;
	template<typename T, std::size_t N>
	using simd_mask = mask_data<T, N, storage_policy::OPTIMAL>;

	template<>
	struct mask_set<std::uint32_t>
	{
		template<typename U>
		constexpr void operator()(std::uint32_t &to, U &&from) const noexcept
		{
			to = from ? std::numeric_limits<std::uint32_t>::max() : 0;
		}
	};
	template<>
	struct mask_get<std::uint32_t>
	{
		constexpr bool operator()(std::uint32_t &v) const noexcept { return v; }
	};
	template<>
	struct mask_set<std::uint64_t>
	{
		template<typename U>
		constexpr void operator()(std::uint64_t &to, U &&from) const noexcept
		{
			to = from ? std::numeric_limits<std::uint64_t>::max() : 0;
		}
	};
	template<>
	struct mask_get<std::uint64_t>
	{
		constexpr bool operator()(std::uint64_t &v) const noexcept { return v; }
	};

	template<std::size_t J, std::size_t I, std::size_t... Is>
	constexpr std::uint8_t x86_128_shuffle4_unwrap(std::index_sequence<I, Is...>) noexcept
	{
		constexpr auto bit = static_cast<std::uint8_t>(I) << J;
		if constexpr (sizeof...(Is) != 0)
			return bit | x86_128_shuffle4_unwrap<J + 2>(std::index_sequence<Is...>{});
		else
			return bit;
	}
	template<std::size_t... Is>
	constexpr std::uint8_t x86_128_shuffle4_mask(std::index_sequence<Is...> s) noexcept
	{
		return x86_128_shuffle4_unwrap<0>(s);
	}

	template<std::size_t J, std::size_t I, std::size_t... Is>
	constexpr std::uint8_t x86_128_shuffle2_unwrap(std::index_sequence<I, Is...>) noexcept
	{
		constexpr auto bit = static_cast<std::uint8_t>(I) << J;
		if constexpr (sizeof...(Is) != 0)
			return bit | x86_128_shuffle2_unwrap<J + 1>(std::index_sequence<Is...>{});
		else
			return bit;
	}
	template<std::size_t... Is>
	constexpr std::uint8_t x86_128_shuffle2_mask(std::index_sequence<Is...> s) noexcept
	{
		return x86_128_shuffle2_unwrap<0>(s);
	}
}	 // namespace sek::math::detail

#endif