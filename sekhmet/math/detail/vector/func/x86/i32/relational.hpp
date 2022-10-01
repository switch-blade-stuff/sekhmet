/*
 * Created by switchblade on 26/06/22
 */

#pragma once

#include "../common.hpp"

#ifdef SEK_USE_SSE2
namespace sek::detail
{
	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline void mask_and(mask_data<T, N, P> &out, const mask_data<T, N, P> &l, const mask_data<T, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_and_si128(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline void mask_or(mask_data<T, N, P> &out, const mask_data<T, N, P> &l, const mask_data<T, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_or_si128(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline void mask_neg(mask_data<T, N, P> &out, const mask_data<T, N, P> &m) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_xor_si128(m.simd, _mm_set1_epi32(-1));
	}
	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline void mask_eq(mask_data<T, N, P> &out, const mask_data<T, N, P> &l, const mask_data<T, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_cmpeq_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline void mask_ne(mask_data<T, N, P> &out, const mask_data<T, N, P> &l, const mask_data<T, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_xor_si128(l.simd, r.simd);
	}

	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline void vector_max(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_max_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline void vector_min(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_min_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline void vector_eq(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_cmpeq_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline void vector_ne(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		vector_eq(out, l, r);
		mask_neg(out, out);
	}
	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline void vector_lt(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_cmplt_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline void vector_gt(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_cmpgt_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline void vector_le(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		vector_gt(out, l, r);
		mask_neg(out, out);
	}
	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline void vector_ge(mask_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		vector_lt(out, l, r);
		mask_neg(out, out);
	}
}	 // namespace sek::detail
#endif