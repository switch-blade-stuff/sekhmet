/*
 * Created by switchblade on 26/06/22
 */

#include "trigonometric.hpp"

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
	static const float pi4_f = 1.2732395447351626861510701069801148962756771659236515899813387524f;
	static const float pi2_f = 1.5707963267948966192313216916397514420985846996875529104874722961f;

	__m128 x86_sin_ps(__m128 v) noexcept
	{
		const auto sign_mask = _mm_set1_ps(std::bit_cast<float>(0x8000'0000));
		const auto abs_mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));

		auto a = _mm_and_ps(v, abs_mask);			/* a = |v| */
		auto b = _mm_mul_ps(a, _mm_set1_ps(pi4_f)); /* b = a * (4 / PI) */
		auto c = _mm_cvttps_epi32(b);				/* c = (int32) b */
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

#ifdef SEK_USE_FMA
		a = _mm_fmadd_ps(_mm_set1_ps(dp_f[0]), b, a); /* a = (dp_f[0] * b) + a */
		a = _mm_fmadd_ps(_mm_set1_ps(dp_f[1]), b, a); /* a = (dp_f[1] * b) + a */
		a = _mm_fmadd_ps(_mm_set1_ps(dp_f[2]), b, a); /* a = (dp_f[2] * b) + a */
#else
		a = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(dp_f[0]), b), a);
		a = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(dp_f[1]), b), a);
		a = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(dp_f[2]), b), a);
#endif
		const auto a2 = _mm_mul_ps(a, a);

		/* P1 (0 <= a <= Pi/4) */
		auto p1 = _mm_set1_ps(coscof_f[0]);
#ifdef SEK_USE_FMA
		p1 = _mm_fmadd_ps(p1, a2, _mm_set1_ps(coscof_f[1])); /* p1 = (p1 * a2) + coscof_f[1] */
		p1 = _mm_fmadd_ps(p1, a2, _mm_set1_ps(coscof_f[2])); /* p1 = (p1 * a2) + coscof_f[2] */
#else
		p1 = _mm_add_ps(_mm_mul_ps(p1, a2), _mm_set1_ps(coscof_f[1]));
		p1 = _mm_add_ps(_mm_mul_ps(p1, a2), _mm_set1_ps(coscof_f[2]));
#endif
		p1 = _mm_mul_ps(_mm_mul_ps(p1, a2), a2);
#ifdef SEK_USE_FMA
		p1 = _mm_fmadd_ps(a2, _mm_set1_ps(-0.5f), p1); /* p1 = (a2 * -0.5) + p1 */
#else
		p1 = _mm_sub_ps(p1, _mm_mul_ps(a2, _mm_set1_ps(0.5f))); /* p1 = p1 - (a2 * 0.5) */
#endif
		p1 = _mm_add_ps(p1, _mm_set1_ps(1.0f));

		/* P2  (Pi/4 <= a <= 0) */
		auto p2 = _mm_set1_ps(sincof_f[0]);
#ifdef SEK_USE_FMA
		p2 = _mm_fmadd_ps(p2, a2, _mm_set1_ps(sincof_f[1])); /* p2 = (p2 * a2) + sincof_f[1] */
		p2 = _mm_fmadd_ps(p2, a2, _mm_set1_ps(sincof_f[2])); /* p2 = (p2 * a2) + sincof_f[2] */
		p2 = _mm_fmadd_ps(_mm_mul_ps(p2, a2), a, a);		 /* p2 = ((p2 * a2) * a) + a */
#else
		p2 = _mm_add_ps(_mm_mul_ps(p2, a2), _mm_set1_ps(sincof_f[1]));
		p2 = _mm_add_ps(_mm_mul_ps(p2, a2), _mm_set1_ps(sincof_f[2]));
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
		a = _mm_fmadd_ps(_mm_set1_ps(dp_f[0]), b, a); /* a = (dp_f[0] * b) + a */
		a = _mm_fmadd_ps(_mm_set1_ps(dp_f[1]), b, a); /* a = (dp_f[1] * b) + a */
		a = _mm_fmadd_ps(_mm_set1_ps(dp_f[2]), b, a); /* a = (dp_f[2] * b) + a */
#else
		a = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(dp_f[0]), b), a);
		a = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(dp_f[1]), b), a);
		a = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(dp_f[2]), b), a);
#endif
		const auto a2 = _mm_mul_ps(a, a);

		/* P1 (0 <= a <= Pi/4) */
		auto p1 = _mm_set1_ps(coscof_f[0]);
#ifdef SEK_USE_FMA
		p1 = _mm_fmadd_ps(p1, a2, _mm_set1_ps(coscof_f[1])); /* p1 = (p1 * a2) + coscof_f[1] */
		p1 = _mm_fmadd_ps(p1, a2, _mm_set1_ps(coscof_f[2])); /* p1 = (p1 * a2) + coscof_f[2] */
#else
		p1 = _mm_add_ps(_mm_mul_ps(p1, a2), _mm_set1_ps(coscof_f[1]));
		p1 = _mm_add_ps(_mm_mul_ps(p1, a2), _mm_set1_ps(coscof_f[2]));
#endif
		p1 = _mm_mul_ps(_mm_mul_ps(p1, a2), a2);

#ifdef SEK_USE_FMA
		p1 = _mm_fmadd_ps(a2, _mm_set1_ps(-0.5f), p1); /* p1 = (a2 * -0.5) + p1 */
#else
		p1 = _mm_sub_ps(p1, _mm_mul_ps(a2, _mm_set1_ps(0.5f))); /* p1 = p1 - (a2 * 0.5) */
#endif
		p1 = _mm_add_ps(p1, _mm_set1_ps(1.0f));

		/* P2 (Pi/4 <= a <= 0) */
		auto p2 = _mm_set1_ps(sincof_f[0]);
#ifdef SEK_USE_FMA
		p2 = _mm_fmadd_ps(p2, a2, _mm_set1_ps(sincof_f[1])); /* p2 = (p2 * a2) + sincof_f[1] */
		p2 = _mm_fmadd_ps(p2, a2, _mm_set1_ps(sincof_f[2])); /* p2 = (p2 * a2) + sincof_f[2] */
		p2 = _mm_fmadd_ps(_mm_mul_ps(p2, a2), a, a);		 /* p2 = ((p2 * a2) * a) + a */
#else
		p2 = _mm_add_ps(_mm_mul_ps(p2, a2), _mm_set1_ps(sincof_f[1]));
		p2 = _mm_add_ps(_mm_mul_ps(p2, a2), _mm_set1_ps(sincof_f[2]));
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

		auto a = _mm_and_ps(v, abs_mask);			/* a = |v| */
		auto b = _mm_mul_ps(a, _mm_set1_ps(pi4_f)); /* b = abs * (4 / PI) */
		auto c = _mm_cvttps_epi32(b);				/* c = (int32) b */

		/* c = (c + 1) & (~1) */
		c = _mm_add_epi32(c, _mm_set1_epi32(1));
		c = _mm_and_si128(c, _mm_set1_epi32(~1));
		b = _mm_cvtepi32_ps(c);

		/* Calculate polynomial selection mask */
		const auto select_mask = _mm_cmpngt_ps(a, _mm_set1_ps(0.0001f));

#ifdef SEK_USE_FMA
		a = _mm_fmadd_ps(_mm_set1_ps(dp_f[0]), b, a); /* a = (dp_f[0] * b) + a */
		a = _mm_fmadd_ps(_mm_set1_ps(dp_f[1]), b, a); /* a = (dp_f[1] * b) + a */
		a = _mm_fmadd_ps(_mm_set1_ps(dp_f[2]), b, a); /* a = (dp_f[2] * b) + a */
#else
		a = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(dp_f[0]), b), a);
		a = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(dp_f[1]), b), a);
		a = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(dp_f[2]), b), a);
#endif

		/* b = a > 0.0001 ? poly(a, coscof_d) : a */
		const auto a2 = _mm_mul_ps(a, a);
		auto p = _mm_set1_ps(tancof_f[0]);
#ifdef SEK_USE_FMA
		p = _mm_fmadd_ps(p, a2, _mm_set1_ps(tancof_f[1])); /* p = (p * a2) + coscof_d[1] */
		p = _mm_fmadd_ps(p, a2, _mm_set1_ps(tancof_f[2])); /* p = (p * a2) + coscof_d[2] */
		p = _mm_fmadd_ps(p, a2, _mm_set1_ps(tancof_f[3])); /* p = (p * a2) + coscof_d[3] */
		p = _mm_fmadd_ps(p, a2, _mm_set1_ps(tancof_f[4])); /* p = (p * a2) + coscof_d[4] */
		p = _mm_fmadd_ps(p, a2, _mm_set1_ps(tancof_f[5])); /* p = (p * a2) + coscof_d[5] */
		p = _mm_fmadd_ps(_mm_mul_ps(p, a2), a, a);		   /* p = ((p * a2) * a) + a */
#else
		p = _mm_add_ps(_mm_mul_ps(p, a2), _mm_set1_ps(tancof_f[1]));
		p = _mm_add_ps(_mm_mul_ps(p, a2), _mm_set1_ps(tancof_f[2]));
		p = _mm_add_ps(_mm_mul_ps(p, a2), _mm_set1_ps(tancof_f[3]));
		p = _mm_add_ps(_mm_mul_ps(p, a2), _mm_set1_ps(tancof_f[4]));
		p = _mm_add_ps(_mm_mul_ps(p, a2), _mm_set1_ps(tancof_f[5]));
		p = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(p, a2), a), a);
#endif
#ifdef SEK_USE_SSE4_1
		b = _mm_blendv_ps(p, a, select_mask);
#else
		b = _mm_add_ps(_mm_and_ps(select_mask, a), _mm_andnot_ps(select_mask, p));
#endif

		const auto bit2 = _mm_cmpeq_epi32(_mm_and_si128(c, _mm_set1_epi32(2)), _mm_set1_epi32(2));
		const auto select1 = _mm_and_si128(bit2, cot_mask);	   /* select1 = (c & 2) && cot_mask */
		const auto select2 = _mm_andnot_si128(cot_mask, bit2); /* select2 = (c & 2) && !cot_mask */
		const auto select3 = _mm_andnot_si128(bit2, cot_mask); /* select3 = !(c & 2) && cot_mask */
		const auto b1 = _mm_xor_ps(b, sign_mask);			   /* b1 = -b */
		const auto b2 = _mm_div_ps(_mm_set1_ps(-1.0f), b);	   /* b2 = -1.0f/b */
		const auto b3 = _mm_div_ps(_mm_set1_ps(1.0f), b);	   /* b2 = 1.0f/b */
#ifdef SEK_USE_SSE4_1
		auto result = _mm_blendv_ps(b, b3, _mm_castsi128_ps(select3)); /* result = select3 ? b3 : b */
		result = _mm_blendv_ps(result, b2, _mm_castsi128_ps(select2)); /* result = select2 ? b2 : result */
		result = _mm_blendv_ps(result, b1, _mm_castsi128_ps(select1)); /* result = select2 ? b1 : result */
#else
		auto result = _mm_andnot_ps(select3, b);
		result = _mm_add_ps(_mm_and_ps(_mm_castsi128_ps(select3), b3), result);
		result = _mm_add_ps(_mm_and_ps(_mm_castsi128_ps(select2), b2), result);
		result = _mm_add_ps(_mm_and_ps(_mm_castsi128_ps(select1), b1), result);
#endif
		return _mm_xor_ps(result, sign);
	}
	__m128 x86_tan_ps(__m128 v) noexcept { return x86_tancot_ps(v, _mm_setzero_si128()); }
	__m128 x86_cot_ps(__m128 v) noexcept { return x86_tancot_ps(v, _mm_set1_epi32(-1)); }

	static const float sinhcof_f[3] = {
		2.03721912945E-4f,
		8.33028376239E-3f,
		1.66667160211E-1f,
	};

	__m128 x86_sinh_ps(__m128 v) noexcept
	{
		const auto sign_mask = _mm_set1_ps(std::bit_cast<float>(0x8000'0000));
		const auto abs_mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));
		const auto a = _mm_and_ps(v, abs_mask); /* a = |v| */

		/* P1 (a > 1.0) */
		auto p1 = x86_exp_ps(a);
		const auto tmp = _mm_div_ps(_mm_set1_ps(-0.5f), p1);
#ifdef SEK_USE_FMA
		p1 = _mm_fmadd_ps(_mm_set1_ps(0.5f), p1, tmp); /* p1 = (0.5 * p1) - (0.5 / p1) */
#else
		p1 = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(0.5f), p1), tmp);
#endif
		p1 = _mm_xor_ps(p1, _mm_and_ps(v, sign_mask)); /* p1 = v < 0 ? -p1 : p1 */

		/* P2 (a <= 1.0) */
		const auto v2 = _mm_mul_ps(v, v);
		auto p2 = _mm_set1_ps(sinhcof_f[0]);
#ifdef SEK_USE_FMA
		p2 = _mm_fmadd_ps(p2, v2, _mm_set1_ps(sinhcof_f[1])); /* p2 = (p2 * v2) + sinhcof_f[1] */
		p2 = _mm_fmadd_ps(p2, v2, _mm_set1_ps(sinhcof_f[2])); /* p2 = (p2 * v2) + sinhcof_f[2] */
		p2 = _mm_fmadd_ps(_mm_mul_ps(p2, v2), v, v);		  /* p2 = ((p2 * v2) * v) + v */
#else
		p1 = _mm_add_ps(_mm_mul_ps(p2, v2), _mm_set1_ps(sinhcof_f[1]));
		p1 = _mm_add_ps(_mm_mul_ps(p2, v2), _mm_set1_ps(sinhcof_f[2]));
		p1 = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(p2, v2), v), v);
#endif

		/* return (a > 1.0) ? p1 : p2 */
		const auto select_mask = _mm_cmpngt_ps(a, _mm_set1_ps(1.0f));
#ifdef SEK_USE_SSE4_1
		return _mm_blendv_ps(p1, p2, select_mask);
#else
		return _mm_add_ps(_mm_and_ps(select_mask, p2), _mm_andnot_ps(select_mask, p1));
#endif
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
#ifdef SEK_USE_FMA
		p2 = _mm_fmadd_ps(p2, a2, _mm_set1_ps(tanhcof_f[1])); /* p2 = (p2 * a2) + tanhcof_f[1] */
		p2 = _mm_fmadd_ps(p2, a2, _mm_set1_ps(tanhcof_f[2])); /* p2 = (p2 * a2) + tanhcof_f[2] */
		p2 = _mm_fmadd_ps(p2, a2, _mm_set1_ps(tanhcof_f[3])); /* p2 = (p2 * a2) + tanhcof_f[3] */
		p2 = _mm_fmadd_ps(p2, a2, _mm_set1_ps(tanhcof_f[4])); /* p2 = (p2 * a2) + tanhcof_f[4] */
		p2 = _mm_fmadd_ps(_mm_mul_ps(p2, a2), v, v);		  /* p2 = ((p2 * a2) * v) + v */
#else
		p2 = _mm_add_ps(_mm_mul_ps(p2, a2), _mm_set1_ps(sinhcof_f[1]));
		p2 = _mm_add_ps(_mm_mul_ps(p2, a2), _mm_set1_ps(sinhcof_f[2]));
		p2 = _mm_add_ps(_mm_mul_ps(p2, a2), _mm_set1_ps(sinhcof_f[3]));
		p2 = _mm_add_ps(_mm_mul_ps(p2, a2), _mm_set1_ps(sinhcof_f[4]));
		p2 = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(p2, a2), v), v);
#endif

		/* return (a >= 0.625) ? p1 : p2 */
		const auto select_mask = _mm_cmplt_ps(a, _mm_set1_ps(0.625));
#ifdef SEK_USE_SSE4_1
		return _mm_blendv_ps(p1, p2, select_mask);
#else
		return _mm_add_ps(_mm_and_ps(select_mask, p2), _mm_andnot_ps(select_mask, p1));
#endif
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
		const auto half_mask = _mm_cmpngt_ps(a, half);						/* half_mask = a > 0.5 */
		const auto c1 = _mm_mul_ps(half, _mm_sub_ps(_mm_set1_ps(1.0f), a)); /* c1 = 0.5 * (1.0 - a) */
		const auto b1 = _mm_sqrt_ps(c1);									/* b1 = sqrt(c1) */
		const auto c2 = _mm_mul_ps(v, v);									/* c1 = v * v */
		const auto b2 = a;													/* b2 = a */

		/* if (half_mask) { c = c1; b = b1; } else { c = c2; b = b2; } */
#ifdef SEK_USE_SSE4_1
		const auto b = _mm_blendv_ps(b1, b2, half_mask);
		const auto c = _mm_blendv_ps(c1, c2, half_mask);
#else
		const auto b = _mm_add_ps(_mm_and_ps(half_mask, b2), _mm_andnot_ps(half_mask, b1));
		const auto c = _mm_add_ps(_mm_and_ps(half_mask, z2), _mm_andnot_ps(half_mask, z1));
#endif

#ifdef SEK_USE_FMA
		auto p = _mm_set1_ps(asincof_f[0]);
		p = _mm_fmadd_ps(p, c, _mm_set1_ps(asincof_f[1])); /* p = (p * c) + asincof_f[1];*/
		p = _mm_fmadd_ps(p, c, _mm_set1_ps(asincof_f[2])); /* p = (p * c) + asincof_f[2];*/
		p = _mm_fmadd_ps(p, c, _mm_set1_ps(asincof_f[3])); /* p = (p * c) + asincof_f[3];*/
		p = _mm_fmadd_ps(p, c, _mm_set1_ps(asincof_f[4])); /* p = (p * c) + asincof_f[4];*/
		p = _mm_fmadd_ps(_mm_mul_ps(p, c), b, b);		   /* p = ((p * c) * b) + b;*/
#else
		p = _mm_add_ps(_mm_mul_ps(p, c), _mm_set1_ps(sinhcof_f[1]));
		p = _mm_add_ps(_mm_mul_ps(p, c), _mm_set1_ps(sinhcof_f[2]));
		p = _mm_add_ps(_mm_mul_ps(p, c), _mm_set1_ps(sinhcof_f[3]));
		p = _mm_add_ps(_mm_mul_ps(p, c), _mm_set1_ps(sinhcof_f[4]));
		p = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(p, c), b), b);
#endif
		/* p = half_mask ? (Pi / 2) - (p + p) : p */
		const auto p_pi = _mm_sub_ps(_mm_set1_ps(pi2_f), _mm_add_ps(p, p));
#ifdef SEK_USE_SSE4_1
		p = _mm_blendv_ps(p_pi, p, half_mask);
#else
		p = _mm_add_ps(_mm_and_ps(half_mask, p), _mm_andnot_ps(half_mask, p_pi));
#endif

		/* result = (a < 0.0001) ? a : p */
		const auto select_mask = _mm_cmpnlt_ps(a, _mm_set1_ps(0.0001f));
#ifdef SEK_USE_SSE4_1
		const auto result = _mm_blendv_ps(a, p, select_mask);
#else
		const auto result = _mm_add_ps(_mm_and_ps(select_mask, p), _mm_andnot_ps(select_mask, a));
#endif
		return _mm_xor_ps(result, _mm_and_ps(v, sign_mask)); /* return (v < 0) ? -result : result */
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
	static const double pi4_d = 1.2732395447351626861510701069801148962756771659236515899813387524;

	__m128d x86_sin_pd(__m128d v) noexcept
	{
		const auto sign_mask = _mm_set1_pd(std::bit_cast<double>(0x8000'0000'0000'0000));
		const auto abs_mask = _mm_set1_pd(std::bit_cast<double>(0x7fff'ffff'ffff'ffff));

		auto a = _mm_and_pd(v, abs_mask);			/* a = |v| */
		auto b = _mm_mul_pd(a, _mm_set1_pd(pi4_d)); /* b = a * (4 / PI) */

		/* c = (int64) b */
#ifdef SEK_USE_AVX512_DQ
		auto c = _mm_cvttpd_epi64(b);
#else
		auto c = x86_cvtpd_epi64(b);
#endif

		/* c = (c + 1) & (~1) */
		c = _mm_add_epi64(c, _mm_set1_epi64x(1));
		c = _mm_and_si128(c, _mm_set1_epi64x(~1));
#ifdef SEK_USE_AVX512_DQ
		b = _mm_cvtepi64_pd(c);
#else
		b = x86_cvtepi64_pd(c);
#endif

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

#ifdef SEK_USE_FMA
		a = _mm_fmadd_pd(_mm_set1_pd(dp_d[0]), b, a); /* a = (dp_d[0] * b) + a */
		a = _mm_fmadd_pd(_mm_set1_pd(dp_d[1]), b, a); /* a = (dp_d[1] * b) + a */
		a = _mm_fmadd_pd(_mm_set1_pd(dp_d[2]), b, a); /* a = (dp_d[2] * b) + a */
#else
		a = _mm_add_pd(_mm_mul_pd(_mm_set1_pd(dp_d[0]), b), a);
		a = _mm_add_pd(_mm_mul_pd(_mm_set1_pd(dp_d[1]), b), a);
		a = _mm_add_pd(_mm_mul_pd(_mm_set1_pd(dp_d[2]), b), a);
#endif
		const auto a2 = _mm_mul_pd(a, a);

		/* P1 (Pi/4 <= a <= 0) */
		auto p1 = _mm_set1_pd(coscof_d[0]);
#ifdef SEK_USE_FMA
		p1 = _mm_fmadd_pd(p1, a2, _mm_set1_pd(coscof_d[1])); /* p1 = (p1 * a2) + coscof_d[1] */
		p1 = _mm_fmadd_pd(p1, a2, _mm_set1_pd(coscof_d[2])); /* p1 = (p1 * a2) + coscof_d[2] */
		p1 = _mm_fmadd_pd(p1, a2, _mm_set1_pd(coscof_d[3])); /* p1 = (p1 * a2) + coscof_d[3] */
		p1 = _mm_fmadd_pd(p1, a2, _mm_set1_pd(coscof_d[4])); /* p1 = (p1 * a2) + coscof_d[4] */
		p1 = _mm_fmadd_pd(p1, a2, _mm_set1_pd(coscof_d[5])); /* p1 = (p1 * a2) + coscof_d[5] */
#else
		p1 = _mm_add_pd(_mm_mul_pd(p1, a2), _mm_set1_pd(coscof_d[1]));
		p1 = _mm_add_pd(_mm_mul_pd(p1, a2), _mm_set1_pd(coscof_d[2]));
		p1 = _mm_add_pd(_mm_mul_pd(p1, a2), _mm_set1_pd(coscof_d[3]));
		p1 = _mm_add_pd(_mm_mul_pd(p1, a2), _mm_set1_pd(coscof_d[4]));
		p1 = _mm_add_pd(_mm_mul_pd(p1, a2), _mm_set1_pd(coscof_d[5]));
#endif
		p1 = _mm_mul_pd(_mm_mul_pd(p1, a2), a2); /* p1 = p1 * a2 * a2 */
#ifdef SEK_USE_FMA
		p1 = _mm_fmadd_pd(a2, _mm_set1_pd(-0.5), p1); /* p1 = (a2 * -0.5) + p1 */
#else
		p1 = _mm_sub_pd(p1, _mm_mul_pd(a2, _mm_set1_ps(0.5))); /* p1 = p1 - (a2 * 0.5) */
#endif
		p1 = _mm_add_pd(p1, _mm_set1_pd(1.0));

		/* P2 (0 <= a <= Pi/4) */
		auto p2 = _mm_set1_pd(sincof_d[0]);
#ifdef SEK_USE_FMA
		p2 = _mm_fmadd_pd(p2, a2, _mm_set1_pd(sincof_d[1])); /* p2 = (p2 * a2) + sincof_d[1] */
		p2 = _mm_fmadd_pd(p2, a2, _mm_set1_pd(sincof_d[2])); /* p2 = (p2 * a2) + sincof_d[2] */
		p2 = _mm_fmadd_pd(p2, a2, _mm_set1_pd(sincof_d[3])); /* p2 = (p2 * a2) + sincof_d[3] */
		p2 = _mm_fmadd_pd(p2, a2, _mm_set1_pd(sincof_d[4])); /* p2 = (p2 * a2) + sincof_d[4] */
		p2 = _mm_fmadd_pd(p2, a2, _mm_set1_pd(sincof_d[5])); /* p2 = (p2 * a2) + sincof_d[5] */
		p2 = _mm_fmadd_pd(_mm_mul_pd(p2, a2), a, a);		 /* p2 = ((p2 * a2) * a) + a */
#else
		p2 = _mm_add_pd(_mm_mul_pd(p2, a2), _mm_set1_pd(sincof_d[1]));
		p2 = _mm_add_pd(_mm_mul_pd(p2, a2), _mm_set1_pd(sincof_d[2]));
		p2 = _mm_add_pd(_mm_mul_pd(p2, a2), _mm_set1_pd(sincof_d[3]));
		p2 = _mm_add_pd(_mm_mul_pd(p2, a2), _mm_set1_pd(sincof_d[4]));
		p2 = _mm_add_pd(_mm_mul_pd(p2, a2), _mm_set1_pd(sincof_d[5]));
		p2 = _mm_add_pd(_mm_mul_pd(_mm_mul_pd(p2, a2), a), a);
#endif

		/* Choose between P1 and P2 */
#ifdef SEK_USE_SSE4_1
		const auto result = _mm_blendv_pd(p1, p2, select_mask);
#else
		const auto result = _mm_add_pd(_mm_and_pd(select_mask, p2), _mm_andnot_pd(select_mask, p1));
#endif
		return _mm_xor_pd(result, sign);
	}
	__m128d x86_cos_pd(__m128d v) noexcept
	{
		const auto abs_mask = _mm_set1_pd(std::bit_cast<double>(0x7fff'ffff'ffff'ffff));

		auto a = _mm_and_pd(v, abs_mask);			/* a = |v| */
		auto b = _mm_mul_pd(a, _mm_set1_pd(pi4_d)); /* b = a * (4 / PI) */

		/* c = (int64) b */
#ifdef SEK_USE_AVX512_DQ
		auto c = _mm_cvttpd_epi64(b);
#else
		auto c = x86_cvtpd_epi64(b);
#endif

		/* c = (c + 1) & (~1) */
		c = _mm_add_epi64(c, _mm_set1_epi64x(1));
		c = _mm_and_si128(c, _mm_set1_epi64x(~1));
#ifdef SEK_USE_AVX512_DQ
		b = _mm_cvtepi64_pd(c);
#else
		b = x86_cvtepi64_pd(c);
#endif

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

#ifdef SEK_USE_FMA
		a = _mm_fmadd_pd(_mm_set1_pd(dp_d[0]), b, a); /* a = (dp_d[0] * b) + a */
		a = _mm_fmadd_pd(_mm_set1_pd(dp_d[1]), b, a); /* a = (dp_d[1] * b) + a */
		a = _mm_fmadd_pd(_mm_set1_pd(dp_d[2]), b, a); /* a = (dp_d[2] * b) + a */
#else
		a = _mm_add_pd(_mm_mul_pd(_mm_set1_pd(dp_d[0]), b), a);
		a = _mm_add_pd(_mm_mul_pd(_mm_set1_pd(dp_d[1]), b), a);
		a = _mm_add_pd(_mm_mul_pd(_mm_set1_pd(dp_d[2]), b), a);
#endif
		const auto a2 = _mm_mul_pd(a, a);

		/* P1 (0 <= a <= Pi/4) */
		auto p1 = _mm_set1_pd(coscof_d[0]);
#ifdef SEK_USE_FMA
		p1 = _mm_fmadd_pd(p1, a2, _mm_set1_pd(coscof_d[1])); /* p1 = (p1 * a2) + coscof_d[1] */
		p1 = _mm_fmadd_pd(p1, a2, _mm_set1_pd(coscof_d[2])); /* p1 = (p1 * a2) + coscof_d[2] */
		p1 = _mm_fmadd_pd(p1, a2, _mm_set1_pd(coscof_d[3])); /* p1 = (p1 * a2) + coscof_d[3] */
		p1 = _mm_fmadd_pd(p1, a2, _mm_set1_pd(coscof_d[4])); /* p1 = (p1 * a2) + coscof_d[4] */
		p1 = _mm_fmadd_pd(p1, a2, _mm_set1_pd(coscof_d[5])); /* p1 = (p1 * a2) + coscof_d[5] */
#else
		p1 = _mm_add_pd(_mm_mul_pd(p1, a2), _mm_set1_pd(coscof_d[1]));
		p1 = _mm_add_pd(_mm_mul_pd(p1, a2), _mm_set1_pd(coscof_d[2]));
		p1 = _mm_add_pd(_mm_mul_pd(p1, a2), _mm_set1_pd(coscof_d[3]));
		p1 = _mm_add_pd(_mm_mul_pd(p1, a2), _mm_set1_pd(coscof_d[4]));
		p1 = _mm_add_pd(_mm_mul_pd(p1, a2), _mm_set1_pd(coscof_d[5]));
#endif
		p1 = _mm_mul_pd(_mm_mul_pd(p1, a2), a2);
#ifdef SEK_USE_FMA
		p1 = _mm_fmadd_pd(a2, _mm_set1_pd(-0.5), p1); /* p1 = (a2 * -0.5) + p1 */
#else
		p1 = _mm_sub_pd(p1, _mm_mul_pd(a2, _mm_set1_ps(0.5))); /* p1 = p1 - (a2 * 0.5) */
#endif
		p1 = _mm_add_pd(p1, _mm_set1_pd(1.0));

		/* P2 (Pi/4 <= a <= 0) */
		auto p2 = _mm_set1_pd(sincof_d[0]);
#ifdef SEK_USE_FMA
		p2 = _mm_fmadd_pd(p2, a2, _mm_set1_pd(sincof_d[1])); /* p2 = (p2 * a2) + sincof_d[1] */
		p2 = _mm_fmadd_pd(p2, a2, _mm_set1_pd(sincof_d[2])); /* p2 = (p2 * a2) + sincof_d[2] */
		p2 = _mm_fmadd_pd(p2, a2, _mm_set1_pd(sincof_d[3])); /* p2 = (p2 * a2) + sincof_d[3] */
		p2 = _mm_fmadd_pd(p2, a2, _mm_set1_pd(sincof_d[4])); /* p2 = (p2 * a2) + sincof_d[4] */
		p2 = _mm_fmadd_pd(p2, a2, _mm_set1_pd(sincof_d[5])); /* p2 = (p2 * a2) + sincof_d[5] */
		p2 = _mm_fmadd_pd(_mm_mul_pd(p2, a2), a, a);		 /* p2 = ((p2 * a2) * a) + a */
#else
		p2 = _mm_add_pd(_mm_mul_pd(p2, a2), _mm_set1_pd(sincof_d[1]));
		p2 = _mm_add_pd(_mm_mul_pd(p2, a2), _mm_set1_pd(sincof_d[2]));
		p2 = _mm_add_pd(_mm_mul_pd(p2, a2), _mm_set1_pd(sincof_d[3]));
		p2 = _mm_add_pd(_mm_mul_pd(p2, a2), _mm_set1_pd(sincof_d[4]));
		p2 = _mm_add_pd(_mm_mul_pd(p2, a2), _mm_set1_pd(sincof_d[5]));
		p2 = _mm_add_pd(_mm_mul_pd(_mm_mul_pd(p2, a2), a), a);
#endif

		/* Choose between P1 and P2 */
#ifdef SEK_USE_SSE4_1
		const auto result = _mm_blendv_pd(p1, p2, select_mask);
#else
		const auto result = _mm_add_pd(_mm_and_pd(select_mask, p2), _mm_andnot_pd(select_mask, p1));
#endif
		return _mm_xor_pd(result, sign);
	}
}	 // namespace sek::math::detail
#endif
