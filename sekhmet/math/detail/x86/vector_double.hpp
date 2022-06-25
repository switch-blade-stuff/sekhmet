/*
 * Created by switchblade on 2022-01-31
 */

#pragma once

#include <bit>

#include "../util.hpp"
#include "common.hpp"
#include "mask_double.hpp"

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
	template<>
	union vector_data<double, 2, storage_policy::OPTIMAL>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(double x, double y) noexcept : values{x, y} {}

		template<std::size_t M>
		constexpr explicit vector_data(const double (&data)[M]) noexcept
		{
			std::copy_n(data, min<std::size_t>(2, M), values);
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		double values[2];
		__m128d simd;
	};

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

	inline void vector_neg(simd_vector<double, 2> &out, const simd_vector<double, 2> &l) noexcept
	{
		out.simd = _mm_sub_pd(_mm_setzero_pd(), l.simd);
	}
	inline void vector_abs(simd_vector<double, 2> &out, const simd_vector<double, 2> &l) noexcept
	{
		constexpr auto mask = std::bit_cast<double>(0x7fff'ffff'ffff'ffff);
		out.simd = _mm_and_pd(_mm_set1_pd(mask), l.simd);
	}
	inline void vector_max(simd_vector<double, 2> &out, const simd_vector<double, 2> &l, const simd_vector<double, 2> &r) noexcept
	{
		out.simd = _mm_max_pd(l.simd, r.simd);
	}
	inline void vector_min(simd_vector<double, 2> &out, const simd_vector<double, 2> &l, const simd_vector<double, 2> &r) noexcept
	{
		out.simd = _mm_min_pd(l.simd, r.simd);
	}

	inline void vector_sqrt(simd_vector<double, 2> &out, const simd_vector<double, 2> &l) noexcept
	{
		out.simd = _mm_sqrt_pd(l.simd);
	}
	inline void vector_rsqrt(simd_vector<double, 2> &out, const simd_vector<double, 2> &l) noexcept
	{
		out.simd = _mm_div_pd(_mm_set1_pd(1), _mm_sqrt_pd(l.simd));
	}

	template<std::size_t... Is>
	inline void vector_shuffle(simd_vector<double, 2> &out, const simd_vector<double, 2> &l, std::index_sequence<Is...> s) noexcept
	{
		constexpr auto mask = x86_128_shuffle2_mask(s);
		out.simd = _mm_shuffle_pd(l.simd, l.simd, mask);
	}

	inline void vector_eq(simd_mask<double, 2> &out, const simd_vector<double, 2> &l, const simd_vector<double, 2> &r) noexcept
	{
		out.simd = _mm_cmpeq_pd(l.simd, r.simd);
	}
	inline void vector_ne(simd_mask<double, 2> &out, const simd_vector<double, 2> &l, const simd_vector<double, 2> &r) noexcept
	{
		out.simd = _mm_cmpneq_pd(l.simd, r.simd);
	}
	inline void vector_lt(simd_mask<double, 2> &out, const simd_vector<double, 2> &l, const simd_vector<double, 2> &r) noexcept
	{
		out.simd = _mm_cmplt_pd(l.simd, r.simd);
	}
	inline void vector_le(simd_mask<double, 2> &out, const simd_vector<double, 2> &l, const simd_vector<double, 2> &r) noexcept
	{
		out.simd = _mm_cmple_pd(l.simd, r.simd);
	}
	inline void vector_gt(simd_mask<double, 2> &out, const simd_vector<double, 2> &l, const simd_vector<double, 2> &r) noexcept
	{
		out.simd = _mm_cmpgt_pd(l.simd, r.simd);
	}
	inline void vector_ge(simd_mask<double, 2> &out, const simd_vector<double, 2> &l, const simd_vector<double, 2> &r) noexcept
	{
		out.simd = _mm_cmpge_pd(l.simd, r.simd);
	}

#ifdef SEK_USE_SSE4_1
	inline void vector_round(simd_vector<double, 2> &out, const simd_vector<double, 2> &l) noexcept
	{
		out.simd = _mm_round_pd(l.simd, _MM_FROUND_RINT);
	}
	inline void vector_floor(simd_vector<double, 2> &out, const simd_vector<double, 2> &l) noexcept
	{
		out.simd = _mm_floor_pd(l.simd);
	}
	inline void vector_ceil(simd_vector<double, 2> &out, const simd_vector<double, 2> &l) noexcept
	{
		out.simd = _mm_ceil_pd(l.simd);
	}
	inline void vector_trunc(simd_vector<double, 2> &out, const simd_vector<double, 2> &l) noexcept
	{
		out.simd = _mm_round_pd(l.simd, _MM_FROUND_TRUNC);
	}
#endif

#ifndef SEK_USE_AVX
	template<>
	union vector_data<double, 3, storage_policy::OPTIMAL>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(double x, double y, double z) noexcept : values{x, y, z} {}

		template<std::size_t M>
		constexpr explicit vector_data(const double (&data)[M]) noexcept
		{
			std::copy_n(data, min<std::size_t>(3, M), values);
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		double values[3];
		__m128d simd[2];
	};
	template<>
	union vector_data<double, 4, storage_policy::OPTIMAL>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(double x, double y, double z, double w) noexcept : values{x, y, z, w} {}

		template<std::size_t M>
		constexpr explicit vector_data(const double (&data)[M]) noexcept
		{
			std::copy_n(data, min<std::size_t>(4, M), values);
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		double values[4];
		__m128d simd[2];
	};

	template<std::size_t N>
	inline void vector_add(simd_vector<double, N> &out, const simd_vector<double, N> &l, const simd_vector<double, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_add_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_add_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N>
	inline void vector_sub(simd_vector<double, N> &out, const simd_vector<double, N> &l, const simd_vector<double, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_sub_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_sub_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N>
	inline void vector_mul(simd_vector<double, N> &out, const simd_vector<double, N> &l, const simd_vector<double, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_mul_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_mul_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N>
	inline void vector_div(simd_vector<double, N> &out, const simd_vector<double, N> &l, const simd_vector<double, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_div_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_div_pd(l.simd[1], r.simd[1]);
	}

	template<std::size_t N>
	inline void vector_neg(simd_vector<double, N> &out, const simd_vector<double, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		const auto z = _mm_setzero_pd();
		out.simd[0] = _mm_sub_pd(z, l.simd[0]);
		out.simd[1] = _mm_sub_pd(z, l.simd[1]);
	}
	template<std::size_t N>
	inline void vector_abs(simd_vector<double, N> &out, const simd_vector<double, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		constexpr auto mask = std::bit_cast<double>(0x7fff'ffff'ffff'ffff);
		const auto m = _mm_set1_pd(mask);
		out.simd[0] = _mm_and_pd(m, l.simd[0]);
		out.simd[1] = _mm_and_pd(m, l.simd[1]);
	}
	template<std::size_t N>
	inline void vector_max(simd_vector<double, N> &out, const simd_vector<double, N> &l, const simd_vector<double, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_max_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_max_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N>
	inline void vector_min(simd_vector<double, N> &out, const simd_vector<double, N> &l, const simd_vector<double, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_min_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_min_pd(l.simd[1], r.simd[1]);
	}

	template<std::size_t N>
	inline void vector_sqrt(simd_vector<double, N> &out, const simd_vector<double, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_sqrt_pd(l.simd[0]);
		out.simd[1] = _mm_sqrt_pd(l.simd[1]);
	}
	template<std::size_t N>
	inline void vector_rsqrt(simd_vector<double, N> &out, const simd_vector<double, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		const auto v1 = _mm_set1_pd(1);
		out.simd[0] = _mm_div_pd(v1, _mm_sqrt_pd(l.simd[0]));
		out.simd[1] = _mm_div_pd(v1, _mm_sqrt_pd(l.simd[1]));
	}

	inline void vector_cross(simd_vector<double, 3> &out, const simd_vector<double, 3> &l, const simd_vector<double, 3> &r) noexcept
	{
		/* 4 shuffles are needed here since the 3 doubles are split across 2 __m128d vectors. */
		const auto a = _mm_shuffle_pd(l.simd[0], l.simd[1], _MM_SHUFFLE2(0, 1));
		const auto b = _mm_shuffle_pd(r.simd[0], r.simd[1], _MM_SHUFFLE2(0, 1));

		out.simd[0] = _mm_sub_pd(_mm_mul_pd(a, _mm_shuffle_pd(r.simd[1], r.simd[0], _MM_SHUFFLE2(0, 0))),
								 _mm_mul_pd(b, _mm_shuffle_pd(l.simd[1], l.simd[0], _MM_SHUFFLE2(0, 0))));
		out.simd[1] = _mm_sub_pd(_mm_mul_pd(l.simd[0], b), _mm_mul_pd(r.simd[0], a));
	}

	template<std::size_t N, std::size_t I0, std::size_t I1, std::size_t... Is>
	inline void vector_shuffle(simd_vector<double, N> &out,
							   const simd_vector<double, 2> &l,
							   std::index_sequence<I0, I1, Is...>) noexcept
		requires(N != 2 && SEK_DETAIL_IS_SIMD(out, l))
	{
		constexpr auto mask0 = x86_128_shuffle2_mask(std::index_sequence<I0, I1>{});
		constexpr auto mask1 = x86_128_shuffle2_mask(std::index_sequence<Is...>{});
		out.simd[0] = _mm_shuffle_pd(l.simd, l.simd, mask0);
		out.simd[1] = _mm_shuffle_pd(l.simd, l.simd, mask1);
	}

	template<std::size_t N>
	inline void vector_eq(simd_mask<double, N> &out, const simd_vector<double, N> &l, const simd_vector<double, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_cmpeq_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmpeq_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N>
	inline void vector_ne(simd_mask<double, N> &out, const simd_vector<double, N> &l, const simd_vector<double, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_cmpneq_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmpneq_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N>
	inline void vector_lt(simd_mask<double, N> &out, const simd_vector<double, N> &l, const simd_vector<double, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_cmplt_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmplt_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N>
	inline void vector_le(simd_mask<double, N> &out, const simd_vector<double, N> &l, const simd_vector<double, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_cmple_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmple_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N>
	inline void vector_gt(simd_mask<double, N> &out, const simd_vector<double, N> &l, const simd_vector<double, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_cmpgt_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmpgt_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N>
	inline void vector_ge(simd_mask<double, N> &out, const simd_vector<double, N> &l, const simd_vector<double, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_cmpge_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmpge_pd(l.simd[1], r.simd[1]);
	}

#ifdef SEK_USE_SSE4_1
	template<std::size_t N>
	inline void vector_round(simd_vector<double, N> &out, const simd_vector<double, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		const int mask = _MM_FROUND_RINT;
		out.simd[0] = _mm_round_pd(l.simd[0], mask);
		out.simd[1] = _mm_round_pd(l.simd[1], mask);
	}
	template<std::size_t N>
	inline void vector_floor(simd_vector<double, N> &out, const simd_vector<double, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_floor_pd(l.simd[0]);
		out.simd[1] = _mm_floor_pd(l.simd[1]);
	}
	template<std::size_t N>
	inline void vector_ceil(simd_vector<double, N> &out, const simd_vector<double, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_ceil_pd(l.simd[0]);
		out.simd[1] = _mm_ceil_pd(l.simd[1]);
	}
	template<std::size_t N>
	inline void vector_trunc(simd_vector<double, N> &out, const simd_vector<double, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		const int mask = _MM_FROUND_TRUNC;
		out.simd[0] = _mm_round_pd(l.simd[0], mask);
		out.simd[1] = _mm_round_pd(l.simd[1], mask);
	}
#endif
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
		requires(SEK_DETAIL_IS_SIMD(l, r))
	{
		const __m128d a[2] = {_mm_mul_pd(r.simd[0], l.simd[0]), _mm_mul_pd(r.simd[1], l.simd[1])};
		const __m128d b[2] = {_mm_shuffle_pd(a[0], a[0], _MM_SHUFFLE2(0, 1)), _mm_shuffle_pd(a[1], a[1], _MM_SHUFFLE2(0, 1))};
		return _mm_cvtsd_f64(_mm_add_sd(_mm_add_sd(a[0], b[0]), _mm_add_sd(a[1], b[1])));
	}
	template<std::size_t N>
	inline void vector_norm(simd_vector<double, N> &out, const simd_vector<double, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		const auto magn = _mm_sqrt_pd(_mm_set1_pd(vector_dot(l, l)));
		out.simd[0] = _mm_div_pd(l.simd[0], magn);
		out.simd[1] = _mm_div_pd(l.simd[1], magn);
	}
#endif
#endif
}	 // namespace sek::math::detail
#endif
