/*
 * Created by switchblade on 26/06/22
 */

#include "trigonometric.hpp"

#include "../arithmetic.hpp"
#include "../exponential.hpp"

/* Implementations of trigonometric functions derived from netlib's cephes library (http://www.netlib.org/cephes/)
 * Inspired by http://gruntthepeon.free.fr/ssemath */

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
	static const double sincof_d[6] = {
		1.58962301576546568060e-10,
		-2.50507477628578072866e-8,
		2.75573136213857245213e-6,
		-1.98412698295895385996e-4,
		8.33333333332211858878e-3,
		-1.66666666666666307295e-1,
	};
	static const double coscof_d[6] = {
		-1.13585365213876817300e-11,
		2.08757008419747316778e-9,
		-2.75573141792967388112e-7,
		2.48015872888517045348e-5,
		-1.38888888888730564116e-3,
		4.16666666666665929218e-2,
	};
	static const double dp_sincos_d[3] = {-7.85398125648498535156e-1, -3.77489470793079817668e-8, -2.69515142907905952645e-15};
	static const double fopi_d = 4.0 / std::numbers::pi_v<double>; /* 4 / Pi */
	static const double pio2_d = std::numbers::pi_v<double> / 2.0; /* Pi / 2 */
	static const double pio4_d = std::numbers::pi_v<double> / 4.0; /* Pi / 4 */

	__m128d x86_sin_pd(__m128d v) noexcept
	{
		const auto sign_mask = _mm_set1_pd(std::bit_cast<double>(0x8000'0000'0000'0000));
		const auto abs_mask = _mm_set1_pd(std::bit_cast<double>(0x7fff'ffff'ffff'ffff));

		auto a = _mm_and_pd(v, abs_mask);			 /* a = |v| */
		auto b = _mm_mul_pd(a, _mm_set1_pd(fopi_d)); /* b = a * (4 / PI) */
		auto c = x86_cvtpd_epi64(b);				 /* c = (int64) b */

		/* c = (c + 1) & (~1) */
		c = _mm_add_epi64(c, _mm_set1_epi64x(1));
		c = _mm_and_si128(c, _mm_set1_epi64x(~1));
		b = x86_cvtepi64_pd(c);

		const auto sign_bit = _mm_and_pd(v, sign_mask);
		const auto flag = _mm_slli_epi64(_mm_and_si128(c, _mm_set1_epi64x(4)), 61);
		const auto sign = _mm_xor_pd(sign_bit, _mm_castsi128_pd(flag)); /* Extract sign bit */

		/* Calculate polynomial selection mask */
		c = _mm_and_si128(c, _mm_set1_epi64x(2));
#ifndef SEK_USE_SSE4_1
		c = _mm_or_si128(c, _mm_slli_epi64(c, 32)); /* c |= c << 32 */
		c = _mm_cmpeq_epi32(c, _mm_setzero_si128());
#else
		c = _mm_cmpeq_epi64(c, _mm_setzero_si128());
#endif
		const auto select_mask = _mm_castsi128_pd(c);

		a = x86_fmadd_pd(_mm_set1_pd(dp_sincos_d[0]), b, a); /* a = (dp_sincos_d[0] * b) + a */
		a = x86_fmadd_pd(_mm_set1_pd(dp_sincos_d[1]), b, a); /* a = (dp_sincos_d[1] * b) + a */
		a = x86_fmadd_pd(_mm_set1_pd(dp_sincos_d[2]), b, a); /* a = (dp_sincos_d[2] * b) + a */
		const auto a2 = _mm_mul_pd(a, a);

		/* P1 (Pi/4 <= a <= 0) */
		auto p1 = x86_polevl_pd(a2, coscof_d);		  /* p1 = coscof_d(a2) */
		p1 = _mm_mul_pd(_mm_mul_pd(p1, a2), a2);	  /* p1 = p1 * a2 * a2 */
		p1 = x86_fmadd_pd(a2, _mm_set1_pd(-0.5), p1); /* p1 = (a2 * -0.5) + p1 */
		p1 = _mm_add_pd(p1, _mm_set1_pd(1.0));

		/* P2 (0 <= a <= Pi/4) */
		auto p2 = _mm_mul_pd(x86_polevl_pd(a2, sincof_d), a2); /* p2 = sincof_d(a2) * a2 */
		p2 = x86_fmadd_pd(p2, a, a);						   /* p2 = ((p2 * a2) * a) + a */

		/* Choose between P1 and P2 */
		return _mm_xor_pd(x86_blendv_pd(p1, p2, select_mask), sign);
	}
	__m128d x86_cos_pd(__m128d v) noexcept
	{
		const auto abs_mask = _mm_set1_pd(std::bit_cast<double>(0x7fff'ffff'ffff'ffff));

		auto a = _mm_and_pd(v, abs_mask);			 /* a = |v| */
		auto b = _mm_mul_pd(a, _mm_set1_pd(fopi_d)); /* b = a * (4 / PI) */
		auto c = x86_cvtpd_epi64(b);				 /* c = (int64) b */

		/* c = (c + 1) & (~1) */
		c = _mm_add_epi64(c, _mm_set1_epi64x(1));
		c = _mm_and_si128(c, _mm_set1_epi64x(~1));
		b = x86_cvtepi64_pd(c);

		c = _mm_sub_epi64(c, _mm_set1_epi64x(2));
		const auto sign = _mm_castsi128_pd(_mm_slli_epi64(_mm_andnot_si128(c, _mm_set1_epi64x(4)), 61)); /* Extract sign bit */

		/* Calculate polynomial selection mask */
		c = _mm_and_si128(c, _mm_set1_epi64x(2));
#ifndef SEK_USE_SSE4_1
		c = _mm_or_si128(c, _mm_slli_epi64(c, 32)); /* c |= c << 32 */
		c = _mm_cmpeq_epi32(c, _mm_setzero_si128());
#else
		c = _mm_cmpeq_epi64(c, _mm_setzero_si128());
#endif
		const auto select_mask = _mm_castsi128_pd(c);

		a = x86_fmadd_pd(_mm_set1_pd(dp_sincos_d[0]), b, a); /* a = (dp_sincos_d[0] * b) + a */
		a = x86_fmadd_pd(_mm_set1_pd(dp_sincos_d[1]), b, a); /* a = (dp_sincos_d[1] * b) + a */
		a = x86_fmadd_pd(_mm_set1_pd(dp_sincos_d[2]), b, a); /* a = (dp_sincos_d[2] * b) + a */
		const auto a2 = _mm_mul_pd(a, a);

		/* P1 (0 <= a <= Pi/4) */
		auto p1 = x86_polevl_pd(a2, coscof_d);		  /* p1 = coscof_d(a2) */
		p1 = _mm_mul_pd(_mm_mul_pd(p1, a2), a2);	  /* p1 = p1 * a2 * a2 */
		p1 = x86_fmadd_pd(a2, _mm_set1_pd(-0.5), p1); /* p1 = (a2 * -0.5) + p1 */
		p1 = _mm_add_pd(p1, _mm_set1_pd(1.0));

		/* P2 (Pi/4 <= a <= 0) */
		auto p2 = _mm_mul_pd(x86_polevl_pd(a2, sincof_d), a2); /* p2 = sincof_d(a2) * a2 */
		p2 = x86_fmadd_pd(p2, a, a);						   /* p2 = ((p2 * a2) * a) + a */

		/* Choose between P1 and P2 */
		return _mm_xor_pd(x86_blendv_pd(p1, p2, select_mask), sign);
	}

	static const double tancotq_d[4] = {
		1.36812963470692954678e4,
		-1.32089234440210967447e6,
		2.50083801823357915839e7,
		-5.38695755929454629881e7,
	};
	static const double tancotp_d[3] = {-1.30936939181383777646e4, 1.15351664838587416140e6, -1.79565251976484877988e7};
	static const double dp_tancot_d[3] = {-7.853981554508209228515625e-1, -7.94662735614792836714e-9, -3.06161699786838294307e-17};

	__m128d x86_tancot_pd(__m128d v, __m128i cot_mask) noexcept
	{
		const auto sign_mask = _mm_set1_pd(std::bit_cast<double>(0x8000'0000'0000'0000));
		const auto abs_mask = _mm_set1_pd(std::bit_cast<double>(0x7fff'ffff'ffff'ffff));
		const auto sign = _mm_and_pd(v, sign_mask);

		auto a = _mm_and_pd(v, abs_mask);			 /* a = |v| */
		auto b = _mm_mul_pd(a, _mm_set1_pd(fopi_d)); /* b = a * (4 / PI) */
		auto c = x86_cvtpd_epi64(b);				 /* c = (int64) b */

		/* c = (c + 1) & (~1) */
		c = _mm_add_epi64(c, _mm_set1_epi64x(1));
		c = _mm_and_si128(c, _mm_set1_epi64x(~1));
		b = x86_cvtepi64_pd(c);

		a = x86_fmadd_pd(_mm_set1_pd(dp_tancot_d[0]), b, a); /* a = (dp_f[0] * b) + a */
		a = x86_fmadd_pd(_mm_set1_pd(dp_tancot_d[1]), b, a); /* a = (dp_f[1] * b) + a */
		a = x86_fmadd_pd(_mm_set1_pd(dp_tancot_d[2]), b, a); /* a = (dp_f[2] * b) + a */
		const auto a2 = _mm_mul_pd(a, a);

		/* Calculate polynomial selection mask (a2 > 1.0e-14) */
		const auto select_mask = _mm_cmpngt_pd(a2, _mm_set1_pd(1.0e-14));
		auto p = _mm_mul_pd(x86_polevl_pd(a2, tancotp_d), a2);	  /* p = tancotp_d(a2) * a2 */
		p = _mm_div_pd(p, x86_polevl1_pd(a2, tancotq_d));		  /* p = p / tancotq_d(a2) */
		p = x86_blendv_pd(x86_fmadd_pd(p, a, a), a, select_mask); /* p = select_mask ? a : (p * a) + a */

		auto bit2 = _mm_and_si128(c, _mm_set1_epi64x(2));
#ifdef SEK_USE_SSE4_1
		bit2 = _mm_cmpeq_epi64(bit2, _mm_set1_epi64x(2));
#else
		bit2 = _mm_or_si128(bit2, _mm_slli_epi64(bit2, 32)); /* bit2 |= bit2 << 32 */
		bit2 = _mm_cmpeq_epi32(bit2, _mm_set1_epi32(2));
#endif
		const auto select1 = _mm_castsi128_pd(_mm_and_si128(bit2, cot_mask));	 /* select1 = (c & 2) && cot_mask */
		const auto select2 = _mm_castsi128_pd(_mm_andnot_si128(cot_mask, bit2)); /* select2 = (c & 2) && !cot_mask */
		const auto select3 = _mm_castsi128_pd(_mm_andnot_si128(bit2, cot_mask)); /* select3 = !(c & 2) && cot_mask */
		const auto p1 = _mm_xor_pd(p, sign_mask);								 /* p1 = -p */
		const auto p2 = _mm_div_pd(_mm_set1_pd(-1.0), p);						 /* p2 = -1.0 / p */
		const auto p3 = _mm_div_pd(_mm_set1_pd(1.0), p);						 /* p3 = 1.0 / p */

		p = x86_blendv_pd(p, p3, select3); /* p = select3 ? p3 : p */
		p = x86_blendv_pd(p, p2, select2); /* p = select2 ? p2 : result */
		p = x86_blendv_pd(p, p1, select1); /* p = select1 ? p1 : result */
		return _mm_xor_pd(p, sign);
	}
	__m128d x86_tan_pd(__m128d v) noexcept { return x86_tancot_pd(v, _mm_setzero_si128()); }
	__m128d x86_cot_pd(__m128d v) noexcept { return x86_tancot_pd(v, _mm_set1_epi64x(-1)); }

	static double sinhp_p[4] = {
		-7.89474443963537015605e-1,
		-1.63725857525983828727e2,
		-1.15614435765005216044e4,
		-3.51754964808151394800e5,
	};
	static double sinhq_p[3] = {-2.77711081420602794433e2, 3.61578279834431989373e4, -2.11052978884890840399e6};
	static const double maxlog_d = 7.09782712893383996843e2;
	static const double loge2_d = 6.93147180559945309417e-1;

	__m128d x86_sinh_pd(__m128d v) noexcept
	{
		const auto sign_mask = _mm_set1_pd(std::bit_cast<double>(0x8000'0000'0000'0000));
		const auto abs_mask = _mm_set1_pd(std::bit_cast<double>(0x7fff'ffff'ffff'ffff));
		const auto half = _mm_set1_pd(0.5);
		const auto a = _mm_and_pd(v, abs_mask); /* a = |v| */
		const auto a2 = _mm_mul_pd(a, a);

		const auto select1 = _mm_cmpnle_pd(a, _mm_set1_pd(1.0));				/* select1 = a <= 1.0 */
		const auto select2 = _mm_cmpnge_pd(a, _mm_set1_pd(maxlog_d - loge2_d)); /* select1 = a >= (maxlog_d - loge2_d) */

		/* P1 (a <= 1.0) */
		const auto p1_p = _mm_mul_pd(x86_polevl_pd(a2, sinhp_p), a2); /* p1_p = sinhp_p(a2) * a2 */
		const auto p1_q = x86_polevl1_pd(a2, sinhq_p);				  /* p1_q = sinhq_p(a2) */
		const auto p1 = x86_fmadd_pd(_mm_div_pd(p1_p, p1_q), v, v);	  /* p1 = (p1_p / p1_q) * v + v */

		/* P2 (a > 1.0 && a >= (maxlog_d - loge2_d)) */
		const auto b_p2 = x86_exp_pd(_mm_mul_pd(a, half));		  /* b_p2 = exp(0.5 * a) */
		const auto p2 = _mm_mul_pd(_mm_mul_pd(b_p2, b_p2), half); /* p2 = b_p2 * b_p2 * 0.5 */

		/* P3 (a > 1.0 && a < (maxlog_d - loge2_d)) */
		const auto b_p3 = x86_exp_pd(a);											   /* b_p2 = exp(a) */
		const auto p3 = x86_fmadd_pd(b_p3, half, _mm_div_pd(_mm_set1_pd(-0.5), b_p3)); /* p3 = 0.5 * b_p3 + (-0.5 / b_p3) */

		/* return select1 ? p1 : ((select2 ? p2 : p3) ^ (v & sign_mask)) */
		return x86_blendv_pd(p1, _mm_xor_pd(x86_blendv_pd(p2, p3, select2), _mm_and_pd(v, sign_mask)), select1);
	}
	__m128d x86_cosh_pd(__m128d v) noexcept
	{
		const auto abs_mask = _mm_set1_pd(std::bit_cast<double>(0x7fff'ffff'ffff'ffff));
		const auto half = _mm_set1_pd(0.5);
		const auto a = _mm_and_pd(v, abs_mask); /* a = |v| */

		/* B1 (a >= (maxlog_d - loge2_d)) */
		auto b1 = x86_exp_pd(_mm_mul_pd(a, half)); /* b1 = exp(0.5 * a) */
		b1 = _mm_mul_pd(_mm_mul_pd(b1, b1), half); /* b1 = 0.5 * b1 * b1 */

		/* B2 (a < (maxlog_d - loge2_d)) */
		auto b2 = x86_exp_pd(a);						   /* b2 = exp(a) */
		b2 = x86_fmadd_pd(b2, half, _mm_div_pd(half, b2)); /* b2 = 0.5 * b2 + 0.5 / b2 */

		const auto select = _mm_cmpnge_pd(a, _mm_set1_pd(maxlog_d - loge2_d));
		return x86_blendv_pd(b2, b2, select); /* return (a >= (maxlog_d - loge2_d)) ? b1 : b2 */
	}
}	 // namespace sek::math::detail
#endif
