//
// Created by switchblade on 2022-01-31.
//

#pragma once

#include "simd_util.hpp"

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
	struct simd_data<float, 4>
	{
		__m128 value;
	};

	template<std::size_t I>
	constexpr void x86_simd_add(simd_data<float, 4> (&out)[I],
								const simd_data<float, 4> (&l)[I],
								const simd_data<float, 4> (&r)[I]) noexcept
	{
		simd_array_invoke(out, l, r, [](auto &out, auto l, auto r) { out = _mm_add_ps(l, r); });
	}
	template<std::size_t I>
	constexpr void x86_simd_sub(simd_data<float, 4> (&out)[I],
								const simd_data<float, 4> (&l)[I],
								const simd_data<float, 4> (&r)[I]) noexcept
	{
		simd_array_invoke(out, l, r, [](auto &out, auto l, auto r) { out = _mm_sub_ps(l, r); });
	}

	template<std::size_t I>
	constexpr void x86_simd_mul_s(simd_data<float, 4> (&out)[I], const simd_data<float, 4> (&l)[I], float r) noexcept
	{
		simd_array_invoke(out, l, [s = _mm_set1_ps(r)](auto &out, auto l) { out = _mm_mul_ps(l, s); });
	}
	template<std::size_t I>
	constexpr void x86_simd_div_s(simd_data<float, 4> (&out)[I], const simd_data<float, 4> (&l)[I], float r) noexcept
	{
		simd_array_invoke(out, l, [s = _mm_set1_ps(r)](auto &out, auto l) { out = _mm_div_ps(l, s); });
	}

	template<std::size_t I>
	constexpr void x86_simd_neg(simd_data<float, 4> (&out)[I], const simd_data<float, 4> (&data)[I]) noexcept
	{
		simd_array_invoke(out, data, [s = _mm_setzero_ps()](auto &out, auto l) { out = _mm_sub_ps(s, l); });
	}
	template<std::size_t I>
	constexpr void x86_simd_abs(simd_data<float, 4> (&out)[I], const simd_data<float, 4> (&data)[I]) noexcept
	{
		constexpr auto mask = ieee574_mask<float>{0x7fff'ffff};
		simd_array_invoke(out, data, [m = _mm_set1_ps(mask)](auto &out, auto l) { out = _mm_and_ps(m, l); });
	}

	template<std::size_t I>
	constexpr void x86_simd_max(simd_data<float, 4> (&out)[I],
								const simd_data<float, 4> (&l)[I],
								const simd_data<float, 4> (&r)[I]) noexcept
	{
		simd_array_invoke(out, l, r, [](auto &out, auto l, auto r) { out = _mm_max_ps(l, r); });
	}
	template<std::size_t I>
	constexpr void x86_simd_min(simd_data<float, 4> (&out)[I],
								const simd_data<float, 4> (&l)[I],
								const simd_data<float, 4> (&r)[I]) noexcept
	{
		simd_array_invoke(out, l, r, [](auto &out, auto l, auto r) { out = _mm_min_ps(l, r); });
	}

	template<std::size_t I>
	constexpr void x86_simd_sqrt(simd_data<float, 4> (&out)[I], const simd_data<float, 4> (&data)[I]) noexcept
	{
		simd_array_invoke(out, data, [](auto &out, auto l) { out = _mm_sqrt_ps(l); });
	}
	template<std::size_t I>
	constexpr void x86_simd_rsqrt(simd_data<float, 4> (&out)[I], const simd_data<float, 4> (&data)[I]) noexcept
	{
		simd_array_invoke(out, data, [](auto &out, auto l) { out = _mm_rsqrt_ps(l); });
	}

#ifdef SEK_USE_SSE2
	template<>
	struct simd_data<double, 2>
	{
		__m128d value;
	};

	template<std::size_t I>
	constexpr void x86_simd_add(simd_data<double, 2> (&out)[I],
								const simd_data<double, 2> (&l)[I],
								const simd_data<double, 2> (&r)[I]) noexcept
	{
		simd_array_invoke(out, l, r, [](auto &out, auto l, auto r) { out = _mm_add_pd(l, r); });
	}
	template<std::size_t I>
	constexpr void x86_simd_sub(simd_data<double, 2> (&out)[I],
								const simd_data<double, 2> (&l)[I],
								const simd_data<double, 2> (&r)[I]) noexcept
	{
		simd_array_invoke(out, l, r, [](auto &out, auto l, auto r) { out = _mm_sub_pd(l, r); });
	}

	template<std::size_t I>
	constexpr void x86_simd_mul_s(simd_data<double, 2> (&out)[I], const simd_data<double, 2> (&l)[I], double r) noexcept
	{
		simd_array_invoke(out, l, [s = _mm_set1_pd(r)](auto &out, auto l) { out = _mm_mul_pd(l, s); });
	}
	template<std::size_t I>
	constexpr void x86_simd_div_s(simd_data<double, 2> (&out)[I], const simd_data<double, 2> (&l)[I], double r) noexcept
	{
		simd_array_invoke(out, l, [s = _mm_set1_pd(r)](auto &out, auto l) { out = _mm_div_pd(l, s); });
	}

	template<std::size_t I>
	constexpr void x86_simd_neg(simd_data<double, 2> (&out)[I], const simd_data<double, 2> (&data)[I]) noexcept
	{
		simd_array_invoke(out, data, [s = _mm_setzero_pd()](auto &out, auto l) { out = _mm_sub_pd(s, l); });
	}
	template<std::size_t I>
	constexpr void x86_simd_abs(simd_data<double, 2> (&out)[I], const simd_data<double, 2> (&data)[I]) noexcept
	{
		constexpr auto mask = ieee574_mask<double>{0x7fff'ffff'ffff'ffff};
		simd_array_invoke(out, data, [m = _mm_set1_pd(mask)](auto &out, auto l) { out = _mm_and_pd(m, l); });
	}

	template<std::size_t I>
	constexpr void x86_simd_max(simd_data<double, 2> (&out)[I],
								const simd_data<double, 2> (&l)[I],
								const simd_data<double, 2> (&r)[I]) noexcept
	{
		simd_array_invoke(out, l, r, [](auto &out, auto l, auto r) { out = _mm_max_pd(l, r); });
	}
	template<std::size_t I>
	constexpr void x86_simd_min(simd_data<double, 2> (&out)[I],
								const simd_data<double, 2> (&l)[I],
								const simd_data<double, 2> (&r)[I]) noexcept
	{
		simd_array_invoke(out, l, r, [](auto &out, auto l, auto r) { out = _mm_min_pd(l, r); });
	}

	template<std::size_t I>
	constexpr void x86_simd_sqrt(simd_data<double, 2> (&out)[I], const simd_data<double, 2> (&data)[I]) noexcept
	{
		simd_array_invoke(out, data, [](auto &out, auto l) { out = _mm_sqrt_pd(l); });
	}
	template<std::size_t I>
	constexpr void x86_simd_rsqrt(simd_data<double, 2> (&out)[I], const simd_data<double, 2> (&data)[I]) noexcept
	{
		simd_array_invoke(out, data, [](auto &out, auto l) { out = _mm_rsqrt_pd(l); });
	}

	template<integral_of_size<8> T>
	struct simd_data<T, 2>
	{
		__m128i value;
	};

	template<integral_of_size<8> T, std::size_t I>
	constexpr void x86_simd_add(simd_data<T, 2> (&out)[I], const simd_data<T, 2> (&l)[I], const simd_data<T, 2> (&r)[I]) noexcept
	{
		simd_array_invoke(out, l, r, [](auto &out, auto l, auto r) { out = _mm_add_epi64(l, r); });
	}
	template<integral_of_size<8> T, std::size_t I>
	constexpr void x86_simd_sub(simd_data<T, 2> (&out)[I], const simd_data<T, 2> (&l)[I], const simd_data<T, 2> (&r)[I]) noexcept
	{
		simd_array_invoke(out, l, r, [](auto &out, auto l, auto r) { out = _mm_sub_epi64(l, r); });
	}

	template<integral_of_size<8> T, std::size_t I>
	constexpr void x86_simd_neg(simd_data<T, 2> (&out)[I], const simd_data<T, 2> (&data)[I]) noexcept
	{
		simd_array_invoke(out, data, [s = _mm_setzero_si128()](auto &out, auto l) { out = _mm_sub_epi64(s, l); });
	}

	template<integral_of_size<4> T>
	struct simd_data<T, 4>
	{
		__m128i value;
	};

	template<integral_of_size<4> T, std::size_t I>
	constexpr void x86_simd_add(simd_data<T, 4> (&out)[I], const simd_data<T, 4> (&l)[I], const simd_data<T, 4> (&r)[I]) noexcept
	{
		simd_array_invoke(out, l, r, [](auto &out, auto l, auto r) { out = _mm_add_epi32(l, r); });
	}
	template<integral_of_size<4> T, std::size_t I>
	constexpr void x86_simd_sub(simd_data<T, 4> (&out)[I], const simd_data<T, 4> (&l)[I], const simd_data<T, 4> (&r)[I]) noexcept
	{
		simd_array_invoke(out, l, r, [](auto &out, auto l, auto r) { out = _mm_sub_epi32(l, r); });
	}

	template<integral_of_size<4> T, std::size_t I>
	constexpr void x86_simd_mul_s(simd_data<T, 4> (&out)[I], const simd_data<T, 4> (&l)[I], T r) noexcept
	{
		simd_array_invoke(out, l, [s = _mm_set1_epi32(r)](auto &out, auto l) { out = _mm_div_epi32(l, s); });
	}
	template<integral_of_size<4> T, std::size_t I>
	constexpr void x86_simd_div_s(simd_data<T, 4> (&out)[I], const simd_data<T, 4> (&l)[I], T r) noexcept
	{
		simd_array_invoke(out, l, [s = _mm_set1_epi32(r)](auto &out, auto l) { out = _mm_div_epi32(l, s); });
	}

	template<integral_of_size<4> T, std::size_t I>
	constexpr void x86_simd_neg(simd_data<T, 4> (&out)[I], const simd_data<T, 4> (&data)[I]) noexcept
	{
		simd_array_invoke(out, data, [s = _mm_setzero_si128()](auto &out, auto l) { out = _mm_sub_epi32(s, l); });
	}
#ifdef SEK_USE_SSSE3
	template<integral_of_size<4> T, std::size_t I>
	constexpr void x86_simd_abs(simd_data<T, 4> (&out)[I], const simd_data<T, 4> (&data)[I]) noexcept
	{
		simd_array_invoke(out, data, [](auto &out, auto l) { out = _mm_abs_epi32(l); });
	}
#endif

#ifdef SEK_USE_SSE4_1
	template<integral_of_size<4> T, std::size_t I>
	constexpr void x86_simd_max(simd_data<T, 4> (&out)[I], const simd_data<T, 4> (&l)[I], const simd_data<T, 4> (&r)[I]) noexcept
	{
		simd_array_invoke(out, l, r, [](auto &out, auto l, auto r) { out = _mm_max_epi32(l, r); });
	}
	template<integral_of_size<4> T, std::size_t I>
	constexpr void x86_simd_min(simd_data<T, 4> (&out)[I], const simd_data<T, 4> (&l)[I], const simd_data<T, 4> (&r)[I]) noexcept
	{
		simd_array_invoke(out, l, r, [](auto &out, auto l, auto r) { out = _mm_min_epi32(l, r); });
	}
#endif

	template<integral_of_size<2> T>
	struct simd_data<T, 8>
	{
		__m128i value;
	};

	template<integral_of_size<2> T, std::size_t I>
	constexpr void x86_simd_add(simd_data<T, 8> (&out)[I], const simd_data<T, 8> (&l)[I], const simd_data<T, 8> (&r)[I]) noexcept
	{
		simd_array_invoke(out, l, r, [](auto &out, auto l, auto r) { out = _mm_add_epi16(l, r); });
	}
	template<integral_of_size<2> T, std::size_t I>
	constexpr void x86_simd_sub(simd_data<T, 8> (&out)[I], const simd_data<T, 8> (&l)[I], const simd_data<T, 8> (&r)[I]) noexcept
	{
		simd_array_invoke(out, l, r, [](auto &out, auto l, auto r) { out = _mm_sub_epi16(l, r); });
	}

	template<integral_of_size<2> T, std::size_t I>
	constexpr void x86_simd_mul_s(simd_data<T, 8> (&out)[I], const simd_data<T, 8> (&l)[I], T r) noexcept
	{
		simd_array_invoke(out, l, [s = _mm_set1_epi16(r)](auto &out, auto l) { out = _mm_div_epi16(l, s); });
	}
	template<integral_of_size<2> T, std::size_t I>
	constexpr void x86_simd_div_s(simd_data<T, 8> (&out)[I], const simd_data<T, 8> (&l)[I], T r) noexcept
	{
		simd_array_invoke(out, l, [s = _mm_set1_epi16(r)](auto &out, auto l) { out = _mm_div_epi16(l, s); });
	}

	template<integral_of_size<2> T, std::size_t I>
	constexpr void x86_simd_neg(simd_data<T, 8> (&out)[I], const simd_data<T, 8> (&data)[I]) noexcept
	{
		simd_array_invoke(out, data, [s = _mm_setzero_si128()](auto &out, auto l) { out = _mm_sub_epi16(s, l); });
	}
#ifdef SEK_USE_SSSE3
	template<integral_of_size<2> T, std::size_t I>
	constexpr void x86_simd_abs(simd_data<T, 8> (&out)[I], const simd_data<T, 8> (&data)[I]) noexcept
	{
		simd_array_invoke(out, data, [](auto &out, auto l) { out = _mm_abs_epi16(l); });
	}
#endif

	template<integral_of_size<2> T, std::size_t I>
	constexpr void x86_simd_max(simd_data<T, 8> (&out)[I], const simd_data<T, 8> (&l)[I], const simd_data<T, 8> (&r)[I]) noexcept
	{
		simd_array_invoke(out, l, r, [](auto &out, auto l, auto r) { out = _mm_max_epi16(l, r); });
	}
	template<integral_of_size<2> T, std::size_t I>
	constexpr void x86_simd_min(simd_data<T, 8> (&out)[I], const simd_data<T, 8> (&l)[I], const simd_data<T, 8> (&r)[I]) noexcept
	{
		simd_array_invoke(out, l, r, [](auto &out, auto l, auto r) { out = _mm_min_epi16(l, r); });
	}

	template<integral_of_size<1> T>
	struct simd_data<T, 16>
	{
		__m128i value;
	};

	template<integral_of_size<1> T, std::size_t I>
	constexpr void x86_simd_add(simd_data<T, 16> (&out)[I], const simd_data<T, 16> (&l)[I], const simd_data<T, 16> (&r)[I]) noexcept
	{
		simd_array_invoke(out, l, r, [](auto &out, auto l, auto r) { out = _mm_add_epi8(l, r); });
	}
	template<integral_of_size<1> T, std::size_t I>
	constexpr void x86_simd_sub(simd_data<T, 16> (&out)[I], const simd_data<T, 16> (&l)[I], const simd_data<T, 16> (&r)[I]) noexcept
	{
		simd_array_invoke(out, l, r, [](auto &out, auto l, auto r) { out = _mm_sub_epi8(l, r); });
	}

	template<integral_of_size<1> T, std::size_t I>
	constexpr void x86_simd_mul_s(simd_data<T, 16> (&out)[I], const simd_data<T, 16> (&l)[I], T r) noexcept
	{
		simd_array_invoke(out, l, [s = _mm_set1_epi8(r)](auto &out, auto l) { out = _mm_div_epi8(l, s); });
	}
	template<integral_of_size<1> T, std::size_t I>
	constexpr void x86_simd_div_s(simd_data<T, 16> (&out)[I], const simd_data<T, 16> (&l)[I], T r) noexcept
	{
		simd_array_invoke(out, l, [s = _mm_set1_epi8(r)](auto &out, auto l) { out = _mm_div_epi8(l, s); });
	}

	template<integral_of_size<1> T, std::size_t I>
	constexpr void x86_simd_neg(simd_data<T, 16> (&out)[I], const simd_data<T, 16> (&data)[I]) noexcept
	{
		simd_array_invoke(out, data, [s = _mm_setzero_si128()](auto &out, auto l) { out = _mm_sub_epi8(s, l); });
	}
#ifdef SEK_USE_SSSE3
	template<integral_of_size<1> T, std::size_t I>
	constexpr void x86_simd_abs(simd_data<T, 16> (&out)[I], const simd_data<T, 16> (&data)[I]) noexcept
	{
		simd_array_invoke(out, data, [](auto &out, auto l) { out = _mm_abs_epi8(l); });
	}
#endif

#ifdef SEK_USE_SSE4_1
	template<integral_of_size<1> T, std::size_t I>
	constexpr void x86_simd_max(simd_data<T, 16> (&out)[I], const simd_data<T, 16> (&l)[I], const simd_data<T, 16> (&r)[I]) noexcept
	{
		simd_array_invoke(out, l, r, [](auto &out, auto l, auto r) { out = _mm_max_epi8(l, r); });
	}
	template<integral_of_size<1> T, std::size_t I>
	constexpr void x86_simd_min(simd_data<T, 16> (&out)[I], const simd_data<T, 16> (&l)[I], const simd_data<T, 16> (&r)[I]) noexcept
	{
		simd_array_invoke(out, l, r, [](auto &out, auto l, auto r) { out = _mm_min_epi8(l, r); });
	}
#endif

	template<std::integral T, std::size_t N, std::size_t I>
	constexpr void x86_simd_and(simd_data<T, N> (&out)[I], const simd_data<T, N> (&l)[I], const simd_data<T, N> (&r)[I]) noexcept
		requires(sizeof(T[N]) <= 16)
	{
		simd_array_invoke(out, l, r, [](auto &out, auto l, auto r) { out = _mm_and_si128(l, r); });
	}
	template<std::integral T, std::size_t N, std::size_t I>
	constexpr void x86_simd_xor(simd_data<T, N> (&out)[I], const simd_data<T, N> (&l)[I], const simd_data<T, N> (&r)[I]) noexcept
		requires(sizeof(T[N]) <= 16)
	{
		simd_array_invoke(out, l, r, [](auto &out, auto l, auto r) { out = _mm_xor_si128(l, r); });
	}
	template<std::integral T, std::size_t N, std::size_t I>
	constexpr void x86_simd_or(simd_data<T, N> (&out)[I], const simd_data<T, N> (&l)[I], const simd_data<T, N> (&r)[I]) noexcept
		requires(sizeof(T[N]) <= 16)
	{
		simd_array_invoke(out, l, r, [](auto &out, auto l, auto r) { out = _mm_or_si128(l, r); });
	}
	template<std::integral T, std::size_t N, std::size_t I>
	constexpr void x86_simd_inv(simd_data<T, N> (&out)[I], const simd_data<T, N> (&l)[I]) noexcept
		requires(sizeof(T[N]) <= 16)
	{
		simd_array_invoke(out, l, [m = _mm_set1_epi8((int8_t) 0xff)](auto &out, auto l) { out = _mm_xor_si128(l, m); });
	}
#endif

	/* There are ugly loops in x86_simd_dot & x86_simd_norm (first iteration is separate),
	 * this is needed however, since for some reason if `dp` is first 0-initialized and then added to,
	 * GCC (at least) produces weird assembly output with useless `+ 0`s.
	 * Doing things this way (moving first iteration out of the loop) produces less assembly (and no useless additions). */
#ifdef SEK_USE_SSE4_1
	template<std::size_t N>
	constexpr float x86_simd_dot(const simd_data<float, 4> (&l)[N], const simd_data<float, 4> (&r)[N]) noexcept
	{
		auto result = _mm_cvtss_f32(_mm_dp_ps(l[0].value, r[0].value, 0xf1));
		for (auto i = N; i-- > 1;) result += _mm_cvtss_f32(_mm_dp_ps(l[i].value, r[i].value, 0xf1));
		return result;
	}
	template<std::size_t N>
	constexpr void x86_simd_norm(simd_data<float, 4> (&out)[N], const simd_data<float, 4> (&l)[N]) noexcept
	{
		auto dp = _mm_dp_ps(l[0].value, l[0].value, 0xff);
		for (auto i = N; i-- > 1;) dp = _mm_add_ps(dp, _mm_dp_ps(l[i].value, l[i].value, 0xff));
		auto magn = _mm_sqrt_ps(dp);
		for (auto i = N; i-- > 0;) out[i].value = _mm_div_ps(l[i].value, magn);
	}

#ifdef SEK_USE_SSE2
	template<std::size_t N>
	constexpr double x86_simd_dot(const simd_data<double, 2> (&l)[N], const simd_data<double, 2> (&r)[N]) noexcept
	{
		auto result = _mm_cvtsd_f64(_mm_dp_pd(l[0].value, r[0].value, 0xf1));
		for (auto i = N; i-- > 1;) result += _mm_cvtsd_f64(_mm_dp_pd(l[i].value, r[i].value, 0xf1));
		return result;
	}
	template<std::size_t N>
	constexpr void x86_simd_norm(simd_data<double, 2> (&out)[N], const simd_data<double, 2> (&l)[N]) noexcept
	{
		auto dp = _mm_dp_pd(l[0].value, l[0].value, 0xff);
		for (auto i = N; i-- > 1;) dp = _mm_add_pd(dp, _mm_dp_pd(l[i].value, l[i].value, 0xff));
		auto magn = _mm_sqrt_pd(dp);
		for (auto i = N; i-- > 0;) out[i].value = _mm_div_pd(l[i].value, magn);
	}
#endif
#else
	template<std::size_t N>
	constexpr float x86_simd_dot(const simd_data<float, 4> (&l)[N], const simd_data<float, 4> (&r)[N]) noexcept
	{
		auto a = _mm_mul_ps(r[0].value, l[0].value);
		auto b = _mm_shuffle_ps(a, a, _MM_SHUFFLE(2, 3, 0, 1));
		auto c = _mm_add_ps(a, b);
		b = _mm_movehl_ps(b, c);
		auto result = _mm_cvtss_f32(_mm_add_ss(c, b));
		for (auto i = N; i-- > 1;)
		{
			a = _mm_mul_ps(r[i].value, l[i].value);
			b = _mm_shuffle_ps(a, a, _MM_SHUFFLE(2, 3, 0, 1));
			c = _mm_add_ps(a, b);
			b = _mm_movehl_ps(b, c);
			result += _mm_cvtss_f32(_mm_add_ss(c, b));
		}
		return result;
	}
	template<std::size_t N>
	constexpr void x86_simd_norm(simd_data<float, 4> (&out)[N], const simd_data<float, 4> (&l)[N]) noexcept
	{
		auto a = _mm_mul_ps(l[0].value, l[0].value);
		auto b = _mm_shuffle_ps(a, a, _MM_SHUFFLE(2, 3, 0, 1));
		auto c = _mm_add_ps(a, b);
		b = _mm_movehl_ps(b, c);
		auto dp = _mm_set1_ps(_mm_cvtss_f32(_mm_add_ss(c, b)));

		for (auto i = N; i-- > 1;)
		{
			a = _mm_mul_ps(l[i].value, l[i].value);
			b = _mm_shuffle_ps(a, a, _MM_SHUFFLE(2, 3, 0, 1));
			c = _mm_add_ps(a, b);
			b = _mm_movehl_ps(b, c);
			dp = _mm_add_ps(dp, _mm_set1_ps(_mm_cvtss_f32(_mm_add_ss(c, b))));
		}
		auto magn = _mm_sqrt_ps(dp);
		for (auto i = N; i-- > 0;) out[i].value = _mm_div_ps(l[i].value, magn);
	}

#ifdef SEK_USE_SSE2
	template<std::size_t N>
	constexpr double x86_simd_dot(const simd_data<double, 2> (&l)[N], const simd_data<double, 2> (&r)[N]) noexcept
	{
		auto a = _mm_mul_pd(r[0].value, l[0].value);
		auto b = _mm_shuffle_pd(a, a, _MM_SHUFFLE2(0, 1));
		auto result = _mm_cvtsd_f64(_mm_add_sd(a, b));

		for (auto i = N; i-- > 1;)
		{
			auto a = _mm_mul_pd(r[i].value, l[i].value);
			auto b = _mm_shuffle_pd(a, a, _MM_SHUFFLE2(0, 1));
			result += _mm_cvtsd_f64(_mm_add_sd(a, b));
		}
		return result;
	}
	template<std::size_t N>
	constexpr void x86_simd_norm(simd_data<double, 2> (&out)[N], const simd_data<double, 2> (&l)[N]) noexcept
	{
		auto a = _mm_mul_pd(l[0].value, l[0].value);
		auto b = _mm_shuffle_pd(a, a, _MM_SHUFFLE2(0, 1));
		auto dp = _mm_set1_pd(_mm_cvtsd_f64(_mm_add_sd(a, b)));

		for (auto i = N; i-- > 1;)
		{
			a = _mm_mul_pd(l[i].value, l[i].value);
			b = _mm_shuffle_pd(a, a, _MM_SHUFFLE2(0, 1));
			dp += _mm_add_pd(dp, _mm_set1_pd(_mm_cvtsd_f64(_mm_add_sd(a, b))));
		}
		auto magn = _mm_sqrt_pd(dp);
		for (auto i = N; i-- > 0;) out[i].value = _mm_div_pd(l[i].value, magn);
	}
#endif
#endif
}	 // namespace sek::math::detail

#endif
