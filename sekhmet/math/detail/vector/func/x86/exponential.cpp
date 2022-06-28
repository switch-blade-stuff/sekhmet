/*
 * Created by switchblade on 26/06/22
 */

#include "exponential.hpp"

#include "arithmetic.hpp"
#include "util.hpp"

/* Implementations of exponential functions derived from netlib's cephes library (http://www.netlib.org/cephes/)
 * Inspired by http://gruntthepeon.free.fr/ssemath */

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
	SEK_FORCE_INLINE __m128 x86_pow2_ps(__m128i v) noexcept
	{
		const auto adjusted = _mm_add_epi32(v, _mm_set1_epi32(0x7f));
		return _mm_castsi128_ps(_mm_slli_epi32(adjusted, 23));
	}
	SEK_FORCE_INLINE __m128 x86_pow2_ps(__m128 v) noexcept { return x86_pow2_ps(_mm_cvtps_epi32(v)); }

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
		b = x86_floor_ps(b);														 /* b = floor(b) */

		const auto tmp = _mm_mul_ps(b, _mm_set1_ps(expc_f[0]));
		auto c = _mm_mul_ps(b, _mm_set1_ps(expc_f[1]));
		a = _mm_sub_ps(_mm_sub_ps(a, tmp), c);
		c = _mm_mul_ps(a, a);

		auto p = _mm_set1_ps(expp_f[0]);
		p = x86_fmadd_ps(p, a, _mm_set1_ps(expp_f[1])); /* p = (p * a) + expp_f[1] */
		p = x86_fmadd_ps(p, a, _mm_set1_ps(expp_f[2])); /* p = (p * a) + expp_f[2] */
		p = x86_fmadd_ps(p, a, _mm_set1_ps(expp_f[3])); /* p = (p * a) + expp_f[3] */
		p = x86_fmadd_ps(p, a, _mm_set1_ps(expp_f[4])); /* p = (p * a) + expp_f[4] */
		p = x86_fmadd_ps(p, a, _mm_set1_ps(expp_f[5])); /* p = (p * a) + expp_f[5] */
		p = x86_fmadd_ps(p, c, a);						/* p = (p * c) + a */

		p = _mm_add_ps(p, _mm_set1_ps(1.0f));
		return _mm_mul_ps(p, x86_pow2_ps(b)); /* return p * 2 ^ b */
	}

	static const float exp2p_f[6] = {
		1.535336188319500E-004f,
		1.339887440266574E-003f,
		9.618437357674640E-003f,
		5.550332471162809E-002f,
		2.402264791363012E-001f,
		6.931472028550421E-001f,
	};
	static const float exp2hi_f = 127.0f;
	static const float exp2lo_f = -127.0f;

	__m128 x86_exp2_ps(__m128 v) noexcept
	{
		/* Clamp the input. */
		auto a = _mm_max_ps(_mm_min_ps(v, _mm_set1_ps(exp2hi_f)), _mm_set1_ps(exp2lo_f));

		/* b = floor(a) */
		const auto b = x86_floor_ps(a);
		auto i = _mm_cvtps_epi32(b);
		a = _mm_sub_ps(a, b);

		const auto one = _mm_set1_ps(1.0f);
		const auto mask_half = _mm_castps_si128(_mm_cmpngt_ps(a, _mm_set1_ps(0.5f))); /* a > 0.5 */
		i = x86_blendv_epi8(_mm_add_epi32(i, _mm_set1_epi32(1)), i, mask_half);		  /* i = (a > 0.5) ? (i + 1) : i */
		a = x86_blendv_ps(_mm_sub_ps(a, one), a, _mm_castsi128_ps(mask_half)); /* a = (a > 0.5) ? (a - 1.0) : a */

		/* exp2(a) = 1.0 + xP(a) */
		auto p = _mm_set1_ps(exp2p_f[0]);
		p = x86_fmadd_ps(p, a, _mm_set1_ps(exp2p_f[1])); /* p = (p * a) + exp2p_f[1] */
		p = x86_fmadd_ps(p, a, _mm_set1_ps(exp2p_f[2])); /* p = (p * a) + exp2p_f[2] */
		p = x86_fmadd_ps(p, a, _mm_set1_ps(exp2p_f[3])); /* p = (p * a) + exp2p_f[3] */
		p = x86_fmadd_ps(p, a, _mm_set1_ps(exp2p_f[4])); /* p = (p * a) + exp2p_f[4] */
		p = x86_fmadd_ps(p, a, _mm_set1_ps(exp2p_f[5])); /* p = (p * a) + exp2p_f[5] */
		p = x86_fmadd_ps(p, a, one);					 /* p = (p * a) + 1.0 */
		p = _mm_mul_ps(p, x86_pow2_ps(i));				 /* p = p * 2 ^ i */

		return x86_blendv_ps(one, p, _mm_cmpneq_ps(a, _mm_setzero_ps())); /* return (a == 0) ? 1.0 : b */
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
		p = x86_fmadd_ps(p, a, _mm_set1_ps(logp_f[1])); /* p = (p * a) + logp_f[1] */
		p = x86_fmadd_ps(p, a, _mm_set1_ps(logp_f[2])); /* p = (p * a) + logp_f[2] */
		p = x86_fmadd_ps(p, a, _mm_set1_ps(logp_f[3])); /* p = (p * a) + logp_f[3] */
		p = x86_fmadd_ps(p, a, _mm_set1_ps(logp_f[4])); /* p = (p * a) + logp_f[4] */
		p = x86_fmadd_ps(p, a, _mm_set1_ps(logp_f[5])); /* p = (p * a) + logp_f[5] */
		p = x86_fmadd_ps(p, a, _mm_set1_ps(logp_f[6])); /* p = (p * a) + logp_f[6] */
		p = x86_fmadd_ps(p, a, _mm_set1_ps(logp_f[7])); /* p = (p * a) + logp_f[7] */
		p = x86_fmadd_ps(p, a, _mm_set1_ps(logp_f[8])); /* p = (p * a) + logp_f[8] */
		p = _mm_mul_ps(_mm_mul_ps(p, a), a2);			/* p = p * a * a2 */
		p = x86_fmadd_ps(c, _mm_set1_ps(logq_f[0]), p); /* p = (c * logq_f[0]) + p */
		p = x86_fmadd_ps(a2, _mm_set1_ps(-0.5f), p);	/* p = (a2 * -0.5) + p */

		a = x86_fmadd_ps(c, _mm_set1_ps(logq_f[1]), _mm_add_ps(a, p)); /* a = (c * logq_f[1]) + (a + p) */
		return _mm_or_ps(a, nan_mask);
	}
}	 // namespace sek::math::detail
#endif