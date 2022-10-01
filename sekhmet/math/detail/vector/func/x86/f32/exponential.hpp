/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../common.hpp"

#ifdef SEK_USE_SSE
namespace sek::detail
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
	SEK_FORCE_INLINE __m128 x86_pow2_ps(__m128i v) noexcept
	{
		const auto adjusted = _mm_add_epi32(v, _mm_set1_epi32(0x7f));
		return _mm_castsi128_ps(_mm_slli_epi32(adjusted, 23));
	}
	SEK_FORCE_INLINE __m128 x86_pow2_ps(__m128 v) noexcept { return x86_pow2_ps(_mm_cvtps_epi32(v)); }

	SEK_API __m128 x86_exp_ps(__m128 v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_exp(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_exp_ps);
	}
	template<std::size_t N, policy_t P>
	inline void vector_expm1(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, [](auto v) { return _mm_sub_ps(x86_exp_ps(v), _mm_set1_ps(1.0f)); });
	}

	SEK_API __m128 x86_exp2_ps(__m128 v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_exp2(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_exp2_ps);
	}

	SEK_API __m128 x86_log_ps(__m128 v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_log(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_log_ps);
	}
	template<std::size_t N, policy_t P>
	inline void vector_log1p(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, [one = _mm_set1_ps(1.0)](auto v) { return x86_log_ps(_mm_add_ps(v, one)); });
	}

	SEK_API __m128 x86_log2_ps(__m128 v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_log2(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_log2_ps);
	}

	SEK_API __m128 x86_log10_ps(__m128 v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_log10(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_log10_ps);
	}
#endif
}	 // namespace sek::detail
#endif