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

	SEK_API __m128 x86_tancot_ps(__m128 v, __m128i m) noexcept;
	SEK_API __m128 x86_tan_ps(__m128 v) noexcept;
	SEK_API __m128 x86_cot_ps(__m128 v) noexcept;

	template<storage_policy P>
	inline void vector_sin(vector_data<float, 2, P> &out, const vector_data<float, 2, P> &v) noexcept
	{
		const auto a = x86_sin_ps(_mm_set_ps(0, v[1], 0, v[0]));
		const auto h = _mm_cvtss_f32(_mm_unpackhi_ps(a, a));
		const auto l = _mm_cvtss_f32(a);

		out[0] = l;
		out[1] = h;
	}
	template<storage_policy P>
	inline void vector_cos(vector_data<float, 2, P> &out, const vector_data<float, 2, P> &v) noexcept
	{
		const auto a = x86_cos_ps(_mm_set_ps(0, v[1], 0, v[0]));
		const auto h = _mm_cvtss_f32(_mm_unpackhi_ps(a, a));
		const auto l = _mm_cvtss_f32(a);

		out[0] = l;
		out[1] = h;
	}
	template<storage_policy P>
	inline void vector_tan(vector_data<float, 2, P> &out, const vector_data<float, 2, P> &v) noexcept
	{
		const auto a = x86_tan_ps(_mm_set_ps(0, v[1], 0, v[0]));
		const auto h = _mm_cvtss_f32(_mm_unpackhi_ps(a, a));
		const auto l = _mm_cvtss_f32(a);

		out[0] = l;
		out[1] = h;
	}
	template<storage_policy P>
	inline void vector_cot(vector_data<float, 2, P> &out, const vector_data<float, 2, P> &v) noexcept
	{
		const auto a = x86_cot_ps(_mm_set_ps(0, v[1], 0, v[0]));
		const auto h = _mm_cvtss_f32(_mm_unpackhi_ps(a, a));
		const auto l = _mm_cvtss_f32(a);

		out[0] = l;
		out[1] = h;
	}

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
	template<std::size_t N>
	inline void vector_tan(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = x86_tan_ps(v.simd);
	}
	template<std::size_t N>
	inline void vector_cot(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = x86_cot_ps(v.simd);
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