/*
 * Created by switchblade on 26/06/22
 */

#include "util.hpp"

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
#ifndef SEK_USE_SSE4_1
	__m128d x86_floor_pd(__m128d v) noexcept
	{
		const auto exp52 = _mm_set1_pd(0x0010000000000000);
		const auto mask = _mm_cmpnlt_pd(v, exp52);

		const auto magic = _mm_set1_pd(std::bit_cast<double>(0x4338000000000000));
		const auto a = _mm_sub_pd(_mm_add_pd(v, magic), magic);
		const auto b = _mm_and_pd(_mm_cmplt_pd(v, a), _mm_set1_pd(1.0));
		const auto result = _mm_sub_pd(a, b);

		return _mm_or_pd(_mm_and_pd(mask, v), _mm_andnot_pd(mask, result));
	}
#endif

#ifndef SEK_USE_AVX512_DQ
	__m128i x86_cvtpd_epu64(__m128d v) noexcept
	{
		const auto offset = _mm_set1_pd(0x0010000000000000);
		return _mm_xor_si128(_mm_castpd_si128(_mm_add_pd(v, offset)), _mm_castpd_si128(offset));
	}
	__m128i x86_cvtpd_epi64(__m128d v) noexcept
	{
		const auto offset = _mm_set1_pd(0x0018000000000000);
		return _mm_sub_epi64(_mm_castpd_si128(_mm_add_pd(v, offset)), _mm_castpd_si128(offset));
	}
	__m128d x86_cvtepu64_pd(__m128i v) noexcept
	{
		const auto exp84 = _mm_castpd_si128(_mm_set1_pd(19342813113834066795298816.)); /* 2^84 */
		const auto exp52 = _mm_castpd_si128(_mm_set1_pd(0x0010000000000000));		   /* 2^52 */
		const auto adjust = _mm_set1_pd(19342813118337666422669312.);				   /* 2^84 + 2^52 */

		const auto a = _mm_or_si128(_mm_srli_epi64(v, 32), exp84);
#ifdef SEK_USE_SSE4_1
		const auto b = _mm_blend_epi16(v, exp52, 0xcc);
#else
		const auto mask = _mm_set1_epi64x(static_cast<std::int64_t>(0xffff'ffff'0000'0000));
		const auto b = _mm_or_si128(_mm_and_si128(mask, exp52), _mm_andnot_si128(mask, v));
#endif
		return _mm_add_pd(_mm_sub_pd(_mm_castsi128_pd(a), adjust), _mm_castsi128_pd(b));
	}
	__m128d x86_cvtepi64_pd(__m128i v) noexcept
	{
		const auto exp67m3 = _mm_castpd_si128(_mm_set1_pd(442721857769029238784.)); /* 2^67 * 3 */
		const auto exp52 = _mm_castpd_si128(_mm_set1_pd(0x0010000000000000));		/* 2^52 */
		const auto adjust = _mm_set1_pd(442726361368656609280.);					/* 2^67 * 3 + 2^52 */

		const auto tmp = _mm_srai_epi32(v, 16);
#ifdef SEK_USE_SSE4_1
		auto a = _mm_blend_epi16(tmp, _mm_setzero_si128(), 0x33);
#else
		auto mask = _mm_set1_epi64x(static_cast<std::int64_t>(0x0000'0000'ffff'ffff));
		auto a = _mm_or_si128(_mm_and_si128(mask, _mm_setzero_si128()), _mm_andnot_si128(mask, tmp));
#endif

#ifdef SEK_USE_SSE4_1
		auto b = _mm_blend_epi16(v, exp52, 0x88);
#else
		mask = _mm_set1_epi64x(static_cast<std::int64_t>(0xffff'0000'0000'0000));
		const auto b = _mm_or_si128(_mm_and_si128(mask, exp52), _mm_andnot_si128(mask, v));
#endif

		return _mm_add_pd(_mm_sub_pd(_mm_castsi128_pd(_mm_add_epi64(a, exp67m3)), adjust), _mm_castsi128_pd(b));
	}
#endif
}	 // namespace sek::math::detail
#endif