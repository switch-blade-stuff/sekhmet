/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../common.hpp"
#include "../utility.hpp"

#ifdef SEK_USE_SSE2
namespace sek::detail
{
	SEK_FORCE_INLINE __m128d x86_pow2_pd(__m128i v) noexcept
	{
		const auto adjusted = _mm_add_epi64(v, _mm_set1_epi64x(0x3ff));
		return _mm_castsi128_pd(_mm_slli_epi64(adjusted, 52));
	}
	SEK_FORCE_INLINE __m128d x86_pow2_pd(__m128d v) noexcept { return x86_pow2_pd(x86_cvtpd_epi64(v)); }

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

	SEK_API __m128d x86_exp_pd(__m128d v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_exp(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_exp_pd);
	}
	template<std::size_t N, policy_t P>
	inline void vector_expm1(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, [one = _mm_set1_pd(1.0)](auto v) { return _mm_sub_pd(x86_exp_pd(v), one); });
	}

	SEK_API __m128d x86_exp2_pd(__m128d v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_exp2(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_exp2_pd);
	}

	SEK_API __m128d x86_log_pd(__m128d v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_log(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_log_pd);
	}
	template<std::size_t N, policy_t P>
	inline void vector_log1p(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, [one = _mm_set1_pd(1.0)](auto v) { return x86_log_pd(_mm_add_pd(v, one)); });
	}

	SEK_API __m128d x86_log2_pd(__m128d v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_log2(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_log2_pd);
	}

	SEK_API __m128d x86_log10_pd(__m128d v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_log10(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_log10_pd);
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
		const auto one = _mm_set1_pd(1.0);
		out.simd[0] = _mm_div_pd(one, _mm_sqrt_pd(v.simd[0]));
		out.simd[1] = _mm_div_pd(one, _mm_sqrt_pd(v.simd[1]));
	}
#endif
}	 // namespace sek::detail
#endif