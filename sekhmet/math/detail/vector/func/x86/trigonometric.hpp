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
}	 // namespace sek::math::detail
#endif