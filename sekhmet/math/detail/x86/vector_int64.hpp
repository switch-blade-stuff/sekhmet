/*
 * Created by switchblade on 2022-01-31
 */

#pragma once

#include "../util.hpp"
#include "common.hpp"
#include "mask_int64.hpp"

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
	template<integral_of_size<8> T>
	union vector_data<T, 2, storage_policy::OPTIMAL>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(T x, T y) noexcept : values{x, y} {}

		template<std::size_t M>
		constexpr explicit vector_data(const T (&data)[M]) noexcept
		{
			std::copy_n(data, min<std::size_t>(2, M), values);
		}
		template<typename... Args>
		constexpr explicit vector_data(Args &&...args) noexcept : vector_data({std::forward<Args>(args)...})
		{
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		T values[2];
		__m128i simd;
	};

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
	inline void vector_neg(simd_vector<T, 2> &out, const simd_vector<T, 2> &l) noexcept
	{
		out.simd = _mm_sub_epi64(_mm_setzero_si128(), l.simd);
	}

#ifdef SEK_USE_SSE4_1
	template<integral_of_size<8> T>
	inline void vector_eq(simd_mask<T, 2> &out, const simd_vector<T, 2> &l, const simd_vector<T, 2> &r) noexcept
	{
		out.simd = _mm_cmpeq_epi64(l.simd, r.simd);
	}
	template<integral_of_size<8> T>
	inline void vector_ne(simd_mask<T, 2> &out, const simd_vector<T, 2> &l, const simd_vector<T, 2> &r) noexcept
	{
		out.simd = _mm_cmpneq_epi64(l.simd, r.simd);
	}
	template<integral_of_size<8> T>
	inline void vector_lt(simd_mask<T, 2> &out, const simd_vector<T, 2> &l, const simd_vector<T, 2> &r) noexcept
	{
		out.simd = _mm_cmplt_epi64(l.simd, r.simd);
	}
	template<integral_of_size<8> T>
	inline void vector_le(simd_mask<T, 2> &out, const simd_vector<T, 2> &l, const simd_vector<T, 2> &r) noexcept
	{
		out.simd = _mm_cmple_epi64(l.simd, r.simd);
	}
	template<integral_of_size<8> T>
	inline void vector_gt(simd_mask<T, 2> &out, const simd_vector<T, 2> &l, const simd_vector<T, 2> &r) noexcept
	{
		out.simd = _mm_cmpgt_epi64(l.simd, r.simd);
	}
	template<integral_of_size<8> T>
	inline void vector_ge(simd_mask<T, 2> &out, const simd_vector<T, 2> &l, const simd_vector<T, 2> &r) noexcept
	{
		out.simd = _mm_cmpge_epi64(l.simd, r.simd);
	}
#endif

#ifndef SEK_USE_AVX2
	template<integral_of_size<8> T>
	union vector_data<T, 3, storage_policy::OPTIMAL>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(T x, T y, T z) noexcept : values{x, y, z} {}

		template<std::size_t M>
		constexpr explicit vector_data(const T (&data)[M]) noexcept
		{
			std::copy_n(data, min<std::size_t>(3, M), values);
		}
		template<typename... Args>
		constexpr explicit vector_data(Args &&...args) noexcept : vector_data({std::forward<Args>(args)...})
		{
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		T values[3];
		__m128i simd[2];
	};
	template<integral_of_size<8> T>
	union vector_data<T, 4, storage_policy::OPTIMAL>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(T x, T y, T z, T w) noexcept : values{x, y, z, w} {}

		template<std::size_t M>
		constexpr explicit vector_data(const double (&data)[M]) noexcept
		{
			std::copy_n(data, min<std::size_t>(4, M), values);
		}
		template<typename... Args>
		constexpr explicit vector_data(Args &&...args) noexcept : vector_data({std::forward<Args>(args)...})
		{
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		T values[4];
		__m128i simd[2];
	};

	template<integral_of_size<8> T, std::size_t N>
	inline void vector_add(simd_vector<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_add_epi64(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_add_epi64(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N>
	inline void vector_sub(simd_vector<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_sub_epi64(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_sub_epi64(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N>
	inline void vector_neg(simd_vector<T, N> &out, const simd_vector<T, N> &l) noexcept requires(SEK_DETAIL_IS_SIMD(out))
	{
		const auto z = _mm_setzero_si128();
		out.simd[0] = _mm_sub_epi64(z, l.simd[0]);
		out.simd[1] = _mm_sub_epi64(z, l.simd[1]);
	}

	template<integral_of_size<8> T, std::size_t N>
	inline void vector_and(simd_vector<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_and_si128(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_and_si128(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N>
	inline void vector_xor(simd_vector<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_xor_si128(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_xor_si128(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N>
	inline void vector_or(simd_vector<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_or_si128(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_or_si128(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N>
	inline void vector_inv(simd_vector<T, N> &out, const simd_vector<T, N> &l) noexcept requires(SEK_DETAIL_IS_SIMD(out))
	{
		const auto m = _mm_set1_epi8((int8_t) 0xff);
		out.simd[0] = _mm_xor_si128(l.simd[0], m);
		out.simd[1] = _mm_xor_si128(l.simd[1], m);
	}

#ifdef SEK_USE_SSE4_1
	template<integral_of_size<8> T, std::size_t N>
	inline void vector_eq(simd_mask<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_cmpeq_epi64(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmpeq_epi64(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N>
	inline void vector_ne(simd_mask<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_cmpneq_epi64(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmpneq_epi64(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N>
	inline void vector_lt(simd_mask<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_cmplt_epi64(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmplt_epi64(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N>
	inline void vector_le(simd_mask<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_cmple_epi64(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmple_epi64(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N>
	inline void vector_gt(simd_mask<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_cmpgt_epi64(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmpgt_epi64(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N>
	inline void vector_ge(simd_mask<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_cmpge_epi64(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmpge_epi64(l.simd[1], r.simd[1]);
	}
#endif
#endif
}	 // namespace sek::math::detail
#endif
