/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "common.hpp"

#ifdef SEK_USE_SSE
namespace sek::math::detail
{
	template<std::size_t N>
	inline void vector_sqrt(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = _mm_sqrt_ps(v.simd);
	}
	template<std::size_t N>
	inline void vector_rsqrt(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = _mm_rsqrt_ps(v.simd);
	}

#ifdef SEK_USE_SSE2
	SEK_API __m128 x86_exp_ps(__m128 v) noexcept;
	SEK_API __m128 x86_log_ps(__m128 v) noexcept;

	template<storage_policy P>
	inline void vector_exp(vector_data<float, 2, P> &out, const vector_data<float, 2, P> &v) noexcept
	{
		const auto a = x86_exp_ps(_mm_set_ps(0, v[1], 0, v[0]));
		const auto h = _mm_cvtss_f32(_mm_unpackhi_ps(a, a));
		const auto l = _mm_cvtss_f32(a);

		out[0] = l;
		out[1] = h;
	}
	template<storage_policy P>
	inline void vector_log(vector_data<float, 2, P> &out, const vector_data<float, 2, P> &v) noexcept
	{
		const auto a = x86_log_ps(_mm_set_ps(0, v[1], 0, v[0]));
		const auto h = _mm_cvtss_f32(_mm_unpackhi_ps(a, a));
		const auto l = _mm_cvtss_f32(a);

		out[0] = l;
		out[1] = h;
	}

	template<std::size_t N>
	inline void vector_exp(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = x86_exp_ps(v.simd);
	}
	template<std::size_t N>
	inline void vector_log(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = x86_log_ps(v.simd);
	}

	inline void vector_sqrt(simd_vector<double, 2> &out, const simd_vector<double, 2> &v) noexcept
	{
		out.simd = _mm_sqrt_pd(v.simd);
	}
	inline void vector_rsqrt(simd_vector<double, 2> &out, const simd_vector<double, 2> &v) noexcept
	{
		out.simd = _mm_div_pd(_mm_set1_pd(1), _mm_sqrt_pd(v.simd));
	}

#ifndef SEK_USE_AVX
	template<std::size_t N>
	inline void vector_sqrt(simd_vector<double, N> &out, const simd_vector<double, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd[0] = _mm_sqrt_pd(v.simd[0]);
		out.simd[1] = _mm_sqrt_pd(v.simd[1]);
	}
	template<std::size_t N>
	inline void vector_rsqrt(simd_vector<double, N> &out, const simd_vector<double, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		const auto v1 = _mm_set1_pd(1);
		out.simd[0] = _mm_div_pd(v1, _mm_sqrt_pd(v.simd[0]));
		out.simd[1] = _mm_div_pd(v1, _mm_sqrt_pd(v.simd[1]));
	}
#endif
#endif
}	 // namespace sek::math::detail
#endif