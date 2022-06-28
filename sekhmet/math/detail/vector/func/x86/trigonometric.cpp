/*
 * Created by switchblade on 26/06/22
 */

#include "trigonometric.hpp"

#include "arithmetic.hpp"
#include "exponential.hpp"
#include "util.hpp"

/* Implementations of trigonometric functions derived from netlib's cephes library (http://www.netlib.org/cephes/)
 * Inspired by http://gruntthepeon.free.fr/ssemath */

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
	static const float sincof_f[3] = {-1.9515295891E-4f, 8.3321608736E-3f, -1.6666654611E-1f};
	static const float coscof_f[3] = {2.443315711809948E-005f, -1.388731625493765E-003f, 4.166664568298827E-002f};
	static const float dp_f[3] = {-0.78515625f, -2.4187564849853515625e-4f, -3.77489497744594108e-8f};
	static const float fopi_f = 4.0f / std::numbers::pi_v<float>; /* 4 / Pi */
	static const float pio2_f = std::numbers::pi_v<float> / 2.0f; /* Pi / 2 */
	static const float pio4_f = std::numbers::pi_v<float> / 4.0f; /* Pi / 4 */
	static const float pi_f = std::numbers::pi_v<float>;

	__m128 x86_sin_ps(__m128 v) noexcept
	{
		const auto sign_mask = _mm_set1_ps(std::bit_cast<float>(0x8000'0000));
		const auto abs_mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));

		auto a = _mm_and_ps(v, abs_mask);			 /* a = |v| */
		auto b = _mm_mul_ps(a, _mm_set1_ps(fopi_f)); /* b = a * (4 / PI) */
		auto c = _mm_cvttps_epi32(b);				 /* c = (int32) b */
		/* c = (c + 1) & (~1) */
		c = _mm_add_epi32(c, _mm_set1_epi32(1));
		c = _mm_and_si128(c, _mm_set1_epi32(~1));
		b = _mm_cvtepi32_ps(c);

		const auto flag = _mm_slli_epi32(_mm_and_si128(c, _mm_set1_epi32(4)), 29);		/* Swap sign flag */
		const auto sign = _mm_xor_ps(_mm_and_ps(v, sign_mask), _mm_castsi128_ps(flag)); /* Extract sign bit */

		/* Calculate polynomial selection mask */
		c = _mm_and_si128(c, _mm_set1_epi32(2));
		c = _mm_cmpeq_epi32(c, _mm_setzero_si128());
		const auto select_mask = _mm_castsi128_ps(c);

		a = x86_fmadd_ps(_mm_set1_ps(dp_f[0]), b, a); /* a = (dp_f[0] * b) + a */
		a = x86_fmadd_ps(_mm_set1_ps(dp_f[1]), b, a); /* a = (dp_f[1] * b) + a */
		a = x86_fmadd_ps(_mm_set1_ps(dp_f[2]), b, a); /* a = (dp_f[2] * b) + a */
		const auto a2 = _mm_mul_ps(a, a);

		/* P1 (0 <= a <= Pi/4) */
		auto p1 = _mm_set1_ps(coscof_f[0]);
		p1 = x86_fmadd_ps(p1, a2, _mm_set1_ps(coscof_f[1])); /* p1 = (p1 * a2) + coscof_f[1] */
		p1 = x86_fmadd_ps(p1, a2, _mm_set1_ps(coscof_f[2])); /* p1 = (p1 * a2) + coscof_f[2] */
		p1 = _mm_mul_ps(_mm_mul_ps(p1, a2), a2);
		p1 = x86_fmadd_ps(a2, _mm_set1_ps(-0.5f), p1); /* p1 = (a2 * -0.5) + p1 */
		p1 = _mm_add_ps(p1, _mm_set1_ps(1.0f));

		/* P2  (Pi/4 <= a <= 0) */
		auto p2 = _mm_set1_ps(sincof_f[0]);
		p2 = x86_fmadd_ps(p2, a2, _mm_set1_ps(sincof_f[1])); /* p2 = (p2 * a2) + sincof_f[1] */
		p2 = x86_fmadd_ps(p2, a2, _mm_set1_ps(sincof_f[2])); /* p2 = (p2 * a2) + sincof_f[2] */
		p2 = x86_fmadd_ps(_mm_mul_ps(p2, a2), a, a);		 /* p2 = ((p2 * a2) * a) + a */

		/* Choose between P1 and P2 */
		return _mm_xor_ps(x86_blendv_ps(p1, p2, select_mask), sign);
	}
	__m128 x86_cos_ps(__m128 v) noexcept
	{
		const auto abs_mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));

		auto a = _mm_and_ps(v, abs_mask);			 /* a = |v| */
		auto b = _mm_mul_ps(a, _mm_set1_ps(fopi_f)); /* b = a * (4 / PI) */
		auto c = _mm_cvttps_epi32(b);				 /* c = (int32) b */
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

		a = x86_fmadd_ps(_mm_set1_ps(dp_f[0]), b, a); /* a = (dp_f[0] * b) + a */
		a = x86_fmadd_ps(_mm_set1_ps(dp_f[1]), b, a); /* a = (dp_f[1] * b) + a */
		a = x86_fmadd_ps(_mm_set1_ps(dp_f[2]), b, a); /* a = (dp_f[2] * b) + a */
		const auto a2 = _mm_mul_ps(a, a);

		/* P1 (0 <= a <= Pi/4) */
		auto p1 = _mm_set1_ps(coscof_f[0]);
		p1 = x86_fmadd_ps(p1, a2, _mm_set1_ps(coscof_f[1])); /* p1 = (p1 * a2) + coscof_f[1] */
		p1 = x86_fmadd_ps(p1, a2, _mm_set1_ps(coscof_f[2])); /* p1 = (p1 * a2) + coscof_f[2] */
		p1 = _mm_mul_ps(_mm_mul_ps(p1, a2), a2);
		p1 = x86_fmadd_ps(a2, _mm_set1_ps(-0.5f), p1); /* p1 = (a2 * -0.5) + p1 */
		p1 = _mm_add_ps(p1, _mm_set1_ps(1.0f));

		/* P2 (Pi/4 <= a <= 0) */
		auto p2 = _mm_set1_ps(sincof_f[0]);
		p2 = x86_fmadd_ps(p2, a2, _mm_set1_ps(sincof_f[1])); /* p2 = (p2 * a2) + sincof_f[1] */
		p2 = x86_fmadd_ps(p2, a2, _mm_set1_ps(sincof_f[2])); /* p2 = (p2 * a2) + sincof_f[2] */
		p2 = x86_fmadd_ps(_mm_mul_ps(p2, a2), a, a);		 /* p2 = ((p2 * a2) * a) + a */

		/* Choose between P1 and P2 */
		return _mm_xor_ps(x86_blendv_ps(p1, p2, select_mask), sign);
	}

	static const float tancof_f[6] = {
		9.38540185543E-3f,
		3.11992232697E-3f,
		2.44301354525E-2f,
		5.34112807005E-2f,
		1.33387994085E-1f,
		3.33331568548E-1f,
	};

	__m128 x86_tancot_ps(__m128 v, __m128i cot_mask) noexcept
	{
		const auto sign_mask = _mm_set1_ps(std::bit_cast<float>(0x8000'0000));
		const auto abs_mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));
		const auto sign = _mm_and_ps(v, sign_mask);

		auto a = _mm_and_ps(v, abs_mask);			 /* a = |v| */
		auto b = _mm_mul_ps(a, _mm_set1_ps(fopi_f)); /* b = abs * (4 / PI) */
		auto c = _mm_cvttps_epi32(b);				 /* c = (int32) b */

		/* c = (c + 1) & (~1) */
		c = _mm_add_epi32(c, _mm_set1_epi32(1));
		c = _mm_and_si128(c, _mm_set1_epi32(~1));
		b = _mm_cvtepi32_ps(c);

		/* Calculate polynomial selection mask */
		const auto select_mask = _mm_cmpngt_ps(a, _mm_set1_ps(0.0001f));

		a = x86_fmadd_ps(_mm_set1_ps(dp_f[0]), b, a); /* a = (dp_f[0] * b) + a */
		a = x86_fmadd_ps(_mm_set1_ps(dp_f[1]), b, a); /* a = (dp_f[1] * b) + a */
		a = x86_fmadd_ps(_mm_set1_ps(dp_f[2]), b, a); /* a = (dp_f[2] * b) + a */

		/* b = a > 0.0001 ? poly(a, coscof_d) : a */
		const auto a2 = _mm_mul_ps(a, a);
		auto p = _mm_set1_ps(tancof_f[0]);
		p = x86_fmadd_ps(p, a2, _mm_set1_ps(tancof_f[1])); /* p = (p * a2) + coscof_d[1] */
		p = x86_fmadd_ps(p, a2, _mm_set1_ps(tancof_f[2])); /* p = (p * a2) + coscof_d[2] */
		p = x86_fmadd_ps(p, a2, _mm_set1_ps(tancof_f[3])); /* p = (p * a2) + coscof_d[3] */
		p = x86_fmadd_ps(p, a2, _mm_set1_ps(tancof_f[4])); /* p = (p * a2) + coscof_d[4] */
		p = x86_fmadd_ps(p, a2, _mm_set1_ps(tancof_f[5])); /* p = (p * a2) + coscof_d[5] */
		p = x86_fmadd_ps(_mm_mul_ps(p, a2), a, a);		   /* p = ((p * a2) * a) + a */
		b = x86_blendv_ps(p, a, select_mask);

		const auto bit2 = _mm_cmpeq_epi32(_mm_and_si128(c, _mm_set1_epi32(2)), _mm_set1_epi32(2));
		const auto select1 = _mm_castsi128_ps(_mm_and_si128(bit2, cot_mask));	 /* select1 = (c & 2) && cot_mask */
		const auto select2 = _mm_castsi128_ps(_mm_andnot_si128(cot_mask, bit2)); /* select2 = (c & 2) && !cot_mask */
		const auto select3 = _mm_castsi128_ps(_mm_andnot_si128(bit2, cot_mask)); /* select3 = !(c & 2) && cot_mask */
		const auto b1 = _mm_xor_ps(b, sign_mask);								 /* b1 = -b */
		const auto b2 = _mm_div_ps(_mm_set1_ps(-1.0f), b);						 /* b2 = -1.0f/b */
		const auto b3 = _mm_div_ps(_mm_set1_ps(1.0f), b);						 /* b2 = 1.0f/b */

		auto result = x86_blendv_ps(b, b3, select3); /* result = select3 ? b3 : b */
		result = x86_blendv_ps(result, b2, select2); /* result = select2 ? b2 : result */
		result = x86_blendv_ps(result, b1, select1); /* result = select1 ? b1 : result */
		return _mm_xor_ps(result, sign);
	}
	__m128 x86_tan_ps(__m128 v) noexcept { return x86_tancot_ps(v, _mm_setzero_si128()); }
	__m128 x86_cot_ps(__m128 v) noexcept { return x86_tancot_ps(v, _mm_set1_epi32(-1)); }

	static const float sinhcof_f[3] = {2.03721912945E-4f, 8.33028376239E-3f, 1.66667160211E-1f};

	__m128 x86_sinh_ps(__m128 v) noexcept
	{
		const auto sign_mask = _mm_set1_ps(std::bit_cast<float>(0x8000'0000));
		const auto abs_mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));
		const auto a = _mm_and_ps(v, abs_mask); /* a = |v| */

		/* P1 (a > 1.0) */
		auto p1 = x86_exp_ps(a);
		const auto tmp = _mm_div_ps(_mm_set1_ps(-0.5f), p1); /* tmp = (-0.5 / p1) */
		p1 = x86_fmadd_ps(_mm_set1_ps(0.5f), p1, tmp);		 /* p1 = (0.5 * p1) + tmp */
		p1 = _mm_xor_ps(p1, _mm_and_ps(v, sign_mask));		 /* p1 = v < 0 ? -p1 : p1 */

		/* P2 (a <= 1.0) */
		const auto v2 = _mm_mul_ps(v, v);
		auto p2 = _mm_set1_ps(sinhcof_f[0]);
		p2 = x86_fmadd_ps(p2, v2, _mm_set1_ps(sinhcof_f[1])); /* p2 = (p2 * v2) + sinhcof_f[1] */
		p2 = x86_fmadd_ps(p2, v2, _mm_set1_ps(sinhcof_f[2])); /* p2 = (p2 * v2) + sinhcof_f[2] */
		p2 = x86_fmadd_ps(_mm_mul_ps(p2, v2), v, v);		  /* p2 = ((p2 * v2) * v) + v */

		/* return (a > 1.0) ? p1 : p2 */
		return x86_blendv_ps(p1, p2, _mm_cmpngt_ps(a, _mm_set1_ps(1.0f)));
	}
	__m128 x86_cosh_ps(__m128 v) noexcept
	{
		const auto abs_mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));
		auto a = x86_exp_ps(_mm_and_ps(v, abs_mask));		 /* a = exp(|v|) */
		a = _mm_add_ps(_mm_div_ps(_mm_set1_ps(1.0f), a), a); /* a = 1.0 / a + a */
		return _mm_mul_ps(a, _mm_set1_ps(0.5f));			 /* return a * 0.5 */
	}

	static const float tanhcof_f[5] = {
		-5.70498872745E-3f,
		2.06390887954E-2f,
		-5.37397155531E-2f,
		1.33314422036E-1f,
		-3.33332819422E-1f,
	};

	__m128 x86_tanh_ps(__m128 v) noexcept
	{
		const auto sign_mask = _mm_set1_ps(std::bit_cast<float>(0x8000'0000));
		const auto abs_mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));
		const auto a = _mm_and_ps(v, abs_mask); /* a = |v| */

		/* P1 (a >= 0.625) */
		auto p1 = x86_exp_ps(_mm_add_ps(a, a));
		p1 = _mm_add_ps(_mm_set1_ps(1.0f), p1);		   /* p1 = 1.0 + p1 */
		p1 = _mm_div_ps(_mm_set1_ps(2.0f), p1);		   /* p1 = 2.0 / p1 */
		p1 = _mm_sub_ps(_mm_set1_ps(1.0f), p1);		   /* p1 = 1.0 - p1 */
		p1 = _mm_xor_ps(_mm_and_ps(v, sign_mask), p1); /* p1 = v < 0 ? -p1 : p1 */

		/* P1 (a < 0.625) */
		const auto a2 = _mm_mul_ps(a, a);
		auto p2 = _mm_set1_ps(tanhcof_f[0]);
		p2 = x86_fmadd_ps(p2, a2, _mm_set1_ps(tanhcof_f[1])); /* p2 = (p2 * a2) + tanhcof_f[1] */
		p2 = x86_fmadd_ps(p2, a2, _mm_set1_ps(tanhcof_f[2])); /* p2 = (p2 * a2) + tanhcof_f[2] */
		p2 = x86_fmadd_ps(p2, a2, _mm_set1_ps(tanhcof_f[3])); /* p2 = (p2 * a2) + tanhcof_f[3] */
		p2 = x86_fmadd_ps(p2, a2, _mm_set1_ps(tanhcof_f[4])); /* p2 = (p2 * a2) + tanhcof_f[4] */
		p2 = x86_fmadd_ps(_mm_mul_ps(p2, a2), v, v);		  /* p2 = ((p2 * a2) * v) + v */

		/* return (a >= 0.625) ? p1 : p2 */
		return x86_blendv_ps(p1, p2, _mm_cmplt_ps(a, _mm_set1_ps(0.625f)));
	}

	static const float asincof_f[5] = {
		4.2163199048E-2f,
		2.4181311049E-2f,
		4.5470025998E-2f,
		7.4953002686E-2f,
		1.6666752422E-1f,
	};

	__m128 x86_asin_ps(__m128 v) noexcept
	{
		const auto sign_mask = _mm_set1_ps(std::bit_cast<float>(0x8000'0000));
		const auto abs_mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));
		const auto a = _mm_and_ps(v, abs_mask); /* a = |v| */

		/* P (a >= 0.0001) */
		const auto half = _mm_set1_ps(0.5f);
		const auto half_mask = _mm_cmpngt_ps(a, half);			   /* half_mask = a > 0.5 */
		const auto c1 = x86_fmadd_ps(a, _mm_set1_ps(-0.5f), half); /* c1 = (a * -0.5) + 0.5 */
		const auto b1 = _mm_sqrt_ps(c1);						   /* b1 = sqrt(c1) */
		const auto c2 = _mm_mul_ps(v, v);						   /* c2 = v * v */
		const auto b2 = a;										   /* b2 = a */

		const auto b = x86_blendv_ps(b1, b2, half_mask); /* b = (a > 0.5) ? b1 : b2 */
		const auto c = x86_blendv_ps(c1, c2, half_mask); /* c = (a > 0.5) ? c1 : c2 */

		auto p = _mm_set1_ps(asincof_f[0]);
		p = x86_fmadd_ps(p, c, _mm_set1_ps(asincof_f[1])); /* p = (p * c) + asincof_f[1] */
		p = x86_fmadd_ps(p, c, _mm_set1_ps(asincof_f[2])); /* p = (p * c) + asincof_f[2] */
		p = x86_fmadd_ps(p, c, _mm_set1_ps(asincof_f[3])); /* p = (p * c) + asincof_f[3] */
		p = x86_fmadd_ps(p, c, _mm_set1_ps(asincof_f[4])); /* p = (p * c) + asincof_f[4] */
		p = x86_fmadd_ps(_mm_mul_ps(p, c), b, b);		   /* p = ((p * c) * b) + b */

		/* p = half_mask ? (Pi / 2) - (p + p) : p */
		p = x86_blendv_ps(_mm_sub_ps(_mm_set1_ps(pio2_f), _mm_add_ps(p, p)), p, half_mask);

		/* result = (a < 0.0001) ? a : p */
		const auto result = x86_blendv_ps(a, p, _mm_cmpnlt_ps(a, _mm_set1_ps(0.0001f)));
		return _mm_xor_ps(result, _mm_and_ps(v, sign_mask)); /* return (v < 0) ? -result : result */
	}
	__m128 x86_acos_ps(__m128 v) noexcept
	{
		const auto half_minus = _mm_set1_ps(-0.5f);
		const auto half = _mm_set1_ps(0.5f);

		/* v < -0.5 */
		auto a = x86_asin_ps(_mm_sqrt_ps(x86_fmadd_ps(v, half, half))); /* a = asinf(sqrtf((v * 0.5) + 0.5)) */
		a = x86_fmadd_ps(a, _mm_set1_ps(-2.0f), _mm_set1_ps(pi_f));		/* a = (a * -2.0) + pi */

		/* v > 0.5 */
		auto b = x86_fmadd_ps(v, half_minus, half);						/* b = (v * -0.5) + 0.5 */
		b = _mm_mul_ps(_mm_set1_ps(2.0f), x86_asin_ps(_mm_sqrt_ps(b))); /* b = 2.0 * asinf(sqrtf(b)) */

		/* |v| <= 0.5 */
		const auto c = _mm_sub_ps(_mm_set1_ps(pio2_f), x86_asin_ps(v));

		/* Calculate masks & select between a, b & c */
		const auto a_mask = _mm_cmpnlt_ps(v, half_minus);
		const auto b_mask = _mm_cmpngt_ps(v, half);
		return x86_blendv_ps(a, x86_blendv_ps(b, c, b_mask), a_mask); /* return (v < -0.5) ? a : ((v > 0.5) ? b : c) */
	}

	static const float atancof_f[4] = {8.05374449538e-2f, -1.38776856032E-1f, 1.99777106478E-1f, -3.33329491539E-1f};
	static const float tan3pi8_f = 2.414213562373095f; /* tan((3 * Pi) / 8) */
	static const float tanpi8_f = 0.4142135623730950f; /* tan(Pi / 8) */

	/* TODO: Find a better algorithm for arc tangent. Precision of this is dubious at best.
	 * Potential candidates:
	 *  1. `newlib (fdlibm)` atanf - High precision, but a lot of branches. Probably unsuitable for SIMD. */
	__m128 x86_atan_ps(__m128 v) noexcept
	{
		const auto sign_mask = _mm_set1_ps(std::bit_cast<float>(0x8000'0000));
		const auto abs_mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));
		auto a = _mm_and_ps(v, abs_mask); /* a = |v| */

		/* Range reduction */
		const auto select1 = _mm_cmpngt_ps(a, _mm_set1_ps(tan3pi8_f)); /* a > tan3pi8_f */
		const auto select2 = _mm_cmpngt_ps(a, _mm_set1_ps(tanpi8_f));  /* a > tanpi8_f */
		/* if (a > tan3pi8_f) */
		const auto a1 = _mm_div_ps(_mm_set1_ps(-1.0f), a); /* a = -1.0 / a */
		const auto b1 = _mm_set1_ps(pio2_f);
		/* else if (a > tanpi8_f) */
		const auto one = _mm_set1_ps(1.0f);
		const auto a2 = _mm_div_ps(_mm_sub_ps(a, one), _mm_add_ps(a, one)); /* a = (a - 1.0) / (a + 1.0) */
		const auto b2 = _mm_set1_ps(pio4_f);

		auto b = _mm_setzero_ps();
		a = x86_blendv_ps(a1, a, select1); /* b = (a > tan3pi8_f) ? a1 : a */
		b = x86_blendv_ps(b1, b, select1); /* b = (a > tan3pi8_f) ? b1 : b */
		a = x86_blendv_ps(a2, a, select2); /* b = (a > tanpi8_f) ? a2 : a */
		b = x86_blendv_ps(b2, b, select2); /* b = (a > tanpi8_f) ? b2 : b */

		const auto c = _mm_mul_ps(a, a);
		auto p = _mm_set1_ps(atancof_f[0]);
		p = x86_fmadd_ps(p, c, _mm_set1_ps(atancof_f[1])); /* p = (p * c) + atancof_f[1] */
		p = x86_fmadd_ps(p, c, _mm_set1_ps(atancof_f[2])); /* p = (p * c) + atancof_f[2] */
		p = x86_fmadd_ps(p, c, _mm_set1_ps(atancof_f[3])); /* p = (p * c) + atancof_f[3] */
		p = x86_fmadd_ps(_mm_mul_ps(p, c), a, a);		   /* p = ((p * c) * a) + a */

		return _mm_xor_ps(_mm_add_ps(b, p), _mm_and_ps(v, sign_mask));
	}

	static const float asinhcof_f[4] = {2.0122003309E-2f, -4.2699340972E-2f, 7.4847586088E-2f, -1.6666288134E-1f};
	static const float acoshcof_f[5] = {1.7596881071E-3f, -7.5272886713E-3f, 2.6454905019E-2f, -1.1784741703E-1f, 1.4142135263E0f};
	static const float loge2_f = 0.693147180559945309f;

	__m128 x86_asinh_ps(__m128 v) noexcept
	{
		const auto sign_mask = _mm_set1_ps(std::bit_cast<float>(0x8000'0000));
		const auto abs_mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));
		const auto a = _mm_and_ps(v, abs_mask); /* a = |v| */

		/* a > 1500.0 */
		const auto b1 = _mm_add_ps(x86_log_ps(a), _mm_set1_ps(loge2_f)); /* b1 = log(a) + loge2_f */

		/* a <= 1500.0 && a < 0.5 */
		const auto a2 = _mm_mul_ps(a, a);
		auto b2 = _mm_set1_ps(asinhcof_f[0]);
		b2 = x86_fmadd_ps(b2, a2, _mm_set1_ps(asinhcof_f[1])); /* b2 = (b2 * a2) + asinhcof_f[1] */
		b2 = x86_fmadd_ps(b2, a2, _mm_set1_ps(asinhcof_f[2])); /* b2 = (b2 * a2) + asinhcof_f[2] */
		b2 = x86_fmadd_ps(b2, a2, _mm_set1_ps(asinhcof_f[3])); /* b2 = (b2 * a2) + asinhcof_f[3] */
		b2 = x86_fmadd_ps(_mm_mul_ps(b2, a2), a, a);		   /* b2 = ((b2 * a2) * a) + a */

		/* a <= 1500.0 && a >= 0.5 */
		const auto tmp = _mm_sqrt_ps(_mm_add_ps(a2, _mm_set1_ps(1.0f))); /* tmp = sqrt(a2 + 1.0) */
		const auto b3 = x86_log_ps(_mm_add_ps(a, tmp));					 /* b3 = log(a + tmp) */

		/* b = (a > 1500.0) ? b1 : ((a < 0.5) ? b2 : b3) */
		const auto select1 = _mm_cmpngt_ps(a, _mm_set1_ps(1500.0f));			   /* a > 1500.0 */
		const auto select2 = _mm_cmpnlt_ps(a, _mm_set1_ps(0.5f));				   /* a < 0.5 */
		const auto b = x86_blendv_ps(b1, x86_blendv_ps(b2, b3, select2), select1); /* b = (select2) ? b1 : (select1) ? b2 : b3 */
		return _mm_xor_ps(b, _mm_and_ps(v, sign_mask));							   /* return v < 0 ? -b : b */
	}
	__m128 x86_acosh_ps(__m128 v) noexcept
	{
		const auto a = _mm_sub_ps(v, _mm_set1_ps(1.0f)); /* a = v - 1.0 */

		/* v > 1500.0 */
		const auto b1 = _mm_add_ps(x86_log_ps(v), _mm_set1_ps(loge2_f)); /* b1 = log(v) + loge2_f */

		/* v <= 1500.0 && a < 0.5 */
		auto b2 = _mm_set1_ps(acoshcof_f[0]);
		b2 = x86_fmadd_ps(b2, a, _mm_set1_ps(acoshcof_f[1])); /* b2 = (b2 * a) + acoshcof_f[1] */
		b2 = x86_fmadd_ps(b2, a, _mm_set1_ps(acoshcof_f[2])); /* b2 = (b2 * a) + acoshcof_f[2] */
		b2 = x86_fmadd_ps(b2, a, _mm_set1_ps(acoshcof_f[3])); /* b2 = (b2 * a) + acoshcof_f[3] */
		b2 = x86_fmadd_ps(b2, a, _mm_set1_ps(acoshcof_f[4])); /* b2 = (b2 * a) + acoshcof_f[4] */
		b2 = _mm_mul_ps(b2, _mm_sqrt_ps(a));				  /* b2 = b2 * sqrtf(a) */

		/* a <= 1500.0 && a >= 0.5 */
		const auto b3 = x86_log_ps(_mm_add_ps(v, _mm_sqrt_ps(x86_fmadd_ps(v, a, a)))); /* b3 = logf(v + sqrt((v * a) + a)) */

		const auto select1 = _mm_cmpngt_ps(v, _mm_set1_ps(1500.0f)); /* v > 1500.0 */
		const auto select2 = _mm_cmpnlt_ps(a, _mm_set1_ps(0.5f));	 /* a < 0.5 */

		return x86_blendv_ps(b1, x86_blendv_ps(b2, b3, select2), select1); /* return (select1) ? b1 : (select2) ? b2 : b3 */
	}

	static const float atanhcof_f[5] = {
		1.81740078349E-1f,
		8.24370301058E-2f,
		1.46691431730E-1f,
		1.99782164500E-1f,
		3.33337300303E-1f,
	};

	__m128 x86_atanh_ps(__m128 v) noexcept
	{
		const auto abs_mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));

		/* a >= 0.0001 && a >= 0.5 */
		const auto one = _mm_set1_ps(1.0f);
		const auto tmp = _mm_div_ps(_mm_add_ps(one, v), _mm_sub_ps(one, v)); /* tmp = (1.0 + v) / (1.0 - v) */
		const auto a2 = _mm_mul_ps(_mm_set1_ps(0.5f), x86_log_ps(tmp));		 /* a2 = 0.5 * log(tmp); */

		/* a >= 0.0001 && a < 0.5 */
		const auto v2 = _mm_mul_ps(v, v);
		auto a1 = _mm_set1_ps(atanhcof_f[0]);
		a1 = x86_fmadd_ps(a1, v2, _mm_set1_ps(atanhcof_f[1])); /* a1 = (a1 * v2) + atanhcof_f[1] */
		a1 = x86_fmadd_ps(a1, v2, _mm_set1_ps(atanhcof_f[2])); /* a1 = (a1 * v2) + atanhcof_f[2] */
		a1 = x86_fmadd_ps(a1, v2, _mm_set1_ps(atanhcof_f[3])); /* a1 = (a1 * v2) + atanhcof_f[3] */
		a1 = x86_fmadd_ps(a1, v2, _mm_set1_ps(atanhcof_f[4])); /* a1 = (a1 * v2) + atanhcof_f[4] */
		a1 = x86_fmadd_ps(_mm_mul_ps(a1, v2), v, v);		   /* a1 = ((a1 * v2) * v) + v */

		const auto a = _mm_and_ps(v, abs_mask);							  /* a = |v| */
		const auto select1 = _mm_cmpnlt_ps(a, _mm_set1_ps(0.5f));		  /* a < 0.5 */
		const auto select2 = _mm_cmpnlt_ps(a, _mm_set1_ps(0.0001f));	  /* a < 0.0001 */
		return x86_blendv_ps(v, x86_blendv_ps(a1, a2, select1), select2); /* a = (select2) ? v : (select1) ? a1 : a2 */
	}

	static const double sincof_d[6] = {
		1.58962301576546568060E-10,
		-2.50507477628578072866E-8,
		2.75573136213857245213E-6,
		-1.98412698295895385996E-4,
		8.33333333332211858878E-3,
		-1.66666666666666307295E-1,
	};
	static const double coscof_d[6] = {
		-1.13585365213876817300E-11,
		2.08757008419747316778E-9,
		-2.75573141792967388112E-7,
		2.48015872888517045348E-5,
		-1.38888888888730564116E-3,
		4.16666666666665929218E-2,
	};
	static const double dp_d[3] = {-7.85398125648498535156E-1, -3.77489470793079817668E-8, -2.69515142907905952645E-15};
	static const double fopi_d = 4.0 / std::numbers::pi_v<double>; /* 4 / Pi */
	static const double pio2_d = std::numbers::pi_v<double> / 2.0; /* Pi / 2 */
	static const double pio4_d = std::numbers::pi_v<double> / 4.0; /* Pi / 4 */

	__m128d x86_sin_pd(__m128d v) noexcept
	{
		const auto sign_mask = _mm_set1_pd(std::bit_cast<double>(0x8000'0000'0000'0000));
		const auto abs_mask = _mm_set1_pd(std::bit_cast<double>(0x7fff'ffff'ffff'ffff));

		auto a = _mm_and_pd(v, abs_mask);			 /* a = |v| */
		auto b = _mm_mul_pd(a, _mm_set1_pd(fopi_d)); /* b = a * (4 / PI) */

		/* c = (int64) b */
		auto c = x86_cvtpd_epi64(b);

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

		a = x86_fmadd_pd(_mm_set1_pd(dp_d[0]), b, a); /* a = (dp_d[0] * b) + a */
		a = x86_fmadd_pd(_mm_set1_pd(dp_d[1]), b, a); /* a = (dp_d[1] * b) + a */
		a = x86_fmadd_pd(_mm_set1_pd(dp_d[2]), b, a); /* a = (dp_d[2] * b) + a */
		const auto a2 = _mm_mul_pd(a, a);

		/* P1 (Pi/4 <= a <= 0) */
		auto p1 = _mm_set1_pd(coscof_d[0]);
		p1 = x86_fmadd_pd(p1, a2, _mm_set1_pd(coscof_d[1])); /* p1 = (p1 * a2) + coscof_d[1] */
		p1 = x86_fmadd_pd(p1, a2, _mm_set1_pd(coscof_d[2])); /* p1 = (p1 * a2) + coscof_d[2] */
		p1 = x86_fmadd_pd(p1, a2, _mm_set1_pd(coscof_d[3])); /* p1 = (p1 * a2) + coscof_d[3] */
		p1 = x86_fmadd_pd(p1, a2, _mm_set1_pd(coscof_d[4])); /* p1 = (p1 * a2) + coscof_d[4] */
		p1 = x86_fmadd_pd(p1, a2, _mm_set1_pd(coscof_d[5])); /* p1 = (p1 * a2) + coscof_d[5] */
		p1 = _mm_mul_pd(_mm_mul_pd(p1, a2), a2);			 /* p1 = p1 * a2 * a2 */
		p1 = x86_fmadd_pd(a2, _mm_set1_pd(-0.5), p1);		 /* p1 = (a2 * -0.5) + p1 */
		p1 = _mm_add_pd(p1, _mm_set1_pd(1.0));

		/* P2 (0 <= a <= Pi/4) */
		auto p2 = _mm_set1_pd(sincof_d[0]);
		p2 = x86_fmadd_pd(p2, a2, _mm_set1_pd(sincof_d[1])); /* p2 = (p2 * a2) + sincof_d[1] */
		p2 = x86_fmadd_pd(p2, a2, _mm_set1_pd(sincof_d[2])); /* p2 = (p2 * a2) + sincof_d[2] */
		p2 = x86_fmadd_pd(p2, a2, _mm_set1_pd(sincof_d[3])); /* p2 = (p2 * a2) + sincof_d[3] */
		p2 = x86_fmadd_pd(p2, a2, _mm_set1_pd(sincof_d[4])); /* p2 = (p2 * a2) + sincof_d[4] */
		p2 = x86_fmadd_pd(p2, a2, _mm_set1_pd(sincof_d[5])); /* p2 = (p2 * a2) + sincof_d[5] */
		p2 = x86_fmadd_pd(_mm_mul_pd(p2, a2), a, a);		 /* p2 = ((p2 * a2) * a) + a */

		/* Choose between P1 and P2 */
		return _mm_xor_pd(x86_blendv_pd(p1, p2, select_mask), sign);
	}
	__m128d x86_cos_pd(__m128d v) noexcept
	{
		const auto abs_mask = _mm_set1_pd(std::bit_cast<double>(0x7fff'ffff'ffff'ffff));

		auto a = _mm_and_pd(v, abs_mask);			 /* a = |v| */
		auto b = _mm_mul_pd(a, _mm_set1_pd(fopi_d)); /* b = a * (4 / PI) */

		/* c = (int64) b */
		auto c = x86_cvtpd_epi64(b);

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

		a = x86_fmadd_pd(_mm_set1_pd(dp_d[0]), b, a); /* a = (dp_d[0] * b) + a */
		a = x86_fmadd_pd(_mm_set1_pd(dp_d[1]), b, a); /* a = (dp_d[1] * b) + a */
		a = x86_fmadd_pd(_mm_set1_pd(dp_d[2]), b, a); /* a = (dp_d[2] * b) + a */
		const auto a2 = _mm_mul_pd(a, a);

		/* P1 (0 <= a <= Pi/4) */
		auto p1 = _mm_set1_pd(coscof_d[0]);
		p1 = x86_fmadd_pd(p1, a2, _mm_set1_pd(coscof_d[1])); /* p1 = (p1 * a2) + coscof_d[1] */
		p1 = x86_fmadd_pd(p1, a2, _mm_set1_pd(coscof_d[2])); /* p1 = (p1 * a2) + coscof_d[2] */
		p1 = x86_fmadd_pd(p1, a2, _mm_set1_pd(coscof_d[3])); /* p1 = (p1 * a2) + coscof_d[3] */
		p1 = x86_fmadd_pd(p1, a2, _mm_set1_pd(coscof_d[4])); /* p1 = (p1 * a2) + coscof_d[4] */
		p1 = x86_fmadd_pd(p1, a2, _mm_set1_pd(coscof_d[5])); /* p1 = (p1 * a2) + coscof_d[5] */
		p1 = _mm_mul_pd(_mm_mul_pd(p1, a2), a2);			 /* p1 = p1 * a2 * a2 */
		p1 = x86_fmadd_pd(a2, _mm_set1_pd(-0.5), p1);		 /* p1 = (a2 * -0.5) + p1 */
		p1 = _mm_add_pd(p1, _mm_set1_pd(1.0));

		/* P2 (Pi/4 <= a <= 0) */
		auto p2 = _mm_set1_pd(sincof_d[0]);
		p2 = x86_fmadd_pd(p2, a2, _mm_set1_pd(sincof_d[1])); /* p2 = (p2 * a2) + sincof_d[1] */
		p2 = x86_fmadd_pd(p2, a2, _mm_set1_pd(sincof_d[2])); /* p2 = (p2 * a2) + sincof_d[2] */
		p2 = x86_fmadd_pd(p2, a2, _mm_set1_pd(sincof_d[3])); /* p2 = (p2 * a2) + sincof_d[3] */
		p2 = x86_fmadd_pd(p2, a2, _mm_set1_pd(sincof_d[4])); /* p2 = (p2 * a2) + sincof_d[4] */
		p2 = x86_fmadd_pd(p2, a2, _mm_set1_pd(sincof_d[5])); /* p2 = (p2 * a2) + sincof_d[5] */
		p2 = x86_fmadd_pd(_mm_mul_pd(p2, a2), a, a);		 /* p2 = ((p2 * a2) * a) + a */

		/* Choose between P1 and P2 */
		return _mm_xor_pd(x86_blendv_pd(p1, p2, select_mask), sign);
	}
}	 // namespace sek::math::detail
#endif
