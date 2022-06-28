/*
 * Created by switchblade on 26/06/22
 */

#include "exponential.hpp"

/* Implementations of exponential functions derived from netlib's cephes library (http://www.netlib.org/cephes/)
 * Inspired by http://gruntthepeon.free.fr/ssemath */

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
	static const float expp_f[6] = {
		1.9875691500E-4f,
		1.3981999507E-3f,
		8.3334519073E-3f,
		4.1665795894E-2f,
		1.6666665459E-1f,
		5.0000001201E-1f,
	};
	static const float expc_f[2] = {0.693359375f, -2.12194440e-4f};
	static const float exphi_f = 88.3762626647949f;
	static const float explo_f = -88.3762626647949f;
	static const float log2e_f = 1.44269504088896341f;

	__m128 x86_exp_ps(__m128 v) noexcept
	{
		auto a = _mm_max_ps(_mm_min_ps(v, _mm_set1_ps(exphi_f)), _mm_set1_ps(explo_f)); /* Clamp the input. */
		auto b = _mm_add_ps(_mm_mul_ps(a, _mm_set1_ps(log2e_f)), _mm_set1_ps(0.5f)); /* exp(x) = exp(g + n * log(2)) */

		/* b = floor(b) */
#ifndef SEK_USE_SSE4_1
		{
			const auto tmp = _mm_cvtepi32_ps(_mm_cvtps_epi32(b));
			b = _mm_sub_ps(tmp, _mm_and_ps(_mm_cmpgt_ps(tmp, b), _mm_set1_ps(1.0f)));
		}
#else
		b = _mm_floor_ps(b);
#endif

		const auto tmp = _mm_mul_ps(b, _mm_set1_ps(expc_f[0]));
		auto c = _mm_mul_ps(b, _mm_set1_ps(expc_f[1]));
		a = _mm_sub_ps(_mm_sub_ps(a, tmp), c);
		c = _mm_mul_ps(a, a);

		auto poly = _mm_set1_ps(expp_f[0]);
#ifdef SEK_USE_FMA
		poly = _mm_fmadd_ps(poly, a, _mm_set1_ps(expp_f[1])); /* poly = (poly * a) + expp_f[1] */
		poly = _mm_fmadd_ps(poly, a, _mm_set1_ps(expp_f[2])); /* poly = (poly * a) + expp_f[2] */
		poly = _mm_fmadd_ps(poly, a, _mm_set1_ps(expp_f[3])); /* poly = (poly * a) + expp_f[3] */
		poly = _mm_fmadd_ps(poly, a, _mm_set1_ps(expp_f[4])); /* poly = (poly * a) + expp_f[4] */
		poly = _mm_fmadd_ps(poly, a, _mm_set1_ps(expp_f[5])); /* poly = (poly * a) + expp_f[5] */
		poly = _mm_fmadd_ps(poly, c, a);					  /* poly = (poly * c) + a */
#else
		poly = _mm_add_ps(_mm_mul_ps(poly, a), _mm_set1_ps(expp_f[1]));
		poly = _mm_add_ps(_mm_mul_ps(poly, a), _mm_set1_ps(expp_f[2]));
		poly = _mm_add_ps(_mm_mul_ps(poly, a), _mm_set1_ps(expp_f[3]));
		poly = _mm_add_ps(_mm_mul_ps(poly, a), _mm_set1_ps(expp_f[4]));
		poly = _mm_add_ps(_mm_mul_ps(poly, a), _mm_set1_ps(expp_f[5]));
		poly = _mm_add_ps(_mm_mul_ps(poly, c), a);
#endif
		poly = _mm_add_ps(poly, _mm_set1_ps(1.0f));

		/* 2^n */
		const auto pow2n = _mm_slli_epi32(_mm_add_epi32(_mm_cvttps_epi32(b), _mm_set1_epi32(0x7f)), 23);
		return _mm_mul_ps(poly, _mm_castsi128_ps(pow2n));
	}

	static const float logp_f[9] = {
		7.0376836292E-2f,
		-1.1514610310E-1f,
		1.1676998740E-1f,
		-1.2420140846E-1f,
		+1.4249322787E-1f,
		-1.6668057665E-1f,
		+2.0000714765E-1f,
		-2.4999993993E-1f,
		+3.3333331174E-1f,
	};
	static const float logq_f[2] = {-2.12194440e-4f, 0.693359375f};
	static const float sqrth_f = 0.707106781186547524f;

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

		auto p = _mm_set1_ps(logp_f[0]);
#ifdef SEK_USE_FMA
		p = _mm_fmadd_ps(p, a, _mm_set1_ps(logp_f[1])); /* p = (p * a) + logp_f[1] */
		p = _mm_fmadd_ps(p, a, _mm_set1_ps(logp_f[2])); /* p = (p * a) + logp_f[2] */
		p = _mm_fmadd_ps(p, a, _mm_set1_ps(logp_f[3])); /* p = (p * a) + logp_f[3] */
		p = _mm_fmadd_ps(p, a, _mm_set1_ps(logp_f[4])); /* p = (p * a) + logp_f[4] */
		p = _mm_fmadd_ps(p, a, _mm_set1_ps(logp_f[5])); /* p = (p * a) + logp_f[5] */
		p = _mm_fmadd_ps(p, a, _mm_set1_ps(logp_f[6])); /* p = (p * a) + logp_f[6] */
		p = _mm_fmadd_ps(p, a, _mm_set1_ps(logp_f[7])); /* p = (p * a) + logp_f[7] */
		p = _mm_fmadd_ps(p, a, _mm_set1_ps(logp_f[8])); /* p = (p * a) + logp_f[8] */
#else
		p = _mm_add_ps(_mm_mul_ps(p, a), _mm_set1_ps(logp_f[1]));
		p = _mm_add_ps(_mm_mul_ps(p, a), _mm_set1_ps(logp_f[2]));
		p = _mm_add_ps(_mm_mul_ps(p, a), _mm_set1_ps(logp_f[3]));
		p = _mm_add_ps(_mm_mul_ps(p, a), _mm_set1_ps(logp_f[4]));
		p = _mm_add_ps(_mm_mul_ps(p, a), _mm_set1_ps(logp_f[5]));
		p = _mm_add_ps(_mm_mul_ps(p, a), _mm_set1_ps(logp_f[6]));
		p = _mm_add_ps(_mm_mul_ps(p, a), _mm_set1_ps(logp_f[7]));
		p = _mm_add_ps(_mm_mul_ps(p, a), _mm_set1_ps(logp_f[8]));
#endif
		p = _mm_mul_ps(_mm_mul_ps(p, a), a2);

#ifdef SEK_USE_FMA
		p = _mm_fmadd_ps(c, _mm_set1_ps(logq_f[0]), p);				   /* p = (c * logq_f[0]) + p */
		p = _mm_fmadd_ps(a2, _mm_set1_ps(-0.5f), p);				   /* p = (a2 * -0.5) + p */
		a = _mm_fmadd_ps(c, _mm_set1_ps(logq_f[1]), _mm_add_ps(a, p)); /* a = (c * logq_f[1]) + (a + p) */
#else
		p = _mm_add_ps(_mm_mul_ps(c, _mm_set1_ps(logq_f[0])), p);
		p = _mm_sub_ps(p, _mm_mul_ps(a2, _mm_set1_ps(0.5f)));
		a = _mm_add_ps(_mm_mul_ps(c, _mm_set1_ps(logq_f[1])), _mm_add_ps(a, p));
#endif

		return _mm_or_ps(a, nan_mask);
	}
}	 // namespace sek::math::detail
#endif