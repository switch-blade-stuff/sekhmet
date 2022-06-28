/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "common.hpp"

#ifdef SEK_USE_SSE
namespace sek::math::detail
{
	SEK_FORCE_INLINE __m128 x86_blendv_ps(__m128 a, __m128 b, __m128 m) noexcept
	{
#ifdef SEK_USE_SSE4_1
		return _mm_blendv_ps(a, b, m);
#else
		return _mm_add_ps(_mm_and_ps(m, b), _mm_andnot_ps(m, a));
#endif
	}

	template<std::size_t N, std::size_t M, storage_policy P, std::size_t... Is>
	inline void mask_shuffle(mask_data<float, N, P> &out, const mask_data<float, N, P> &m, std::index_sequence<Is...> s) noexcept
		requires simd_enabled<mask_data<float, N, P>>
	{
		constexpr auto mask = x86_128_shuffle4_mask(s);
		out.simd = _mm_shuffle_ps(m.simd, m.simd, mask);
	}

	template<std::size_t N, std::size_t M, storage_policy P, std::size_t... Is>
	inline void vector_shuffle(vector_data<float, N, P> &out, const vector_data<float, M, P> &l, std::index_sequence<Is...> s) noexcept
		requires simd_enabled<vector_data<float, N, P>>
	{
		constexpr auto mask = x86_128_shuffle4_mask(s);
		out.simd = _mm_shuffle_ps(l.simd, l.simd, mask);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_interleave(vector_data<float, N, P> &out,
								  const vector_data<float, N, P> &l,
								  const vector_data<float, N, P> &r,
								  mask_data<float, N, P> &m) noexcept
		requires simd_enabled<vector_data<float, N, P>> && simd_enabled<mask_data<float, N, P>>
	{
		out.simd = x86_blendv_ps(r.simd, l.simd, m.simd);
	}

#ifdef SEK_USE_SSE4_1
	SEK_FORCE_INLINE __m128 x86_floor_ps(__m128 v) noexcept { return _mm_floor_ps(v); }

	template<std::size_t N, storage_policy P>
	inline void vector_round(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires simd_enabled<vector_data<float, N, P>>
	{
		out.simd = _mm_round_ps(v.simd, _MM_FROUND_RINT);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_ceil(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires simd_enabled<vector_data<float, N, P>>
	{
		out.simd = _mm_ceil_ps(v.simd);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_floor(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires simd_enabled<vector_data<float, N, P>>
	{
		out.simd = x86_floor_ps(v.simd);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_trunc(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires simd_enabled<vector_data<float, N, P>>
	{
		out.simd = _mm_round_ps(v.simd, _MM_FROUND_TRUNC);
	}
#elif defined(SEK_USE_SSE2)
	SEK_FORCE_INLINE __m128 x86_floor_ps(__m128 v) noexcept
	{
		/* Convert to int and subtract 1 to round down. */
		const auto tmp = _mm_cvtepi32_ps(_mm_cvtps_epi32(v));
		return _mm_sub_ps(tmp, _mm_and_ps(_mm_cmpgt_ps(tmp, v), _mm_set1_ps(1.0f)));
	}
	template<std::size_t N, storage_policy P>
	inline void vector_floor(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires simd_enabled<vector_data<float, N, P>>
	{
		out.simd = x86_floor_ps(v.simd);
	}
#endif

#ifdef SEK_USE_SSE2
	SEK_FORCE_INLINE __m128i x86_blendv_epi8(__m128i a, __m128i b, __m128i m) noexcept
	{
#ifdef SEK_USE_SSE4_1
		return _mm_blendv_epi8(a, b, m);
#else
		return _mm_add_si128(_mm_and_si128(m, b), _mm_andnot_si128(m, a));
#endif
	}
	SEK_FORCE_INLINE __m128d x86_blendv_pd(__m128d a, __m128d b, __m128d m) noexcept
	{
#ifdef SEK_USE_SSE4_1
		return _mm_blendv_pd(a, b, m);
#else
		return _mm_add_pd(_mm_and_pd(m, b), _mm_andnot_pd(m, a));
#endif
	}

	template<std::size_t I0, std::size_t I1, storage_policy P>
	inline void mask_shuffle(mask_data<double, 2, P> &out, const mask_data<double, 2, P> &m, std::index_sequence<I0, I1>) noexcept
		requires simd_enabled<mask_data<double, 2, P>>
	{
		constexpr auto mask = x86_128_shuffle2_mask(std::index_sequence<I0, I1>{});
		out.simd = _mm_shuffle_pd(m.simd, m.simd, mask);
	}

	template<storage_policy P, std::size_t... Is>
	inline void vector_shuffle(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v, std::index_sequence<Is...> s) noexcept
		requires simd_enabled<vector_data<double, 2, P>>
	{
		constexpr auto mask = x86_128_shuffle2_mask(s);
		out.simd = _mm_shuffle_pd(v.simd, v.simd, mask);
	}
	template<storage_policy P>
	inline void vector_interleave(vector_data<double, 2, P> &out,
								  const vector_data<double, 2, P> &l,
								  const vector_data<double, 2, P> &r,
								  mask_data<double, 2, P> &m) noexcept
		requires simd_enabled<vector_data<double, 2, P>> && simd_enabled<mask_data<double, 2, P>>
	{
		out.simd = x86_blendv_pd(r.simd, l.simd, m.simd);
	}

#ifdef SEK_USE_SSE4_1
	SEK_FORCE_INLINE __m128d x86_floor_pd(__m128d v) noexcept { return _mm_floor_pd(v); }

	template<storage_policy P>
	inline void vector_round(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires simd_enabled<vector_data<double, 2, P>>
	{
		out.simd = _mm_round_pd(v.simd, _MM_FROUND_RINT);
	}
	template<storage_policy P>
	inline void vector_ceil(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires simd_enabled<vector_data<double, 2, P>>
	{
		out.simd = _mm_ceil_pd(v.simd);
	}
	template<storage_policy P>
	inline void vector_trunc(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires simd_enabled<vector_data<double, 2, P>>
	{
		out.simd = _mm_round_pd(v.simd, _MM_FROUND_TRUNC);
	}
#else
	SEK_API __m128d x86_floor_pd(__m128d v) noexcept;
#endif
	template<storage_policy P>
	inline void vector_floor(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires simd_enabled<vector_data<double, 2, P>>
	{
		out.simd = x86_floor_pd(v.simd);
	}

	template<integral_of_size<4> T, std::size_t N, std::size_t M, storage_policy P, std::size_t... Is>
	inline void mask_shuffle(mask_data<T, N, P> &out, const mask_data<T, N, P> &m, std::index_sequence<Is...> s) noexcept
		requires simd_enabled<mask_data<T, N, P>>
	{
		constexpr auto mask = x86_128_shuffle4_mask(s);
		out.simd = _mm_shuffle_epi32(m.simd, mask);
	}

	template<integral_of_size<4> T, std::size_t N, std::size_t M, storage_policy P, std::size_t... Is>
	inline void vector_shuffle(vector_data<T, N, P> &out, const vector_data<T, M, P> &v, std::index_sequence<Is...> s) noexcept
		requires simd_enabled<vector_data<T, N, P>>
	{
		constexpr auto mask = x86_128_shuffle4_mask(s);
		out.simd = _mm_shuffle_epi32(v.simd, mask);
	}
	template<integral_of_size<4> T, std::size_t N, storage_policy P>
	inline void vector_interleave(vector_data<T, N, P> &out,
								  const vector_data<T, N, P> &l,
								  const vector_data<T, N, P> &r,
								  const mask_data<T, N, P> &m) noexcept
		requires simd_enabled<vector_data<T, N, P>> && simd_enabled<mask_data<T, N, P>>
	{
		out.simd = x86_blendv_epi8(r.simd, l.simd, m.simd);
	}

	template<integral_of_size<8> T, std::size_t I0, std::size_t I1, storage_policy P>
	inline void mask_shuffle(mask_data<T, 2, P> &out, const mask_data<T, 2, P> &m, std::index_sequence<I0, I1>) noexcept
	{
		constexpr auto mask = x86_128_shuffle2_mask(std::index_sequence<I0, I1>{});
		out.simd = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(m.simd), _mm_castsi128_pd(m.simd), mask));
	}

	template<integral_of_size<8> T, storage_policy P, std::size_t... Is>
	inline void vector_shuffle(vector_data<T, 2, P> &out, const vector_data<T, 2, P> &v, std::index_sequence<Is...> s) noexcept
		requires simd_enabled<vector_data<T, 2, P>>
	{
		constexpr auto mask = x86_128_shuffle2_mask(s);
		const auto a = _mm_castsi128_pd(v.simd);
		out.simd = _mm_castpd_si128(_mm_shuffle_pd(a, a, mask));
	}
	template<integral_of_size<8> T, storage_policy P>
	inline void vector_interleave(vector_data<T, 2, P> &out,
								  const vector_data<T, 2, P> &l,
								  const vector_data<T, 2, P> &r,
								  const mask_data<T, 2, P> &m) noexcept
		requires simd_enabled<vector_data<T, 2, P>> && simd_enabled<mask_data<T, 2, P>>
	{
		out.simd = x86_blendv_epi8(r.simd, l.simd, m.simd);
	}

#ifndef SEK_USE_AVX
	template<std::size_t N, std::size_t I0, std::size_t I1, storage_policy P, std::size_t... Is>
	inline void mask_shuffle(mask_data<double, N, P> &out, const mask_data<double, 2, P> &m, std::index_sequence<I0, I1, Is...>) noexcept
		requires(N != 2 && simd_enabled<mask_data<double, N, P>> && simd_enabled<mask_data<double, 2, P>>)
	{
		constexpr auto mask0 = x86_128_shuffle2_mask(std::index_sequence<I0, I1>{});
		constexpr auto mask1 = x86_128_shuffle2_mask(std::index_sequence<Is...>{});
		out.simd[0] = _mm_shuffle_pd(m.simd, m.simd, mask0);
		out.simd[1] = _mm_shuffle_pd(m.simd, m.simd, mask1);
	}

	template<std::size_t N, std::size_t I0, std::size_t I1, storage_policy P, std::size_t... Is>
	inline void vector_shuffle(vector_data<double, N, P> &out,
							   const vector_data<double, 2, P> &v,
							   std::index_sequence<I0, I1, Is...>) noexcept
		requires(N != 2 && simd_enabled<vector_data<double, N, P>> && simd_enabled<vector_data<double, 2, P>>)
	{
		constexpr auto mask0 = x86_128_shuffle2_mask(std::index_sequence<I0, I1>{});
		constexpr auto mask1 = x86_128_shuffle2_mask(std::index_sequence<Is...>{});
		out.simd[0] = _mm_shuffle_pd(v.simd, v.simd, mask0);
		out.simd[1] = _mm_shuffle_pd(v.simd, v.simd, mask1);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_interleave(vector_data<double, N, P> &out,
								  const vector_data<double, N, P> &l,
								  const vector_data<double, N, P> &r,
								  mask_data<double, N, P> &m) noexcept
		requires simd_enabled<vector_data<double, N, P>> && simd_enabled<mask_data<double, N, P>>
	{
		out.simd[0] = x86_blendv_pd(r.simd[0], l.simd[0], m.simd[0]);
		out.simd[1] = x86_blendv_pd(r.simd[1], l.simd[1], m.simd[1]);
	}

#ifdef SEK_USE_SSE4_1
	template<std::size_t N, storage_policy P>
	inline void vector_round(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires simd_enabled<vector_data<double, N, P>>
	{
		const int mask = _MM_FROUND_RINT;
		out.simd[0] = _mm_round_pd(v.simd[0], mask);
		out.simd[1] = _mm_round_pd(v.simd[1], mask);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_ceil(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires simd_enabled<vector_data<double, N, P>>
	{
		out.simd[0] = _mm_ceil_pd(v.simd[0]);
		out.simd[1] = _mm_ceil_pd(v.simd[1]);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_trunc(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires simd_enabled<vector_data<double, N, P>>
	{
		const int mask = _MM_FROUND_TRUNC;
		out.simd[0] = _mm_round_pd(v.simd[0], mask);
		out.simd[1] = _mm_round_pd(v.simd[1], mask);
	}
#endif
	template<std::size_t N, storage_policy P>
	inline void vector_floor(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires simd_enabled<vector_data<double, N, P>>
	{
		out.simd[0] = x86_floor_pd(v.simd[0]);
		out.simd[1] = x86_floor_pd(v.simd[1]);
	}

#ifndef SEK_USE_AVX2
	template<integral_of_size<8> T, std::size_t N, std::size_t I0, std::size_t I1, storage_policy P, std::size_t... Is>
	inline void mask_shuffle(mask_data<T, N, P> &out, const mask_data<T, 2, P> &m, std::index_sequence<I0, I1, Is...>) noexcept
		requires(N != 2 && simd_enabled<mask_data<T, N, P>> && simd_enabled<mask_data<T, 2, P>>)
	{
		constexpr auto mask0 = x86_128_shuffle2_mask(std::index_sequence<I0, I1>{});
		constexpr auto mask1 = x86_128_shuffle2_mask(std::index_sequence<Is...>{});
		const auto a = _mm_castsi128_pd(m.simd);
		out.simd[0] = _mm_castpd_si128(_mm_shuffle_pd(a, a, mask0));
		out.simd[1] = _mm_castpd_si128(_mm_shuffle_pd(a, a, mask1));
	}

	template<integral_of_size<8> T, std::size_t N, std::size_t I0, std::size_t I1, storage_policy P, std::size_t... Is>
	inline void vector_shuffle(vector_data<T, N, P> &out, const vector_data<T, 2, P> &v, std::index_sequence<I0, I1, Is...>) noexcept
		requires(N != 2 && simd_enabled<vector_data<T, N, P>> && simd_enabled<vector_data<T, 2, P>>)
	{
		constexpr auto mask0 = x86_128_shuffle2_mask(std::index_sequence<I0, I1>{});
		constexpr auto mask1 = x86_128_shuffle2_mask(std::index_sequence<Is...>{});
		const auto a = _mm_castsi128_pd(v.simd);
		out.simd[0] = _mm_castpd_si128(_mm_shuffle_pd(a, a, mask0));
		out.simd[1] = _mm_castpd_si128(_mm_shuffle_pd(a, a, mask1));
	}
	template<integral_of_size<8> T, std::size_t N, storage_policy P>
	inline void vector_interleave(vector_data<T, N, P> &out,
								  const vector_data<T, N, P> &l,
								  const vector_data<T, N, P> &r,
								  const mask_data<T, N, P> &m) noexcept
		requires simd_enabled<vector_data<T, N, P>> && simd_enabled<mask_data<T, N, P>>
	{
		out.simd[0] = x86_blendv_epi8(r.simd[0], l.simd[0], m.simd[0]);
		out.simd[1] = x86_blendv_epi8(r.simd[1], l.simd[1], m.simd[1]);
	}
#endif
#endif

#ifndef SEK_USE_AVX512_DQ
	SEK_API __m128i x86_cvtpd_epu64(__m128d v) noexcept;
	SEK_API __m128i x86_cvtpd_epi64(__m128d v) noexcept;
	SEK_API __m128d x86_cvtepu64_pd(__m128i v) noexcept;
	SEK_API __m128d x86_cvtepi64_pd(__m128i v) noexcept;
#else
	SEK_FORCE_INLINE __m128i x86_cvtpd_epu64(__m128d v) noexcept { return _mm_cvtpd_epu64(v); }
	SEK_FORCE_INLINE __m128i x86_cvtpd_epi64(__m128d v) noexcept { return _mm_cvtpd_epi64(v); }
	SEK_FORCE_INLINE __m128d x86_cvtepu64_pd(__m128i v) noexcept { return _mm_cvtepu64_pd(v); }
	SEK_FORCE_INLINE __m128d x86_cvtepi64_pd(__m128i v) noexcept { return _mm_cvtepi64_pd(v); }
#endif
#endif
}	 // namespace sek::math::detail
#endif