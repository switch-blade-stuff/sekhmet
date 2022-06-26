/*
 * Created by switchblade on 26/06/22
 */

#include "trigonometric.hpp"

/* Implementations of trigonometric functions derived from netlib's cephes library (http://www.netlib.org/cephes/)
 * Inspired by http://gruntthepeon.free.fr/ssemath */

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
	constexpr static float coscof0_f = 2.443315711809948E-005f;
	constexpr static float coscof1_f = -1.388731625493765E-003f;
	constexpr static float coscof2_f = 4.166664568298827E-002f;
	constexpr static float sincof0_f = -1.9515295891E-4f;
	constexpr static float sincof1_f = 8.3321608736E-3f;
	constexpr static float sincof2_f = -1.6666654611E-1f;
	constexpr static float pi4_f = 1.27323954473516f;
	constexpr static float dp1_f = -0.78515625f;
	constexpr static float dp2_f = -2.4187564849853515625e-4f;
	constexpr static float dp3_f = -3.77489497744594108e-8f;

	__m128 x86_sin_ps(__m128 v) noexcept
	{
		const auto sign_mask = _mm_set1_ps(std::bit_cast<float>(0x8000'0000));
		const auto abs_mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));

		auto a = _mm_and_ps(v, abs_mask);			/* a = |v| */
		auto b = _mm_mul_ps(a, _mm_set1_ps(pi4_f)); /* b = a * (4 / PI) */
		auto c = _mm_cvttps_epi32(b);				/* c = (int32) b */
		/* j = (j + 1) & (~1) */
		c = _mm_add_epi32(c, _mm_set1_epi32(1));
		c = _mm_and_si128(c, _mm_set1_epi32(~1));
		b = _mm_cvtepi32_ps(c);

		const auto flag = _mm_slli_epi32(_mm_and_si128(c, _mm_set1_epi32(4)), 29);		/* Swap sign flag */
		const auto sign = _mm_xor_ps(_mm_and_ps(v, sign_mask), _mm_castsi128_ps(flag)); /* Extract sign bit */

		/* Calculate polynomial selection mask */
		c = _mm_and_si128(c, _mm_set1_epi32(2));
		c = _mm_cmpeq_epi32(c, _mm_setzero_si128());
		const auto select_mask = _mm_castsi128_ps(c);

#ifdef SEK_USE_FMA
		a = _mm_fmadd_ps(_mm_set1_ps(dp1_f), b, a); /* a = (dp1_f * b) + a */
		a = _mm_fmadd_ps(_mm_set1_ps(dp2_f), b, a); /* a = (dp2_f * b) + a */
		a = _mm_fmadd_ps(_mm_set1_ps(dp3_f), b, a); /* a = (dp3_f * b) + a */
#else
		a = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(dp1_f), b), a);
		a = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(dp2_f), b), a);
		a = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(dp3_f), b), a);
#endif
		const auto a2 = _mm_mul_ps(a, a);

		/* P1 (0 <= a <= Pi/4) */
		auto p1 = _mm_set1_ps(coscof0_f);
#ifdef SEK_USE_FMA
		p1 = _mm_fmadd_ps(p1, a2, _mm_set1_ps(coscof1_f)); /* p1 = (p1 * a2) + coscof1_f */
		p1 = _mm_fmadd_ps(p1, a2, _mm_set1_ps(coscof2_f)); /* p1 = (p1 * a2) + coscof2_f */
#else
		p1 = _mm_add_ps(_mm_mul_ps(p1, a2), _mm_set1_ps(coscof1_f));
		p1 = _mm_add_ps(_mm_mul_ps(p1, a2), _mm_set1_ps(coscof2_f));
#endif

		p1 = _mm_mul_ps(_mm_mul_ps(p1, a2), a2);

#ifdef SEK_USE_FMA
		p1 = _mm_fmadd_ps(a2, _mm_set1_ps(-0.5f), p1); /* p1 = (a2 * -0.5) + p1 */
#else
		p1 = _mm_sub_ps(p1, _mm_mul_ps(a2, _mm_set1_ps(0.5f))); /* p1 = p1 - (a2 * 0.5) */
#endif
		p1 = _mm_add_ps(p1, _mm_set1_ps(1.0f));

		/* P2  (Pi/4 <= a <= 0) */
		auto p2 = _mm_set1_ps(sincof0_f);
#ifdef SEK_USE_FMA
		p2 = _mm_fmadd_ps(p2, a2, _mm_set1_ps(sincof1_f)); /* p2 = (p2 * a2) + sincof1_f */
		p2 = _mm_fmadd_ps(p2, a2, _mm_set1_ps(sincof2_f)); /* p2 = (p2 * a2) + sincof2_f */
		p2 = _mm_fmadd_ps(_mm_mul_ps(p2, a2), a, a);	   /* p2 = ((p2 * a2) * a) + a */
#else
		p2 = _mm_add_ps(_mm_mul_ps(p2, a2), _mm_set1_ps(sincof1_f));
		p2 = _mm_add_ps(_mm_mul_ps(p2, a2), _mm_set1_ps(sincof2_f));
		p2 = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(p2, a2), a), a);
#endif

		/* Choose between P1 and P2 */
#ifdef SEK_USE_SSE4_1
		const auto result = _mm_blendv_ps(p1, p2, select_mask);
#else
		const auto result = _mm_add_ps(_mm_and_ps(select_mask, p2), _mm_andnot_ps(select_mask, p1));
#endif
		return _mm_xor_ps(result, sign);
	}
	__m128 x86_cos_ps(__m128 v) noexcept
	{
		const auto abs_mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));

		auto a = _mm_and_ps(v, abs_mask);			/* a = |v| */
		auto b = _mm_mul_ps(a, _mm_set1_ps(pi4_f)); /* b = a * (4 / PI) */
		auto c = _mm_cvttps_epi32(b);				/* c = (int32) b */

		/* j = (j + 1) & (~1) */
		c = _mm_add_epi32(c, _mm_set1_epi32(1));
		c = _mm_and_si128(c, _mm_set1_epi32(~1));
		b = _mm_cvtepi32_ps(c);
		c = _mm_sub_epi32(c, _mm_set1_epi32(2));

		const auto sign = _mm_castsi128_ps(_mm_slli_epi32(_mm_andnot_si128(c, _mm_set1_epi32(4)), 29)); /* Extract sign bit */

		/* Calculate polynomial selection mask */
		c = _mm_and_si128(c, _mm_set1_epi32(2));
		c = _mm_cmpeq_epi32(c, _mm_setzero_si128());
		const auto select_mask = _mm_castsi128_ps(c);

#ifdef SEK_USE_FMA
		a = _mm_fmadd_ps(_mm_set1_ps(dp1_f), b, a); /* a = (dp1_f * b) + a */
		a = _mm_fmadd_ps(_mm_set1_ps(dp2_f), b, a); /* a = (dp2_f * b) + a */
		a = _mm_fmadd_ps(_mm_set1_ps(dp3_f), b, a); /* a = (dp3_f * b) + a */
#else
		a = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(dp1_f), b), a);
		a = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(dp2_f), b), a);
		a = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(dp3_f), b), a);
#endif
		const auto a2 = _mm_mul_ps(a, a);

		/* P1 (0 <= a <= Pi/4) */
		auto p1 = _mm_set1_ps(coscof0_f);
#ifdef SEK_USE_FMA
		p1 = _mm_fmadd_ps(p1, a2, _mm_set1_ps(coscof1_f)); /* p1 = (p1 * a2) + coscof1_f */
		p1 = _mm_fmadd_ps(p1, a2, _mm_set1_ps(coscof2_f)); /* p1 = (p1 * a2) + coscof2_f */
#else
		p1 = _mm_add_ps(_mm_mul_ps(p1, a2), _mm_set1_ps(coscof1_f));
		p1 = _mm_add_ps(_mm_mul_ps(p1, a2), _mm_set1_ps(coscof2_f));
#endif
		p1 = _mm_mul_ps(_mm_mul_ps(p1, a2), a2);

#ifdef SEK_USE_FMA
		p1 = _mm_fmadd_ps(a2, _mm_set1_ps(-0.5f), p1); /* p1 = (a2 * -0.5) + p1 */
#else
		p1 = _mm_sub_ps(p1, _mm_mul_ps(a2, _mm_set1_ps(0.5f))); /* p1 = p1 - (a2 * 0.5) */
#endif
		p1 = _mm_add_ps(p1, _mm_set1_ps(1.0f));

		/* P2 (Pi/4 <= a <= 0) */
		auto p2 = _mm_set1_ps(sincof0_f);
#ifdef SEK_USE_FMA
		p2 = _mm_fmadd_ps(p2, a2, _mm_set1_ps(sincof1_f)); /* p2 = (p2 * a2) + sincof1_f */
		p2 = _mm_fmadd_ps(p2, a2, _mm_set1_ps(sincof2_f)); /* p2 = (p2 * a2) + sincof2_f */
		p2 = _mm_fmadd_ps(_mm_mul_ps(p2, a2), a, a);	   /* p2 = ((p2 * a2) * a) + a */
#else
		p2 = _mm_add_ps(_mm_mul_ps(p2, a2), _mm_set1_ps(sincof1_f));
		p2 = _mm_add_ps(_mm_mul_ps(p2, a2), _mm_set1_ps(sincof2_f));
		p2 = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(p2, a2), a), a);
#endif

		/* Choose between P1 and P2 */
#ifdef SEK_USE_SSE4_1
		const auto result = _mm_blendv_ps(p1, p2, select_mask);
#else
		const auto result = _mm_add_ps(_mm_and_ps(select_mask, p2), _mm_andnot_ps(select_mask, p1));
#endif
		return _mm_xor_ps(result, sign);
	}
}	 // namespace sek::math::detail
#endif