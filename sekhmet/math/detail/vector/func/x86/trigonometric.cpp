/*
 * Created by switchblade on 26/06/22
 */

#include "trigonometric.hpp"

#include "util.hpp"

/* Implementations of trigonometric functions derived from netlib's cephes library (http://www.netlib.org/cephes/)
 * Inspired by http://gruntthepeon.free.fr/ssemath */

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
	static const float sincof_f[3] = {
		-1.9515295891E-4f,
		8.3321608736E-3f,
		-1.6666654611E-1f,
	};
	static const float coscof_f[3] = {
		2.443315711809948E-005f,
		-1.388731625493765E-003f,
		4.166664568298827E-002f,
	};
	static const float dp_f[3] = {
		-0.78515625f,
		-2.4187564849853515625e-4f,
		-3.77489497744594108e-8f,
	};
	static const float pi4_f = 1.2732395447351626861510701069801148962756771659236515899813387524f;

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
		a = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(dp1_f), b), a);
		a = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(dp2_f), b), a);
		a = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(dp3_f), b), a);
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
	static const double dp_d[3] = {
		-7.85398125648498535156E-1,
		-3.77489470793079817668E-8,
		-2.69515142907905952645E-15,
	};
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

		c = _mm_and_si128(c, _mm_set1_epi64x(0xf));

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
		p1 = _mm_sub_pd(p1, _mm_mul_pd(a2, _mm_set1_ps(0.5f))); /* p1 = p1 - (a2 * 0.5) */
#endif
		p1 = _mm_add_pd(p1, _mm_set1_pd(1.0f));

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
}	 // namespace sek::math::detail
#endif
//
// double cos(double x)
//{
//	double y, z, zz;
//	long i;
//	int j, sign;
//
//#ifdef NANS
//	if (isnan(x)) return x;
//	if (!isfinite(x))
//	{
//		mtherr("cos", DOMAIN);
//		return (NAN);
//	}
//#endif
//
//	/* make argument positive */
//	sign = 1;
//	if (x < 0) x = -x;
//
//	if (x > lossth)
//	{
//		mtherr("cos", TLOSS);
//		return (0.0);
//	}
//
//	y = floor(x / PIO4);
//	z = ldexp(y, -4);
//	z = floor(z);		 /* integer part of y/8 */
//	z = y - ldexp(z, 4); /* y - 16 * (y/16) */
//
//	/* integer and fractional part modulo one octant */
//	i = z;
//	if (i & 1) /* map zeros to origin */
//	{
//		i += 1;
//		y += 1.0;
//	}
//	j = i & 07;
//	if (j > 3)
//	{
//		j -= 4;
//		sign = -sign;
//	}
//
//	if (j > 1) sign = -sign;
//
//	/* Extended precision modular arithmetic */
//	z = ((x - y * DP1) - y * DP2) - y * DP3;
//	zz = z * z;
//
//	if ((j == 1) || (j == 2))
//		y = z + z * z * z * polevl(zz, sincof, 5);
//	else { y = 1.0 - ldexp(zz, -1) + zz * zz * polevl(zz, coscof, 5); }
//
//	if (sign < 0) y = -y;
//	return y;
//}
