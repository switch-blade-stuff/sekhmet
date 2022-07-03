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
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = x86_cos_pd(v.simd);
		else
			x86_unpack_pd(out, x86_cos_pd(x86_pack_pd(v)));
	}

#ifndef SEK_USE_AVX
	template<std::size_t N, policy_t P>
	inline void vector_sin(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
		{
			out.simd[0] = x86_sin_pd(v.simd[0]);
			out.simd[1] = x86_sin_pd(v.simd[1]);
		}
		else
		{
			vector_data<double, 2, policy_t::FAST_SIMD> tmp = {v[0], v[1]};
			vector_sin(tmp, tmp);
			out[0] = tmp[0];
			out[1] = tmp[1];

			if constexpr (N == 4)
			{
				tmp = {v[3], v[4]};
				vector_sin(tmp, tmp);
				out[3] = tmp[0];
				out[4] = tmp[1];
			}
			else
			{
				tmp = {v[3], double{}};
				vector_sin(tmp, tmp);
				out[3] = tmp[0];
			}
		}
	}
	template<std::size_t N, policy_t P>
	inline void vector_cos(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
		{
			out.simd[0] = x86_cos_pd(v.simd[0]);
			out.simd[1] = x86_cos_pd(v.simd[1]);
		}
		else
		{
			vector_data<double, 2, policy_t::FAST_SIMD> tmp = {v[0], v[1]};
			vector_cos(tmp, tmp);
			out[0] = tmp[0];
			out[1] = tmp[1];

			if constexpr (N == 4)
			{
				tmp = {v[3], v[4]};
				vector_cos(tmp, tmp);
				out[3] = tmp[0];
				out[4] = tmp[1];
			}
			else
			{
				tmp = {v[3], double{}};
				vector_cos(tmp, tmp);
				out[3] = tmp[0];
			}
		}
	}
#endif
}	 // namespace sek::math::detail
#endif