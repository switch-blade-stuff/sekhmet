/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "common.hpp"

#ifdef SEK_USE_SSE
namespace sek::math::detail
{
	template<std::size_t N>
	inline void vector_add(simd_vector<float, N> &out, const simd_vector<float, N> &l, const simd_vector<float, N> &r) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = _mm_add_ps(l.simd, r.simd);
	}
	template<std::size_t N>
	inline void vector_sub(simd_vector<float, N> &out, const simd_vector<float, N> &l, const simd_vector<float, N> &r) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = _mm_sub_ps(l.simd, r.simd);
	}
	template<std::size_t N>
	inline void vector_mul(simd_vector<float, N> &out, const simd_vector<float, N> &l, const simd_vector<float, N> &r) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = _mm_mul_ps(l.simd, r.simd);
	}
	template<std::size_t N>
	inline void vector_div(simd_vector<float, N> &out, const simd_vector<float, N> &l, const simd_vector<float, N> &r) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = _mm_div_ps(l.simd, r.simd);
	}
	template<std::size_t N>
	inline void vector_neg(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = _mm_sub_ps(_mm_setzero_ps(), v.simd);
	}
	template<std::size_t N>
	inline void vector_abs(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
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

	template<std::size_t N>
	inline void vector_fmadd(simd_vector<float, N> &out,
							 const simd_vector<float, N> &a,
							 const simd_vector<float, N> &b,
							 const simd_vector<float, N> &c) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = x86_fmadd_ps(a.simd, b.simd, c.simd);
	}
	template<std::size_t N>
	inline void vector_fmsub(simd_vector<float, N> &out,
							 const simd_vector<float, N> &a,
							 const simd_vector<float, N> &b,
							 const simd_vector<float, N> &c) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = x86_fmsub_ps(a.simd, b.simd, c.simd);
	}

#ifdef SEK_USE_SSE2
	inline void vector_add(simd_vector<double, 2> &out, const simd_vector<double, 2> &l, const simd_vector<double, 2> &r) noexcept
	{
		out.simd = _mm_add_pd(l.simd, r.simd);
	}
	inline void vector_sub(simd_vector<double, 2> &out, const simd_vector<double, 2> &l, const simd_vector<double, 2> &r) noexcept
	{
		out.simd = _mm_sub_pd(l.simd, r.simd);
	}
	inline void vector_mul(simd_vector<double, 2> &out, const simd_vector<double, 2> &l, const simd_vector<double, 2> &r) noexcept
	{
		out.simd = _mm_mul_pd(l.simd, r.simd);
	}
	inline void vector_div(simd_vector<double, 2> &out, const simd_vector<double, 2> &l, const simd_vector<double, 2> &r) noexcept
	{
		out.simd = _mm_div_pd(l.simd, r.simd);
	}
	inline void vector_neg(simd_vector<double, 2> &out, const simd_vector<double, 2> &v) noexcept
	{
		out.simd = _mm_sub_pd(_mm_setzero_pd(), v.simd);
	}
	inline void vector_abs(simd_vector<double, 2> &out, const simd_vector<double, 2> &v) noexcept
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

	inline void vector_fmadd(simd_vector<double, 2> &out,
							 const simd_vector<double, 2> &a,
							 const simd_vector<double, 2> &b,
							 const simd_vector<double, 2> &c) noexcept
	{
		out.simd = x86_fmadd_pd(a.simd, b.simd, c.simd);
	}
	inline void vector_fmsub(simd_vector<double, 2> &out,
							 const simd_vector<double, 2> &a,
							 const simd_vector<double, 2> &b,
							 const simd_vector<double, 2> &c) noexcept
	{
		out.simd = x86_fmsub_pd(a.simd, b.simd, c.simd);
	}

	template<integral_of_size<4> T, std::size_t N>
	inline void vector_add(simd_vector<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires simd_enabled<simd_vector<T, N>>
	{
		out.simd = _mm_add_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void vector_sub(simd_vector<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires simd_enabled<simd_vector<T, N>>
	{
		out.simd = _mm_sub_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void vector_neg(simd_vector<T, N> &out, const simd_vector<T, N> &v) noexcept
		requires simd_enabled<simd_vector<T, N>>
	{
		out.simd = _mm_sub_epi32(_mm_setzero_si128(), v.simd);
	}
#ifdef SEK_USE_SSSE3
	template<integral_of_size<4> T, std::size_t N>
	inline void vector_abs(simd_vector<T, N> &out, const simd_vector<T, N> &v) noexcept
		requires simd_enabled<simd_vector<T, N>>
	{
		out.simd = _mm_abs_epi32(v.simd);
	}
#endif

	template<integral_of_size<8> T>
	inline void vector_add(simd_vector<T, 2> &out, const simd_vector<T, 2> &l, const simd_vector<T, 2> &r) noexcept
	{
		out.simd = _mm_add_epi64(l.simd, r.simd);
	}
	template<integral_of_size<8> T>
	inline void vector_sub(simd_vector<T, 2> &out, const simd_vector<T, 2> &l, const simd_vector<T, 2> &r) noexcept
	{
		out.simd = _mm_sub_epi64(l.simd, r.simd);
	}
	template<integral_of_size<8> T>
	inline void vector_neg(simd_vector<T, 2> &out, const simd_vector<T, 2> &v) noexcept
	{
		out.simd = _mm_sub_epi64(_mm_setzero_si128(), v.simd);
	}

#ifndef SEK_USE_AVX
	template<std::size_t N>
	inline void vector_add(simd_vector<double, N> &out, const simd_vector<double, N> &l, const simd_vector<double, N> &r) noexcept
		requires simd_enabled<simd_vector<double, N>>
	{
		out.simd[0] = _mm_add_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_add_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N>
	inline void vector_sub(simd_vector<double, N> &out, const simd_vector<double, N> &l, const simd_vector<double, N> &r) noexcept
		requires simd_enabled<simd_vector<double, N>>
	{
		out.simd[0] = _mm_sub_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_sub_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N>
	inline void vector_mul(simd_vector<double, N> &out, const simd_vector<double, N> &l, const simd_vector<double, N> &r) noexcept
		requires simd_enabled<simd_vector<double, N>>
	{
		out.simd[0] = _mm_mul_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_mul_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N>
	inline void vector_div(simd_vector<double, N> &out, const simd_vector<double, N> &l, const simd_vector<double, N> &r) noexcept
		requires simd_enabled<simd_vector<double, N>>
	{
		out.simd[0] = _mm_div_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_div_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N>
	inline void vector_neg(simd_vector<double, N> &out, const simd_vector<double, N> &v) noexcept
		requires simd_enabled<simd_vector<double, N>>
	{
		const auto z = _mm_setzero_pd();
		out.simd[0] = _mm_sub_pd(z, v.simd[0]);
		out.simd[1] = _mm_sub_pd(z, v.simd[1]);
	}
	template<std::size_t N>
	inline void vector_abs(simd_vector<double, N> &out, const simd_vector<double, N> &v) noexcept
		requires simd_enabled<simd_vector<double, N>>
	{
		constexpr auto mask = std::bit_cast<double>(0x7fff'ffff'ffff'ffff);
		const auto m = _mm_set1_pd(mask);
		out.simd[0] = _mm_and_pd(m, v.simd[0]);
		out.simd[1] = _mm_and_pd(m, v.simd[1]);
	}

	template<std::size_t N>
	inline void vector_fmadd(simd_vector<double, N> &out,
							 const simd_vector<double, N> &a,
							 const simd_vector<double, N> &b,
							 const simd_vector<double, N> &c) noexcept
		requires simd_enabled<simd_vector<double, N>>
	{
		out.simd[0] = x86_fmadd_pd(a.simd[0], b.simd[0], c.simd[0]);
		out.simd[1] = x86_fmadd_pd(a.simd[1], b.simd[1], c.simd[1]);
	}
	template<std::size_t N>
	inline void vector_fmsub(simd_vector<double, N> &out,
							 const simd_vector<double, N> &a,
							 const simd_vector<double, N> &b,
							 const simd_vector<double, N> &c) noexcept
		requires simd_enabled<simd_vector<double, N>>
	{
		out.simd[0] = x86_fmsub_pd(a.simd[0], b.simd[0], c.simd[0]);
		out.simd[1] = x86_fmsub_pd(a.simd[1], b.simd[1], c.simd[1]);
	}

#ifndef SEK_USE_AVX2
	template<integral_of_size<8> T, std::size_t N>
	inline void vector_add(simd_vector<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires simd_enabled<simd_vector<T, N>>
	{
		out.simd[0] = _mm_add_epi64(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_add_epi64(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N>
	inline void vector_sub(simd_vector<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires simd_enabled<simd_vector<T, N>>
	{
		out.simd[0] = _mm_sub_epi64(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_sub_epi64(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N>
	inline void vector_neg(simd_vector<T, N> &out, const simd_vector<T, N> &v) noexcept
		requires simd_enabled<simd_vector<T, N>>
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