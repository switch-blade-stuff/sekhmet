/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "common.hpp"

#ifdef SEK_USE_SSE
namespace sek::detail
{
	template<policy_t P>
	inline void vector_cross(vector_data<float, 3, P> &out,
							 const vector_data<float, 3, P> &l,
							 const vector_data<float, 3, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		const auto a = _mm_shuffle_ps(l.simd, l.simd, _MM_SHUFFLE(3, 0, 2, 1));
		const auto b = _mm_shuffle_ps(r.simd, r.simd, _MM_SHUFFLE(3, 1, 0, 2));
		const auto c = _mm_mul_ps(a, r.simd);
		out.simd = _mm_sub_ps(_mm_mul_ps(a, b), _mm_shuffle_ps(c, c, _MM_SHUFFLE(3, 0, 2, 1)));
	}

#ifdef SEK_USE_SSE4_1
	template<policy_t P>
	inline float vector_dot(const vector_data<float, 3, P> &l, const vector_data<float, 3, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		return _mm_cvtss_f32(_mm_dp_ps(l.simd, r.simd, 0x71));
	}
	template<policy_t P>
	inline void vector_norm(vector_data<float, 3, P> &out, const vector_data<float, 3, P> &l) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_div_ps(l.simd, _mm_sqrt_ps(_mm_dp_ps(l.simd, l.simd, 0x7f)));
	}
	template<policy_t P>
	inline float vector_dot(const vector_data<float, 4, P> &l, const vector_data<float, 4, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		return _mm_cvtss_f32(_mm_dp_ps(l.simd, r.simd, 0xf1));
	}
	template<policy_t P>
	inline void vector_norm(vector_data<float, 4, P> &out, const vector_data<float, 4, P> &l) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_div_ps(l.simd, _mm_sqrt_ps(_mm_dp_ps(l.simd, l.simd, 0xff)));
	}
#else
	template<std::size_t N, policy_t P>
	inline float vector_dot(const vector_data<float, N, P> &l, const vector_data<float, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		const auto a = _mm_mul_ps(r.simd, l.simd);
		const auto b = _mm_shuffle_ps(a, a, _MM_SHUFFLE(2, 3, 0, 1));
		const auto c = _mm_add_ps(a, b);
		return _mm_cvtss_f32(_mm_add_ss(c, _mm_movehl_ps(b, c)));
	}
	template<std::size_t N, policy_t P>
	inline void vector_norm(vector_data<float, N, P> &out, const vector_data<float, N, P> &l) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_div_ps(l.simd, _mm_sqrt_ps(_mm_set1_ps(vector_dot(l, l))));
	}
#endif

#if defined(SEK_USE_SSE2) && defined(SEK_USE_SSE4_1)
	template<policy_t P>
	inline double vector_dot(const vector_data<double, 2, P> &l, const vector_data<double, 2, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		return _mm_cvtsd_f64(_mm_dp_pd(l.simd, r.simd, 0xf1));
	}
	template<policy_t P>
	inline void vector_norm(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &l) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_div_pd(l.simd, _mm_sqrt_pd(_mm_dp_pd(l.simd, l.simd, 0xff)));
	}

#ifndef SEK_USE_AVX
	template<policy_t P>
	inline void vector_cross(vector_data<double, 3, P> &out,
							 const vector_data<double, 3, P> &l,
							 const vector_data<double, 3, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		/* 4 shuffles are needed here since the 3 doubles are split across 2 __m128d vectors. */
		const auto a = _mm_shuffle_pd(l.simd[0], l.simd[1], _MM_SHUFFLE2(0, 1));
		const auto b = _mm_shuffle_pd(r.simd[0], r.simd[1], _MM_SHUFFLE2(0, 1));

		out.simd[0] = _mm_sub_pd(_mm_mul_pd(a, _mm_shuffle_pd(r.simd[1], r.simd[0], _MM_SHUFFLE2(0, 0))),
								 _mm_mul_pd(b, _mm_shuffle_pd(l.simd[1], l.simd[0], _MM_SHUFFLE2(0, 0))));
		out.simd[1] = _mm_sub_pd(_mm_mul_pd(l.simd[0], b), _mm_mul_pd(r.simd[0], a));
	}
	template<policy_t P>
	inline double vector_dot(const vector_data<double, 3, P> &l, const vector_data<double, 3, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		// clang-format off
		return _mm_cvtsd_f64(_mm_add_pd(
			_mm_dp_pd(l.simd[0], r.simd[0], 0xf1),
			_mm_dp_pd(l.simd[1], r.simd[1], 0x11)));
		// clang-format on
	}
	template<policy_t P>
	inline void vector_norm(vector_data<double, 3, P> &out, const vector_data<double, 3, P> &l) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		// clang-format off
		const auto magn = _mm_sqrt_pd(_mm_add_pd(
			_mm_dp_pd(l.simd[0], l.simd[0], 0xff),
			_mm_dp_pd(l.simd[1], l.simd[1], 0x1f)));
		// clang-format on
		out.simd[0] = _mm_div_pd(l.simd[0], magn);
		out.simd[1] = _mm_div_pd(l.simd[1], magn);
	}
	template<policy_t P>
	inline double vector_dot(const vector_data<double, 4, P> &l, const vector_data<double, 4, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		// clang-format off
		return _mm_cvtsd_f64(_mm_add_pd(
			_mm_dp_pd(l.simd[0], r.simd[0], 0xf1),
			_mm_dp_pd(l.simd[1], r.simd[1], 0xf1)));
		// clang-format on
	}
	template<policy_t P>
	inline void vector_norm(vector_data<double, 4, P> &out, const vector_data<double, 4, P> &l) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
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
	template<policy_t P>
	inline double vector_dot(const vector_data<double, 2, P> &l, const vector_data<double, 2, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		const auto a = _mm_mul_pd(r.simd, l.simd);
		const auto b = _mm_shuffle_pd(a, a, _MM_SHUFFLE2(0, 1));
		return _mm_cvtsd_f64(_mm_add_sd(a, b));
	}
	template<policy_t P>
	inline void vector_norm(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &l) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_div_pd(l.simd, _mm_sqrt_pd(_mm_set1_pd(vector_dot(l, l))));
	}

#ifndef SEK_USE_AVX
	template<std::size_t N, policy_t P>
	inline double vector_dot(const vector_data<double, N, P> &l, const vector_data<double, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		const __m128d a[2] = {_mm_mul_pd(r.simd[0], l.simd[0]), _mm_mul_pd(r.simd[1], l.simd[1])};
		const __m128d b[2] = {_mm_shuffle_pd(a[0], a[0], _MM_SHUFFLE2(0, 1)), _mm_shuffle_pd(a[1], a[1], _MM_SHUFFLE2(0, 1))};
		return _mm_cvtsd_f64(_mm_add_sd(_mm_add_sd(a[0], b[0]), _mm_add_sd(a[1], b[1])));
	}
	template<std::size_t N, policy_t P>
	inline void vector_norm(vector_data<double, N, P> &out, const vector_data<double, N, P> &l) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		const auto magn = _mm_sqrt_pd(_mm_set1_pd(vector_dot(l, l)));
		out.simd[0] = _mm_div_pd(l.simd[0], magn);
		out.simd[1] = _mm_div_pd(l.simd[1], magn);
	}
#endif
#endif
}	 // namespace sek::detail
#endif