/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "common.hpp"

#ifdef SEK_USE_SSE
namespace sek::math::detail
{
	template<std::size_t N, policy_t P>
	inline void vector_sqrt(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_sqrt_ps(v.simd);
	}
	template<std::size_t N, policy_t P>
	inline void vector_rsqrt(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		if constexpr (check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>)
			out.simd = _mm_rsqrt_ps(v.simd);
		else
			out.simd = _mm_div_ps(_mm_set1_ps(1.0f), _mm_sqrt_ps(v.simd));
	}

#ifdef SEK_USE_SSE2
	SEK_API __m128 x86_exp_ps(__m128 v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_exp(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = x86_exp_ps(v.simd);
		else
			x86_unpack_ps(out, x86_exp_ps(x86_pack_ps(v)));
	}
	template<std::size_t N, policy_t P>
	inline void vector_expm1(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = _mm_sub_ps(x86_exp_ps(v.simd), _mm_set1_ps(1.0f));
		else
			x86_unpack_ps(out, _mm_sub_ps(x86_exp_ps(x86_pack_ps(v)), _mm_set1_ps(1.0f)));
	}

	SEK_API __m128 x86_exp2_ps(__m128 v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_exp2(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = x86_exp2_ps(v.simd);
		else
			x86_unpack_ps(out, x86_exp2_ps(x86_pack_ps(v)));
	}

	SEK_API __m128 x86_log_ps(__m128 v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_log(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = x86_log_ps(v.simd);
		else
			x86_unpack_ps(out, x86_log_ps(x86_pack_ps(v)));
	}
	template<std::size_t N, policy_t P>
	inline void vector_log1p(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = x86_log_ps(_mm_add_ps(_mm_set1_ps(1.0f), v.simd));
		else
			x86_unpack_ps(out, x86_log_ps(_mm_add_ps(_mm_set1_ps(1.0f), x86_pack_ps(v))));
	}

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
#endif
}	 // namespace sek::math::detail
#endif