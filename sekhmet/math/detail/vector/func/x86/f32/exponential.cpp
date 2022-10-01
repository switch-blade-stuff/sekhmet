/*
 * Created by switchblade on 26/06/22
 */

#include "exponential.hpp"

#include "../arithmetic.hpp"
#include "../utility.hpp"

/* Implementations of exponential functions derived from netlib's cephes library (http://www.netlib.org/cephes/)
 * Inspired by http://gruntthepeon.free.fr/ssemath */

#ifdef SEK_USE_SSE2
namespace sek::detail
{
	static const float expp_f[6] = {
		1.9875691500e-4f,
		1.3981999507e-3f,
		8.3334519073e-3f,
		4.1665795894e-2f,
		1.6666665459e-1f,
		5.0000001201e-1f,
	};
	static const float expc_f[2] = {0.693359375f, -2.12194440e-4f};
	static const float exphi_f = 88.3762626647949f;
	static const float explo_f = -103.278929903431851103f;
	static const float log2e_f = 1.44269504088896341f;

	__m128 x86_exp_ps(__m128 v) noexcept
	{
		auto a = _mm_max_ps(_mm_min_ps(v, _mm_set1_ps(exphi_f)), _mm_set1_ps(explo_f)); /* Clamp the input. */

		/* exp(x) = exp(g + n * log(2)) */
		auto b = _mm_add_ps(_mm_mul_ps(a, _mm_set1_ps(log2e_f)), _mm_set1_ps(0.5f));
		b = x86_floor_ps(b); /* b = floor(b) */

		const auto tmp1 = _mm_mul_ps(b, _mm_set1_ps(expc_f[0]));
		const auto tmp2 = _mm_mul_ps(b, _mm_set1_ps(expc_f[1]));
		a = _mm_sub_ps(_mm_sub_ps(a, tmp1), tmp2);
		const auto a2 = _mm_mul_ps(a, a);
		const auto p = x86_fmadd_ps(x86_polevl_ps(a, expp_f), a2, a);		 /* p = (expp_f(a) * a2) + a */
		return _mm_mul_ps(_mm_add_ps(p, _mm_set1_ps(1.0f)), x86_pow2_ps(b)); /* return (p + 1) * 2 ^ b */
	}

	static const float exp2p_f[6] = {
		1.535336188319500e-4f,
		1.339887440266574e-3f,
		9.618437357674640e-3f,
		5.550332471162809e-2f,
		2.402264791363012e-1f,
		6.931472028550421e-1f,
	};
	static const float exp2hi_f = 127.0f;
	static const float exp2lo_f = -127.0f;

	__m128 x86_exp2_ps(__m128 v) noexcept
	{
		auto a = v = _mm_max_ps(_mm_min_ps(v, _mm_set1_ps(exp2hi_f)), _mm_set1_ps(exp2lo_f)); /* Clamp the input. */

		/* b = floor(a) */
		const auto b = x86_floor_ps(a);
		auto i = _mm_cvtps_epi32(b);
		a = _mm_sub_ps(a, b);

		const auto one = _mm_set1_ps(1.0f);
		const auto mask_half = _mm_castps_si128(_mm_cmpngt_ps(a, _mm_set1_ps(0.5f))); /* a > 0.5 */
		i = x86_blendv_epi8(_mm_add_epi32(i, _mm_set1_epi32(1)), i, mask_half);		  /* i = (a > 0.5) ? (i + 1) : i */
		a = x86_blendv_ps(_mm_sub_ps(a, one), a, _mm_castsi128_ps(mask_half)); /* a = (a > 0.5) ? (a - 1.0) : a */

		auto p = x86_fmadd_ps(x86_polevl_ps(a, exp2p_f), a, one);		  /* p = (exp2p_f(a) * a) + 1.0 */
		p = _mm_mul_ps(p, x86_pow2_ps(i));								  /* p = p * 2 ^ i */
		return x86_blendv_ps(one, p, _mm_cmpneq_ps(v, _mm_setzero_ps())); /* return (v == 0) ? 1.0 : p */
	}

	static const float logp_f[9] = {
		7.0376836292e-2f,
		-1.1514610310e-1f,
		1.1676998740e-1f,
		-1.2420140846e-1f,
		1.4249322787e-1f,
		-1.6668057665e-1f,
		2.0000714765e-1f,
		-2.4999993993e-1f,
		3.3333331174e-1f,
	};
	static const float logq_f[2] = {-2.12194440e-4f, 0.693359375f};
	static const float sqrth_f = 0.70710678118654752440f;

	__m128 x86_log_ps(__m128 v) noexcept
	{
		const auto min_norm = _mm_set1_ps(std::bit_cast<float>(0x0080'0000));
		const auto nan_mask = _mm_cmple_ps(v, _mm_setzero_ps());
		__m128 e, a = x86_frexp_ps(_mm_max_ps(v, min_norm), e);

		const auto one = _mm_set1_ps(1.0f);
		const auto mask = _mm_cmplt_ps(a, _mm_set1_ps(sqrth_f));
		a = _mm_sub_ps(_mm_add_ps(a, _mm_and_ps(a, mask)), one); /* a = a + (a & mask) - 1 */
		e = _mm_sub_ps(e, _mm_and_ps(one, mask));				 /* e = e - 1 & mask */

		const auto a2 = _mm_mul_ps(a, a);
		auto b = _mm_mul_ps(_mm_mul_ps(x86_polevl_ps(a, logp_f), a), a2); /* b = (logp_f(a) * a) * a2 */
		b = x86_fmadd_ps(e, _mm_set1_ps(logq_f[0]), b);					  /* b = (e * logq_f[0]) + b */
		b = x86_fmadd_ps(a2, _mm_set1_ps(-0.5f), b);					  /* b = (a2 * -0.5) + b */
		a = x86_fmadd_ps(e, _mm_set1_ps(logq_f[1]), _mm_add_ps(a, b));	  /* a = (e * logq_f[1]) + (a + b) */
		return _mm_or_ps(a, nan_mask);
	}

	static const float l2ea_f = 0.44269504088896340735992f;

	__m128 x86_log2_ps(__m128 v) noexcept
	{
		const auto min_norm = _mm_set1_ps(std::bit_cast<float>(0x0080'0000));
		const auto nan_mask = _mm_cmple_ps(v, _mm_setzero_ps());
		__m128 e, a = x86_frexp_ps(_mm_max_ps(v, min_norm), e);

		const auto one = _mm_set1_ps(1.0f);
		const auto mask = _mm_cmplt_ps(a, _mm_set1_ps(sqrth_f));
		a = _mm_sub_ps(_mm_add_ps(a, _mm_and_ps(a, mask)), one); /* a = a + (a & mask) - 1 */
		e = _mm_sub_ps(e, _mm_and_ps(one, mask));				 /* c = c - 1 & mask */

		const auto a2 = _mm_mul_ps(a, a);
		auto b = _mm_mul_ps(_mm_mul_ps(x86_polevl_ps(a, logp_f), a), a2); /* b = (logp_f(a) * a) * a2 */
		b = x86_fmadd_ps(a2, _mm_set1_ps(-0.5f), b);					  /* p = (a2 * -0.5) + p */

		const auto l2ea = _mm_set1_ps(l2ea_f);
		auto c = _mm_add_ps(_mm_mul_ps(b, l2ea), _mm_mul_ps(a, l2ea));
		c = _mm_add_ps(_mm_add_ps(_mm_add_ps(a, b), e), c);
		return _mm_or_ps(c, nan_mask);
	}

	static const float l102a_f = 3.0078125e-1f;
	static const float l102b_f = 2.48745663981195213739e-4f;
	static const float l10ea_f = 4.3359375e-1f;
	static const float l10eb_f = 7.00731903251827651129e-4f;

	__m128 x86_log10_ps(__m128 v) noexcept
	{
		const auto min_norm = _mm_set1_ps(std::bit_cast<float>(0x0080'0000));
		const auto nan_mask = _mm_cmple_ps(v, _mm_setzero_ps());
		__m128 e, a = x86_frexp_ps(_mm_max_ps(v, min_norm), e);

		const auto one = _mm_set1_ps(1.0f);
		const auto mask = _mm_cmple_ps(a, _mm_set1_ps(sqrth_f));
		a = _mm_sub_ps(_mm_add_ps(a, _mm_and_ps(a, mask)), one); /* a = a + (a & mask) - offset */
		e = _mm_sub_ps(e, _mm_and_ps(one, mask));				 /* e = e - 1 & mask */

		const auto a2 = _mm_mul_ps(a, a);
		auto b = _mm_mul_ps(_mm_mul_ps(x86_polevl_ps(a, logp_f), a2), a); /* b = (logp_f(a) * a2) * a */
		b = x86_fmadd_ps(a2, _mm_set1_ps(-0.5f), b);					  /* b = (a2 * -0.5) + b */

		auto c = _mm_mul_ps(_mm_add_ps(a, b), _mm_set1_ps(l10eb_f)); /* c = (a + b) * l10eb_f */
		c = x86_fmadd_ps(b, _mm_set1_ps(l10ea_f), c);				 /* c = (b * l10ea_f) + c */
		c = x86_fmadd_ps(a, _mm_set1_ps(l10ea_f), c);				 /* c = (a * l10ea_f) + c */
		c = x86_fmadd_ps(e, _mm_set1_ps(l102b_f), c);				 /* c = (e * l102b_f) + c */
		c = x86_fmadd_ps(e, _mm_set1_ps(l102a_f), c);				 /* c = (e * l102a_f) + c */
		return _mm_or_ps(c, nan_mask);
	}
}	 // namespace sek::detail
#endif