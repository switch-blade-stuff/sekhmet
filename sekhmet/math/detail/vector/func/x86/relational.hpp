/*
 * Created by switchblade on 26/06/22
 */

#pragma once

#include "common.hpp"

#ifdef SEK_USE_SSE
namespace sek::math::detail
{
	template<std::size_t N>
	inline void mask_and(simd_mask<float, N> &out, const simd_mask<float, N> &l, const simd_mask<float, N> &r) noexcept
		requires simd_enabled<simd_mask<float, N>>
	{
		out.simd = _mm_and_ps(l.simd, r.simd);
	}
	template<std::size_t N>
	inline void mask_or(simd_mask<float, N> &out, const simd_mask<float, N> &l, const simd_mask<float, N> &r) noexcept
		requires simd_enabled<simd_mask<float, N>>
	{
		out.simd = _mm_or_ps(l.simd, r.simd);
	}
	template<std::size_t N>
	inline void mask_neg(simd_mask<float, N> &out, const simd_mask<float, N> &m) noexcept
		requires simd_enabled<simd_mask<float, N>>
	{
		constexpr auto mask = std::bit_cast<float>(0xffff'ffff);
		out.simd = _mm_xor_ps(m.simd, _mm_set1_ps(mask));
	}
	template<std::size_t N>
	inline void mask_ne(simd_mask<float, N> &out, const simd_mask<float, N> &l, const simd_mask<float, N> &r) noexcept
		requires simd_enabled<simd_mask<float, N>>
	{
		out.simd = _mm_xor_ps(l.simd, r.simd);
	}

	template<std::size_t N>
	inline void vector_max(simd_vector<float, N> &out, const simd_vector<float, N> &l, const simd_vector<float, N> &r) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = _mm_max_ps(l.simd, r.simd);
	}
	template<std::size_t N>
	inline void vector_min(simd_vector<float, N> &out, const simd_vector<float, N> &l, const simd_vector<float, N> &r) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = _mm_min_ps(l.simd, r.simd);
	}
	template<std::size_t N>
	inline void vector_eq(simd_mask<float, N> &out, const simd_vector<float, N> &l, const simd_vector<float, N> &r) noexcept
		requires simd_enabled<simd_mask<float, N>> && simd_enabled<simd_vector<float, N>>
	{
		out.simd = _mm_cmpeq_ps(l.simd, r.simd);
	}
	template<std::size_t N>
	inline void vector_ne(simd_mask<float, N> &out, const simd_vector<float, N> &l, const simd_vector<float, N> &r) noexcept
		requires simd_enabled<simd_mask<float, N>> && simd_enabled<simd_vector<float, N>>
	{
		out.simd = _mm_cmpne_ps(l.simd, r.simd);
	}
	template<std::size_t N>
	inline void vector_lt(simd_mask<float, N> &out, const simd_vector<float, N> &l, const simd_vector<float, N> &r) noexcept
		requires simd_enabled<simd_mask<float, N>> && simd_enabled<simd_vector<float, N>>
	{
		out.simd = _mm_cmplt_ps(l.simd, r.simd);
	}
	template<std::size_t N>
	inline void vector_le(simd_mask<float, N> &out, const simd_vector<float, N> &l, const simd_vector<float, N> &r) noexcept
		requires simd_enabled<simd_mask<float, N>> && simd_enabled<simd_vector<float, N>>
	{
		out.simd = _mm_cmple_ps(l.simd, r.simd);
	}
	template<std::size_t N>
	inline void vector_gt(simd_mask<float, N> &out, const simd_vector<float, N> &l, const simd_vector<float, N> &r) noexcept
		requires simd_enabled<simd_mask<float, N>> && simd_enabled<simd_vector<float, N>>
	{
		out.simd = _mm_cmpgt_ps(l.simd, r.simd);
	}
	template<std::size_t N>
	inline void vector_ge(simd_mask<float, N> &out, const simd_vector<float, N> &l, const simd_vector<float, N> &r) noexcept
		requires simd_enabled<simd_mask<float, N>> && simd_enabled<simd_vector<float, N>>
	{
		out.simd = _mm_cmpge_ps(l.simd, r.simd);
	}

#ifdef SEK_USE_SSE2
	template<std::size_t N>
	inline void mask_eq(simd_mask<float, N> &out, const simd_mask<float, N> &l, const simd_mask<float, N> &r) noexcept
		requires simd_enabled<simd_mask<float, N>>
	{
		out.simd = _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_castps_si128(l.simd), _mm_castps_si128(r.simd)));
	}

	inline void mask_and(simd_mask<double, 2> &out, const simd_mask<double, 2> &l, const simd_mask<double, 2> &r) noexcept
	{
		out.simd = _mm_and_pd(l.simd, r.simd);
	}
	inline void mask_or(simd_mask<double, 2> &out, const simd_mask<double, 2> &l, const simd_mask<double, 2> &r) noexcept
	{
		out.simd = _mm_or_pd(l.simd, r.simd);
	}
	inline void mask_neg(simd_mask<double, 2> &out, const simd_mask<double, 2> &m) noexcept
	{
		constexpr auto mask = std::bit_cast<double>(0xffff'ffff'ffff'ffff);
		out.simd = _mm_xor_pd(m.simd, _mm_set1_pd(mask));
	}
	inline void mask_eq(simd_mask<double, 2> &out, const simd_mask<double, 2> &l, const simd_mask<double, 2> &r) noexcept
	{
		out.simd = _mm_castsi128_pd(_mm_cmpeq_epi32(_mm_castpd_si128(l.simd), _mm_castpd_si128(r.simd)));
	}
	inline void mask_ne(simd_mask<double, 2> &out, const simd_mask<double, 2> &l, const simd_mask<double, 2> &r) noexcept
	{
		out.simd = _mm_xor_pd(l.simd, r.simd);
	}

	template<integral_of_size<4> T, std::size_t N>
	inline void mask_and(simd_mask<T, N> &out, const simd_mask<T, N> &l, const simd_mask<T, N> &r) noexcept
		requires simd_enabled<simd_mask<T, N>>
	{
		out.simd = _mm_and_si128(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void mask_or(simd_mask<T, N> &out, const simd_mask<T, N> &l, const simd_mask<T, N> &r) noexcept
		requires simd_enabled<simd_mask<T, N>>
	{
		out.simd = _mm_or_si128(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void mask_neg(simd_mask<T, N> &out, const simd_mask<T, N> &m) noexcept
		requires simd_enabled<simd_mask<T, N>>
	{
		out.simd = _mm_xor_si128(m.simd, _mm_set1_epi32(-1));
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void mask_eq(simd_mask<T, N> &out, const simd_mask<T, N> &l, const simd_mask<T, N> &r) noexcept
		requires simd_enabled<simd_mask<T, N>>
	{
		out.simd = _mm_cmpeq_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void mask_ne(simd_mask<T, N> &out, const simd_mask<T, N> &l, const simd_mask<T, N> &r) noexcept
		requires simd_enabled<simd_mask<T, N>>
	{
		out.simd = _mm_xor_si128(l.simd, r.simd);
	}

	template<integral_of_size<8> T>
	inline void mask_and(simd_mask<T, 2> &out, const simd_mask<T, 2> &l, const simd_mask<T, 2> &r) noexcept
	{
		out.simd = _mm_and_si128(l.simd, r.simd);
	}
	template<integral_of_size<8> T>
	inline void mask_or(simd_mask<T, 2> &out, const simd_mask<T, 2> &l, const simd_mask<T, 2> &r) noexcept
	{
		out.simd = _mm_or_si128(l.simd, r.simd);
	}
	template<integral_of_size<8> T>
	inline void mask_neg(simd_mask<T, 2> &out, const simd_mask<T, 2> &m) noexcept
	{
		out.simd = _mm_xor_si128(m.simd, _mm_set1_epi32(-1));
	}
	template<integral_of_size<8> T>
	inline void mask_eq(simd_mask<T, 2> &out, const simd_mask<T, 2> &l, const simd_mask<T, 2> &r) noexcept
	{
		out.simd = _mm_cmpeq_epi32(l.simd, r.simd);
	}
	template<integral_of_size<8> T>
	inline void mask_ne(simd_mask<T, 2> &out, const simd_mask<T, 2> &l, const simd_mask<T, 2> &r) noexcept
	{
		out.simd = _mm_xor_si128(l.simd, r.simd);
	}

	inline void vector_max(simd_vector<double, 2> &out, const simd_vector<double, 2> &l, const simd_vector<double, 2> &r) noexcept
	{
		out.simd = _mm_max_pd(l.simd, r.simd);
	}
	inline void vector_min(simd_vector<double, 2> &out, const simd_vector<double, 2> &l, const simd_vector<double, 2> &r) noexcept
	{
		out.simd = _mm_min_pd(l.simd, r.simd);
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

	template<integral_of_size<4> T, std::size_t N>
	inline void vector_max(simd_vector<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires simd_enabled<simd_mask<T, N>> && simd_enabled<simd_vector<T, N>>
	{
		out.simd = _mm_max_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void vector_min(simd_vector<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires simd_enabled<simd_mask<T, N>> && simd_enabled<simd_vector<T, N>>
	{
		out.simd = _mm_min_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void vector_eq(simd_mask<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires simd_enabled<simd_mask<T, N>> && simd_enabled<simd_vector<T, N>>
	{
		out.simd = _mm_cmpeq_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void vector_ne(simd_mask<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires simd_enabled<simd_mask<T, N>> && simd_enabled<simd_vector<T, N>>
	{
		vector_eq(out, l, r);
		mask_neg(out, out);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void vector_lt(simd_mask<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires simd_enabled<simd_mask<T, N>> && simd_enabled<simd_vector<T, N>>
	{
		out.simd = _mm_cmplt_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void vector_gt(simd_mask<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires simd_enabled<simd_mask<T, N>> && simd_enabled<simd_vector<T, N>>
	{
		out.simd = _mm_cmpgt_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void vector_le(simd_mask<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires simd_enabled<simd_mask<T, N>> && simd_enabled<simd_vector<T, N>>
	{
		vector_gt(out, l, r);
		mask_neg(out, out);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void vector_ge(simd_mask<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires simd_enabled<simd_mask<T, N>> && simd_enabled<simd_vector<T, N>>
	{
		vector_lt(out, l, r);
		mask_neg(out, out);
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
		vector_eq(out, l, r);
		mask_neg(out, out);
	}
#endif

#ifndef SEK_USE_AVX
	template<std::size_t N>
	inline void mask_and(simd_mask<double, N> &out, const simd_mask<double, N> &l, const simd_mask<double, N> &r) noexcept
		requires simd_enabled<simd_mask<double, N>>
	{
		out.simd[0] = _mm_and_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_and_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N>
	inline void mask_or(simd_mask<double, N> &out, const simd_mask<double, N> &l, const simd_mask<double, N> &r) noexcept
		requires simd_enabled<simd_mask<double, N>>
	{
		out.simd[0] = _mm_or_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_or_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N>
	inline void mask_neg(simd_mask<double, N> &out, const simd_mask<double, N> &m) noexcept
		requires simd_enabled<simd_mask<double, N>>
	{
		const auto mask = _mm_set1_pd(std::bit_cast<double>(0xffff'ffff'ffff'ffff));
		out.simd[0] = _mm_xor_pd(m.simd[0], mask);
		out.simd[1] = _mm_xor_pd(m.simd[1], mask);
	}
	template<std::size_t N>
	inline void mask_eq(simd_mask<double, N> &out, const simd_mask<double, N> &l, const simd_mask<double, N> &r) noexcept
		requires simd_enabled<simd_mask<double, N>>
	{
		out.simd[0] = _mm_castsi128_pd(_mm_cmpeq_epi32(_mm_castpd_si128(l.simd[0]), _mm_castpd_si128(r.simd[0])));
		out.simd[1] = _mm_castsi128_pd(_mm_cmpeq_epi32(_mm_castpd_si128(l.simd[1]), _mm_castpd_si128(r.simd[1])));
	}
	template<std::size_t N>
	inline void mask_ne(simd_mask<double, N> &out, const simd_mask<double, N> &l, const simd_mask<double, N> &r) noexcept
		requires simd_enabled<simd_mask<double, N>>
	{
		out.simd[0] = _mm_xor_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_xor_pd(l.simd[1], r.simd[1]);
	}

	template<std::size_t N>
	inline void vector_max(simd_vector<double, N> &out, const simd_vector<double, N> &l, const simd_vector<double, N> &r) noexcept
		requires simd_enabled<simd_vector<double, N>>
	{
		out.simd[0] = _mm_max_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_max_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N>
	inline void vector_min(simd_vector<double, N> &out, const simd_vector<double, N> &l, const simd_vector<double, N> &r) noexcept
		requires simd_enabled<simd_vector<double, N>>
	{
		out.simd[0] = _mm_min_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_min_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N>
	inline void vector_eq(simd_mask<double, N> &out, const simd_vector<double, N> &l, const simd_vector<double, N> &r) noexcept
		requires simd_enabled<simd_mask<double, N>> && simd_enabled<simd_vector<double, N>>
	{
		out.simd[0] = _mm_cmpeq_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmpeq_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N>
	inline void vector_ne(simd_mask<double, N> &out, const simd_vector<double, N> &l, const simd_vector<double, N> &r) noexcept
		requires simd_enabled<simd_mask<double, N>> && simd_enabled<simd_vector<double, N>>
	{
		out.simd[0] = _mm_cmpneq_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmpneq_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N>
	inline void vector_lt(simd_mask<double, N> &out, const simd_vector<double, N> &l, const simd_vector<double, N> &r) noexcept
		requires simd_enabled<simd_mask<double, N>> && simd_enabled<simd_vector<double, N>>
	{
		out.simd[0] = _mm_cmplt_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmplt_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N>
	inline void vector_le(simd_mask<double, N> &out, const simd_vector<double, N> &l, const simd_vector<double, N> &r) noexcept
		requires simd_enabled<simd_mask<double, N>> && simd_enabled<simd_vector<double, N>>
	{
		out.simd[0] = _mm_cmple_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmple_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N>
	inline void vector_gt(simd_mask<double, N> &out, const simd_vector<double, N> &l, const simd_vector<double, N> &r) noexcept
		requires simd_enabled<simd_mask<double, N>> && simd_enabled<simd_vector<double, N>>
	{
		out.simd[0] = _mm_cmpgt_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmpgt_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N>
	inline void vector_ge(simd_mask<double, N> &out, const simd_vector<double, N> &l, const simd_vector<double, N> &r) noexcept
		requires simd_enabled<simd_mask<double, N>> && simd_enabled<simd_vector<double, N>>
	{
		out.simd[0] = _mm_cmpge_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmpge_pd(l.simd[1], r.simd[1]);
	}

#ifndef SEK_USE_AVX2
	template<integral_of_size<8> T, std::size_t N>
	inline void mask_and(simd_mask<T, N> &out, const simd_mask<T, N> &l, const simd_mask<T, N> &r) noexcept
		requires simd_enabled<simd_mask<T, N>>
	{
		out.simd[0] = _mm_and_si128(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_and_si128(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N>
	inline void mask_or(simd_mask<T, N> &out, const simd_mask<T, N> &l, const simd_mask<T, N> &r) noexcept
		requires simd_enabled<simd_mask<T, N>>
	{
		out.simd[0] = _mm_or_si128(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_or_si128(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N>
	inline void mask_neg(simd_mask<T, N> &out, const simd_mask<T, N> &m) noexcept
		requires simd_enabled<simd_mask<T, N>>
	{
		const auto mask = _mm_set1_epi32(-1);
		out.simd[0] = _mm_xor_si128(m.simd[0], mask);
		out.simd[1] = _mm_xor_si128(m.simd[1], mask);
	}
	template<integral_of_size<8> T, std::size_t N>
	inline void mask_eq(simd_mask<T, N> &out, const simd_mask<T, N> &l, const simd_mask<T, N> &r) noexcept
		requires simd_enabled<simd_mask<T, N>>
	{
		out.simd[0] = _mm_cmpeq_epi32(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmpeq_epi32(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N>
	inline void mask_ne(simd_mask<T, N> &out, const simd_mask<T, N> &l, const simd_mask<T, N> &r) noexcept
		requires simd_enabled<simd_mask<T, N>>
	{
		out.simd[0] = _mm_xor_si128(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_xor_si128(l.simd[1], r.simd[1]);
	}

#ifdef SEK_USE_SSE4_1
	template<integral_of_size<8> T, std::size_t N>
	inline void vector_eq(simd_mask<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires simd_enabled<simd_mask<T, N>> && simd_enabled<simd_vector<T, N>>
	{
		out.simd[0] = _mm_cmpeq_epi64(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmpeq_epi64(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N>
	inline void vector_ne(simd_mask<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires simd_enabled<simd_mask<T, N>> && simd_enabled<simd_vector<T, N>>
	{
		vector_eq(out, l, r);
		mask_neg(out, out);
	}
#endif
#endif
#endif
#else
	template<std::size_t N>
	inline void mask_eq(simd_mask<float, N> &out, const simd_mask<float, N> &l, const simd_mask<float, N> &r) noexcept
		requires simd_enabled<simd_mask<float, N>>
	{
		mask_ne(out, l, r);
		mask_neg(out, out);
	}
#endif
}	 // namespace sek::math::detail
#endif