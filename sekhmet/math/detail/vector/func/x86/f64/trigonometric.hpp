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

	template<std::size_t N, policy_t P>
	inline void vector_sin(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_sin_pd);
	}
	template<std::size_t N, policy_t P>
	inline void vector_cos(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_cos_pd);
	}

	SEK_API __m128d x86_tan_pd(__m128d v) noexcept;
	SEK_API __m128d x86_cot_pd(__m128d v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_tan(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_tan_pd);
	}
	template<std::size_t N, policy_t P>
	inline void vector_cot(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_cot_pd);
	}

	SEK_API __m128d x86_sinh_pd(__m128d v) noexcept;
	SEK_API __m128d x86_cosh_pd(__m128d v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_sinh(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_sinh_pd);
	}
	template<std::size_t N, policy_t P>
	inline void vector_cosh(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_cosh_pd);
	}
}	 // namespace sek::math::detail
#endif