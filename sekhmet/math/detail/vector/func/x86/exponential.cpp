/*
 * Created by switchblade on 26/06/22
 */

#include "exponential.hpp"

/* Implementations of exponential functions derived from netlib's cephes library (http://www.netlib.org/cephes/)
 * Inspired by http://gruntthepeon.free.fr/ssemath */

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
	constexpr static float exphi_f = 88.3762626647949f;
	constexpr static float explo_f = -88.3762626647949f;
	constexpr static float log2e_f = 1.44269504088896341f;
	constexpr static float expc1_f = 0.693359375f;
	constexpr static float expc2_f = -2.12194440e-4f;
	constexpr static float expp0_f = 1.9875691500E-4f;
	constexpr static float expp1_f = 1.3981999507E-3f;
	constexpr static float expp2_f = 8.3334519073E-3f;
	constexpr static float expp3_f = 4.1665795894E-2f;
	constexpr static float expp4_f = 1.6666665459E-1f;
	constexpr static float expp5_f = 5.0000001201E-1f;

	__m128 x86_exp_ps(__m128 v) noexcept
	{
		auto a = _mm_max_ps(_mm_min_ps(v, _mm_set1_ps(exphi_f)), _mm_set1_ps(explo_f)); /* Clamp the input. */
		auto b = _mm_add_ps(_mm_mul_ps(a, _mm_set1_ps(log2e_f)), _mm_set1_ps(0.5f)); /* exp(x) = exp(g + n * log(2)) */

		/* b = floor(b) */
#ifndef SEK_USE_SSE4_1
		{
			const auto tmp = _mm_cvtepi32_ps(_mm_cvtps_epi32(b));
			b = _mm_sub_ps(tmp, _mm_and_ps(_mm_cmpgt_ps(tmp, b), one));
		}
#else
		b = _mm_floor_ps(b);
#endif

		const auto tmp = _mm_mul_ps(b, _mm_set1_ps(expc1_f));
		auto c = _mm_mul_ps(b, _mm_set1_ps(expc2_f));
		a = _mm_sub_ps(_mm_sub_ps(a, tmp), c);
		c = _mm_mul_ps(a, a);

		auto poly = _mm_set1_ps(expp0_f);
#ifdef SEK_USE_FMA
		poly = _mm_fmadd_ps(poly, a, _mm_set1_ps(expp1_f)); /* poly = (poly * a) + expp1_f */
		poly = _mm_fmadd_ps(poly, a, _mm_set1_ps(expp2_f)); /* poly = (poly * a) + expp2_f */
		poly = _mm_fmadd_ps(poly, a, _mm_set1_ps(expp3_f)); /* poly = (poly * a) + expp3_f */
		poly = _mm_fmadd_ps(poly, a, _mm_set1_ps(expp4_f)); /* poly = (poly * a) + expp4_f */
		poly = _mm_fmadd_ps(poly, a, _mm_set1_ps(expp5_f)); /* poly = (poly * a) + expp5_f */
		poly = _mm_fmadd_ps(poly, c, a);					/* poly = (poly * c) + a */
#else
		poly = _mm_add_ps(_mm_mul_ps(poly, a), _mm_set1_ps(expp1_f));
		poly = _mm_add_ps(_mm_mul_ps(poly, a), _mm_set1_ps(expp2_f));
		poly = _mm_add_ps(_mm_mul_ps(poly, a), _mm_set1_ps(expp3_f));
		poly = _mm_add_ps(_mm_mul_ps(poly, a), _mm_set1_ps(expp4_f));
		poly = _mm_add_ps(_mm_mul_ps(poly, a), _mm_set1_ps(expp5_f));
		poly = _mm_add_ps(_mm_mul_ps(poly, c), a);
#endif
		poly = _mm_add_ps(poly, _mm_set1_ps(1.0f));

		/* 2^n */
		const auto pow2n = _mm_slli_epi32(_mm_add_epi32(_mm_cvttps_epi32(b), _mm_set1_epi32(0x7f)), 23);
		return _mm_mul_ps(poly, _mm_castsi128_ps(pow2n));
	}

	constexpr static float sqrth_f = 0.707106781186547524f;
	constexpr static float logp0_f = 7.0376836292E-2f;
	constexpr static float logp1_f = -1.1514610310E-1f;
	constexpr static float logp2_f = 1.1676998740E-1f;
	constexpr static float logp3_f = -1.2420140846E-1f;
	constexpr static float logp4_f = +1.4249322787E-1f;
	constexpr static float logp5_f = -1.6668057665E-1f;
	constexpr static float logp6_f = +2.0000714765E-1f;
	constexpr static float logp7_f = -2.4999993993E-1f;
	constexpr static float logp8_f = +3.3333331174E-1f;
	constexpr static float logq1_f = -2.12194440e-4f;
	constexpr static float logq2_f = 0.693359375f;

	__m128 x86_log_ps(__m128 v) noexcept
	{
		const auto mant_mask = _mm_set1_ps(std::bit_cast<float>(0x807f'ffff));
		const auto min_norm = _mm_set1_ps(std::bit_cast<float>(0x0080'0000));
		const auto nan_mask = _mm_cmple_ps(v, _mm_setzero_ps());
		const auto one = _mm_set1_ps(1.0f);

		auto a = _mm_max_ps(v, min_norm); /* Must be normal. */
		auto b = _mm_srli_epi32(_mm_castps_si128(a), 23);
		a = _mm_or_ps(_mm_and_ps(a, mant_mask), _mm_set1_ps(0.5f));

		b = _mm_sub_epi32(b, _mm_set1_epi32(0x7f));
		auto c = _mm_add_ps(_mm_cvtepi32_ps(b), one);

		const auto mask = _mm_cmplt_ps(a, _mm_set1_ps(sqrth_f));
		auto tmp = _mm_and_ps(a, mask);
		c = _mm_sub_ps(c, _mm_and_ps(one, mask));
		a = _mm_add_ps(_mm_sub_ps(a, one), tmp);
		auto a2 = _mm_mul_ps(a, a);

		auto poly = _mm_set1_ps(logp0_f);
#ifdef SEK_USE_FMA
		poly = _mm_fmadd_ps(poly, a, _mm_set1_ps(logp1_f)); /* poly = (poly * a) + logp1_f */
		poly = _mm_fmadd_ps(poly, a, _mm_set1_ps(logp2_f)); /* poly = (poly * a) + logp2_f */
		poly = _mm_fmadd_ps(poly, a, _mm_set1_ps(logp3_f)); /* poly = (poly * a) + logp3_f */
		poly = _mm_fmadd_ps(poly, a, _mm_set1_ps(logp4_f)); /* poly = (poly * a) + logp4_f */
		poly = _mm_fmadd_ps(poly, a, _mm_set1_ps(logp5_f)); /* poly = (poly * a) + logp5_f */
		poly = _mm_fmadd_ps(poly, a, _mm_set1_ps(logp6_f)); /* poly = (poly * a) + logp6_f */
		poly = _mm_fmadd_ps(poly, a, _mm_set1_ps(logp7_f)); /* poly = (poly * a) + logp7_f */
		poly = _mm_fmadd_ps(poly, a, _mm_set1_ps(logp8_f)); /* poly = (poly * a) + logp8_f */
#else
		poly = _mm_add_ps(_mm_mul_ps(poly, a), _mm_set1_ps(logp1_f));
		poly = _mm_add_ps(_mm_mul_ps(poly, a), _mm_set1_ps(logp2_f));
		poly = _mm_add_ps(_mm_mul_ps(poly, a), _mm_set1_ps(logp3_f));
		poly = _mm_add_ps(_mm_mul_ps(poly, a), _mm_set1_ps(logp4_f));
		poly = _mm_add_ps(_mm_mul_ps(poly, a), _mm_set1_ps(logp5_f));
		poly = _mm_add_ps(_mm_mul_ps(poly, a), _mm_set1_ps(logp6_f));
		poly = _mm_add_ps(_mm_mul_ps(poly, a), _mm_set1_ps(logp7_f));
		poly = _mm_add_ps(_mm_mul_ps(poly, a), _mm_set1_ps(logp8_f));
#endif
		poly = _mm_mul_ps(_mm_mul_ps(poly, a), a2);

#ifdef SEK_USE_FMA
		poly = _mm_fmadd_ps(c, _mm_set1_ps(logq1_f), poly);				/* poly = (c * logq1_f) + poly */
		poly = _mm_fmadd_ps(a2, _mm_set1_ps(-0.5f), poly);				/* poly = (a2 * -0.5) + poly */
		a = _mm_fmadd_ps(c, _mm_set1_ps(logq2_f), _mm_add_ps(a, poly)); /* a = (c * logq2_f) + (a + poly) */
#else
		poly = _mm_add_ps(_mm_mul_ps(c, _mm_set1_ps(logq1_f)), poly);
		poly = _mm_sub_ps(poly, _mm_mul_ps(a2, _mm_set1_ps(0.5f)));
		a = _mm_add_ps(_mm_mul_ps(c, _mm_set1_ps(logq2_f)), _mm_add_ps(a, poly));
#endif

		return _mm_or_ps(a, nan_mask);
	}
}	 // namespace sek::math::detail
#endif