/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "common.hpp"

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline void vector_and(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_and_si128(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline void vector_xor(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_xor_si128(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline void vector_or(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_or_si128(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline void vector_inv(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_xor_si128(v.simd, _mm_set1_epi8(static_cast<std::int8_t>(0xff)));
	}

	template<integral_of_size<8> T, policy_t P>
	inline void vector_and(vector_data<T, 2, P> &out, const vector_data<T, 2, P> &l, const vector_data<T, 2, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_and_si128(l.simd, r.simd);
	}
	template<integral_of_size<8> T, policy_t P>
	inline void vector_xor(vector_data<T, 2, P> &out, const vector_data<T, 2, P> &l, const vector_data<T, 2, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_xor_si128(l.simd, r.simd);
	}
	template<integral_of_size<8> T, policy_t P>
	inline void vector_or(vector_data<T, 2, P> &out, const vector_data<T, 2, P> &l, const vector_data<T, 2, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_or_si128(l.simd, r.simd);
	}
	template<integral_of_size<8> T, policy_t P>
	inline void vector_inv(vector_data<T, 2, P> &out, const vector_data<T, 2, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_xor_si128(v.simd, _mm_set1_epi8(static_cast<std::int8_t>(0xff)));
	}

#ifndef SEK_USE_AVX2
	template<integral_of_size<8> T, std::size_t N, policy_t P>
	inline void vector_and(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd[0] = _mm_and_si128(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_and_si128(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N, policy_t P>
	inline void vector_xor(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd[0] = _mm_xor_si128(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_xor_si128(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N, policy_t P>
	inline void vector_or(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd[0] = _mm_or_si128(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_or_si128(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N, policy_t P>
	inline void vector_inv(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		const auto m = _mm_set1_epi8(static_cast<std::int8_t>(0xff));
		out.simd[0] = _mm_xor_si128(v.simd[0], m);
		out.simd[1] = _mm_xor_si128(v.simd[1], m);
	}
#endif
}	 // namespace sek::math::detail
#endif