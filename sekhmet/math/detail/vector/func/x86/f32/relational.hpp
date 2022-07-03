/*
 * Created by switchblade on 03/07/22
 */

#pragma once

#include "../common.hpp"

#ifdef SEK_USE_SSE
namespace sek::math::detail
{
	template<std::size_t N, policy_t P>
	inline void mask_and(mask_data<float, N, P> &out, const mask_data<float, N, P> &l, const mask_data<float, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_and_ps(l.simd, r.simd);
	}
	template<std::size_t N, policy_t P>
	inline void mask_or(mask_data<float, N, P> &out, const mask_data<float, N, P> &l, const mask_data<float, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_or_ps(l.simd, r.simd);
	}
	template<std::size_t N, policy_t P>
	inline void mask_neg(mask_data<float, N, P> &out, const mask_data<float, N, P> &m) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		constexpr auto mask = std::bit_cast<float>(0xffff'ffff);
		out.simd = _mm_xor_ps(m.simd, _mm_set1_ps(mask));
	}
	template<std::size_t N, policy_t P>
	inline void mask_ne(mask_data<float, N, P> &out, const mask_data<float, N, P> &l, const mask_data<float, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_xor_ps(l.simd, r.simd);
	}

	template<std::size_t N, policy_t P>
	inline void vector_max(vector_data<float, N, P> &out, const vector_data<float, N, P> &l, const vector_data<float, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_max_ps(l.simd, r.simd);
	}
	template<std::size_t N, policy_t P>
	inline void vector_min(vector_data<float, N, P> &out, const vector_data<float, N, P> &l, const vector_data<float, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_min_ps(l.simd, r.simd);
	}
	template<std::size_t N, policy_t P>
	inline void vector_eq(mask_data<float, N, P> &out, const vector_data<float, N, P> &l, const vector_data<float, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_cmpeq_ps(l.simd, r.simd);
	}
	template<std::size_t N, policy_t P>
	inline void vector_ne(mask_data<float, N, P> &out, const vector_data<float, N, P> &l, const vector_data<float, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_cmpne_ps(l.simd, r.simd);
	}
	template<std::size_t N, policy_t P>
	inline void vector_lt(mask_data<float, N, P> &out, const vector_data<float, N, P> &l, const vector_data<float, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_cmplt_ps(l.simd, r.simd);
	}
	template<std::size_t N, policy_t P>
	inline void vector_le(mask_data<float, N, P> &out, const vector_data<float, N, P> &l, const vector_data<float, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_cmple_ps(l.simd, r.simd);
	}
	template<std::size_t N, policy_t P>
	inline void vector_gt(mask_data<float, N, P> &out, const vector_data<float, N, P> &l, const vector_data<float, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_cmpgt_ps(l.simd, r.simd);
	}
	template<std::size_t N, policy_t P>
	inline void vector_ge(mask_data<float, N, P> &out, const vector_data<float, N, P> &l, const vector_data<float, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_cmpge_ps(l.simd, r.simd);
	}

#ifdef SEK_USE_SSE2
	template<std::size_t N, policy_t P>
	inline void mask_eq(mask_data<float, N, P> &out, const mask_data<float, N, P> &l, const mask_data<float, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_castps_si128(l.simd), _mm_castps_si128(r.simd)));
	}
#else
	template<std::size_t N, policy_t P>
	inline void mask_eq(mask_data<float, N, P> &out, const mask_data<float, N, P> &l, const mask_data<float, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		mask_ne(out, l, r);
		mask_neg(out, out);
	}
#endif
}	 // namespace sek::math::detail
#endif