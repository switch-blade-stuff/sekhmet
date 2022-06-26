/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "common.hpp"

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
	template<integral_of_size<4> T, std::size_t N>
	inline void vector_and(simd_vector<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires simd_enabled<simd_vector<T, N>>
	{
		out.simd = _mm_and_si128(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void vector_xor(simd_vector<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires simd_enabled<simd_vector<T, N>>
	{
		out.simd = _mm_xor_si128(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void vector_or(simd_vector<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires simd_enabled<simd_vector<T, N>>
	{
		out.simd = _mm_or_si128(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void vector_inv(simd_vector<T, N> &out, const simd_vector<T, N> &v) noexcept
		requires simd_enabled<simd_vector<T, N>>
	{
		out.simd = _mm_xor_si128(v.simd, _mm_set1_epi8(static_cast<std::int8_t>(0xff)));
	}

	template<integral_of_size<8> T>
	inline void vector_and(simd_vector<T, 2> &out, const simd_vector<T, 2> &l, const simd_vector<T, 2> &r) noexcept
	{
		out.simd = _mm_and_si128(l.simd, r.simd);
	}
	template<integral_of_size<8> T>
	inline void vector_xor(simd_vector<T, 2> &out, const simd_vector<T, 2> &l, const simd_vector<T, 2> &r) noexcept
	{
		out.simd = _mm_xor_si128(l.simd, r.simd);
	}
	template<integral_of_size<8> T>
	inline void vector_or(simd_vector<T, 2> &out, const simd_vector<T, 2> &l, const simd_vector<T, 2> &r) noexcept
	{
		out.simd = _mm_or_si128(l.simd, r.simd);
	}
	template<integral_of_size<8> T>
	inline void vector_inv(simd_vector<T, 2> &out, const simd_vector<T, 2> &v) noexcept
	{
		out.simd = _mm_xor_si128(v.simd, _mm_set1_epi8(static_cast<std::int8_t>(0xff)));
	}

#ifndef SEK_USE_AVX2
	template<integral_of_size<8> T, std::size_t N>
	inline void vector_and(simd_vector<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires simd_enabled<simd_vector<T, N>>
	{
		out.simd[0] = _mm_and_si128(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_and_si128(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N>
	inline void vector_xor(simd_vector<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires simd_enabled<simd_vector<T, N>>
	{
		out.simd[0] = _mm_xor_si128(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_xor_si128(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N>
	inline void vector_or(simd_vector<T, N> &out, const simd_vector<T, N> &l, const simd_vector<T, N> &r) noexcept
		requires simd_enabled<simd_vector<T, N>>
	{
		out.simd[0] = _mm_or_si128(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_or_si128(l.simd[1], r.simd[1]);
	}
	template<integral_of_size<8> T, std::size_t N>
	inline void vector_inv(simd_vector<T, N> &out, const simd_vector<T, N> &v) noexcept
		requires simd_enabled<simd_vector<T, N>>
	{
		const auto m = _mm_set1_epi8(static_cast<std::int8_t>(0xff));
		out.simd[0] = _mm_xor_si128(v.simd[0], m);
		out.simd[1] = _mm_xor_si128(v.simd[1], m);
	}
#endif
}	 // namespace sek::math::detail
#endif