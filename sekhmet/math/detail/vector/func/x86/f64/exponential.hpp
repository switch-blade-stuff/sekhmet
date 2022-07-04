/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../common.hpp"
#include "../utility.hpp"

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
	SEK_FORCE_INLINE __m128d x86_pow2_pd(__m128i v) noexcept
	{
		const auto adjusted = _mm_add_epi64(v, _mm_set1_epi64x(0x3ff));
		return _mm_castsi128_pd(_mm_slli_epi64(adjusted, 52));
	}
	SEK_FORCE_INLINE __m128d x86_pow2_pd(__m128d v) noexcept { return x86_pow2_pd(x86_cvtpd_epi64(v)); }

	template<policy_t P>
	inline void vector_sqrt(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_sqrt_pd(v.simd);
	}
	template<policy_t P>
	inline void vector_rsqrt(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd = _mm_div_pd(_mm_set1_pd(1.0), _mm_sqrt_pd(v.simd));
	}

	SEK_API __m128d x86_exp_pd(__m128d v) noexcept;

	template<policy_t P>
	inline void vector_exp(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = x86_exp_pd(v.simd);
		else
			x86_unpack_pd(out, x86_exp_pd(x86_pack_pd(v)));
	}
	template<policy_t P>
	inline void vector_expm1(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = _mm_sub_pd(x86_exp_pd(v.simd), _mm_set1_pd(1.0));
		else
			x86_unpack_pd(out, _mm_sub_pd(x86_exp_pd(x86_pack_pd(v)), _mm_set1_pd(1.0)));
	}

	SEK_API __m128d x86_exp2_pd(__m128d v) noexcept;

	template<policy_t P>
	inline void vector_exp2(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = x86_exp2_pd(v.simd);
		else
			x86_unpack_pd(out, x86_exp2_pd(x86_pack_pd(v)));
	}

	SEK_API __m128d x86_log_pd(__m128d v) noexcept;

	template<policy_t P>
	inline void vector_log(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = x86_log_pd(v.simd);
		else
			x86_unpack_pd(out, x86_log_pd(x86_pack_pd(v)));
	}
	template<policy_t P>
	inline void vector_log1p(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		const auto one = _mm_set1_pd(1.0);
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = x86_log_pd(_mm_add_pd(v.simd, one));
		else
			x86_unpack_pd(out, x86_log_pd(_mm_add_pd(x86_pack_pd(v), one)));
	}

	//	SEK_API __m128d x86_log2_pd(__m128d v) noexcept;
	//
	//	template<policy_t P>
	//	inline void vector_log2(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
	//		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	//	{
	//		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
	//			out.simd = x86_log2_pd(v.simd);
	//		else
	//			x86_unpack_pd(out, x86_log2_pd(x86_pack_pd(v)));
	//	}
	//
	//	SEK_API __m128d x86_log10_pd(__m128d v) noexcept;
	//
	//	template<policy_t P>
	//	inline void vector_log10(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
	//		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	//	{
	//		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
	//			out.simd = x86_log10_pd(v.simd);
	//		else
	//			x86_unpack_pd(out, x86_log10_pd(x86_pack_pd(v)));
	//	}

#ifndef SEK_USE_AVX
	template<std::size_t N, policy_t P>
	inline void vector_sqrt(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		out.simd[0] = _mm_sqrt_pd(v.simd[0]);
		out.simd[1] = _mm_sqrt_pd(v.simd[1]);
	}
	template<std::size_t N, policy_t P>
	inline void vector_rsqrt(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		const auto v1 = _mm_set1_pd(1);
		out.simd[0] = _mm_div_pd(v1, _mm_sqrt_pd(v.simd[0]));
		out.simd[1] = _mm_div_pd(v1, _mm_sqrt_pd(v.simd[1]));
	}

	template<std::size_t N, policy_t P>
	inline void vector_exp(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
		{
			out.simd[0] = x86_exp_pd(v.simd[0]);
			out.simd[1] = x86_exp_pd(v.simd[1]);
		}
		else
		{
			vector_data<double, 2, policy_t::FAST_SIMD> tmp;

			tmp = {v[0], v[1]};
			tmp.simd = x86_exp_pd(tmp.simd);
			out[0] = tmp[0];
			out[1] = tmp[1];

			if constexpr (N > 3)
			{
				tmp = {v[2], v[3]};
				tmp.simd = x86_exp_pd(tmp.simd);
				out[2] = tmp[0];
				out[3] = tmp[1];
			}
			else
			{
				tmp = {v[2], double{}};
				tmp.simd = x86_exp_pd(tmp.simd);
				out[2] = tmp[0];
			}
		}
	}
	template<std::size_t N, policy_t P>
	inline void vector_expm1(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		const auto one = _mm_set1_pd(1.0);
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
		{
			out.simd[0] = _mm_sub_pd(x86_exp_pd(v.simd[0]), one);
			out.simd[1] = _mm_sub_pd(x86_exp_pd(v.simd[1]), one);
		}
		else
		{
			vector_data<double, 2, policy_t::FAST_SIMD> tmp;

			tmp = {v[0], v[1]};
			tmp.simd = _mm_sub_pd(x86_exp_pd(tmp.simd), one);
			out[0] = tmp[0];
			out[1] = tmp[1];

			if constexpr (N > 3)
			{
				tmp = {v[2], v[3]};
				tmp.simd = _mm_sub_pd(x86_exp_pd(tmp.simd), one);
				out[2] = tmp[0];
				out[3] = tmp[1];
			}
			else
			{
				tmp = {v[2], double{}};
				tmp.simd = _mm_sub_pd(x86_exp_pd(tmp.simd), one);
				out[2] = tmp[0];
			}
		}
	}
	template<std::size_t N, policy_t P>
	inline void vector_exp2(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
		{
			out.simd[0] = x86_exp2_pd(v.simd[0]);
			out.simd[1] = x86_exp2_pd(v.simd[1]);
		}
		else
		{
			vector_data<double, 2, policy_t::FAST_SIMD> tmp;

			tmp = {v[0], v[1]};
			tmp.simd = x86_exp2_pd(tmp.simd);
			out[0] = tmp[0];
			out[1] = tmp[1];

			if constexpr (N > 3)
			{
				tmp = {v[2], v[3]};
				tmp.simd = x86_exp2_pd(tmp.simd);
				out[2] = tmp[0];
				out[3] = tmp[1];
			}
			else
			{
				tmp = {v[2], double{}};
				tmp.simd = x86_exp2_pd(tmp.simd);
				out[2] = tmp[0];
			}
		}
	}

	template<std::size_t N, policy_t P>
	inline void vector_log(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
		{
			out.simd[0] = x86_log_pd(v.simd[0]);
			out.simd[1] = x86_log_pd(v.simd[1]);
		}
		else
		{
			vector_data<double, 2, policy_t::FAST_SIMD> tmp;

			tmp = {v[0], v[1]};
			tmp.simd = x86_log_pd(tmp.simd);
			out[0] = tmp[0];
			out[1] = tmp[1];

			if constexpr (N > 3)
			{
				tmp = {v[2], v[3]};
				tmp.simd = x86_log_pd(tmp.simd);
				out[2] = tmp[0];
				out[3] = tmp[1];
			}
			else
			{
				tmp = {v[2], double{}};
				tmp.simd = x86_log_pd(tmp.simd);
				out[2] = tmp[0];
			}
		}
	}
	template<std::size_t N, policy_t P>
	inline void vector_log1p(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
	{
		const auto one = _mm_set1_pd(1.0);
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
		{
			out.simd[0] = x86_log_pd(_mm_add_pd(v.simd[0], one));
			out.simd[1] = x86_log_pd(_mm_add_pd(v.simd[1], one));
		}
		else
		{
			vector_data<double, 2, policy_t::FAST_SIMD> tmp;

			tmp = {v[0], v[1]};
			tmp.simd = x86_log_pd(_mm_add_pd(tmp.simd, one));
			out[0] = tmp[0];
			out[1] = tmp[1];

			if constexpr (N > 3)
			{
				tmp = {v[2], v[3]};
				tmp.simd = x86_log_pd(_mm_add_pd(tmp.simd, one));
				out[2] = tmp[0];
				out[3] = tmp[1];
			}
			else
			{
				tmp = {v[2], double{}};
				tmp.simd = x86_log_pd(_mm_add_pd(tmp.simd, one));
				out[2] = tmp[0];
			}
		}
	}
//	template<std::size_t N, policy_t P>
//	inline void vector_log2(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
//		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
//	{
//		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
//		{
//			out.simd[0] = x86_log2_pd(v.simd[0]);
//			out.simd[1] = x86_log2_pd(v.simd[1]);
//		}
//		else
//		{
//			vector_data<double, 2, policy_t::FAST_SIMD> tmp;
//
//			tmp = {v[0], v[1]};
//			tmp.simd = x86_log2_pd(tmp.simd);
//			out[0] = tmp[0];
//			out[1] = tmp[1];
//
//			if constexpr (N > 3)
//			{
//				tmp = {v[2], v[3]};
//				tmp.simd = x86_log2_pd(tmp.simd);
//				out[2] = tmp[0];
//				out[3] = tmp[1];
//			}
//			else
//			{
//				tmp = {v[2], double{}};
//				tmp.simd = x86_log2_pd(tmp.simd);
//				out[2] = tmp[0];
//			}
//		}
//	}
//	template<std::size_t N, policy_t P>
//	inline void vector_log10(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
//		requires check_policy_v<P, policy_t::PRECISION_MASK, policy_t::FAST>
//	{
//		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
//		{
//			out.simd[0] = x86_log10_pd(v.simd[0]);
//			out.simd[1] = x86_log10_pd(v.simd[1]);
//		}
//		else
//		{
//			vector_data<double, 2, policy_t::FAST_SIMD> tmp;
//
//			tmp = {v[0], v[1]};
//			tmp.simd = x86_log10_pd(tmp.simd);
//			out[0] = tmp[0];
//			out[1] = tmp[1];
//
//			if constexpr (N > 3)
//			{
//				tmp = {v[2], v[3]};
//				tmp.simd = x86_log10_pd(tmp.simd);
//				out[2] = tmp[0];
//				out[3] = tmp[1];
//			}
//			else
//			{
//				tmp = {v[2], double{}};
//				tmp.simd = x86_log10_pd(tmp.simd);
//				out[2] = tmp[0];
//			}
//		}
//	}
#endif
}	 // namespace sek::math::detail
#endif