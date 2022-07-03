/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "f32/utility.hpp"
#include "f64/utility.hpp"
#include "i32/utility.hpp"
#include "i64/utility.hpp"

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
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
}	 // namespace sek::math::detail
#endif