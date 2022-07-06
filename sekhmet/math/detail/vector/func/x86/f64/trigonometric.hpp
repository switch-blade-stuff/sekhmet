/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../common.hpp"

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
	SEK_API __m128d x86_sin_pd(__m128d v) noexcept;
	SEK_API __m128d x86_cos_pd(__m128d v) noexcept;

	template<policy_t P>
	inline void vector_sin(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = x86_sin_pd(v.simd);
		else
			x86_unpack_pd(out, x86_sin_pd(x86_pack_pd(v)));
	}
	template<policy_t P>
	inline void vector_cos(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = x86_cos_pd(v.simd);
		else
			x86_unpack_pd(out, x86_cos_pd(x86_pack_pd(v)));
	}

	SEK_API __m128d x86_tancot_pd(__m128d v, __m128i m) noexcept;
	SEK_API __m128d x86_tan_pd(__m128d v) noexcept;
	SEK_API __m128d x86_cot_pd(__m128d v) noexcept;

	template<policy_t P>
	inline void vector_tan(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = x86_tan_pd(v.simd);
		else
			x86_unpack_pd(out, x86_tan_pd(x86_pack_pd(v)));
	}
	template<policy_t P>
	inline void vector_cot(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = x86_cot_pd(v.simd);
		else
			x86_unpack_pd(out, x86_cot_pd(x86_pack_pd(v)));
	}

#ifndef SEK_USE_AVX
	template<std::size_t N, policy_t P>
	inline void vector_sin(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, [](auto v) { return x86_sin_pd(v); });
	}
	template<std::size_t N, policy_t P>
	inline void vector_cos(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, [](auto v) { return x86_cos_pd(v); });
	}
	template<std::size_t N, policy_t P>
	inline void vector_tan(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, [](auto v) { return x86_tan_pd(v); });
	}
	template<std::size_t N, policy_t P>
	inline void vector_cot(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, [](auto v) { return x86_cot_pd(v); });
	}
#endif
}	 // namespace sek::math::detail
#endif