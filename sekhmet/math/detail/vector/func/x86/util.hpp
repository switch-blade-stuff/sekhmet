/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "common.hpp"

#ifdef SEK_USE_SSE
namespace sek::math::detail
{
	template<std::size_t N, std::size_t M, std::size_t... Is>
	inline void mask_shuffle(simd_mask<float, N> &out, const simd_mask<float, N> &m, std::index_sequence<Is...> s) noexcept
		requires simd_enabled<simd_mask<float, N>>
	{
		constexpr auto mask = x86_128_shuffle4_mask(s);
		out.simd = _mm_shuffle_ps(m.simd, m.simd, mask);
	}

	template<std::size_t N, std::size_t M, std::size_t... Is>
	inline void vector_shuffle(simd_vector<float, N> &out, const simd_vector<float, M> &l, std::index_sequence<Is...> s) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		constexpr auto mask = x86_128_shuffle4_mask(s);
		out.simd = _mm_shuffle_ps(l.simd, l.simd, mask);
	}
	template<std::size_t N>
	inline void vector_interleave(simd_vector<float, N> &out,
								  const simd_vector<float, N> &l,
								  const simd_vector<float, N> &r,
								  simd_mask<float, N> &m) noexcept
		requires simd_enabled<simd_vector<float, N>> && simd_enabled<simd_mask<float, N>>
	{
#ifdef SEK_USE_SSE4_1
		out.simd = _mm_blendv_ps(r.simd, l.simd, m.simd);
#else
		out.simd = _mm_or_ps(_mm_and_ps(m.simd, l.simd), _mm_andnot_ps(m.simd, r.simd));
#endif
	}

#ifdef SEK_USE_SSE4_1
	template<std::size_t N>
	inline void vector_round(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = _mm_round_ps(v.simd, _MM_FROUND_RINT);
	}
	template<std::size_t N>
	inline void vector_floor(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = _mm_floor_ps(v.simd);
	}
	template<std::size_t N>
	inline void vector_ceil(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = _mm_ceil_ps(v.simd);
	}
	template<std::size_t N>
	inline void vector_trunc(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = _mm_round_ps(v.simd, _MM_FROUND_TRUNC);
	}
#elif defined(SEK_USE_SSE2)
	template<std::size_t N>
	inline void vector_floor(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		/* Convert to int and subtract 1 to round down. */
		const auto tmp = _mm_cvtepi32_ps(_mm_cvtps_epi32(v.simd));
		out.simd = _mm_sub_ps(tmp, _mm_and_ps(_mm_cmpgt_ps(tmp, v.simd), _mm_set1_ps(1.0f)));
	}
#endif

#ifdef SEK_USE_SSE2
	SEK_API __m128i x86_cvtpd_epu64(__m128d v) noexcept;
	SEK_API __m128i x86_cvtpd_epi64(__m128d v) noexcept;
	SEK_API __m128d x86_cvtepu64_pd(__m128i x) noexcept;
	SEK_API __m128d x86_cvtepi64_pd(__m128i x) noexcept;

	template<std::size_t I0, std::size_t I1>
	inline void mask_shuffle(simd_mask<double, 2> &out, const simd_mask<double, 2> &m, std::index_sequence<I0, I1>) noexcept
	{
		constexpr auto mask = x86_128_shuffle2_mask(std::index_sequence<I0, I1>{});
		out.simd = _mm_shuffle_pd(m.simd, m.simd, mask);
	}

	template<std::size_t... Is>
	inline void vector_shuffle(simd_vector<double, 2> &out, const simd_vector<double, 2> &v, std::index_sequence<Is...> s) noexcept
	{
		constexpr auto mask = x86_128_shuffle2_mask(s);
		out.simd = _mm_shuffle_pd(v.simd, v.simd, mask);
	}
	inline void vector_interleave(simd_vector<double, 2> &out,
								  const simd_vector<double, 2> &l,
								  const simd_vector<double, 2> &r,
								  simd_mask<double, 2> &m) noexcept
	{
#ifdef SEK_USE_SSE4_1
		out.simd = _mm_blendv_pd(r.simd, l.simd, m.simd);
#else
		out.simd = _mm_or_pd(_mm_and_pd(m.simd, l.simd), _mm_andnot_pd(m.simd, r.simd));
#endif
	}

#ifdef SEK_USE_SSE4_1
	inline void vector_round(simd_vector<double, 2> &out, const simd_vector<double, 2> &v) noexcept
	{
		out.simd = _mm_round_pd(v.simd, _MM_FROUND_RINT);
	}
	inline void vector_floor(simd_vector<double, 2> &out, const simd_vector<double, 2> &v) noexcept
	{
		out.simd = _mm_floor_pd(v.simd);
	}
	inline void vector_ceil(simd_vector<double, 2> &out, const simd_vector<double, 2> &v) noexcept
	{
		out.simd = _mm_ceil_pd(v.simd);
	}
	inline void vector_trunc(simd_vector<double, 2> &out, const simd_vector<double, 2> &v) noexcept
	{
		out.simd = _mm_round_pd(v.simd, _MM_FROUND_TRUNC);
	}
#else
	SEK_API __m128d x86_floor_pd(__m128d v) noexcept;

	inline void vector_floor(simd_vector<double, 2> &out, const simd_vector<double, 2> &v) noexcept
	{
		out.simd = x86_floor_pd(v.simd);
	}
#endif

	template<integral_of_size<4> T, std::size_t N, std::size_t M, std::size_t... Is>
	inline void mask_shuffle(simd_mask<T, N> &out, const simd_mask<T, N> &m, std::index_sequence<Is...> s) noexcept
		requires simd_enabled<simd_mask<T, N>>
	{
		constexpr auto mask = x86_128_shuffle4_mask(s);
		out.simd = _mm_shuffle_epi32(m.simd, mask);
	}

	template<integral_of_size<4> T, std::size_t N, std::size_t M, std::size_t... Is>
	inline void vector_shuffle(simd_vector<T, N> &out, const simd_vector<T, M> &v, std::index_sequence<Is...> s) noexcept
		requires simd_enabled<simd_vector<T, N>>
	{
		constexpr auto mask = x86_128_shuffle4_mask(s);
		out.simd = _mm_shuffle_epi32(v.simd, mask);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void vector_interleave(simd_vector<T, N> &out,
								  const simd_vector<T, N> &l,
								  const simd_vector<T, N> &r,
								  const simd_mask<T, N> &m) noexcept
		requires simd_enabled<simd_vector<T, N>> && simd_enabled<simd_mask<T, N>>
	{
#ifdef SEK_USE_SSE4_1
		out.simd = _mm_blendv_epi8(r.simd, l.simd, m.simd);
#else
		out.simd = _mm_or_si128(_mm_and_si128(m.simd, l.simd), _mm_andnot_si128(m.simd, r.simd));
#endif
	}

	template<integral_of_size<8> T, std::size_t I0, std::size_t I1>
	inline void mask_shuffle(simd_mask<T, 2> &out, const simd_mask<T, 2> &m, std::index_sequence<I0, I1>) noexcept
	{
		constexpr auto mask = x86_128_shuffle2_mask(std::index_sequence<I0, I1>{});
		out.simd = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(m.simd), _mm_castsi128_pd(m.simd), mask));
	}

	template<integral_of_size<8> T, std::size_t... Is>
	inline void vector_shuffle(simd_vector<T, 2> &out, const simd_vector<T, 2> &v, std::index_sequence<Is...> s) noexcept
	{
		constexpr auto mask = x86_128_shuffle2_mask(s);
		const auto a = _mm_castsi128_pd(v.simd);
		out.simd = _mm_castpd_si128(_mm_shuffle_pd(a, a, mask));
	}
	template<integral_of_size<8> T>
	inline void vector_interleave(simd_vector<T, 2> &out,
								  const simd_vector<T, 2> &l,
								  const simd_vector<T, 2> &r,
								  const simd_mask<T, 2> &m) noexcept
	{
#ifdef SEK_USE_SSE4_1
		out.simd = _mm_blendv_epi8(r.simd, l.simd, m.simd);
#else
		out.simd = _mm_or_si128(_mm_and_si128(m.simd, l.simd), _mm_andnot_si128(m.simd, r.simd));
#endif
	}

#ifndef SEK_USE_AVX
	template<std::size_t N, std::size_t I0, std::size_t I1, std::size_t... Is>
	inline void mask_shuffle(simd_mask<double, N> &out, const simd_mask<double, 2> &m, std::index_sequence<I0, I1, Is...>) noexcept
		requires(N != 2 && simd_enabled<simd_mask<double, N>> && simd_enabled<simd_mask<double, 2>>)
	{
		constexpr auto mask0 = x86_128_shuffle2_mask(std::index_sequence<I0, I1>{});
		constexpr auto mask1 = x86_128_shuffle2_mask(std::index_sequence<Is...>{});
		out.simd[0] = _mm_shuffle_pd(m.simd, m.simd, mask0);
		out.simd[1] = _mm_shuffle_pd(m.simd, m.simd, mask1);
	}

	template<std::size_t N, std::size_t I0, std::size_t I1, std::size_t... Is>
	inline void vector_shuffle(simd_vector<double, N> &out, const simd_vector<double, 2> &v, std::index_sequence<I0, I1, Is...>) noexcept
		requires(N != 2 && simd_enabled<simd_vector<double, N>> && simd_enabled<simd_vector<double, 2>>)
	{
		constexpr auto mask0 = x86_128_shuffle2_mask(std::index_sequence<I0, I1>{});
		constexpr auto mask1 = x86_128_shuffle2_mask(std::index_sequence<Is...>{});
		out.simd[0] = _mm_shuffle_pd(v.simd, v.simd, mask0);
		out.simd[1] = _mm_shuffle_pd(v.simd, v.simd, mask1);
	}
	template<std::size_t N>
	inline void vector_interleave(simd_vector<double, N> &out,
								  const simd_vector<double, N> &l,
								  const simd_vector<double, N> &r,
								  simd_mask<double, N> &m) noexcept
		requires simd_enabled<simd_vector<double, N>> && simd_enabled<simd_mask<double, N>>
	{
#ifdef SEK_USE_SSE4_1
		out.simd[0] = _mm_blendv_pd(r.simd[0], l.simd[0], m.simd[0]);
		out.simd[1] = _mm_blendv_pd(r.simd[1], l.simd[1], m.simd[1]);
#else
		out.simd[0] = _mm_or_pd(_mm_and_pd(m.simd[0], l.simd[0]), _mm_andnot_pd(m.simd[0], r.simd[0]));
		out.simd[1] = _mm_or_pd(_mm_and_pd(m.simd[1], l.simd[1]), _mm_andnot_pd(m.simd[1], r.simd[1]));
#endif
	}

#ifdef SEK_USE_SSE4_1
	template<std::size_t N>
	inline void vector_round(simd_vector<double, N> &out, const simd_vector<double, N> &v) noexcept
		requires simd_enabled<simd_vector<double, N>>
	{
		const int mask = _MM_FROUND_RINT;
		out.simd[0] = _mm_round_pd(v.simd[0], mask);
		out.simd[1] = _mm_round_pd(v.simd[1], mask);
	}
	template<std::size_t N>
	inline void vector_floor(simd_vector<double, N> &out, const simd_vector<double, N> &v) noexcept
		requires simd_enabled<simd_vector<double, N>>
	{
		out.simd[0] = _mm_floor_pd(v.simd[0]);
		out.simd[1] = _mm_floor_pd(v.simd[1]);
	}
	template<std::size_t N>
	inline void vector_ceil(simd_vector<double, N> &out, const simd_vector<double, N> &v) noexcept
		requires simd_enabled<simd_vector<double, N>>
	{
		out.simd[0] = _mm_ceil_pd(v.simd[0]);
		out.simd[1] = _mm_ceil_pd(v.simd[1]);
	}
	template<std::size_t N>
	inline void vector_trunc(simd_vector<double, N> &out, const simd_vector<double, N> &v) noexcept
		requires simd_enabled<simd_vector<double, N>>
	{
		const int mask = _MM_FROUND_TRUNC;
		out.simd[0] = _mm_round_pd(v.simd[0], mask);
		out.simd[1] = _mm_round_pd(v.simd[1], mask);
	}
#else
	template<std::size_t N>
	inline void vector_floor(simd_vector<double, N> &out, const simd_vector<double, N> &v) noexcept
		requires simd_enabled<simd_vector<double, N>>
	{
		out.simd[0] = x86_floor_pd(v.simd[0]);
		out.simd[1] = x86_floor_pd(v.simd[1]);
	}
#endif

#ifndef SEK_USE_AVX2
	template<integral_of_size<8> T, std::size_t N, std::size_t I0, std::size_t I1, std::size_t... Is>
	inline void mask_shuffle(simd_mask<T, N> &out, const simd_mask<T, 2> &m, std::index_sequence<I0, I1, Is...>) noexcept
		requires(N != 2 && simd_enabled<simd_mask<T, N>> && simd_enabled<simd_mask<T, 2>>)
	{
		constexpr auto mask0 = x86_128_shuffle2_mask(std::index_sequence<I0, I1>{});
		constexpr auto mask1 = x86_128_shuffle2_mask(std::index_sequence<Is...>{});
		const auto a = _mm_castsi128_pd(m.simd);
		out.simd[0] = _mm_castpd_si128(_mm_shuffle_pd(a, a, mask0));
		out.simd[1] = _mm_castpd_si128(_mm_shuffle_pd(a, a, mask1));
	}

	template<integral_of_size<8> T, std::size_t N, std::size_t I0, std::size_t I1, std::size_t... Is>
	inline void vector_shuffle(simd_vector<T, N> &out, const simd_vector<T, 2> &v, std::index_sequence<I0, I1, Is...>) noexcept
		requires(N != 2 && simd_enabled<simd_vector<T, N>> && simd_enabled<simd_vector<T, 2>>)
	{
		constexpr auto mask0 = x86_128_shuffle2_mask(std::index_sequence<I0, I1>{});
		constexpr auto mask1 = x86_128_shuffle2_mask(std::index_sequence<Is...>{});
		const auto a = _mm_castsi128_pd(v.simd);
		out.simd[0] = _mm_castpd_si128(_mm_shuffle_pd(a, a, mask0));
		out.simd[1] = _mm_castpd_si128(_mm_shuffle_pd(a, a, mask1));
	}
	template<integral_of_size<8> T, std::size_t N>
	inline void vector_interleave(simd_vector<T, N> &out,
								  const simd_vector<T, N> &l,
								  const simd_vector<T, N> &r,
								  const simd_mask<T, N> &m) noexcept
		requires simd_enabled<simd_vector<T, N>> && simd_enabled<simd_mask<T, N>>
	{
#ifdef SEK_USE_SSE4_1
		out.simd[0] = _mm_blendv_epi8(r.simd[0], l.simd[0], m.simd[0]);
		out.simd[1] = _mm_blendv_epi8(r.simd[1], l.simd[1], m.simd[1]);
#else
		out.simd[0] = _mm_or_si128(_mm_and_si128(m.simd[0], l.simd[0]), _mm_andnot_si128(m.simd[0], r.simd[0]));
		out.simd[1] = _mm_or_si128(_mm_and_si128(m.simd[1], l.simd[1]), _mm_andnot_si128(m.simd[1], r.simd[1]));
#endif
	}
#endif
#endif
#endif
}	 // namespace sek::math::detail
#endif