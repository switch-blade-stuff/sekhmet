/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
	SEK_FORCE_INLINE __m128 x86_blendv_ps(__m128 a, __m128 b, __m128 m) noexcept
	{
#ifdef SEK_USE_SSE4_1
		return _mm_blendv_ps(a, b, m);
#else
		return _mm_add_ps(_mm_and_ps(m, b), _mm_andnot_ps(m, a));
#endif
	}
	SEK_FORCE_INLINE __m128 x86_frexp_ps(__m128 v, __m128 &e) noexcept
	{
		const auto mant_mask = _mm_set1_ps(std::bit_cast<float>(0x807f'ffff));
		const auto a = _mm_srli_epi32(_mm_castps_si128(v), 23);
		v = _mm_and_ps(v, mant_mask);
		v = _mm_or_ps(v, _mm_set1_ps(0.5f));
		e = _mm_add_ps(_mm_cvtepi32_ps(_mm_sub_epi32(a, _mm_set1_epi32(0x7f))), _mm_set1_ps(1.0f));
		return v;
	}

	SEK_FORCE_INLINE __m128i x86_blendv_epi8(__m128i a, __m128i b, __m128i m) noexcept
	{
#ifdef SEK_USE_SSE4_1
		return _mm_blendv_epi8(a, b, m);
#else
		return _mm_add_si128(_mm_and_si128(m, b), _mm_andnot_si128(m, a));
#endif
	}

	SEK_FORCE_INLINE __m128d x86_blendv_pd(__m128d a, __m128d b, __m128d m) noexcept
	{
#ifdef SEK_USE_SSE4_1
		return _mm_blendv_pd(a, b, m);
#else
		return _mm_add_pd(_mm_and_pd(m, b), _mm_andnot_pd(m, a));
#endif
	}

#ifndef SEK_USE_AVX512_DQ
	SEK_API __m128d x86_cvtepu64_pd(__m128i v) noexcept;
	SEK_API __m128d x86_cvtepi64_pd(__m128i v) noexcept;
	SEK_API __m128i x86_cvtpd_epu64(__m128d v) noexcept;
	SEK_API __m128i x86_cvtpd_epi64(__m128d v) noexcept;
#else
	SEK_FORCE_INLINE __m128d x86_cvtepu64_pd(__m128i v) noexcept { return _mm_cvtepu64_pd(v); }
	SEK_FORCE_INLINE __m128d x86_cvtepi64_pd(__m128i v) noexcept { return _mm_cvtepi64_pd(v); }
	SEK_FORCE_INLINE __m128i x86_cvtpd_epu64(__m128d v) noexcept { return _mm_cvtpd_epu64(v); }
	SEK_FORCE_INLINE __m128i x86_cvtpd_epi64(__m128d v) noexcept { return _mm_cvtpd_epi64(v); }
#endif

	SEK_FORCE_INLINE __m128d x86_frexp_pd(__m128d v, __m128d &e) noexcept
	{
		const auto mant_mask = _mm_set1_pd(std::bit_cast<double>(0x800f'ffff'ffff'ffff));
		const auto a = _mm_srli_epi64(_mm_castpd_si128(v), 52);
		v = _mm_and_pd(v, mant_mask);
		v = _mm_or_pd(v, _mm_set1_pd(0.5));
		e = _mm_add_pd(x86_cvtepi64_pd(_mm_sub_epi64(a, _mm_set1_epi64x(0x3ff))), _mm_set1_pd(1.0));
		return v;
	}
}	 // namespace sek::math::detail
#endif

#include "f32/utility.hpp"
#include "f64/utility.hpp"
#include "i32/utility.hpp"
#include "i64/utility.hpp"