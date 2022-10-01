/*
 * Created by switchblade on 26/06/22
 */

#pragma once

#include "../common.hpp"

#ifdef SEK_USE_SSE2
namespace sek::detail
{
	template<integral_of_size<8> T, policy_t P>
	inline void mask_and(mask_data<T, 2, P> &out, const mask_data<T, 2, P> &l, const mask_data<T, 2, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_and_si128(l.simd, r.simd);
	}
	template<integral_of_size<8> T, policy_t P>
	inline void mask_or(mask_data<T, 2, P> &out, const mask_data<T, 2, P> &l, const mask_data<T, 2, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_or_si128(l.simd, r.simd);
	}
	template<integral_of_size<8> T, policy_t P>
	inline void mask_neg(mask_data<T, 2, P> &out, const mask_data<T, 2, P> &m) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_xor_si128(m.simd, _mm_set1_epi32(-1));
	}
	template<integral_of_size<8> T, policy_t P>
	inline void mask_eq(mask_data<T, 2, P> &out, const mask_data<T, 2, P> &l, const mask_data<T, 2, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_cmpeq_epi32(l.simd, r.simd);
	}
	template<integral_of_size<8> T, policy_t P>
	inline void mask_ne(mask_data<T, 2, P> &out, const mask_data<T, 2, P> &l, const mask_data<T, 2, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_xor_si128(l.simd, r.simd);
	}

#ifdef SEK_USE_SSE4_1
	template<integral_of_size<8> T, policy_t P>
	inline void vector_eq(mask_data<T, 2, P> &out, const vector_data<T, 2, P> &l, const vector_data<T, 2, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_cmpeq_epi64(l.simd, r.simd);
	}
	template<integral_of_size<8> T, policy_t P>
	inline void vector_ne(mask_data<T, 2, P> &out, const vector_data<T, 2, P> &l, const vector_data<T, 2, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		vector_eq(out, l, r);
		mask_neg(out, out);
	}
#endif

#ifndef SEK_USE_AVX2
	template<integral_of_size<8> T, std::size_t N, policy_t P>
	inline void mask_and(mask_data<T, N, P> &out, const mask_data<T, N, P> &l, const mask_data<T, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd[0] = _mm_and_si128(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_and_si128(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N, policy_t P>
	inline void mask_or(mask_data<T, N, P> &out, const mask_data<T, N, P> &l, const mask_data<T, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd[0] = _mm_or_si128(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_or_si128(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N, policy_t P>
	inline void mask_neg(mask_data<T, N, P> &out, const mask_data<T, N, P> &m) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		const auto mask = _mm_set1_epi32(-1);
		out.simd[0] = _mm_xor_si128(m.simd[0], mask);
		out.simd[1] = _mm_xor_si128(m.simd[1], mask);
	}
	template<integral_of_size<8> T, std::size_t N, policy_t P>
	inline void mask_eq(mask_data<T, N, P> &out, const mask_data<T, N, P> &l, const mask_data<T, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd[0] = _mm_cmpeq_epi32(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmpeq_epi32(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N, policy_t P>
	inline void mask_ne(mask_data<T, N, P> &out, const mask_data<T, N, P> &l, const mask_data<T, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd[0] = _mm_xor_si128(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_xor_si128(l.simd[1], r.simd[1]);
	}

#ifdef SEK_USE_SSE4_1
	template<integral_of_size<8> T, std::size_t N, policy_t P>
	inline void vector_eq(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd[0] = _mm_cmpeq_epi64(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_cmpeq_epi64(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N, policy_t P>
	inline void vector_ne(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		vector_eq(out, l, r);
		mask_neg(out, out);
	}
#endif
#endif
}	 // namespace sek::detail
#endif