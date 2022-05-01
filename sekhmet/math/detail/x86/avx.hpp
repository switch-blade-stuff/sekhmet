//
// Created by switchblade on 2022-03-07.
//

#pragma once

#include "../util.hpp"
#include "common.hpp"

#ifdef SEK_USE_AVX

#include <immintrin.h>

namespace sek::math::detail
{
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

	// TODO: Implement AVX SIMD
}	 // namespace sek::math::detail
#endif