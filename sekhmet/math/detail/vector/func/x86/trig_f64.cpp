/*
 * Created by switchblade on 26/06/22
 */

#include "arithm.hpp"
#include "trig.hpp"
#include "util_f64.hpp"
#include "util_i64.hpp"

/* Implementations of trigonometric functions derived from netlib's cephes library (http://www.netlib.org/cephes/)
 * Inspired by http://gruntthepeon.free.fr/ssemath */

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
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
