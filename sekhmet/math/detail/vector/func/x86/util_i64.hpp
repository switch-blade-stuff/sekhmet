/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "common.hpp"
#include "util_f32.hpp"
#include "util_f64.hpp"

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
	template<integral_of_size<8> T, std::size_t I0, std::size_t I1, policy_t P>
	inline void mask_shuffle(mask_data<T, 2, P> &out, const mask_data<T, 2, P> &m, std::index_sequence<I0, I1>) noexcept
	{
		constexpr auto mask = x86_128_shuffle2_mask(std::index_sequence<I0, I1>{});
		out.simd = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(m.simd), _mm_castsi128_pd(m.simd), mask));
	}

	template<integral_of_size<8> T, policy_t P, std::size_t... Is>
	inline void vector_shuffle(vector_data<T, 2, P> &out, const vector_data<T, 2, P> &v, std::index_sequence<Is...> s) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		constexpr auto mask = x86_128_shuffle2_mask(s);
		const auto a = _mm_castsi128_pd(v.simd);
		out.simd = _mm_castpd_si128(_mm_shuffle_pd(a, a, mask));
	}
	template<integral_of_size<8> T, policy_t P>
	inline void vector_interleave(vector_data<T, 2, P> &out,
								  const vector_data<T, 2, P> &l,
								  const vector_data<T, 2, P> &r,
								  const mask_data<T, 2, P> &m) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = x86_blendv_epi8(r.simd, l.simd, m.simd);
	}

#ifndef SEK_USE_AVX2
	template<integral_of_size<8> T, std::size_t N, std::size_t I0, std::size_t I1, policy_t P, std::size_t... Is>
	inline void mask_shuffle(mask_data<T, N, P> &out, const mask_data<T, 2, P> &m, std::index_sequence<I0, I1, Is...>) noexcept
		requires(N != 2 && check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
	{
		constexpr auto mask0 = x86_128_shuffle2_mask(std::index_sequence<I0, I1>{});
		constexpr auto mask1 = x86_128_shuffle2_mask(std::index_sequence<Is...>{});
		const auto a = _mm_castsi128_pd(m.simd);
		out.simd[0] = _mm_castpd_si128(_mm_shuffle_pd(a, a, mask0));
		out.simd[1] = _mm_castpd_si128(_mm_shuffle_pd(a, a, mask1));
	}

	template<integral_of_size<8> T, std::size_t N, std::size_t I0, std::size_t I1, policy_t P, std::size_t... Is>
	inline void vector_shuffle(vector_data<T, N, P> &out, const vector_data<T, 2, P> &v, std::index_sequence<I0, I1, Is...>) noexcept
		requires(N != 2 && check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
	{
		constexpr auto mask0 = x86_128_shuffle2_mask(std::index_sequence<I0, I1>{});
		constexpr auto mask1 = x86_128_shuffle2_mask(std::index_sequence<Is...>{});
		const auto a = _mm_castsi128_pd(v.simd);
		out.simd[0] = _mm_castpd_si128(_mm_shuffle_pd(a, a, mask0));
		out.simd[1] = _mm_castpd_si128(_mm_shuffle_pd(a, a, mask1));
	}
	template<integral_of_size<8> T, std::size_t N, policy_t P>
	inline void vector_interleave(vector_data<T, N, P> &out,
								  const vector_data<T, N, P> &l,
								  const vector_data<T, N, P> &r,
								  const mask_data<T, N, P> &m) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd[0] = x86_blendv_epi8(r.simd[0], l.simd[0], m.simd[0]);
		out.simd[1] = x86_blendv_epi8(r.simd[1], l.simd[1], m.simd[1]);
	}
#endif

#ifndef SEK_USE_AVX512_DQ
	SEK_API __m128d x86_cvtepu64_pd(__m128i v) noexcept;
	SEK_API __m128d x86_cvtepi64_pd(__m128i v) noexcept;
#else
	SEK_FORCE_INLINE __m128d x86_cvtepu64_pd(__m128i v) noexcept { return _mm_cvtepu64_pd(v); }
	SEK_FORCE_INLINE __m128d x86_cvtepi64_pd(__m128i v) noexcept { return _mm_cvtepi64_pd(v); }
#endif
}	 // namespace sek::math::detail
#endif