/*
 * Created by switchblade on 2022-01-31
 */

#pragma once

#include "common.hpp"
#include "mask_int32.hpp"

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
	template<integral_of_size<4> T>
	union vector_data<T, 3, storage_policy::OPTIMAL>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(T x, T y, T z) noexcept : values{x, y, z} {}

		template<std::size_t M>
		constexpr explicit vector_data(const T (&data)[M]) noexcept
		{
			std::copy_n(data, min<std::size_t>(3, M), values);
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		T values[3];
		__m128i simd;
	};
	template<integral_of_size<4> T>
	union vector_data<T, 4, storage_policy::OPTIMAL>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(T x, T y, T z, T w) noexcept : values{x, y, z, w} {}

		template<std::size_t M>
		constexpr explicit vector_data(const double (&data)[M]) noexcept
		{
			std::copy_n(data, min<std::size_t>(4, M), values);
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		T values[4];
		__m128i simd;
	};

	template<integral_of_size<4> T, std::size_t N>
	inline void vector_add(simd_vector<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_add_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void vector_sub(simd_vector<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_sub_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void vector_mul(simd_vector<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_mul_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void vector_div(simd_vector<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_div_epi32(l.simd, r.simd);
	}

	template<integral_of_size<4> T, std::size_t N>
	inline void vector_neg(simd_vector<T, N> &out, const simd_vector<T, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_sub_epi32(_mm_setzero_si128(), l.simd);
	}

	template<integral_of_size<4> T, std::size_t N, std::size_t M, std::size_t... Is>
	inline void vector_shuffle(simd_vector<T, N> &out, const simd_vector<T, M> &l, std::index_sequence<Is...> s) noexcept
		requires(SEK_DETAIL_IS_SIMD(out, l))
	{
		constexpr auto mask = x86_128_shuffle4_mask(s);
		out.simd = _mm_shuffle_epi32(l.simd, mask);
	}

#ifdef SEK_USE_SSSE3
	template<integral_of_size<4> T, std::size_t N>
	inline void vector_abs(simd_vector<T, N> &out, const simd_vector<T, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_abs_epi32(l.simd);
	}
#endif
	template<integral_of_size<4> T, std::size_t N>
	inline void vector_max(simd_vector<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_max_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void vector_min(simd_vector<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_min_epi32(l.simd, r.simd);
	}

	template<integral_of_size<4> T, std::size_t N, std::size_t M, std::size_t... Is>
	inline void vector_shuffle(simd_vector<float, N> &out, const simd_vector<float, M> &l, std::index_sequence<Is...> s) noexcept
		requires(SEK_DETAIL_IS_SIMD(out, l))
	{
		constexpr auto mask = x86_128_shuffle4_mask(s);
		out.simd = _mm_shuffle_epi32(l.simd, mask);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void vector_interleave(simd_vector<T, N> &out,
								  const simd_vector<T, N> &l,
								  const simd_vector<T, N> &r,
								  const simd_mask<T, N> &m) noexcept
		requires(SEK_DETAIL_IS_SIMD(out, m))
	{
#ifdef SEK_USE_SSE4_1
		out.simd = _mm_blendv_epi8(r.simd, l.simd, m.simd);
#else
		out.simd = _mm_or_si128(_mm_and_si128(m.simd, l.simd), _mm_andnot_si128(m.simd, r.simd));
#endif
	}

	template<integral_of_size<4> T, std::size_t N>
	inline void vector_eq(simd_mask<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_cmpeq_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void vector_ne(simd_mask<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		vector_eq(out, l, r);
		mask_neg(out, out);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void vector_lt(simd_mask<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_cmplt_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void vector_gt(simd_mask<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_cmpgt_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void vector_le(simd_mask<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		vector_gt(out, l, r);
		mask_neg(out, out);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void vector_ge(simd_mask<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		vector_lt(out, l, r);
		mask_neg(out, out);
	}

	template<std::integral T, std::size_t N>
	inline void vector_and(simd_vector<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_and_si128(l.simd, r.simd);
	}
	template<std::integral T, std::size_t N>
	inline void vector_xor(simd_vector<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_xor_si128(l.simd, r.simd);
	}
	template<std::integral T, std::size_t N>
	inline void vector_or(simd_vector<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_or_si128(l.simd, r.simd);
	}
	template<std::integral T, std::size_t N>
	inline void vector_inv(simd_vector<T, N> &out, const simd_vector<T, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_xor_si128(l.simd, _mm_set1_epi8((int8_t) 0xff));
	}
}	 // namespace sek::math::detail
#endif
