/*
 * Created by switchblade on 2022-01-31
 */

#pragma once

#include <bit>

#include "../util.hpp"
#include "common.hpp"

#ifdef SEK_USE_SSE

#include <xmmintrin.h>

#ifdef SEK_USE_SSE2
#include <emmintrin.h>
#endif

#ifdef SEK_USE_SSE3
#include <pmmintrin.h>
#endif

#ifdef SEK_USE_SSSE3
#include <tmmintrin.h>
#endif

#ifdef SEK_USE_SSE4_1
#include <smmintrin.h>
#endif

#ifdef SEK_USE_SSE4_2
#include <nmmintrin.h>
#endif

namespace sek::math::detail
{
	template<>
	struct simd_t<float, 3>
	{
		__m128 value;
	};
	template<>
	struct simd_t<float, 4>
	{
		__m128 value;
	};

	template<std::size_t N>
	inline void x86_simd_add(simd_t<float, N> &out, const simd_t<float, N> &l, const simd_t<float, N> &r) noexcept
	{
		out.value = _mm_add_ps(l.value, r.value);
	}
	template<std::size_t N>
	inline void x86_simd_sub(simd_t<float, N> &out, const simd_t<float, N> &l, const simd_t<float, N> &r) noexcept
	{
		out.value = _mm_sub_ps(l.value, r.value);
	}
	template<std::size_t N>
	inline void x86_simd_mul_s(simd_t<float, N> &out, const simd_t<float, N> &l, float r) noexcept
	{
		out.value = _mm_mul_ps(l.value, _mm_set1_ps(r));
	}
	template<std::size_t N>
	inline void x86_simd_div_s(simd_t<float, N> &out, const simd_t<float, N> &l, float r) noexcept
	{
		out.value = _mm_div_ps(l.value, _mm_set1_ps(r));
	}
	template<std::size_t N>
	inline void x86_simd_div_s(simd_t<float, N> &out, float l, const simd_t<float, N> &r) noexcept
	{
		out.value = _mm_div_ps(_mm_set1_ps(l), r.value);
	}
	template<std::size_t N>
	inline void x86_simd_neg(simd_t<float, N> &out, const simd_t<float, N> &l) noexcept
	{
		out.value = _mm_sub_ps(_mm_setzero_ps(), l.value);
	}
	template<std::size_t N>
	inline void x86_simd_abs(simd_t<float, N> &out, const simd_t<float, N> &l) noexcept
	{
		constexpr auto mask = std::bit_cast<float>(0x7fff'ffff);
		out.value = _mm_and_ps(_mm_set1_ps(mask), l.value);
	}
	template<std::size_t N>
	inline void x86_simd_max(simd_t<float, N> &out, const simd_t<float, N> &l, const simd_t<float, N> &r) noexcept
	{
		out.value = _mm_max_ps(l.value, r.value);
	}
	template<std::size_t N>
	inline void x86_simd_min(simd_t<float, N> &out, const simd_t<float, N> &l, const simd_t<float, N> &r) noexcept
	{
		out.value = _mm_min_ps(l.value, r.value);
	}
	template<std::size_t N>
	inline void x86_simd_sqrt(simd_t<float, N> &out, const simd_t<float, N> &l) noexcept
	{
		out.value = _mm_sqrt_ps(l.value);
	}
	template<std::size_t N>
	inline void x86_simd_rsqrt(simd_t<float, N> &out, const simd_t<float, N> &l) noexcept
	{
		out.value = _mm_rsqrt_ps(l.value);
	}

	inline void x86_simd_cross(simd_t<float, 3> &out, const simd_t<float, 3> &l, const simd_t<float, 3> &r) noexcept
	{
		const auto a = _mm_shuffle_ps(l.value, l.value, _MM_SHUFFLE(3, 0, 2, 1));
		const auto b = _mm_shuffle_ps(r.value, r.value, _MM_SHUFFLE(3, 1, 0, 2));
		const auto c = _mm_mul_ps(a, r.value);
		out.value = _mm_sub_ps(_mm_mul_ps(a, b), _mm_shuffle_ps(c, c, _MM_SHUFFLE(3, 0, 2, 1)));
	}

	template<std::size_t N, std::size_t M, std::size_t... Is>
	inline void x86_simd_shuffle(simd_t<float, N> &out, const simd_t<float, M> &l, std::index_sequence<Is...> s) noexcept
	{
		constexpr auto mask = x86_mm_shuffle4_mask(s);
		out.value = _mm_shuffle_ps(l.value, l.value, mask);
	}

#ifdef SEK_USE_SSE4_1
	template<std::size_t N>
	inline void x86_simd_round(simd_t<float, N> &out, const simd_t<float, N> &l) noexcept
	{
		out.value = _mm_round_ps(l.value, _MM_FROUND_CUR_DIRECTION | _MM_FROUND_NO_EXC);
	}
	template<std::size_t N>
	inline void x86_simd_floor(simd_t<float, N> &out, const simd_t<float, N> &l) noexcept
	{
		out.value = _mm_floor_ps(l.value);
	}
	template<std::size_t N>
	inline void x86_simd_ceil(simd_t<float, N> &out, const simd_t<float, N> &l) noexcept
	{
		out.value = _mm_ceil_ps(l.value);
	}
#endif

#ifdef SEK_USE_SSE2
	template<>
	struct simd_t<double, 2>
	{
		__m128d value;
	};

	inline void x86_simd_add(simd_t<double, 2> &out, const simd_t<double, 2> &l, const simd_t<double, 2> &r) noexcept
	{
		out.value = _mm_add_pd(l.value, r.value);
	}
	inline void x86_simd_sub(simd_t<double, 2> &out, const simd_t<double, 2> &l, const simd_t<double, 2> &r) noexcept
	{
		out.value = _mm_sub_pd(l.value, r.value);
	}
	inline void x86_simd_mul_s(simd_t<double, 2> &out, const simd_t<double, 2> &l, double r) noexcept
	{
		out.value = _mm_mul_pd(l.value, _mm_set1_pd(r));
	}
	inline void x86_simd_div_s(simd_t<double, 2> &out, const simd_t<double, 2> &l, double r) noexcept
	{
		out.value = _mm_div_pd(l.value, _mm_set1_pd(r));
	}
	inline void x86_simd_div_s(simd_t<double, 2> &out, double l, const simd_t<double, 2> &r) noexcept
	{
		out.value = _mm_div_pd(_mm_set1_pd(l), r.value);
	}
	inline void x86_simd_neg(simd_t<double, 2> &out, const simd_t<double, 2> &l) noexcept
	{
		out.value = _mm_sub_pd(_mm_setzero_pd(), l.value);
	}
	inline void x86_simd_abs(simd_t<double, 2> &out, const simd_t<double, 2> &l) noexcept
	{
		constexpr auto mask = std::bit_cast<double>(0x7fff'ffff'ffff'ffff);
		out.value = _mm_and_pd(_mm_set1_pd(mask), l.value);
	}
	inline void x86_simd_max(simd_t<double, 2> &out, const simd_t<double, 2> &l, const simd_t<double, 2> &r) noexcept
	{
		out.value = _mm_max_pd(l.value, r.value);
	}
	inline void x86_simd_min(simd_t<double, 2> &out, const simd_t<double, 2> &l, const simd_t<double, 2> &r) noexcept
	{
		out.value = _mm_min_pd(l.value, r.value);
	}
	inline void x86_simd_sqrt(simd_t<double, 2> &out, const simd_t<double, 2> &l) noexcept
	{
		out.value = _mm_sqrt_pd(l.value);
	}
	inline void x86_simd_rsqrt(simd_t<double, 2> &out, const simd_t<double, 2> &l) noexcept
	{
		out.value = _mm_div_pd(_mm_set1_pd(1), _mm_sqrt_pd(l.value));
	}

	template<std::size_t... Is>
	inline void x86_simd_shuffle(simd_t<double, 2> &out, const simd_t<double, 2> &l, std::index_sequence<Is...> s) noexcept
	{
		constexpr auto mask = x86_mm_shuffle2_mask(s);
		out.value = _mm_shuffle_pd(l.value, l.value, mask);
	}

#ifdef SEK_USE_SSE4_1
	inline void x86_simd_round(simd_t<double, 2> &out, const simd_t<double, 2> &l) noexcept
	{
		out.value = _mm_round_pd(l.value, _MM_FROUND_CUR_DIRECTION | _MM_FROUND_NO_EXC);
	}
	inline void x86_simd_floor(simd_t<double, 2> &out, const simd_t<double, 2> &l) noexcept
	{
		out.value = _mm_floor_pd(l.value);
	}
	inline void x86_simd_ceil(simd_t<double, 2> &out, const simd_t<double, 2> &l) noexcept
	{
		out.value = _mm_ceil_pd(l.value);
	}
#endif

#ifndef SEK_USE_AVX
	template<>
	struct simd_t<double, 3>
	{
		__m128d value[2];
	};
	template<>
	struct simd_t<double, 4>
	{
		__m128d value[2];
	};

	template<std::size_t N>
	inline void x86_simd_add(simd_t<double, N> &out, const simd_t<double, N> &l, const simd_t<double, N> &r) noexcept
	{
		out.value[0] = _mm_add_pd(l.value[0], r.value[0]);
		out.value[1] = _mm_add_pd(l.value[1], r.value[1]);
	}
	template<std::size_t N>
	inline void x86_simd_sub(simd_t<double, N> &out, const simd_t<double, N> &l, const simd_t<double, N> &r) noexcept
	{
		out.value[0] = _mm_sub_pd(l.value[0], r.value[0]);
		out.value[1] = _mm_sub_pd(l.value[1], r.value[1]);
	}
	template<std::size_t N>
	inline void x86_simd_mul_s(simd_t<double, N> &out, const simd_t<double, N> &l, double r) noexcept
	{
		const auto rv = _mm_set1_pd(r);
		out.value[0] = _mm_mul_pd(l.value[0], rv);
		out.value[1] = _mm_mul_pd(l.value[1], rv);
	}
	template<std::size_t N>
	inline void x86_simd_div_s(simd_t<double, N> &out, const simd_t<double, N> &l, double r) noexcept
	{
		const auto rv = _mm_set1_pd(r);
		out.value[0] = _mm_div_pd(l.value[0], rv);
		out.value[1] = _mm_div_pd(l.value[1], rv);
	}
	template<std::size_t N>
	inline void x86_simd_div_s(simd_t<double, N> &out, double l, const simd_t<double, N> &r) noexcept
	{
		const auto lv = _mm_set1_pd(l);
		out.value[0] = _mm_div_pd(lv, r.value[0]);
		out.value[1] = _mm_div_pd(lv, r.value[1]);
	}
	template<std::size_t N>
	inline void x86_simd_neg(simd_t<double, N> &out, const simd_t<double, N> &l) noexcept
	{
		const auto z = _mm_setzero_pd();
		out.value[0] = _mm_sub_pd(z, l.value[0]);
		out.value[1] = _mm_sub_pd(z, l.value[1]);
	}
	template<std::size_t N>
	inline void x86_simd_abs(simd_t<double, N> &out, const simd_t<double, N> &l) noexcept
	{
		constexpr auto mask = std::bit_cast<double>(0x7fff'ffff'ffff'ffff);
		const auto m = _mm_set1_pd(mask);
		out.value[0] = _mm_and_pd(m, l.value[0]);
		out.value[1] = _mm_and_pd(m, l.value[1]);
	}
	template<std::size_t N>
	inline void x86_simd_max(simd_t<double, N> &out, const simd_t<double, N> &l, const simd_t<double, N> &r) noexcept
	{
		out.value[0] = _mm_max_pd(l.value[0], r.value[0]);
		out.value[1] = _mm_max_pd(l.value[1], r.value[1]);
	}
	template<std::size_t N>
	inline void x86_simd_min(simd_t<double, N> &out, const simd_t<double, N> &l, const simd_t<double, N> &r) noexcept
	{
		out.value[0] = _mm_min_pd(l.value[0], r.value[0]);
		out.value[1] = _mm_min_pd(l.value[1], r.value[1]);
	}
	template<std::size_t N>
	inline void x86_simd_sqrt(simd_t<double, N> &out, const simd_t<double, N> &l) noexcept
	{
		out.value[0] = _mm_sqrt_pd(l.value[0]);
		out.value[1] = _mm_sqrt_pd(l.value[1]);
	}
	template<std::size_t N>
	inline void x86_simd_rsqrt(simd_t<double, N> &out, const simd_t<double, N> &l) noexcept
	{
		const auto v1 = _mm_set1_pd(1);
		out.value[0] = _mm_div_pd(v1, _mm_sqrt_pd(l.value[0]));
		out.value[1] = _mm_div_pd(v1, _mm_sqrt_pd(l.value[1]));
	}

	inline void x86_simd_cross(simd_t<double, 3> &out, const simd_t<double, 3> &l, const simd_t<double, 3> &r) noexcept
	{
		/* 4 shuffles are needed here since the 3 doubles are split across 2 __m128d vectors. */
		const auto a = _mm_shuffle_pd(l.value[0], l.value[1], _MM_SHUFFLE2(0, 1));
		const auto b = _mm_shuffle_pd(r.value[0], r.value[1], _MM_SHUFFLE2(0, 1));

		out.value[0] = _mm_sub_pd(_mm_mul_pd(a, _mm_shuffle_pd(r.value[1], r.value[0], _MM_SHUFFLE2(0, 0))),
								  _mm_mul_pd(b, _mm_shuffle_pd(l.value[1], l.value[0], _MM_SHUFFLE2(0, 0))));
		out.value[1] = _mm_sub_pd(_mm_mul_pd(l.value[0], b), _mm_mul_pd(r.value[0], a));
	}

	template<std::size_t N, std::size_t I0, std::size_t I1, std::size_t... Is> /* N is greater than 2 */
	inline void x86_simd_shuffle(simd_t<double, N> &out, const simd_t<double, 2> &l, std::index_sequence<I0, I1, Is...>) noexcept
	{
		constexpr auto mask0 = x86_mm_shuffle2_mask(std::index_sequence<I0, I1>{});
		constexpr auto mask1 = x86_mm_shuffle2_mask(std::index_sequence<Is...>{});
		out.value[0] = _mm_shuffle_pd(l.value, l.value, mask0);
		out.value[1] = _mm_shuffle_pd(l.value, l.value, mask1);
	}

#ifdef SEK_USE_SSE4_1
	template<std::size_t N>
	inline void x86_simd_round(simd_t<double, N> &out, const simd_t<double, N> &l) noexcept
	{
		const int mask = _MM_FROUND_CUR_DIRECTION | _MM_FROUND_NO_EXC;
		out.value[0] = _mm_round_pd(l.value[0], mask);
		out.value[1] = _mm_round_pd(l.value[1], mask);
	}
	template<std::size_t N>
	inline void x86_simd_floor(simd_t<double, N> &out, const simd_t<double, N> &l) noexcept
	{
		out.value[0] = _mm_floor_pd(l.value[0]);
		out.value[1] = _mm_floor_pd(l.value[1]);
	}
	template<std::size_t N>
	inline void x86_simd_ceil(simd_t<double, N> &out, const simd_t<double, N> &l) noexcept
	{
		out.value[0] = _mm_ceil_pd(l.value[0]);
		out.value[1] = _mm_ceil_pd(l.value[1]);
	}
#endif
#endif

	template<integral_of_size<8> T>
	struct simd_t<T, 2>
	{
		__m128i value;
	};

	template<integral_of_size<8> T>
	inline void x86_simd_add(simd_t<T, 2> &out, const simd_t<T, 2> &l, const simd_t<T, 2> &r) noexcept
	{
		out.value = _mm_add_epi64(l.value, r.value);
	}
	template<integral_of_size<8> T>
	inline void x86_simd_sub(simd_t<T, 2> &out, const simd_t<T, 2> &l, const simd_t<T, 2> &r) noexcept
	{
		out.value = _mm_sub_epi64(l.value, r.value);
	}
	template<integral_of_size<8> T>
	inline void x86_simd_neg(simd_t<T, 2> &out, const simd_t<T, 2> &l) noexcept
	{
		out.value = _mm_sub_epi64(_mm_setzero_si128(), l.value);
	}

#ifndef SEK_USE_AVX2
	template<integral_of_size<8> T>
	struct simd_t<T, 3>
	{
		__m128i value[2];
	};
	template<integral_of_size<8> T>
	struct simd_t<T, 4>
	{
		__m128i value[2];
	};

	template<integral_of_size<8> T, std::size_t N>
	inline void x86_simd_add(simd_t<T, N> &out, const simd_t<T, N> &l, const simd_t<T, N> &r) noexcept
	{
		out.value[0] = _mm_add_epi64(l.value[0], r.value[0]);
		out.value[1] = _mm_add_epi64(l.value[1], r.value[1]);
	}
	template<integral_of_size<8> T, std::size_t N>
	inline void x86_simd_sub(simd_t<T, N> &out, const simd_t<T, N> &l, const simd_t<T, N> &r) noexcept
	{
		out.value[0] = _mm_sub_epi64(l.value[0], r.value[0]);
		out.value[1] = _mm_sub_epi64(l.value[1], r.value[1]);
	}
	template<integral_of_size<8> T, std::size_t N>
	inline void x86_simd_neg(simd_t<T, N> &out, const simd_t<T, N> &l) noexcept
	{
		const auto z = _mm_setzero_si128();
		out.value[0] = _mm_sub_epi64(z, l.value[0]);
		out.value[1] = _mm_sub_epi64(z, l.value[1]);
	}

	template<integral_of_size<8> T, std::size_t N>
	inline void x86_simd_and(simd_t<T, N> &out, const simd_t<T, N> &l, const simd_t<T, N> &r) noexcept
		requires(N == 3 || N == 4)
	{
		out.value[0] = _mm_and_si128(l.value[0], r.value[0]);
		out.value[1] = _mm_and_si128(l.value[1], r.value[1]);
	}
	template<integral_of_size<8> T, std::size_t N>
	inline void x86_simd_xor(simd_t<T, N> &out, const simd_t<T, N> &l, const simd_t<T, N> &r) noexcept
		requires(N == 3 || N == 4)
	{
		out.value[0] = _mm_xor_si128(l.value[0], r.value[0]);
		out.value[1] = _mm_xor_si128(l.value[1], r.value[1]);
	}
	template<integral_of_size<8> T, std::size_t N>
	inline void x86_simd_or(simd_t<T, N> &out, const simd_t<T, N> &l, const simd_t<T, N> &r) noexcept
		requires(N == 3 || N == 4)
	{
		out.value[0] = _mm_or_si128(l.value[0], r.value[0]);
		out.value[1] = _mm_or_si128(l.value[1], r.value[1]);
	}
	template<integral_of_size<8> T, std::size_t N>
	inline void x86_simd_inv(simd_t<T, N> &out, const simd_t<T, N> &l) noexcept
		requires(N == 3 || N == 4)
	{
		const auto m = _mm_set1_epi8((int8_t) 0xff);
		out.value[0] = _mm_xor_si128(l.value[0], m);
		out.value[1] = _mm_xor_si128(l.value[1], m);
	}
#endif

	template<integral_of_size<4> T>
	struct simd_t<T, 3>
	{
		__m128i value;
	};
	template<integral_of_size<4> T>
	struct simd_t<T, 4>
	{
		__m128i value;
	};

	template<integral_of_size<4> T, std::size_t N>
	inline void x86_simd_add(simd_t<T, N> &out, const simd_t<T, N> &l, const simd_t<T, N> &r) noexcept
	{
		out.value = _mm_add_epi32(l.value, r.value);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void x86_simd_sub(simd_t<T, N> &out, const simd_t<T, N> &l, const simd_t<T, N> &r) noexcept
	{
		out.value = _mm_sub_epi32(l.value, r.value);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void x86_simd_mul_s(simd_t<T, N> &out, const simd_t<T, N> &l, T r) noexcept
	{
		out.value = _mm_mul_epi32(l.value, _mm_set1_epi32(r));
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void x86_simd_div_s(simd_t<T, N> &out, const simd_t<T, N> &l, T r) noexcept
	{
		out.value = _mm_div_epi32(l.value, _mm_set1_epi32(r));
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void x86_simd_div_s(simd_t<T, N> &out, T l, const simd_t<T, N> &r) noexcept
	{
		out.value = _mm_div_epi32(_mm_set1_epi32(l), r.value);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void x86_simd_neg(simd_t<T, N> &out, const simd_t<T, N> &l) noexcept
	{
		out.value = _mm_sub_epi32(_mm_setzero_si128(), l.value);
	}

	template<integral_of_size<4> T, std::size_t N, std::size_t... Is>
	inline void x86_simd_shuffle(simd_t<T, N> &out, const simd_t<T, N> &l, std::index_sequence<Is...> s) noexcept
	{
		constexpr auto mask = x86_mm_shuffle4_mask(s);
		out.value = _mm_shuffle_epi32(l.value, mask);
	}

#ifdef SEK_USE_SSSE3
	template<integral_of_size<4> T, std::size_t N>
	inline void x86_simd_abs(simd_t<T, N> &out, const simd_t<T, N> &l) noexcept
	{
		out.value = _mm_abs_epi32(l.value);
	}
#endif
#ifdef SEK_USE_SSE4_1
	template<integral_of_size<4> T, std::size_t N>
	inline void x86_simd_max(simd_t<T, N> &out, const simd_t<T, N> &l, const simd_t<T, N> &r) noexcept
	{
		out.value = _mm_max_epi32(l.value, r.value);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void x86_simd_min(simd_t<T, N> &out, const simd_t<T, N> &l, const simd_t<T, N> &r) noexcept
	{
		out.value = _mm_min_epi32(l.value, r.value);
	}
#endif

	template<integral_of_size<2> T>
	struct simd_t<T, 8>
	{
		__m128i value;
	};

	template<integral_of_size<2> T>
	inline void x86_simd_add(simd_t<T, 8> &out, const simd_t<T, 8> &l, const simd_t<T, 8> &r) noexcept
	{
		out.value = _mm_add_epi16(l.value, r.value);
	}
	template<integral_of_size<2> T>
	inline void x86_simd_sub(simd_t<T, 8> &out, const simd_t<T, 8> &l, const simd_t<T, 8> &r) noexcept
	{
		out.value = _mm_sub_epi16(l.value, r.value);
	}
	template<integral_of_size<2> T>
	inline void x86_simd_mul_s(simd_t<T, 8> &out, const simd_t<T, 8> &l, T r) noexcept
	{
		out.value = _mm_mul_epi16(l.value, _mm_set1_epi16(r));
	}
	template<integral_of_size<2> T>
	inline void x86_simd_div_s(simd_t<T, 8> &out, const simd_t<T, 8> &l, T r) noexcept
	{
		out.value = _mm_div_epi16(l.value, _mm_set1_epi16(r));
	}
	template<integral_of_size<2> T, std::size_t N>
	inline void x86_simd_div_s(simd_t<T, N> &out, T l, const simd_t<T, N> &r) noexcept
	{
		out.value = _mm_div_epi16(_mm_set1_epi16(l), r.value);
	}
	template<integral_of_size<2> T>
	inline void x86_simd_neg(simd_t<T, 8> &out, const simd_t<T, 8> &l) noexcept
	{
		out.value = _mm_sub_epi16(_mm_setzero_si128(), l.value);
	}
#ifdef SEK_USE_SSSE3
	template<integral_of_size<2> T>
	inline void x86_simd_abs(simd_t<T, 8> &out, const simd_t<T, 8> &l) noexcept
	{
		out.value = _mm_abs_epi16(l.value);
	}
#endif
	template<integral_of_size<2> T>
	inline void x86_simd_max(simd_t<T, 8> &out, const simd_t<T, 8> &l, const simd_t<T, 8> &r) noexcept
	{
		out.value = _mm_max_epi16(l.value, r.value);
	}
	template<integral_of_size<2> T>
	inline void x86_simd_min(simd_t<T, 8> &out, const simd_t<T, 8> &l, const simd_t<T, 8> &r) noexcept
	{
		out.value = _mm_min_epi16(l.value, r.value);
	}

	template<integral_of_size<1> T>
	struct simd_t<T, 16>
	{
		__m128i value;
	};

	template<integral_of_size<1> T>
	inline void x86_simd_add(simd_t<T, 16> &out, const simd_t<T, 16> &l, const simd_t<T, 16> &r) noexcept
	{
		out.value = _mm_add_epi8(l.value, r.value);
	}
	template<integral_of_size<1> T>
	inline void x86_simd_sub(simd_t<T, 16> &out, const simd_t<T, 16> &l, const simd_t<T, 16> &r) noexcept
	{
		out.value = _mm_sub_epi8(l.value, r.value);
	}
	template<integral_of_size<1> T>
	inline void x86_simd_mul_s(simd_t<T, 16> &out, const simd_t<T, 16> &l, T r) noexcept
	{
		out.value = _mm_mul_epi8(l.value, _mm_set1_epi8(r));
	}
	template<integral_of_size<1> T>
	inline void x86_simd_div_s(simd_t<T, 16> &out, const simd_t<T, 16> &l, T r) noexcept
	{
		out.value = _mm_div_epi8(l.value, _mm_set1_epi8(r));
	}
	template<integral_of_size<1> T, std::size_t N>
	inline void x86_simd_div_s(simd_t<T, N> &out, T l, const simd_t<T, N> &r) noexcept
	{
		out.value = _mm_div_epi8(_mm_set1_epi8(l), r.value);
	}
	template<integral_of_size<1> T>
	inline void x86_simd_neg(simd_t<T, 16> &out, const simd_t<T, 16> &l) noexcept
	{
		out.value = _mm_sub_epi8(_mm_setzero_si128(), l.value);
	}
#ifdef SEK_USE_SSSE3
	template<integral_of_size<1> T>
	inline void x86_simd_abs(simd_t<T, 16> &out, const simd_t<T, 16> &l) noexcept
	{
		out.value = _mm_abs_epi8(l.value);
	}
#endif
#ifdef SEK_USE_SSE4_1
	template<integral_of_size<1> T>
	inline void x86_simd_max(simd_t<T, 16> &out, const simd_t<T, 16> &l, const simd_t<T, 16> &r) noexcept
	{
		out.value = _mm_max_epi8(l.value, r.value);
	}
	template<integral_of_size<1> T>
	inline void x86_simd_min(simd_t<T, 16> &out, const simd_t<T, 16> &l, const simd_t<T, 16> &r) noexcept
	{
		out.value = _mm_min_epi8(l.value, r.value);
	}
#endif

	template<std::integral T, std::size_t N>
	inline void x86_simd_and(simd_t<T, N> &out, const simd_t<T, N> &l, const simd_t<T, N> &r) noexcept
		requires(sizeof(T[N]) <= 16)
	{
		out.value = _mm_and_si128(l.value, r.value);
	}
	template<std::integral T, std::size_t N>
	inline void x86_simd_xor(simd_t<T, N> &out, const simd_t<T, N> &l, const simd_t<T, N> &r) noexcept
		requires(sizeof(T[N]) <= 16)
	{
		out.value = _mm_xor_si128(l.value, r.value);
	}
	template<std::integral T, std::size_t N>
	inline void x86_simd_or(simd_t<T, N> &out, const simd_t<T, N> &l, const simd_t<T, N> &r) noexcept
		requires(sizeof(T[N]) <= 16)
	{
		out.value = _mm_or_si128(l.value, r.value);
	}
	template<std::integral T, std::size_t N>
	inline void x86_simd_inv(simd_t<T, N> &out, const simd_t<T, N> &l) noexcept
		requires(sizeof(T[N]) <= 16)
	{
		out.value = _mm_xor_si128(l.value, _mm_set1_epi8((int8_t) 0xff));
	}
#endif

#ifdef SEK_USE_SSE4_1
	inline float x86_simd_dot(const simd_t<float, 3> &l, const simd_t<float, 3> &r) noexcept
	{
		return _mm_cvtss_f32(_mm_dp_ps(l.value, r.value, 0x71));
	}
	inline void x86_simd_norm(simd_t<float, 3> &out, const simd_t<float, 3> &l) noexcept
	{
		out.value = _mm_div_ps(l.value, _mm_sqrt_ps(_mm_dp_ps(l.value, l.value, 0x7f)));
	}
	inline float x86_simd_dot(const simd_t<float, 4> &l, const simd_t<float, 4> &r) noexcept
	{
		return _mm_cvtss_f32(_mm_dp_ps(l.value, r.value, 0xf1));
	}
	inline void x86_simd_norm(simd_t<float, 4> &out, const simd_t<float, 4> &l) noexcept
	{
		out.value = _mm_div_ps(l.value, _mm_sqrt_ps(_mm_dp_ps(l.value, l.value, 0xff)));
	}

#ifdef SEK_USE_SSE2
	inline double x86_simd_dot(const simd_t<double, 2> &l, const simd_t<double, 2> &r) noexcept
	{
		return _mm_cvtsd_f64(_mm_dp_pd(l.value, r.value, 0xf1));
	}
	inline void x86_simd_norm(simd_t<double, 2> &out, const simd_t<double, 2> &l) noexcept
	{
		out.value = _mm_div_pd(l.value, _mm_sqrt_pd(_mm_dp_pd(l.value, l.value, 0xff)));
	}
#ifndef SEK_USE_AVX
	inline double x86_simd_dot(const simd_t<double, 3> &l, const simd_t<double, 3> &r) noexcept
	{
		// clang-format off
		return _mm_cvtsd_f64(_mm_add_pd(
			_mm_dp_pd(l.value[0], r.value[0], 0xf1),
			_mm_dp_pd(l.value[1], r.value[1], 0x11)));
		// clang-format on
	}
	inline void x86_simd_norm(simd_t<double, 3> &out, const simd_t<double, 3> &l) noexcept
	{
		// clang-format off
		const auto magn = _mm_sqrt_pd(_mm_add_pd(
			_mm_dp_pd(l.value[0], l.value[0], 0xff),
			_mm_dp_pd(l.value[1], l.value[1], 0x1f)));
		// clang-format on
		out.value[0] = _mm_div_pd(l.value[0], magn);
		out.value[1] = _mm_div_pd(l.value[1], magn);
	}
	inline double x86_simd_dot(const simd_t<double, 4> &l, const simd_t<double, 4> &r) noexcept
	{
		// clang-format off
		return _mm_cvtsd_f64(_mm_add_pd(
			_mm_dp_pd(l.value[0], r.value[0], 0xf1),
			_mm_dp_pd(l.value[1], r.value[1], 0xf1)));
		// clang-format on
	}
	inline void x86_simd_norm(simd_t<double, 4> &out, const simd_t<double, 4> &l) noexcept
	{
		// clang-format off
		const auto magn = _mm_sqrt_pd(_mm_add_pd(
			_mm_dp_pd(l.value[0], l.value[0], 0xff),
			_mm_dp_pd(l.value[1], l.value[1], 0xff)));
		// clang-format on
		out.value[0] = _mm_div_pd(l.value[0], magn);
		out.value[1] = _mm_div_pd(l.value[1], magn);
	}
#endif
#endif
#else
	template<std::size_t N>
	inline float x86_simd_dot(const simd_t<float, N> &l, const simd_t<float, N> &r) noexcept
	{
		const auto a = _mm_mul_ps(r.value, l.value);
		const auto b = _mm_shuffle_ps(a, a, _MM_SHUFFLE(2, 3, 0, 1));
		const auto c = _mm_add_ps(a, b);
		return _mm_cvtss_f32(_mm_add_ss(c, _mm_movehl_ps(b, c)));
	}
	template<std::size_t N>
	inline void x86_simd_norm(simd_t<float, N> &out, const simd_t<float, N> &l) noexcept
	{
		out.value = _mm_div_ps(l.value, _mm_sqrt_ps(_mm_set1_ps(x86_simd_dot(l, l))));
	}
#ifdef SEK_USE_SSE2
	inline double x86_simd_dot(const simd_t<double, 2> &l, const simd_t<double, 2> &r) noexcept
	{
		const auto a = _mm_mul_pd(r.value, l.value);
		const auto b = _mm_shuffle_pd(a, a, _MM_SHUFFLE2(0, 1));
		return _mm_cvtsd_f64(_mm_add_sd(a, b));
	}
	inline void x86_simd_norm(simd_t<double, 2> &out, const simd_t<double, 2> &l) noexcept
	{
		out.value = _mm_div_pd(l.value, _mm_sqrt_pd(_mm_set1_pd(x86_simd_dot(l, l))));
	}
#ifndef SEK_USE_AVX
	template<std::size_t N>
	inline double x86_simd_dot(const simd_t<double, N> &l, const simd_t<double, N> &r) noexcept
	{
		const __m128d a[2] = {_mm_mul_pd(r.value[0], l.value[0]), _mm_mul_pd(r.value[1], l.value[1])};
		const __m128d b[2] = {_mm_shuffle_pd(a[0], a[0], _MM_SHUFFLE2(0, 1)), _mm_shuffle_pd(a[1], a[1], _MM_SHUFFLE2(0, 1))};
		return _mm_cvtsd_f64(_mm_add_sd(_mm_add_sd(a[0], b[0]), _mm_add_sd(a[1], b[1])));
	}
	template<std::size_t N>
	inline void x86_simd_norm(simd_t<double, N> &out, const simd_t<double, N> &l) noexcept
	{
		const auto magn = _mm_sqrt_pd(_mm_set1_pd(x86_simd_dot(l, l)));
		out.value[0] = _mm_div_pd(l.value[0], magn);
		out.value[1] = _mm_div_pd(l.value[1], magn);
	}
#endif
#endif
#endif
}	 // namespace sek::math::detail

#endif
