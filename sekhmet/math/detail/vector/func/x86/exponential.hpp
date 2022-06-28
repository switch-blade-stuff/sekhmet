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

	template<std::size_t N, storage_policy P>
	inline void vector_exp(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
	{
		x86_unpack_ps(out, x86_exp_ps(x86_pack_ps(v)));
	}
	template<std::size_t N>
	inline void vector_exp(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = x86_exp_ps(v.simd);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_expm1(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
	{
		x86_unpack_ps(out, _mm_sub_ps(x86_exp_ps(x86_pack_ps(v)), _mm_set1_ps(1.0f)));
	}
	template<std::size_t N>
	inline void vector_expm1(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = _mm_sub_ps(x86_exp_ps(v.simd), _mm_set1_ps(1.0f));
	}

	SEK_API __m128 x86_exp2_ps(__m128 v) noexcept;

	template<std::size_t N, storage_policy P>
	inline void vector_exp2(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
	{
		x86_unpack_ps(out, x86_exp2_ps(x86_pack_ps(v)));
	}
	template<std::size_t N>
	inline void vector_exp2(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = x86_exp2_ps(v.simd);
	}

	SEK_API __m128 x86_log_ps(__m128 v) noexcept;

	template<std::size_t N, storage_policy P>
	inline void vector_log(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
	{
		x86_unpack_ps(out, x86_log_ps(x86_pack_ps(v)));
	}
	template<std::size_t N>
	inline void vector_log(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = x86_log_ps(v.simd);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_log1p(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
	{
		x86_unpack_ps(out, x86_log_ps(_mm_add_ps(_mm_set1_ps(1.0f), x86_pack_ps(v))));
	}
	template<std::size_t N>
	inline void vector_log1p(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = x86_log_ps(_mm_add_ps(_mm_set1_ps(1.0f), v.simd));
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