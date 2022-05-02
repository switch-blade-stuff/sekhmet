//
// Created by switchblade on 2022-03-07.
//

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