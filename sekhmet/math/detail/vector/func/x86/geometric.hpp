/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "common.hpp"

#ifdef SEK_USE_SSE
namespace sek::math::detail
{
	inline void vector_cross(simd_vector<float, 3> &out, const simd_vector<float, 3> &l, const simd_vector<float, 3> &r) noexcept
	{
		const auto a = _mm_shuffle_ps(l.simd, l.simd, _MM_SHUFFLE(3, 0, 2, 1));
		const auto b = _mm_shuffle_ps(r.simd, r.simd, _MM_SHUFFLE(3, 1, 0, 2));
		const auto c = _mm_mul_ps(a, r.simd);
		out.simd = _mm_sub_ps(_mm_mul_ps(a, b), _mm_shuffle_ps(c, c, _MM_SHUFFLE(3, 0, 2, 1)));
	}

#ifdef SEK_USE_SSE4_1
	inline float vector_dot(const simd_vector<float, 3> &l, const simd_vector<float, 3> &r) noexcept
	{
		return _mm_cvtss_f32(_mm_dp_ps(l.simd, r.simd, 0x71));
	}
	inline void vector_norm(simd_vector<float, 3> &out, const simd_vector<float, 3> &l) noexcept
	{
		out.simd = _mm_div_ps(l.simd, _mm_sqrt_ps(_mm_dp_ps(l.simd, l.simd, 0x7f)));
	}
	inline float vector_dot(const simd_vector<float, 4> &l, const simd_vector<float, 4> &r) noexcept
	{
		return _mm_cvtss_f32(_mm_dp_ps(l.simd, r.simd, 0xf1));
	}
	inline void vector_norm(simd_vector<float, 4> &out, const simd_vector<float, 4> &l) noexcept
	{
		out.simd = _mm_div_ps(l.simd, _mm_sqrt_ps(_mm_dp_ps(l.simd, l.simd, 0xff)));
	}
#else
	template<std::size_t N>
	inline float vector_dot(const simd_vector<float, N> &l, const simd_vector<float, N> &r) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		const auto a = _mm_mul_ps(r.simd, l.simd);
		const auto b = _mm_shuffle_ps(a, a, _MM_SHUFFLE(2, 3, 0, 1));
		const auto c = _mm_add_ps(a, b);
		return _mm_cvtss_f32(_mm_add_ss(c, _mm_movehl_ps(b, c)));
	}
	template<std::size_t N>
	inline void vector_norm(simd_vector<float, N> &out, const simd_vector<float, N> &l) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = _mm_div_ps(l.simd, _mm_sqrt_ps(_mm_set1_ps(vector_dot(l, l))));
	}
#endif

#if defined(SEK_USE_SSE2) && defined(SEK_USE_SSE4_1)
	inline double vector_dot(const simd_vector<double, 2> &l, const simd_vector<double, 2> &r) noexcept
	{
		return _mm_cvtsd_f64(_mm_dp_pd(l.simd, r.simd, 0xf1));
	}
	inline void vector_norm(simd_vector<double, 2> &out, const simd_vector<double, 2> &l) noexcept
	{
		out.simd = _mm_div_pd(l.simd, _mm_sqrt_pd(_mm_dp_pd(l.simd, l.simd, 0xff)));
	}

#ifndef SEK_USE_AVX
	inline void vector_cross(simd_vector<double, 3> &out, const simd_vector<double, 3> &l, const simd_vector<double, 3> &r) noexcept
	{
		/* 4 shuffles are needed here since the 3 doubles are split across 2 __m128d vectors. */
		const auto a = _mm_shuffle_pd(l.simd[0], l.simd[1], _MM_SHUFFLE2(0, 1));
		const auto b = _mm_shuffle_pd(r.simd[0], r.simd[1], _MM_SHUFFLE2(0, 1));

		out.simd[0] = _mm_sub_pd(_mm_mul_pd(a, _mm_shuffle_pd(r.simd[1], r.simd[0], _MM_SHUFFLE2(0, 0))),
								 _mm_mul_pd(b, _mm_shuffle_pd(l.simd[1], l.simd[0], _MM_SHUFFLE2(0, 0))));
		out.simd[1] = _mm_sub_pd(_mm_mul_pd(l.simd[0], b), _mm_mul_pd(r.simd[0], a));
	}
	inline double vector_dot(const simd_vector<double, 3> &l, const simd_vector<double, 3> &r) noexcept
	{
		// clang-format off
		return _mm_cvtsd_f64(_mm_add_pd(
			_mm_dp_pd(l.simd[0], r.simd[0], 0xf1),
			_mm_dp_pd(l.simd[1], r.simd[1], 0x11)));
		// clang-format on
	}
	inline void vector_norm(simd_vector<double, 3> &out, const simd_vector<double, 3> &l) noexcept
	{
		// clang-format off
		const auto magn = _mm_sqrt_pd(_mm_add_pd(
			_mm_dp_pd(l.simd[0], l.simd[0], 0xff),
			_mm_dp_pd(l.simd[1], l.simd[1], 0x1f)));
		// clang-format on
		out.simd[0] = _mm_div_pd(l.simd[0], magn);
		out.simd[1] = _mm_div_pd(l.simd[1], magn);
	}
	inline double vector_dot(const simd_vector<double, 4> &l, const simd_vector<double, 4> &r) noexcept
	{
		// clang-format off
		return _mm_cvtsd_f64(_mm_add_pd(
			_mm_dp_pd(l.simd[0], r.simd[0], 0xf1),
			_mm_dp_pd(l.simd[1], r.simd[1], 0xf1)));
		// clang-format on
	}
	inline void vector_norm(simd_vector<double, 4> &out, const simd_vector<double, 4> &l) noexcept
	{
		// clang-format off
		const auto magn = _mm_sqrt_pd(_mm_add_pd(
			_mm_dp_pd(l.simd[0], l.simd[0], 0xff),
			_mm_dp_pd(l.simd[1], l.simd[1], 0xff)));
		// clang-format on
		out.simd[0] = _mm_div_pd(l.simd[0], magn);
		out.simd[1] = _mm_div_pd(l.simd[1], magn);
	}
#endif
#else
	inline double vector_dot(const simd_vector<double, 2> &l, const simd_vector<double, 2> &r) noexcept
	{
		const auto a = _mm_mul_pd(r.simd, l.simd);
		const auto b = _mm_shuffle_pd(a, a, _MM_SHUFFLE2(0, 1));
		return _mm_cvtsd_f64(_mm_add_sd(a, b));
	}
	inline void vector_norm(simd_vector<double, 2> &out, const simd_vector<double, 2> &l) noexcept
	{
		out.simd = _mm_div_pd(l.simd, _mm_sqrt_pd(_mm_set1_pd(vector_dot(l, l))));
	}

#ifndef SEK_USE_AVX
	template<std::size_t N>
	inline double vector_dot(const simd_vector<double, N> &l, const simd_vector<double, N> &r) noexcept
		requires simd_enabled<simd_vector<double, N>>
	{
		const __m128d a[2] = {_mm_mul_pd(r.simd[0], l.simd[0]), _mm_mul_pd(r.simd[1], l.simd[1])};
		const __m128d b[2] = {_mm_shuffle_pd(a[0], a[0], _MM_SHUFFLE2(0, 1)), _mm_shuffle_pd(a[1], a[1], _MM_SHUFFLE2(0, 1))};
		return _mm_cvtsd_f64(_mm_add_sd(_mm_add_sd(a[0], b[0]), _mm_add_sd(a[1], b[1])));
	}
	template<std::size_t N>
	inline void vector_norm(simd_vector<double, N> &out, const simd_vector<double, N> &l) noexcept
		requires simd_enabled<simd_vector<double, N>>
	{
		const auto magn = _mm_sqrt_pd(_mm_set1_pd(vector_dot(l, l)));
		out.simd[0] = _mm_div_pd(l.simd[0], magn);
		out.simd[1] = _mm_div_pd(l.simd[1], magn);
	}
#endif
#endif
}	 // namespace sek::math::detail
#endif