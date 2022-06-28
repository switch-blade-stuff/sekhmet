/*
 * Created by switchblade on 26/06/22
 */

#pragma once

#include "common.hpp"

#ifdef SEK_USE_SSE
namespace sek::math::detail
{
	template<std::size_t N, storage_policy P>
	inline void mask_and(mask_data<float, N, P> &out, const mask_data<float, N, P> &l, const mask_data<float, N, P> &r) noexcept
		requires simd_enabled<mask_data<float, N, P>>
	{
		out.simd = _mm_and_ps(l.simd, r.simd);
	}
	template<std::size_t N, storage_policy P>
	inline void mask_or(mask_data<float, N, P> &out, const mask_data<float, N, P> &l, const mask_data<float, N, P> &r) noexcept
		requires simd_enabled<mask_data<float, N, P>>
	{
		out.simd = _mm_or_ps(l.simd, r.simd);
	}
	template<std::size_t N, storage_policy P>
	inline void mask_neg(mask_data<float, N, P> &out, const mask_data<float, N, P> &m) noexcept
		requires simd_enabled<mask_data<float, N, P>>
	{
		constexpr auto mask = std::bit_cast<float>(0xffff'ffff);
		out.simd = _mm_xor_ps(m.simd, _mm_set1_ps(mask));
	}
	template<std::size_t N, storage_policy P>
	inline void mask_ne(mask_data<float, N, P> &out, const mask_data<float, N, P> &l, const mask_data<float, N, P> &r) noexcept
		requires simd_enabled<mask_data<float, N, P>>
	{
		out.simd = _mm_xor_ps(l.simd, r.simd);
	}

	template<std::size_t N, storage_policy P>
	inline void vector_max(vector_data<float, N, P> &out, const vector_data<float, N, P> &l, const vector_data<float, N, P> &r) noexcept
		requires simd_enabled<vector_data<float, N, P>>
	{
		out.simd = _mm_max_ps(l.simd, r.simd);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_min(vector_data<float, N, P> &out, const vector_data<float, N, P> &l, const vector_data<float, N, P> &r) noexcept
		requires simd_enabled<vector_data<float, N, P>>
	{
		out.simd = _mm_min_ps(l.simd, r.simd);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_eq(mask_data<float, N, P> &out, const vector_data<float, N, P> &l, const vector_data<float, N, P> &r) noexcept
		requires simd_enabled<mask_data<float, N, P>> && simd_enabled<vector_data<float, N, P>>
	{
		out.simd = _mm_cmpeq_ps(l.simd, r.simd);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_ne(mask_data<float, N, P> &out, const vector_data<float, N, P> &l, const vector_data<float, N, P> &r) noexcept
		requires simd_enabled<mask_data<float, N, P>> && simd_enabled<vector_data<float, N, P>>
	{
		out.simd = _mm_cmpne_ps(l.simd, r.simd);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_lt(mask_data<float, N, P> &out, const vector_data<float, N, P> &l, const vector_data<float, N, P> &r) noexcept
		requires simd_enabled<mask_data<float, N, P>> && simd_enabled<vector_data<float, N, P>>
	{
		out.simd = _mm_cmplt_ps(l.simd, r.simd);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_le(mask_data<float, N, P> &out, const vector_data<float, N, P> &l, const vector_data<float, N, P> &r) noexcept
		requires simd_enabled<mask_data<float, N, P>> && simd_enabled<vector_data<float, N, P>>
	{
		out.simd = _mm_cmple_ps(l.simd, r.simd);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_gt(mask_data<float, N, P> &out, const vector_data<float, N, P> &l, const vector_data<float, N, P> &r) noexcept
		requires simd_enabled<mask_data<float, N, P>> && simd_enabled<vector_data<float, N, P>>
	{
		out.simd = _mm_cmpgt_ps(l.simd, r.simd);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_ge(mask_data<float, N, P> &out, const vector_data<float, N, P> &l, const vector_data<float, N, P> &r) noexcept
		requires simd_enabled<mask_data<float, N, P>> && simd_enabled<vector_data<float, N, P>>
	{
		out.simd = _mm_cmpge_ps(l.simd, r.simd);
	}

#ifdef SEK_USE_SSE2
	template<std::size_t N, storage_policy P>
	inline void mask_eq(mask_data<float, N, P> &out, const mask_data<float, N, P> &l, const mask_data<float, N, P> &r) noexcept
		requires simd_enabled<mask_data<float, N, P>>
	{
		out.simd = _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_castps_si128(l.simd), _mm_castps_si128(r.simd)));
	}

	template<storage_policy P>
	inline void mask_and(mask_data<double, 2, P> &out, const mask_data<double, 2, P> &l, const mask_data<double, 2, P> &r) noexcept
		requires simd_enabled<mask_data<double, 2, P>>
	{
		out.simd = _mm_and_pd(l.simd, r.simd);
	}
	template<storage_policy P>
	inline void mask_or(mask_data<double, 2, P> &out, const mask_data<double, 2, P> &l, const mask_data<double, 2, P> &r) noexcept
		requires simd_enabled<mask_data<double, 2, P>>
	{
		out.simd = _mm_or_pd(l.simd, r.simd);
	}
	template<storage_policy P>
	inline void mask_neg(mask_data<double, 2, P> &out, const mask_data<double, 2, P> &m) noexcept
		requires simd_enabled<mask_data<double, 2, P>>
	{
		constexpr auto mask = std::bit_cast<double>(0xffff'ffff'ffff'ffff);
		out.simd = _mm_xor_pd(m.simd, _mm_set1_pd(mask));
	}
	template<storage_policy P>
	inline void mask_eq(mask_data<double, 2, P> &out, const mask_data<double, 2, P> &l, const mask_data<double, 2, P> &r) noexcept
		requires simd_enabled<mask_data<double, 2, P>>
	{
		out.simd = _mm_castsi128_pd(_mm_cmpeq_epi32(_mm_castpd_si128(l.simd), _mm_castpd_si128(r.simd)));
	}
	template<storage_policy P>
	inline void mask_ne(mask_data<double, 2, P> &out, const mask_data<double, 2, P> &l, const mask_data<double, 2, P> &r) noexcept
		requires simd_enabled<mask_data<double, 2, P>>
	{
		out.simd = _mm_xor_pd(l.simd, r.simd);
	}

	template<integral_of_size<4> T, std::size_t N, storage_policy P>
	inline void mask_and(mask_data<T, N, P> &out, const mask_data<T, N, P> &l, const mask_data<T, N, P> &r) noexcept
		requires simd_enabled<mask_data<T, N, P>>
	{
		out.simd = _mm_and_si128(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N, storage_policy P>
	inline void mask_or(mask_data<T, N, P> &out, const mask_data<T, N, P> &l, const mask_data<T, N, P> &r) noexcept
		requires simd_enabled<mask_data<T, N, P>>
	{
		out.simd = _mm_or_si128(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N, storage_policy P>
	inline void mask_neg(mask_data<T, N, P> &out, const mask_data<T, N, P> &m) noexcept
		requires simd_enabled<mask_data<T, N, P>>
	{
		out.simd = _mm_xor_si128(m.simd, _mm_set1_epi32(-1));
	}
	template<integral_of_size<4> T, std::size_t N, storage_policy P>
	inline void mask_eq(mask_data<T, N, P> &out, const mask_data<T, N, P> &l, const mask_data<T, N, P> &r) noexcept
		requires simd_enabled<mask_data<T, N, P>>
	{
		out.simd = _mm_cmpeq_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N, storage_policy P>
	inline void mask_ne(mask_data<T, N, P> &out, const mask_data<T, N, P> &l, const mask_data<T, N, P> &r) noexcept
		requires simd_enabled<mask_data<T, N, P>>
	{
		out.simd = _mm_xor_si128(l.simd, r.simd);
	}

	template<integral_of_size<8> T, storage_policy P>
	inline void mask_and(mask_data<T, 2, P> &out, const mask_data<T, 2, P> &l, const mask_data<T, 2, P> &r) noexcept
		requires simd_enabled<mask_data<T, 2, P>>
	{
		out.simd = _mm_and_si128(l.simd, r.simd);
	}
	template<integral_of_size<8> T, storage_policy P>
	inline void mask_or(mask_data<T, 2, P> &out, const mask_data<T, 2, P> &l, const mask_data<T, 2, P> &r) noexcept
		requires simd_enabled<mask_data<T, 2, P>>
	{
		out.simd = _mm_or_si128(l.simd, r.simd);
	}
	template<integral_of_size<8> T, storage_policy P>
	inline void mask_neg(mask_data<T, 2, P> &out, const mask_data<T, 2, P> &m) noexcept
		requires simd_enabled<mask_data<T, 2, P>>
	{
		out.simd = _mm_xor_si128(m.simd, _mm_set1_epi32(-1));
	}
	template<integral_of_size<8> T, storage_policy P>
	inline void mask_eq(mask_data<T, 2, P> &out, const mask_data<T, 2, P> &l, const mask_data<T, 2, P> &r) noexcept
		requires simd_enabled<mask_data<T, 2, P>>
	{
		out.simd = _mm_cmpeq_epi32(l.simd, r.simd);
	}
	template<integral_of_size<8> T, storage_policy P>
	inline void mask_ne(mask_data<T, 2, P> &out, const mask_data<T, 2, P> &l, const mask_data<T, 2, P> &r) noexcept
		requires simd_enabled<mask_data<T, 2, P>>
	{
		out.simd = _mm_xor_si128(l.simd, r.simd);
	}

	template<storage_policy P>
	inline void vector_max(vector_data<double, 2, P> &out,
						   const vector_data<double, 2, P> &l,
						   const vector_data<double, 2, P> &r) noexcept
		requires simd_enabled<vector_data<double, 2, P>>
	{
		out.simd = _mm_max_pd(l.simd, r.simd);
	}
	template<storage_policy P>
	inline void vector_min(vector_data<double, 2, P> &out,
						   const vector_data<double, 2, P> &l,
						   const vector_data<double, 2, P> &r) noexcept
		requires simd_enabled<vector_data<double, 2, P>>
	{
		out.simd = _mm_min_pd(l.simd, r.simd);
	}
	template<storage_policy P>
	inline void vector_eq(mask_data<double, 2, P> &out, const vector_data<double, 2, P> &l, const vector_data<double, 2, P> &r) noexcept
		requires simd_enabled<mask_data<double, 2, P>> && simd_enabled<vector_data<double, 2, P>>
	{
		out.simd = _mm_cmpeq_pd(l.simd, r.simd);
	}
	template<storage_policy P>
	inline void vector_ne(mask_data<double, 2, P> &out, const vector_data<double, 2, P> &l, const vector_data<double, 2, P> &r) noexcept
		requires simd_enabled<mask_data<double, 2, P>> && simd_enabled<vector_data<double, 2, P>>
	{
		out.simd = _mm_cmpneq_pd(l.simd, r.simd);
	}
	template<storage_policy P>
	inline void vector_lt(mask_data<double, 2, P> &out, const vector_data<double, 2, P> &l, const vector_data<double, 2, P> &r) noexcept
		requires simd_enabled<mask_data<double, 2, P>> && simd_enabled<vector_data<double, 2, P>>
	{
		out.simd = _mm_cmplt_pd(l.simd, r.simd);
	}
	template<storage_policy P>
	inline void vector_le(mask_data<double, 2, P> &out, const vector_data<double, 2, P> &l, const vector_data<double, 2, P> &r) noexcept
		requires simd_enabled<mask_data<double, 2, P>> && simd_enabled<vector_data<double, 2, P>>
	{
		out.simd = _mm_cmple_pd(l.simd, r.simd);
	}
	template<storage_policy P>
	inline void vector_gt(mask_data<double, 2, P> &out, const vector_data<double, 2, P> &l, const vector_data<double, 2, P> &r) noexcept
		requires simd_enabled<mask_data<double, 2, P>> && simd_enabled<vector_data<double, 2, P>>
	{
		out.simd = _mm_cmpgt_pd(l.simd, r.simd);
	}
	template<storage_policy P>
	inline void vector_ge(mask_data<double, 2, P> &out, const vector_data<double, 2, P> &l, const vector_data<double, 2, P> &r) noexcept
		requires simd_enabled<mask_data<double, 2, P>> && simd_enabled<vector_data<double, 2, P>>
	{
		out.simd = _mm_cmpge_pd(l.simd, r.simd);
	}

	template<integral_of_size<4> T, std::size_t N, storage_policy P>
	inline void vector_max(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires simd_enabled<mask_data<T, N, P>> && simd_enabled<vector_data<T, N, P>>
	{
		out.simd = _mm_max_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N, storage_policy P>
	inline void vector_min(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires simd_enabled<mask_data<T, N, P>> && simd_enabled<vector_data<T, N, P>>
	{
		out.simd = _mm_min_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N, storage_policy P>
	inline void vector_eq(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires simd_enabled<mask_data<T, N, P>> && simd_enabled<vector_data<T, N, P>>
	{
		out.simd = _mm_cmpeq_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N, storage_policy P>
	inline void vector_ne(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires simd_enabled<mask_data<T, N, P>> && simd_enabled<vector_data<T, N, P>>
	{
		vector_eq(out, l, r);
		mask_neg(out, out);
	}
	template<integral_of_size<4> T, std::size_t N, storage_policy P>
	inline void vector_lt(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires simd_enabled<mask_data<T, N, P>> && simd_enabled<vector_data<T, N, P>>
	{
		out.simd = _mm_cmplt_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N, storage_policy P>
	inline void vector_gt(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires simd_enabled<mask_data<T, N, P>> && simd_enabled<vector_data<T, N, P>>
	{
		out.simd = _mm_cmpgt_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N, storage_policy P>
	inline void vector_le(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires simd_enabled<mask_data<T, N, P>> && simd_enabled<vector_data<T, N, P>>
	{
		vector_gt(out, l, r);
		mask_neg(out, out);
	}
	template<integral_of_size<4> T, std::size_t N, storage_policy P>
	inline void vector_ge(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires simd_enabled<mask_data<T, N, P>> && simd_enabled<vector_data<T, N, P>>
	{
		vector_lt(out, l, r);
		mask_neg(out, out);
	}

#ifdef SEK_USE_SSE4_1
	template<integral_of_size<8> T, storage_policy P>
	inline void vector_eq(mask_data<T, 2, P> &out, const vector_data<T, 2, P> &l, const vector_data<T, 2, P> &r) noexcept
		requires simd_enabled<mask_data<T, 2, P>> && simd_enabled<vector_data<T, 2, P>>
	{
		out.simd = _mm_cmpeq_epi64(l.simd, r.simd);
	}
	template<integral_of_size<8> T, storage_policy P>
	inline void vector_ne(mask_data<T, 2, P> &out, const vector_data<T, 2, P> &l, const vector_data<T, 2, P> &r) noexcept
		requires simd_enabled<mask_data<T, 2, P>> && simd_enabled<vector_data<T, 2, P>>
	{
		vector_eq(out, l, r);
		mask_neg(out, out);
	}
#endif

#ifndef SEK_USE_AVX
	template<std::size_t N, storage_policy P>
	inline void mask_and(mask_data<double, N, P> &out, const mask_data<double, N, P> &l, const mask_data<double, N, P> &r) noexcept
		requires simd_enabled<mask_data<double, N, P>>
	{
		out.simd[0] = _mm_and_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_and_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N, storage_policy P>
	inline void mask_or(mask_data<double, N, P> &out, const mask_data<double, N, P> &l, const mask_data<double, N, P> &r) noexcept
		requires simd_enabled<mask_data<double, N, P>>
	{
		out.simd[0] = _mm_or_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_or_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N, storage_policy P>
	inline void mask_neg(mask_data<double, N, P> &out, const mask_data<double, N, P> &m) noexcept
		requires simd_enabled<mask_data<double, N, P>>
	{
		const auto mask = _mm_set1_pd(std::bit_cast<double>(0xffff'ffff'ffff'ffff));
		out.simd[0] = _mm_xor_pd(m.simd[0], mask);
		out.simd[1] = _mm_xor_pd(m.simd[1], mask);
	}
	template<std::size_t N, storage_policy P>
	inline void mask_eq(mask_data<double, N, P> &out, const mask_data<double, N, P> &l, const mask_data<double, N, P> &r) noexcept
		requires simd_enabled<mask_data<double, N, P>>
	{
		out.simd[0] = _mm_castsi128_pd(_mm_cmpeq_epi32(_mm_castpd_si128(l.simd[0]), _mm_castpd_si128(r.simd[0])));
		out.simd[1] = _mm_castsi128_pd(_mm_cmpeq_epi32(_mm_castpd_si128(l.simd[1]), _mm_castpd_si128(r.simd[1])));
	}
	template<std::size_t N, storage_policy P>
	inline void mask_ne(mask_data<double, N, P> &out, const mask_data<double, N, P> &l, const mask_data<double, N, P> &r) noexcept
		requires simd_enabled<mask_data<double, N, P>>
	{
		out.simd[0] = _mm_xor_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_xor_pd(l.simd[1], r.simd[1]);
	}

		template<std::size_t N, storage_policy P>
	inline void vector_max(vector_data<double, N, P> &out,
						   const vector_data<double, N, P> &l,
						   const vector_data<double, N, P> &r) noexcept
		requires simd_enabled<vector_data<double, N, P>>
	{
		out.simd[0] = _mm_max_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_max_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_min(vector_data<double, N, P> &out,
						   const vector_data<double, N, P> &l,
						   const vector_data<double, N, P> &r) noexcept
		requires simd_enabled<vector_data<double, N, P>>
	{
		out.simd[0] = _mm_min_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_min_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_eq(mask_data<double, N, P> &out, const vector_data<double, N, P> &l, const vector_data<double, N, P> &r) noexcept
		requires simd_enabled<mask_data<double, N, P>> && simd_enabled<vector_data<double, N, P>>
	{
		out.simd[0] = _mm_cmpeq_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmpeq_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_ne(mask_data<double, N, P> &out, const vector_data<double, N, P> &l, const vector_data<double, N, P> &r) noexcept
		requires simd_enabled<mask_data<double, N, P>> && simd_enabled<vector_data<double, N, P>>
	{
		out.simd[0] = _mm_cmpneq_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmpneq_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_lt(mask_data<double, N, P> &out, const vector_data<double, N, P> &l, const vector_data<double, N, P> &r) noexcept
		requires simd_enabled<mask_data<double, N, P>> && simd_enabled<vector_data<double, N, P>>
	{
		out.simd[0] = _mm_cmplt_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmplt_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_le(mask_data<double, N, P> &out, const vector_data<double, N, P> &l, const vector_data<double, N, P> &r) noexcept
		requires simd_enabled<mask_data<double, N, P>> && simd_enabled<vector_data<double, N, P>>
	{
		out.simd[0] = _mm_cmple_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmple_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_gt(mask_data<double, N, P> &out, const vector_data<double, N, P> &l, const vector_data<double, N, P> &r) noexcept
		requires simd_enabled<mask_data<double, N, P>> && simd_enabled<vector_data<double, N, P>>
	{
		out.simd[0] = _mm_cmpgt_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmpgt_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_ge(mask_data<double, N, P> &out, const vector_data<double, N, P> &l, const vector_data<double, N, P> &r) noexcept
		requires simd_enabled<mask_data<double, N, P>> && simd_enabled<vector_data<double, N, P>>
	{
		out.simd[0] = _mm_cmpge_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmpge_pd(l.simd[1], r.simd[1]);
	}

#ifndef SEK_USE_AVX2
	template<integral_of_size<8> T, std::size_t N, storage_policy P>
	inline void mask_and(mask_data<T, N, P> &out, const mask_data<T, N, P> &l, const mask_data<T, N, P> &r) noexcept
		requires simd_enabled<mask_data<T, N, P>>
	{
		out.simd[0] = _mm_and_si128(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_and_si128(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N, storage_policy P>
	inline void mask_or(mask_data<T, N, P> &out, const mask_data<T, N, P> &l, const mask_data<T, N, P> &r) noexcept
		requires simd_enabled<mask_data<T, N, P>>
	{
		out.simd[0] = _mm_or_si128(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_or_si128(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N, storage_policy P>
	inline void mask_neg(mask_data<T, N, P> &out, const mask_data<T, N, P> &m) noexcept
		requires simd_enabled<mask_data<T, N, P>>
	{
		const auto mask = _mm_set1_epi32(-1);
		out.simd[0] = _mm_xor_si128(m.simd[0], mask);
		out.simd[1] = _mm_xor_si128(m.simd[1], mask);
	}
	template<integral_of_size<8> T, std::size_t N, storage_policy P>
	inline void mask_eq(mask_data<T, N, P> &out, const mask_data<T, N, P> &l, const mask_data<T, N, P> &r) noexcept
		requires simd_enabled<mask_data<T, N, P>>
	{
		out.simd[0] = _mm_cmpeq_epi32(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmpeq_epi32(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N, storage_policy P>
	inline void mask_ne(mask_data<T, N, P> &out, const mask_data<T, N, P> &l, const mask_data<T, N, P> &r) noexcept
		requires simd_enabled<mask_data<T, N, P>>
	{
		out.simd[0] = _mm_xor_si128(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_xor_si128(l.simd[1], r.simd[1]);
	}

#ifdef SEK_USE_SSE4_1
	template<integral_of_size<8> T, std::size_t N, storage_policy P>
	inline void vector_eq(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires simd_enabled<mask_data<T, N, P>> && simd_enabled<vector_data<T, N, P>>
	{
		out.simd[0] = _mm_cmpeq_epi64(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmpeq_epi64(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N, storage_policy P>
	inline void vector_ne(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires simd_enabled<mask_data<T, N, P>> && simd_enabled<vector_data<T, N, P>>
	{
		vector_eq(out, l, r);
		mask_neg(out, out);
	}
#endif
#endif
#endif
#else
	template<std::size_t N, storage_policy P>
	inline void mask_eq(mask_data<float, N, P> &out, const mask_data<float, N, P> &l, const mask_data<float, N, P> &r) noexcept
		requires simd_enabled<mask_data<float, N, P>>
	{
		mask_ne(out, l, r);
		mask_neg(out, out);
	}
#endif
}	 // namespace sek::math::detail
#endif