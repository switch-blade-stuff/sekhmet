//
// Created by switchblade on 2022-03-07.
//

#pragma once

#include "simd_util.hpp"

#ifdef SEK_USE_AVX

#include <immintrin.h>

namespace sek::math::detail
{
	template<>
	struct simd_data<float, 8>
	{
		__m256 value;
	};

	template<>
	struct simd_data<double, 4>
	{
		__m256d value;
	};

}	 // namespace sek::math::detail
#endif