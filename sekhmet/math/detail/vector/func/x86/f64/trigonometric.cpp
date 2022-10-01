/*
 * Created by switchblade on 26/06/22
 */

#include "trigonometric.hpp"

#include "../arithmetic.hpp"
#include "../exponential.hpp"

/* Implementations of trigonometric functions derived from netlib's cephes library (http://www.netlib.org/cephes/)
 * Inspired by http://gruntthepeon.free.fr/ssemath */

#ifdef SEK_USE_SSE2
namespace sek::detail
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

	inline static __m128d x86_tancot_pd(__m128d v, __m128i cot_mask) noexcept
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

	static const double tanhp_d[3] = {-9.64399179425052238628e-1, -9.92877231001918586564e1, -1.61468768441708447952e3};
	static const double tanhq_d[3] = {1.12811678491632931402e2, 2.23548839060100448583e3, 4.84406305325125486048e3};

	__m128d x86_tanh_pd(__m128d v) noexcept
	{
		const auto sign_mask = _mm_set1_pd(std::bit_cast<double>(0x8000'0000'0000'0000));
		const auto abs_mask = _mm_set1_pd(std::bit_cast<double>(0x7fff'ffff'ffff'ffff));
		const auto one = _mm_set1_pd(1.0);
		const auto a = _mm_and_pd(v, abs_mask); /* a = |v| */

		/* Calculate polynomial selection mask */
		const auto select_mask = _mm_cmple_pd(a, _mm_set1_pd(0.625));

		/* P1 (a >= 0.625) */
		auto p1 = _mm_add_pd(x86_exp_pd(_mm_mul_pd(a, _mm_set1_pd(2.0))), one); /* p1 = exp(a * 2.0) + 1.0 */
		p1 = _mm_add_pd(_mm_div_pd(_mm_set1_pd(-2.0), p1), one);				/* p1 = (-2.0 / p1) + 1.0 */
		p1 = _mm_xor_pd(p1, _mm_and_pd(v, sign_mask));							/* p1 = (v < 0) ? -p1 : p1 */

		/* P2 (a < 0.625) */
		const auto v2 = _mm_mul_pd(v, v);
		const auto p2_p = _mm_mul_pd(x86_polevl_pd(v2, tanhp_d), v2); /* p2_p = tanhp_d(v2) * v2 */
		const auto p2_q = x86_polevl1_pd(v2, tanhq_d);				  /* p2_p = tanhp_d(v2) */
		const auto p2 = x86_fmadd_pd(_mm_div_pd(p2_p, p2_q), v, v);	  /* p2 = ((p2_p / p2_q) * v) + v */

		/* return select_mask ? p1 : (a == 0.0) ? 0.0 : p2 */
		return x86_blendv_pd(p1, _mm_and_pd(p2, _mm_cmpeq_pd(a, _mm_setzero_pd())), select_mask);
	}

	static const double asinp_d[6] = {
		4.253011369004428248960e-3,
		-6.019598008014123785661e-1,
		5.444622390564711410273e0,
		-1.626247967210700244449e1,
		1.956261983317594739197e1,
		-8.198089802484824371615e0,
	};
	static const double asinq_d[5] = {
		-1.474091372988853791896e1,
		7.049610280856842141659e1,
		-1.471791292232726029859e2,
		1.395105614657485689735e2,
		-4.918853881490881290097e1,
	};
	static const double asinr_d[5] = {
		2.967721961301243206100e-3,
		-5.634242780008963776856e-1,
		6.968710824104713396794e0,
		-2.556901049652824852289e1,
		2.853665548261061424989e1,
	};
	static const double asins_d[4] = {
		-2.194779531642920639778e1,
		1.470656354026814941758e2,
		-3.838770957603691357202e2,
		3.424398657913078477438e2,
	};
	static const double morebits_d = 6.123233995736765886130e-17;
	static const double morebitsn_d = -6.123233995736765886130e-17;

	__m128d x86_asin_pd(__m128d v) noexcept
	{
		const auto sign_mask = _mm_set1_pd(std::bit_cast<double>(0x8000'0000'0000'0000));
		const auto abs_mask = _mm_set1_pd(std::bit_cast<double>(0x7fff'ffff'ffff'ffff));
		const auto a = _mm_and_pd(v, abs_mask); /* a = |v| */

		/* Calculate polynomial selection mask */
		const auto select_mask = _mm_cmpngt_pd(a, _mm_set1_pd(0.625));

		/* P1 (a > 0.625) */
		auto b = _mm_sub_pd(_mm_set1_pd(1.0), a);
		const auto p1_r = x86_polevl_pd(b, asinr_d);			 /* p1_r = asinr_d(b) */
		const auto p1_s = x86_polevl1_pd(b, asins_d);			 /* p1_s = asins_d(b) */
		auto p1 = _mm_mul_pd(b, _mm_div_pd(p1_r, p1_s));		 /* p1 = b * (p1_r / p1_s) */
		b = _mm_sqrt_pd(_mm_add_pd(b, b));						 /* b = sqrt(b + b) */
		p1 = x86_fmadd_pd(v, p1, _mm_set1_pd(morebitsn_d));		 /* p1 = b * p1 + morebitsn_d */
		p1 = _mm_sub_pd(_mm_sub_pd(_mm_set1_pd(pio4_d), b), p1); /* p1 = PIO4 - b - p1 */
		p1 = _mm_add_pd(_mm_set1_pd(pio4_d), p1);				 /* p1 = PIO4 + p1 */

		/* P2 (a <= 0.625) */
		const auto a2 = _mm_mul_pd(a, a);
		const auto p2_p = _mm_mul_pd(x86_polevl_pd(a2, asinp_d), a2); /* p2_p = asinp_d(a2) * a2 */
		const auto p2_q = x86_polevl1_pd(a2, asinq_d);				  /* p2_q = asinq_d(a2) */
		auto p2 = _mm_div_pd(p2_p, p2_q);							  /* p2 = p2_p / p2_q */
		p2 = x86_fmadd_pd(p2, a, a);								  /* p2 = p2 * a + a */
		/* p2 = (a < 1.0e-8) ? v : p2 */
		p2 = x86_blendv_pd(v, p2, _mm_cmpnle_pd(a, _mm_set1_pd(1.0e-8)));

		/* return (select_mask ? p1 : p2) ^ (v & sign_mask) */
		return _mm_xor_pd(x86_blendv_pd(p1, p2, select_mask), _mm_and_pd(v, sign_mask));
	}
	__m128d x86_acos_pd(__m128d v) noexcept
	{
		/* Calculate polynomial selection mask */
		const auto half = _mm_set1_pd(0.5);
		const auto select_mask = _mm_cmpngt_pd(v, half);

		/* P1 (x > 0.5) */
		auto p1 = _mm_sqrt_pd(x86_fmadd_pd(v, _mm_set1_pd(-0.5), half));
		p1 = _mm_mul_pd(x86_asin_pd(p1), _mm_set1_pd(2.0));

		/* P2 (x <= 0.5) */
		auto p2 = _mm_sub_pd(_mm_set1_pd(pio4_d), x86_asin_pd(v));
		p2 = _mm_add_pd(p2, _mm_set1_pd(morebits_d));
		p2 = _mm_add_pd(p2, _mm_set1_pd(pio4_d));

		return x86_blendv_pd(p1, p2, select_mask);
	}

	static const double atanp_d[5] = {
		-8.750608600031904122785e-1,
		-1.615753718733365076637e1,
		-7.500855792314704667340e1,
		-1.228866684490136173410e2,
		-6.485021904942025371773e1,
	};
	static const double atanq_d[5] = {
		2.485846490142306297962e1,
		1.650270098316988542046e2,
		4.328810604912902668951e2,
		4.853903996359136964868e2,
		1.945506571482613964425e2,
	};
	static const double tan3pi8_d = 2.4142135623730950488016887242096980785696718753769480731766797379;

	__m128d x86_atan_pd(__m128d v) noexcept
	{
		const auto sign_mask = _mm_set1_pd(std::bit_cast<double>(0x8000'0000'0000'0000));
		const auto abs_mask = _mm_set1_pd(std::bit_cast<double>(0x7fff'ffff'ffff'ffff));
		const auto one = _mm_set1_pd(1.0);
		auto a = _mm_and_pd(v, abs_mask); /* a = |v| */

		/* Range reduction */
		const auto select1 = _mm_cmpngt_pd(a, _mm_set1_pd(tan3pi8_d)); /* a > tan3pi8_d */
		const auto select2 = _mm_cmpngt_pd(a, _mm_set1_pd(0.66));	   /* a > 0.66 */

		const auto a1 = _mm_div_pd(_mm_set1_pd(-1.0), a);					/* a1 = -1.0 / a */
		const auto b1 = _mm_set1_pd(pio2_d);								/* b1 = Pi / 2 */
		const auto c1 = _mm_set1_pd(morebits_d);							/* c1 = morebits_d */
		const auto a2 = _mm_div_pd(_mm_sub_pd(a, one), _mm_add_pd(a, one)); /* a2 = (a - 1.0) / (a + 1.0) */
		const auto b2 = _mm_set1_pd(pio4_d);								/* b2 = Pi / 4 */
		const auto c2 = _mm_mul_pd(c1, _mm_set1_pd(0.5));					/* c2 = morebits_d * 0.5 */

		/* a = select1 ? a1 : select2 ? a2 : a
		 * b = select1 ? b1 : select2 ? b2 : 0.0
		 * c = select1 ? c1 : select2 ? c2 : 0.0 */
		a = x86_blendv_pd(a1, x86_blendv_pd(a2, a, select2), select1);
		const auto b = x86_blendv_pd(b1, x86_blendv_pd(b2, _mm_setzero_pd(), select2), select1);
		const auto c = x86_blendv_pd(c1, x86_blendv_pd(c2, _mm_setzero_pd(), select2), select1);

		const auto aa = _mm_mul_pd(a, a);
		const auto pp = x86_polevl_pd(aa, atanp_d);	 /* pp = atanp_d(aa) */
		const auto pq = x86_polevl1_pd(aa, atanq_d); /* pq = atanq_d(aa) */
		auto p = _mm_mul_pd(_mm_div_pd(pp, pq), aa); /* p = (pp / pq) * aa */
		p = x86_fmadd_pd(p, a, a);					 /* p = (p * a) + a */
		p = _mm_add_pd(_mm_add_pd(p, b), c);		 /* p = p + b + c */

		return _mm_xor_pd(p, _mm_and_pd(v, sign_mask));
	}

	static const double asinhp_d[5] = {
		-4.33231683752342103572e-3,
		-5.91750212056387121207e-1,
		-4.37390226194356683570e0,
		-9.09030533308377316566e0,
		-5.56682227230859640450e0,
	};
	static const double asinhq_d[4] = {
		1.28757002067426453537e1,
		4.86042483805291788324e1,
		6.95722521337257608734e1,
		3.34009336338516356383e1,
	};

	__m128d x86_asinh_pd(__m128d v) noexcept
	{
		const auto sign_mask = _mm_set1_pd(std::bit_cast<double>(0x8000'0000'0000'0000));
		const auto abs_mask = _mm_set1_pd(std::bit_cast<double>(0x7fff'ffff'ffff'ffff));
		const auto a = _mm_and_pd(v, abs_mask); /* a = |v| */
		const auto a2 = _mm_mul_pd(a, a);

		/* Calculate polynomial selection masks */
		const auto select1 = _mm_cmpngt_pd(a, _mm_set1_pd(1.0e8)); /* a > 1.0e8 */
		const auto select2 = _mm_cmpnlt_pd(a, _mm_set1_pd(0.5));   /* a < 0.5 */

		/* P1 (a > 1.0e8) */
		const auto p1 = _mm_add_pd(x86_log_pd(a), _mm_set1_pd(loge2_d)); /* p1 = log(a) + loge2_d */

		/* P2 (a < 0.5 && a <= 1.0e8) */
		const auto p2_p = _mm_mul_pd(x86_polevl_pd(a2, asinhp_d), a2); /* p2_p = asinhp_d(a2) * a2 */
		const auto p2_q = x86_polevl1_pd(a2, asinhq_d);				   /* p2_q = asinhq_d(a2) */
		const auto p2 = x86_fmadd_pd(_mm_div_pd(p2_p, p2_q), a, a);	   /* p2 = ((p2_p / p2_q) * a) + a */

		/* P3 (a >= 0.5 && a <= 1.0e8) */
		auto p3 = _mm_sqrt_pd(_mm_add_pd(a2, _mm_set1_pd(1.0))); /* p3 = sqrt(a2 + 1.0) */
		p3 = x86_log_pd(_mm_add_pd(a, p3));					 /* p3 = log(a + p3) */

		/* p = select1 ? p1 : select2 ? p2 : p3 */
		const auto p = x86_blendv_pd(p1, x86_blendv_pd(p2, p3, select2), select1);
		return _mm_xor_pd(p, _mm_and_pd(v, sign_mask));
	}

	static const double acoshp_d[5] = {
		1.18801130533544501356e2,
		3.94726656571334401102e3,
		3.43989375926195455866e4,
		1.08102874834699867335e5,
		1.10855947270161294369e5,
	};
	static const double acoshq_d[5] = {
		1.86145380837903397292e2,
		4.15352677227719831579e3,
		2.97683430363289370382e4,
		8.29725251988426222434e4,
		7.83869920495893927727e4,
	};

	__m128d x86_acosh_pd(__m128d v) noexcept
	{
		const auto abs_mask = _mm_set1_pd(std::bit_cast<double>(0x7fff'ffff'ffff'ffff));
		const auto a = _mm_and_pd(v, abs_mask); /* a = |v| */
		const auto b = _mm_sub_pd(a, _mm_set1_pd(1.0));

		/* Calculate polynomial selection masks */
		const auto select1 = _mm_cmpngt_pd(a, _mm_set1_pd(1.0e8)); /* a > 1.0e8 */
		const auto select2 = _mm_cmpnlt_pd(b, _mm_set1_pd(0.5));   /* b < 0.5 */

		/* P1 (a > 1.0e8) */
		const auto p1 = _mm_add_pd(x86_log_pd(a), _mm_set1_pd(loge2_d));

		/* P2 (b < 0.5 && a <= 1.0e8) */
		const auto p2_p = x86_polevl_pd(b, acoshp_d);						/* p2_p = acoshp_d(b) */
		const auto p2_q = x86_polevl1_pd(b, acoshq_d);						/* p2_q = acoshq_d(b) */
		const auto p2 = _mm_mul_pd(_mm_div_pd(p2_p, p2_q), _mm_sqrt_pd(b)); /* p = (p2_p / p2_q) + sqrt(b) */

		/* P3 (b >= 0.5 && a <= 1.0e8) */
		auto p3 = _mm_sqrt_pd(x86_fmadd_pd(a, b, b)); /* p3 = sqrt((a * b) + b) */
		p3 = x86_log_pd(_mm_add_pd(a, p3));			  /* p3 = log(a + p3) */

		/* return select1 ? p1 : select2 ? p2 : p3 */
		return x86_blendv_pd(p1, x86_blendv_pd(p2, p3, select2), select1);
	}

	static const double atanhp_d[5] = {
		-8.54074331929669305196e-1,
		1.20426861384072379242e1,
		-4.61252884198732692637e1,
		6.54566728676544377376e1,
		-3.09092539379866942570e1,
	};
	static const double atanhq_d[5] = {
		-1.95638849376911654834e1,
		1.08938092147140262656e2,
		-2.49839401325893582852e2,
		2.52006675691344555838e2,
		-9.27277618139601130017e1,
	};

	__m128d x86_atanh_pd(__m128d v) noexcept
	{
		const auto abs_mask = _mm_set1_pd(std::bit_cast<double>(0x7fff'ffff'ffff'ffff));
		const auto a = _mm_and_pd(v, abs_mask); /* a = |v| */

		/* Calculate polynomial selection masks */
		const auto select_mask = _mm_cmpnlt_pd(a, _mm_set1_pd(0.5)); /* a < 0.5 */
		const auto v_mask = _mm_cmpnlt_pd(a, _mm_set1_pd(1.0e-7));	 /* a < 1.0e-7 */

		/* P1 (a < 0.5) */
		const auto v2 = _mm_mul_pd(v, v);
		const auto p1_p = _mm_mul_pd(x86_polevl_pd(v2, atanhp_d), v2); /* p1_p = atanhp_d(v2) * v2 */
		const auto p1_q = x86_polevl1_pd(v2, atanhq_d);				   /* p1_q = atanhq_d(v2) */
		const auto p1 = x86_fmadd_pd(_mm_div_pd(p1_p, p1_q), v, v);	   /* p1 = ((p1_p / p1_q) * v) + v */

		/* P2 (a >= 0.5) */
		const auto one = _mm_set1_pd(1.0);
		auto p2 = _mm_div_pd(_mm_add_pd(one, v), _mm_sub_pd(one, v)); /* p2 = (1.0 + v) / (1.0 - v) */
		p2 = _mm_mul_pd(x86_log_pd(p2), _mm_set1_pd(0.5));			  /* p2 = log(p2) * 0.5 */

		/* return v_mask ? v : select_mask ? p1 : p2 */
		return x86_blendv_pd(v, x86_blendv_pd(p1, p1, select_mask), v_mask);
	}
}	 // namespace sek::detail
#endif
