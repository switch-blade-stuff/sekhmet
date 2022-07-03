/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "common.hpp"
#include "util_f32.hpp"
#include "util_f64.hpp"

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
	template<integral_of_size<4> T, std::size_t N, std::size_t M, policy_t P, std::size_t... Is>
	inline void mask_shuffle(mask_data<T, N, P> &out, const mask_data<T, N, P> &m, std::index_sequence<Is...> s) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		constexpr auto mask = x86_128_shuffle4_mask(s);
		out.simd = _mm_shuffle_epi32(m.simd, mask);
	}

	template<integral_of_size<4> T, std::size_t N, std::size_t M, policy_t P, std::size_t... Is>
	inline void vector_shuffle(vector_data<T, N, P> &out, const vector_data<T, M, P> &v, std::index_sequence<Is...> s) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		constexpr auto mask = x86_128_shuffle4_mask(s);
		out.simd = _mm_shuffle_epi32(v.simd, mask);
	}
	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline void vector_interleave(vector_data<T, N, P> &out,
								  const vector_data<T, N, P> &l,
								  const vector_data<T, N, P> &r,
								  const mask_data<T, N, P> &m) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = x86_blendv_epi8(r.simd, l.simd, m.simd);
	}
}	 // namespace sek::math::detail
#endif