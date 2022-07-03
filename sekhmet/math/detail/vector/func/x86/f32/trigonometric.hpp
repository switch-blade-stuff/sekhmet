/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../common.hpp"

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
	SEK_API __m128 x86_sin_ps(__m128 v) noexcept;
	SEK_API __m128 x86_cos_ps(__m128 v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_sin(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = x86_sin_ps(v.simd);
		else
			x86_unpack_ps(out, x86_sin_ps(x86_pack_ps(v)));
	}
	template<std::size_t N, policy_t P>
	inline void vector_cos(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = x86_cos_ps(v.simd);
		else
			x86_unpack_ps(out, x86_cos_ps(x86_pack_ps(v)));
	}

	SEK_API __m128 x86_tancot_ps(__m128 v, __m128i m) noexcept;
	SEK_API __m128 x86_tan_ps(__m128 v) noexcept;
	SEK_API __m128 x86_cot_ps(__m128 v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_tan(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = x86_tan_ps(v.simd);
		else
			x86_unpack_ps(out, x86_tan_ps(x86_pack_ps(v)));
	}
	template<std::size_t N, policy_t P>
	inline void vector_cot(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = x86_cot_ps(v.simd);
		else
			x86_unpack_ps(out, x86_cot_ps(x86_pack_ps(v)));
	}

	SEK_API __m128 x86_sinh_ps(__m128 v) noexcept;
	SEK_API __m128 x86_cosh_ps(__m128 v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_sinh(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = x86_sinh_ps(v.simd);
		else
			x86_unpack_ps(out, x86_sinh_ps(x86_pack_ps(v)));
	}
	template<std::size_t N, policy_t P>
	inline void vector_cosh(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = x86_cosh_ps(v.simd);
		else
			x86_unpack_ps(out, x86_cosh_ps(x86_pack_ps(v)));
	}

	SEK_API __m128 x86_tanh_ps(__m128 v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_tanh(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = x86_tanh_ps(v.simd);
		else
			x86_unpack_ps(out, x86_tanh_ps(x86_pack_ps(v)));
	}

	SEK_API __m128 x86_asin_ps(__m128 v) noexcept;
	SEK_API __m128 x86_acos_ps(__m128 v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_asin(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = x86_asin_ps(v.simd);
		else
			x86_unpack_ps(out, x86_asin_ps(x86_pack_ps(v)));
	}
	template<std::size_t N, policy_t P>
	inline void vector_acos(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = x86_acos_ps(v.simd);
		else
			x86_unpack_ps(out, x86_acos_ps(x86_pack_ps(v)));
	}

	SEK_API __m128 x86_atan_ps(__m128 v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_atan(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = x86_atan_ps(v.simd);
		else
			x86_unpack_ps(out, x86_atan_ps(x86_pack_ps(v)));
	}

	SEK_API __m128 x86_asinh_ps(__m128 v) noexcept;
	SEK_API __m128 x86_acosh_ps(__m128 v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_asinh(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = x86_asinh_ps(v.simd);
		else
			x86_unpack_ps(out, x86_asinh_ps(x86_pack_ps(v)));
	}
	template<std::size_t N, policy_t P>
	inline void vector_acosh(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = x86_acosh_ps(v.simd);
		else
			x86_unpack_ps(out, x86_acosh_ps(x86_pack_ps(v)));
	}

	SEK_API __m128 x86_atanh_ps(__m128 v) noexcept;

	template<std::size_t N, policy_t P>
	inline void vector_atanh(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = x86_atanh_ps(v.simd);
		else
			x86_unpack_ps(out, x86_atanh_ps(x86_pack_ps(v)));
	}

	SEK_API __m128d x86_sin_pd(__m128d v) noexcept;
	SEK_API __m128d x86_cos_pd(__m128d v) noexcept;
}	 // namespace sek::math::detail
#endif