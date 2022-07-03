/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../common.hpp"

#ifdef SEK_USE_SSE
namespace sek::math::detail
{
	template<std::size_t N, std::size_t M, policy_t P, std::size_t... Is>
	inline void mask_shuffle(mask_data<float, N, P> &out, const mask_data<float, N, P> &m, std::index_sequence<Is...> s) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		constexpr auto mask = x86_128_shuffle4_mask(s);
		out.simd = _mm_shuffle_ps(m.simd, m.simd, mask);
	}

	template<std::size_t N, std::size_t M, policy_t P, std::size_t... Is>
	inline void vector_shuffle(vector_data<float, N, P> &out, const vector_data<float, M, P> &l, std::index_sequence<Is...> s) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		constexpr auto mask = x86_128_shuffle4_mask(s);
		out.simd = _mm_shuffle_ps(l.simd, l.simd, mask);
	}
	template<std::size_t N, policy_t P>
	inline void vector_interleave(vector_data<float, N, P> &out,
								  const vector_data<float, N, P> &l,
								  const vector_data<float, N, P> &r,
								  mask_data<float, N, P> &m) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = x86_blendv_ps(r.simd, l.simd, m.simd);
	}

#ifdef SEK_USE_SSE4_1
	SEK_FORCE_INLINE __m128 x86_floor_ps(__m128 v) noexcept { return _mm_floor_ps(v); }

	template<std::size_t N, policy_t P>
	inline void vector_round(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_round_ps(v.simd, _MM_FROUND_RINT);
	}
	template<std::size_t N, policy_t P>
	inline void vector_ceil(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_ceil_ps(v.simd);
	}
	template<std::size_t N, policy_t P>
	inline void vector_floor(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = x86_floor_ps(v.simd);
	}
	template<std::size_t N, policy_t P>
	inline void vector_trunc(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_round_ps(v.simd, _MM_FROUND_TRUNC);
	}
#elif defined(SEK_USE_SSE2)
	SEK_FORCE_INLINE __m128 x86_floor_ps(__m128 v) noexcept
	{
		/* Convert to int and subtract 1 to round down. */
		const auto tmp = _mm_cvtepi32_ps(_mm_cvtps_epi32(v));
		return _mm_sub_ps(tmp, _mm_and_ps(_mm_cmpgt_ps(tmp, v), _mm_set1_ps(1.0f)));
	}
	template<std::size_t N, policy_t P>
	inline void vector_floor(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = x86_floor_ps(v.simd);
	}
#endif
}	 // namespace sek::math::detail
#endif