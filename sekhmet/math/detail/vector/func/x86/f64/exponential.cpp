/*
 * Created by switchblade on 04/07/22
 */

#include "exponential.hpp"

#include "../arithmetic.hpp"

/* Implementations of exponential functions derived from netlib's cephes library (http://www.netlib.org/cephes/)
 * Inspired by http://gruntthepeon.free.fr/ssemath */

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
	static const double expq_d[4] = {
		3.00198505138664455042e-6,
		2.52448340349684104192e-3,
		2.27265548208155028766e-1,
		2.00000000000000000009e0,
	};
	static const double expp_d[3] = {1.26177193074810590878e-4, 3.02994407707441961300e-2, 9.99999999999999999910e-1};
	static const double expc_d[2] = {6.93145751953125e-1, 1.42860682030941723212e-6};
	static const double exphi_d = 709.78271289338399684324569237317280570931213758490138021957632972;
	static const double explo_d = -708.3964185322641062244112281302564525731611373161808697113349697;
	static const double log2e_d = 1.4426950408889634073599;

	__m128d x86_exp_pd(__m128d v) noexcept
	{
		/* Clamp the input. */
		auto a = _mm_max_pd(_mm_min_pd(v, _mm_set1_pd(exphi_d)), _mm_set1_pd(explo_d));

		/* exp(x) = exp(g + n * log(2)) */
		auto b = _mm_add_pd(_mm_mul_pd(a, _mm_set1_pd(log2e_d)), _mm_set1_pd(0.5));
		b = x86_floor_pd(b); /* b = floor(b) */

		const auto tmp1 = _mm_mul_pd(b, _mm_set1_pd(expc_d[0]));
		const auto tmp2 = _mm_mul_pd(b, _mm_set1_pd(expc_d[1]));
		a = _mm_sub_pd(_mm_sub_pd(a, tmp1), tmp2);
		const auto a2 = _mm_mul_pd(a, a);

		auto p = _mm_set1_pd(expp_d[0]);
		p = x86_fmadd_pd(p, a2, _mm_set1_pd(expp_d[1])); /* p = (p * a2) + expp_d[1] */
		p = x86_fmadd_pd(p, a2, _mm_set1_pd(expp_d[2])); /* p = (p * a2) + expp_d[2] */
		a = _mm_mul_pd(p, a);							 /* a = p * a */

		p = _mm_set1_pd(expq_d[0]);
		p = x86_fmadd_pd(p, a2, _mm_set1_pd(expq_d[1])); /* p = (p * a2) + expq_d[1] */
		p = x86_fmadd_pd(p, a2, _mm_set1_pd(expq_d[2])); /* p = (p * a2) + expq_d[2] */
		p = x86_fmadd_pd(p, a2, _mm_set1_pd(expq_d[3])); /* p = (p * a2) + expq_d[3] */

		p = _mm_mul_pd(_mm_div_pd(a, _mm_sub_pd(p, a)), _mm_set1_pd(2.0));	/* p = (a / (p - a)) * 2 */
		return _mm_mul_pd(_mm_add_pd(p, _mm_set1_pd(1.0)), x86_pow2_pd(b)); /* return (1.0 + p) * 2 ^ b */
	}

	static const double exp2p_d[3] = {2.30933477057345225087e-2, 2.02020656693165307700e1, 1.51390680115615096133e3};
	static const double exp2q_d[2] = {2.33184211722314911771e2, 4.36821166879210612817e3};
	static const double exp2hi_d = 1024.0;
	static const double exp2lo_d = -1024.0;

	__m128d x86_exp2_pd(__m128d v) noexcept
	{
		auto a = v = _mm_max_pd(_mm_min_pd(v, _mm_set1_pd(exp2hi_d)), _mm_set1_pd(exp2lo_d)); /* Clamp the input. */

		/* b = floor(a) */
		const auto b = x86_floor_pd(_mm_add_pd(a, _mm_set1_pd(0.5)));
		a = _mm_sub_pd(a, b);
		const auto a2 = _mm_mul_pd(a, a);

		auto p = _mm_set1_pd(exp2p_d[0]);
		p = x86_fmadd_pd(p, a2, _mm_set1_pd(exp2p_d[1])); /* p = (p * a2) + exp2p_d[1] */
		p = x86_fmadd_pd(p, a2, _mm_set1_pd(exp2p_d[2])); /* p = (p * a2) + exp2p_d[2] */
		a = _mm_mul_pd(p, a);							  /* a = p * a */

		p = _mm_set1_pd(1.0);
		p = x86_fmadd_pd(p, a2, _mm_set1_pd(exp2q_d[0])); /* p = (p * a2) + exp2q_d[1] */
		p = x86_fmadd_pd(p, a2, _mm_set1_pd(exp2q_d[1])); /* p = (p * a2) + exp2q_d[2] */
		p = _mm_div_pd(a, _mm_sub_pd(p, a));			  /* p = a / (p - a) */

		p = _mm_add_pd(_mm_add_pd(p, p), _mm_set1_pd(1.0)); /* p = (p + p) + 1 */
		return _mm_mul_pd(p, x86_pow2_pd(b));				/* p * 2 ^ b */
	}

	static const double logp_d[6] = {
		1.01875663804580931796e-4,
		4.97494994976747001425e-1,
		4.70579119878881725854e0,
		1.44989225341610930846e1,
		1.79368678507819816313e1,
		7.70838733755885391666e0,
	};
	static const double logq_d[5] = {
		1.12873587189167450590e1,
		4.52279145837532221105e1,
		8.29875266912776603211e1,
		7.11544750618563894466e1,
		2.31251620126765340583e1,
	};
	static const double logr_d[3] = {-7.89580278884799154124e-1, 1.63866645699558079767e1, -6.41409952958715622951e1};
	static const double logs_d[3] = {-3.56722798256324312549e1, 3.12093766372244180303e2, -7.69691943550460008604e2};
	static const double logc_d[2] = {-2.121944400546905827679e-4, 0.693359375};
	static const double sqrth_d = 0.7071067811865475244008443621048490392848359376884740365883398689;

	__m128d x86_log_pd(__m128d v) noexcept
	{
		const auto min_norm = _mm_set1_pd(std::bit_cast<double>(0x10'0000'0000'0000));
		const auto nan_mask = _mm_cmple_pd(v, _mm_setzero_pd());
		const auto half = _mm_set1_pd(0.5);
		const auto one = _mm_set1_pd(1.0);
		__m128d e, a = x86_frexp_pd(_mm_max_pd(v, min_norm), e);

		/* Polynomial selection mask (e > 2 || e < -2) and (a < sqrth_d) mask. */
		const auto select_mask = _mm_or_pd(_mm_cmpgt_pd(e, _mm_set1_pd(2.0)), _mm_cmplt_pd(e, _mm_set1_pd(-2.0)));
		const auto sqrth_mask = _mm_cmplt_pd(a, _mm_set1_pd(sqrth_d));
		e = _mm_sub_pd(e, _mm_and_pd(sqrth_mask, one)); /* e = e - 1 & (a < sqrth_d) */

		/* P1 (e <= 2 && e >= -2) */
		const auto a_p1 = _mm_sub_pd(_mm_add_pd(a, _mm_and_pd(sqrth_mask, a)), one); /* a_p1 = a + a & (a < sqrth_d) - 1 */
		const auto a2_p1 = _mm_mul_pd(a_p1, a_p1);

		auto p1_p = _mm_set1_pd(logp_d[0]);
		p1_p = x86_fmadd_pd(p1_p, a_p1, _mm_set1_pd(logp_d[1])); /* p1_p = (p1_p * a_p1) + logp_d[1] */
		p1_p = x86_fmadd_pd(p1_p, a_p1, _mm_set1_pd(logp_d[2])); /* p1_p = (p1_p * a_p1) + logp_d[2] */
		p1_p = x86_fmadd_pd(p1_p, a_p1, _mm_set1_pd(logp_d[3])); /* p1_p = (p1_p * a_p1) + logp_d[3] */
		p1_p = x86_fmadd_pd(p1_p, a_p1, _mm_set1_pd(logp_d[4])); /* p1_p = (p1_p * a_p1) + logp_d[4] */
		p1_p = x86_fmadd_pd(p1_p, a_p1, _mm_set1_pd(logp_d[5])); /* p1_p = (p1_p * a_p1) + logp_d[5] */
		p1_p = _mm_mul_pd(_mm_mul_pd(p1_p, a2_p1), a_p1);		 /* p1_p = p1_p * a2_p1 * a_p1 */
		auto p1_q = one;
		p1_q = x86_fmadd_pd(p1_q, a_p1, _mm_set1_pd(logq_d[0])); /* p1_q = (p1_q * a_p1) + logq_d[0] */
		p1_q = x86_fmadd_pd(p1_q, a_p1, _mm_set1_pd(logq_d[1])); /* p1_q = (p1_q * a_p1) + logq_d[1] */
		p1_q = x86_fmadd_pd(p1_q, a_p1, _mm_set1_pd(logq_d[2])); /* p1_q = (p1_q * a_p1) + logq_d[2] */
		p1_q = x86_fmadd_pd(p1_q, a_p1, _mm_set1_pd(logq_d[3])); /* p1_q = (p1_q * a_p1) + logq_d[3] */
		p1_q = x86_fmadd_pd(p1_q, a_p1, _mm_set1_pd(logq_d[4])); /* p1_q = (p1_q * a_p1) + logq_d[4] */

		const auto e_p1_nz = _mm_and_pd(e, _mm_cmpneq_pd(e, _mm_setzero_pd()));
		auto p1 = _mm_div_pd(p1_p, p1_q);								   /* p1 = p1_p / p1_q */
		p1 = x86_fmadd_pd(e_p1_nz, _mm_set1_pd(logc_d[0]), p1);			   /* p1 = (e_p1_nz * logc_d[0]) + p1 */
		p1 = _mm_add_pd(p1, x86_fmadd_pd(a2_p1, _mm_set1_pd(-0.5), a_p1)); /* p1 = p1 + (a2_p1 * -0.5) + a_p1 */
		p1 = x86_fmadd_pd(e_p1_nz, _mm_set1_pd(logc_d[1]), p1);			   /* p1 = (e_p1_nz * logc_d[1]) + p1 */

		/* P2 (e > 2 || e < -2) */
		const auto s1 = x86_blendv_pd(half, _mm_set1_pd(0.25), sqrth_mask); /* s1 = (a < sqrth_d) ? 0.25 : 0.5 */
		const auto s2 = x86_blendv_pd(one, half, sqrth_mask);				/* s2 = (a < sqrth_d) ? 0.5 : 1.0 */
		const auto a_p2 = _mm_div_pd(_mm_sub_pd(a, s2), x86_fmadd_pd(a, half, s1)); /* a_p2 = (a - s2) / ((0.5 * a) + s1) */
		const auto a2_p2 = _mm_mul_pd(a_p2, a_p2);

		auto p2_r = _mm_set1_pd(logr_d[0]);
		p2_r = x86_fmadd_pd(p2_r, a2_p2, _mm_set1_pd(logr_d[1])); /* p2_r = (p2_r * a2_p2) + logr_d[1] */
		p2_r = x86_fmadd_pd(p2_r, a2_p2, _mm_set1_pd(logr_d[2])); /* p2_r = (p2_r * a2_p2) + logr_d[2] */
		p2_r = _mm_mul_pd(_mm_mul_pd(p2_r, a2_p2), a_p2);		  /* p2_r = p2_r * a2_p2 * a_p2 */

		auto p2_s = one;
		p2_s = x86_fmadd_pd(p2_s, a2_p2, _mm_set1_pd(logs_d[0])); /* p2_s = (p2_s * a2_p2) + logs_d[0] */
		p2_s = x86_fmadd_pd(p2_s, a2_p2, _mm_set1_pd(logs_d[1])); /* p2_s = (p2_s * a2_p2) + logs_d[1] */
		p2_s = x86_fmadd_pd(p2_s, a2_p2, _mm_set1_pd(logs_d[2])); /* p2_s = (p2_s * a2_p2) + logs_d[2] */

		auto p2 = _mm_div_pd(p2_r, p2_s);
		p2 = x86_fmadd_pd(e, _mm_set1_pd(logc_d[0]), p2); /* p2 = (e * logc_d[0]) + p2 */
		p2 = _mm_add_pd(p2, a_p2);						  /* p2 = p2 + a_p2 */
		p2 = x86_fmadd_pd(e, _mm_set1_pd(logc_d[1]), p2); /* p2 = (e * logc_d[1]) + p2 */

		return _mm_xor_pd(x86_blendv_pd(p1, p2, select_mask), nan_mask); /* return (e > 2 || e < -2) ? p2 : p1 */
	}
	//	__m128d x86_log2_pd(__m128d v) noexcept {}
	//	__m128d x86_log10_pd(__m128d v) noexcept {}
}	 // namespace sek::math::detail
#endif