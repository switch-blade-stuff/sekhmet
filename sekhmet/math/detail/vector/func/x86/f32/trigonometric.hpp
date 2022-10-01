/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../common.hpp"

#ifdef SEK_USE_SSE2
namespace sek::detail
{
	SEK_API __m128 x86_sin_ps(__m128 v) noexcept;
	SEK_API __m128 x86_cos_ps(__m128 v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_sin(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_sin_ps);
	}
	template<std::size_t N, policy_t P>
	inline void vector_cos(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_cos_ps);
	}

	SEK_API __m128 x86_tan_ps(__m128 v) noexcept;
	SEK_API __m128 x86_cot_ps(__m128 v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_tan(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_tan_ps);
	}
	template<std::size_t N, policy_t P>
	inline void vector_cot(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_cot_ps);
	}

	SEK_API __m128 x86_sinh_ps(__m128 v) noexcept;
	SEK_API __m128 x86_cosh_ps(__m128 v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_sinh(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_sinh_ps);
	}
	template<std::size_t N, policy_t P>
	inline void vector_cosh(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_cosh_ps);
	}

	SEK_API __m128 x86_tanh_ps(__m128 v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_tanh(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_tanh_ps);
	}

	SEK_API __m128 x86_asin_ps(__m128 v) noexcept;
	SEK_API __m128 x86_acos_ps(__m128 v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_asin(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_asin_ps);
	}
	template<std::size_t N, policy_t P>
	inline void vector_acos(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_acos_ps);
	}

	SEK_API __m128 x86_atan_ps(__m128 v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_atan(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_atan_ps);
	}

	SEK_API __m128 x86_asinh_ps(__m128 v) noexcept;
	SEK_API __m128 x86_acosh_ps(__m128 v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_asinh(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_asinh_ps);
	}
	template<std::size_t N, policy_t P>
	inline void vector_acosh(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		x86_vector_apply(out, v, x86_acosh_ps);
	}

	SEK_API __m128 x86_atanh_ps(__m128 v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_atanh(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		x86_vector_apply(out, v, x86_atanh_ps);
	}
}	 // namespace sek::detail
#endif