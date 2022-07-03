/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "common.hpp"
#include "util_f32.hpp"

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
	template<std::size_t I0, std::size_t I1, policy_t P>
	inline void mask_shuffle(mask_data<double, 2, P> &out, const mask_data<double, 2, P> &m, std::index_sequence<I0, I1>) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		constexpr auto mask = x86_128_shuffle2_mask(std::index_sequence<I0, I1>{});
		out.simd = _mm_shuffle_pd(m.simd, m.simd, mask);
	}

	template<policy_t P, std::size_t... Is>
	inline void vector_shuffle(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v, std::index_sequence<Is...> s) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		constexpr auto mask = x86_128_shuffle2_mask(s);
		out.simd = _mm_shuffle_pd(v.simd, v.simd, mask);
	}
	template<policy_t P>
	inline void vector_interleave(vector_data<double, 2, P> &out,
								  const vector_data<double, 2, P> &l,
								  const vector_data<double, 2, P> &r,
								  mask_data<double, 2, P> &m) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = x86_blendv_pd(r.simd, l.simd, m.simd);
	}

#ifdef SEK_USE_SSE4_1
	SEK_FORCE_INLINE __m128d x86_floor_pd(__m128d v) noexcept { return _mm_floor_pd(v); }

	template<policy_t P>
	inline void vector_round(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_round_pd(v.simd, _MM_FROUND_RINT);
	}
	template<policy_t P>
	inline void vector_ceil(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_ceil_pd(v.simd);
	}
	template<policy_t P>
	inline void vector_trunc(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_round_pd(v.simd, _MM_FROUND_TRUNC);
	}
#else
	SEK_API __m128d x86_floor_pd(__m128d v) noexcept;
#endif
	template<policy_t P>
	inline void vector_floor(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = x86_floor_pd(v.simd);
	}

#ifndef SEK_USE_AVX
	template<std::size_t N, std::size_t I0, std::size_t I1, policy_t P, std::size_t... Is>
	inline void mask_shuffle(mask_data<double, N, P> &out, const mask_data<double, 2, P> &m, std::index_sequence<I0, I1, Is...>) noexcept
		requires(N != 2 && check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
	{
		constexpr auto mask0 = x86_128_shuffle2_mask(std::index_sequence<I0, I1>{});
		constexpr auto mask1 = x86_128_shuffle2_mask(std::index_sequence<Is...>{});
		out.simd[0] = _mm_shuffle_pd(m.simd, m.simd, mask0);
		out.simd[1] = _mm_shuffle_pd(m.simd, m.simd, mask1);
	}

	template<std::size_t N, std::size_t I0, std::size_t I1, policy_t P, std::size_t... Is>
	inline void vector_shuffle(vector_data<double, N, P> &out,
							   const vector_data<double, 2, P> &v,
							   std::index_sequence<I0, I1, Is...>) noexcept
		requires(N != 2 && check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
	{
		constexpr auto mask0 = x86_128_shuffle2_mask(std::index_sequence<I0, I1>{});
		constexpr auto mask1 = x86_128_shuffle2_mask(std::index_sequence<Is...>{});
		out.simd[0] = _mm_shuffle_pd(v.simd, v.simd, mask0);
		out.simd[1] = _mm_shuffle_pd(v.simd, v.simd, mask1);
	}
	template<std::size_t N, policy_t P>
	inline void vector_interleave(vector_data<double, N, P> &out,
								  const vector_data<double, N, P> &l,
								  const vector_data<double, N, P> &r,
								  mask_data<double, N, P> &m) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd[0] = x86_blendv_pd(r.simd[0], l.simd[0], m.simd[0]);
		out.simd[1] = x86_blendv_pd(r.simd[1], l.simd[1], m.simd[1]);
	}

#ifdef SEK_USE_SSE4_1
	template<std::size_t N, policy_t P>
	inline void vector_round(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		const int mask = _MM_FROUND_RINT;
		out.simd[0] = _mm_round_pd(v.simd[0], mask);
		out.simd[1] = _mm_round_pd(v.simd[1], mask);
	}
	template<std::size_t N, policy_t P>
	inline void vector_ceil(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd[0] = _mm_ceil_pd(v.simd[0]);
		out.simd[1] = _mm_ceil_pd(v.simd[1]);
	}
	template<std::size_t N, policy_t P>
	inline void vector_trunc(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		const int mask = _MM_FROUND_TRUNC;
		out.simd[0] = _mm_round_pd(v.simd[0], mask);
		out.simd[1] = _mm_round_pd(v.simd[1], mask);
	}
#endif
	template<std::size_t N, policy_t P>
	inline void vector_floor(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd[0] = x86_floor_pd(v.simd[0]);
		out.simd[1] = x86_floor_pd(v.simd[1]);
	}
#endif

#ifndef SEK_USE_AVX512_DQ
	SEK_API __m128i x86_cvtpd_epu64(__m128d v) noexcept;
	SEK_API __m128i x86_cvtpd_epi64(__m128d v) noexcept;
#else
	SEK_FORCE_INLINE __m128i x86_cvtpd_epu64(__m128d v) noexcept { return _mm_cvtpd_epu64(v); }
	SEK_FORCE_INLINE __m128i x86_cvtpd_epi64(__m128d v) noexcept { return _mm_cvtpd_epi64(v); }
#endif
}	 // namespace sek::math::detail
#endif