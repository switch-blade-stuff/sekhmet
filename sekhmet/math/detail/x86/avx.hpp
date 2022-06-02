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
 * Created by switchblade on 2022-03-07
 */

#pragma once

#include "../util.hpp"
#include "common.hpp"

#ifdef SEK_USE_AVX
#error "AVX support is not implemented"

#include <immintrin.h>

namespace sek::math::detail
{
	// TODO: Implement AVX SIMD

	template<>
	struct simd_t<double, 3>
	{
		__m256d value;
	};
	template<>
	struct simd_t<double, 4>
	{
		__m256d value;
	};

#ifdef SEK_USE_AVX2
#error "AVX2 support is not implemented"

	template<integral_of_size<8> T>
	struct simd_t<T, 3>
	{
		__m256i value;
	};
	template<integral_of_size<8> T>
	struct simd_t<T, 4>
	{
		__m256i value;
	};
#endif
}	 // namespace sek::math::detail
#endif