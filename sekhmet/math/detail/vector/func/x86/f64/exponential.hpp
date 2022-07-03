/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../common.hpp"

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
	template<policy_t P>
	inline void vector_sqrt(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_sqrt_pd(v.simd);
	}
	template<policy_t P>
	inline void vector_rsqrt(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_div_pd(_mm_set1_pd(1.0), _mm_sqrt_pd(v.simd));
	}

#ifndef SEK_USE_AVX
	template<std::size_t N, policy_t P>
	inline void vector_sqrt(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd[0] = _mm_sqrt_pd(v.simd[0]);
		out.simd[1] = _mm_sqrt_pd(v.simd[1]);
	}
	template<std::size_t N, policy_t P>
	inline void vector_rsqrt(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		const auto v1 = _mm_set1_pd(1);
		out.simd[0] = _mm_div_pd(v1, _mm_sqrt_pd(v.simd[0]));
		out.simd[1] = _mm_div_pd(v1, _mm_sqrt_pd(v.simd[1]));
	}
#endif
}	 // namespace sek::math::detail
#endif