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
 * Created by switchblade on 2021-12-16
 */

#pragma once

#include "sekhmet/detail/define.h"

#ifdef SEK_ARCH_x86

#include <emmintrin.h>
#include <immintrin.h>
#include <mmintrin.h>
#include <pmmintrin.h>
#include <smmintrin.h>

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

namespace sek::math::detail
{
	template<std::size_t J, std::size_t I, std::size_t... Is>
	constexpr std::uint8_t x86_mm_shuffle4_unwrap(std::index_sequence<I, Is...>) noexcept
	{
		constexpr auto bit = static_cast<std::uint8_t>(I) << J;
		if constexpr (sizeof...(Is) != 0)
			return bit | x86_mm_shuffle4_unwrap<J + 2>(std::index_sequence<Is...>{});
		else
			return bit;
	}
	template<std::size_t... Is>
	constexpr std::uint8_t x86_mm_shuffle4_mask(std::index_sequence<Is...> s) noexcept
	{
		return x86_mm_shuffle4_unwrap<0>(s);
	}

	template<std::size_t J, std::size_t I, std::size_t... Is>
	constexpr std::uint8_t x86_mm_shuffle2_unwrap(std::index_sequence<I, Is...>) noexcept
	{
		constexpr auto bit = static_cast<std::uint8_t>(I) << J;
		if constexpr (sizeof...(Is) != 0)
			return bit | x86_mm_shuffle2_unwrap<J + 1>(std::index_sequence<Is...>{});
		else
			return bit;
	}
	template<std::size_t... Is>
	constexpr std::uint8_t x86_mm_shuffle2_mask(std::index_sequence<Is...> s) noexcept
	{
		return x86_mm_shuffle2_unwrap<0>(s);
	}
}	 // namespace sek::math::detail

#endif