/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "common.hpp"

#ifdef SEK_USE_SSE
namespace sek::math::detail
{
	template<std::size_t N, storage_policy P>
	inline void vector_add(vector_data<float, N, P> &out, const vector_data<float, N, P> &l, const vector_data<float, N, P> &r) noexcept
		requires simd_enabled<vector_data<float, N, P>>
	{
		out.simd = _mm_add_ps(l.simd, r.simd);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_sub(vector_data<float, N, P> &out, const vector_data<float, N, P> &l, const vector_data<float, N, P> &r) noexcept
		requires simd_enabled<vector_data<float, N, P>>
	{
		out.simd = _mm_sub_ps(l.simd, r.simd);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_mul(vector_data<float, N, P> &out, const vector_data<float, N, P> &l, const vector_data<float, N, P> &r) noexcept
		requires simd_enabled<vector_data<float, N, P>>
	{
		out.simd = _mm_mul_ps(l.simd, r.simd);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_div(vector_data<float, N, P> &out, const vector_data<float, N, P> &l, const vector_data<float, N, P> &r) noexcept
		requires simd_enabled<vector_data<float, N, P>>
	{
		out.simd = _mm_div_ps(l.simd, r.simd);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_neg(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires simd_enabled<vector_data<float, N, P>>
	{
		out.simd = _mm_sub_ps(_mm_setzero_ps(), v.simd);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_abs(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires simd_enabled<vector_data<float, N, P>>
	{
		constexpr auto mask = std::bit_cast<float>(0x7fff'ffff);
		out.simd = _mm_and_ps(_mm_set1_ps(mask), v.simd);
	}

	SEK_FORCE_INLINE __m128 x86_fmadd_ps(__m128 a, __m128 b, __m128 c) noexcept
	{
#ifdef SEK_USE_FMA
		return _mm_fmadd_ps(a, b, c);
#else
		return _mm_add_ps(_mm_mul_ps(a, b), c);
#endif
	}
	SEK_FORCE_INLINE __m128 x86_fmsub_ps(__m128 a, __m128 b, __m128 c) noexcept
	{
#ifdef SEK_USE_FMA
		return _mm_fmsub_ps(a, b, c);
#else
		return _mm_sub_ps(_mm_mul_ps(a, b), c);
#endif
	}

	template<std::size_t N, storage_policy P>
	inline void vector_fmadd(vector_data<float, N, P> &out,
							 const vector_data<float, N, P> &a,
							 const vector_data<float, N, P> &b,
							 const vector_data<float, N, P> &c) noexcept
		requires simd_enabled<vector_data<float, N, P>>
	{
		out.simd = x86_fmadd_ps(a.simd, b.simd, c.simd);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_fmsub(vector_data<float, N, P> &out,
							 const vector_data<float, N, P> &a,
							 const vector_data<float, N, P> &b,
							 const vector_data<float, N, P> &c) noexcept
		requires simd_enabled<vector_data<float, N, P>>
	{
		out.simd = x86_fmsub_ps(a.simd, b.simd, c.simd);
	}

#ifdef SEK_USE_SSE2
	template<storage_policy P>
	inline void vector_add(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &l, const vector_data<double, 2, P> &r) noexcept
		requires simd_enabled<vector_data<double, 2, P>>
	{
		out.simd = _mm_add_pd(l.simd, r.simd);
	}
	template<storage_policy P>
	inline void vector_sub(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &l, const vector_data<double, 2, P> &r) noexcept
		requires simd_enabled<vector_data<double, 2, P>>
	{
		out.simd = _mm_sub_pd(l.simd, r.simd);
	}
	template<storage_policy P>
	inline void vector_mul(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &l, const vector_data<double, 2, P> &r) noexcept
		requires simd_enabled<vector_data<double, 2, P>>
	{
		out.simd = _mm_mul_pd(l.simd, r.simd);
	}
	template<storage_policy P>
	inline void vector_div(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &l, const vector_data<double, 2, P> &r) noexcept
		requires simd_enabled<vector_data<double, 2, P>>
	{
		out.simd = _mm_div_pd(l.simd, r.simd);
	}
	template<storage_policy P>
	inline void vector_neg(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires simd_enabled<vector_data<double, 2, P>>
	{
		out.simd = _mm_sub_pd(_mm_setzero_pd(), v.simd);
	}
	template<storage_policy P>
	inline void vector_abs(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires simd_enabled<vector_data<double, 2, P>>
	{
		constexpr auto mask = std::bit_cast<double>(0x7fff'ffff'ffff'ffff);
		out.simd = _mm_and_pd(_mm_set1_pd(mask), v.simd);
	}

	SEK_FORCE_INLINE __m128d x86_fmadd_pd(__m128d a, __m128d b, __m128d c) noexcept
	{
#ifdef SEK_USE_FMA
		return _mm_fmadd_pd(a, b, c);
#else
		return _mm_add_pd(_mm_mul_pd(a, b), c);
#endif
	}
	SEK_FORCE_INLINE __m128d x86_fmsub_pd(__m128d a, __m128d b, __m128d c) noexcept
	{
#ifdef SEK_USE_FMA
		return _mm_fmsub_pd(a, b, c);
#else
		return _mm_sub_pd(_mm_mul_pd(a, b), c);
#endif
	}

		template<storage_policy P>
	inline void vector_fmadd(vector_data<double, 2, P> &out,
							 const vector_data<double, 2, P> &a,
							 const vector_data<double, 2, P> &b,
							 const vector_data<double, 2, P> &c) noexcept
		requires simd_enabled<vector_data<double, 2, P>>
	{
		out.simd = x86_fmadd_pd(a.simd, b.simd, c.simd);
	}
	template<storage_policy P>
	inline void vector_fmsub(vector_data<double, 2, P> &out,
							 const vector_data<double, 2, P> &a,
							 const vector_data<double, 2, P> &b,
							 const vector_data<double, 2, P> &c) noexcept
		requires simd_enabled<vector_data<double, 2, P>>
	{
		out.simd = x86_fmsub_pd(a.simd, b.simd, c.simd);
	}

	template<integral_of_size<4> T, std::size_t N, storage_policy P>
	inline void vector_add(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires simd_enabled<vector_data<T, N, P>>
	{
		out.simd = _mm_add_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N, storage_policy P>
	inline void vector_sub(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires simd_enabled<vector_data<T, N, P>>
	{
		out.simd = _mm_sub_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N, storage_policy P>
	inline void vector_neg(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
		requires simd_enabled<vector_data<T, N, P>>
	{
		out.simd = _mm_sub_epi32(_mm_setzero_si128(), v.simd);
	}
#ifdef SEK_USE_SSSE3
	template<integral_of_size<4> T, std::size_t N, storage_policy P>
	inline void vector_abs(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
		requires simd_enabled<vector_data<T, N, P>>
	{
		out.simd = _mm_abs_epi32(v.simd);
	}
#endif

	template<integral_of_size<8> T, storage_policy P>
	inline void vector_add(vector_data<T, 2, P> &out, const vector_data<T, 2, P> &l, const vector_data<T, 2, P> &r) noexcept
		requires simd_enabled<vector_data<T, 2, P>>
	{
		out.simd = _mm_add_epi64(l.simd, r.simd);
	}
	template<integral_of_size<8> T, storage_policy P>
	inline void vector_sub(vector_data<T, 2, P> &out, const vector_data<T, 2, P> &l, const vector_data<T, 2, P> &r) noexcept
		requires simd_enabled<vector_data<T, 2, P>>
	{
		out.simd = _mm_sub_epi64(l.simd, r.simd);
	}
	template<integral_of_size<8> T, storage_policy P>
	inline void vector_neg(vector_data<T, 2, P> &out, const vector_data<T, 2, P> &v) noexcept
		requires simd_enabled<vector_data<T, 2, P>>
	{
		out.simd = _mm_sub_epi64(_mm_setzero_si128(), v.simd);
	}

#ifndef SEK_USE_AVX
	template<std::size_t N, storage_policy P>
	inline void vector_add(vector_data<double, N, P> &out, const vector_data<double, N, P> &l, const vector_data<double, N, P> &r) noexcept
		requires simd_enabled<vector_data<double, N, P>>
	{
		out.simd[0] = _mm_add_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_add_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_sub(vector_data<double, N, P> &out, const vector_data<double, N, P> &l, const vector_data<double, N, P> &r) noexcept
		requires simd_enabled<vector_data<double, N, P>>
	{
		out.simd[0] = _mm_sub_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_sub_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_mul(vector_data<double, N, P> &out, const vector_data<double, N, P> &l, const vector_data<double, N, P> &r) noexcept
		requires simd_enabled<vector_data<double, N, P>>
	{
		out.simd[0] = _mm_mul_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_mul_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_div(vector_data<double, N, P> &out, const vector_data<double, N, P> &l, const vector_data<double, N, P> &r) noexcept
		requires simd_enabled<vector_data<double, N, P>>
	{
		out.simd[0] = _mm_div_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_div_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_neg(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires simd_enabled<vector_data<double, N, P>>
	{
		const auto z = _mm_setzero_pd();
		out.simd[0] = _mm_sub_pd(z, v.simd[0]);
		out.simd[1] = _mm_sub_pd(z, v.simd[1]);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_abs(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires simd_enabled<vector_data<double, N, P>>
	{
		constexpr auto mask = std::bit_cast<double>(0x7fff'ffff'ffff'ffff);
		const auto m = _mm_set1_pd(mask);
		out.simd[0] = _mm_and_pd(m, v.simd[0]);
		out.simd[1] = _mm_and_pd(m, v.simd[1]);
	}

	template<std::size_t N, storage_policy P>
	inline void vector_fmadd(vector_data<double, N, P> &out,
							 const vector_data<double, N, P> &a,
							 const vector_data<double, N, P> &b,
							 const vector_data<double, N, P> &c) noexcept
		requires simd_enabled<vector_data<double, N, P>>
	{
		out.simd[0] = x86_fmadd_pd(a.simd[0], b.simd[0], c.simd[0]);
		out.simd[1] = x86_fmadd_pd(a.simd[1], b.simd[1], c.simd[1]);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_fmsub(vector_data<double, N, P> &out,
							 const vector_data<double, N, P> &a,
							 const vector_data<double, N, P> &b,
							 const vector_data<double, N, P> &c) noexcept
		requires simd_enabled<vector_data<double, N, P>>
	{
		out.simd[0] = x86_fmsub_pd(a.simd[0], b.simd[0], c.simd[0]);
		out.simd[1] = x86_fmsub_pd(a.simd[1], b.simd[1], c.simd[1]);
	}

#ifndef SEK_USE_AVX2
	template<integral_of_size<8> T, std::size_t N, storage_policy P>
	inline void vector_add(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires simd_enabled<vector_data<T, N, P>>
	{
		out.simd[0] = _mm_add_epi64(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_add_epi64(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N, storage_policy P>
	inline void vector_sub(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires simd_enabled<vector_data<T, N, P>>
	{
		out.simd[0] = _mm_sub_epi64(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_sub_epi64(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N, storage_policy P>
	inline void vector_neg(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
		requires simd_enabled<vector_data<T, N, P>>
	{
		const auto z = _mm_setzero_si128();
		out.simd[0] = _mm_sub_epi64(z, v.simd[0]);
		out.simd[1] = _mm_sub_epi64(z, v.simd[1]);
	}
#endif
#endif
#endif
}	 // namespace sek::math::detail
#endif