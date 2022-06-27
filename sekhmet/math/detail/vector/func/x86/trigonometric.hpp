/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "common.hpp"

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
	SEK_API __m128 x86_sin_ps(__m128 v) noexcept;
	SEK_API __m128 x86_cos_ps(__m128 v) noexcept;

	template<std::size_t N>
	inline void vector_sin(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = x86_sin_ps(v.simd);
	}
	template<std::size_t N>
	inline void vector_cos(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = x86_cos_ps(v.simd);
	}

	SEK_API __m128d x86_sin_pd(__m128d v) noexcept;
	SEK_API __m128d x86_cos_pd(__m128d v) noexcept;

	inline void vector_sin(simd_vector<double, 2> &out, const simd_vector<double, 2> &v) noexcept
	{
		out.simd = x86_sin_pd(v.simd);
	}
	inline void vector_cos(simd_vector<double, 2> &out, const simd_vector<double, 2> &v) noexcept
	{
		out.simd = x86_cos_pd(v.simd);
	}

#ifndef SEK_USE_AVX
	template<std::size_t N>
	inline void vector_sin(simd_vector<double, N> &out, const simd_vector<double, N> &v) noexcept
		requires simd_enabled<simd_vector<double, N>>
	{
		out.simd[0] = x86_sin_pd(v.simd[0]);
		out.simd[1] = x86_sin_pd(v.simd[1]);
	}
	template<std::size_t N>
	inline void vector_cos(simd_vector<double, N> &out, const simd_vector<double, N> &v) noexcept
		requires simd_enabled<simd_vector<double, N>>
	{
		out.simd[0] = x86_cos_pd(v.simd[0]);
		out.simd[1] = x86_cos_pd(v.simd[1]);
	}
#endif
}	 // namespace sek::math::detail
#endif