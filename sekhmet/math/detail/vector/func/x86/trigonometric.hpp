/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "common.hpp"

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
	SEK_API __m128 x86_sin_ps(__m128 v) noexcept;
	SEK_API __m128 x86_cos_ps(__m128 v) noexcept;

	template<std::size_t N, storage_policy P>
	inline void vector_sin(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
	{
		x86_unpack_ps(out, x86_sin_ps(x86_pack_ps(v)));
	}
	template<std::size_t N, storage_policy P>
	inline void vector_cos(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
	{
		x86_unpack_ps(out, x86_cos_ps(x86_pack_ps(v)));
	}
	template<std::size_t N>
	inline void vector_sin(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = x86_sin_ps(v.simd);
	}
	template<std::size_t N>
	inline void vector_cos(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = x86_cos_ps(v.simd);
	}

	SEK_API __m128 x86_tancot_ps(__m128 v, __m128i m) noexcept;
	SEK_API __m128 x86_tan_ps(__m128 v) noexcept;
	SEK_API __m128 x86_cot_ps(__m128 v) noexcept;

	template<std::size_t N, storage_policy P>
	inline void vector_tan(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
	{
		x86_unpack_ps(out, x86_tan_ps(x86_pack_ps(v)));
	}
	template<std::size_t N, storage_policy P>
	inline void vector_cot(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
	{
		x86_unpack_ps(out, x86_cot_ps(x86_pack_ps(v)));
	}
	template<std::size_t N>
	inline void vector_tan(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = x86_tan_ps(v.simd);
	}
	template<std::size_t N>
	inline void vector_cot(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = x86_cot_ps(v.simd);
	}

	SEK_API __m128 x86_sinh_ps(__m128 v) noexcept;
	SEK_API __m128 x86_cosh_ps(__m128 v) noexcept;

	template<std::size_t N, storage_policy P>
	inline void vector_sinh(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
	{
		x86_unpack_ps(out, x86_sinh_ps(x86_pack_ps(v)));
	}
	template<std::size_t N, storage_policy P>
	inline void vector_cosh(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
	{
		x86_unpack_ps(out, x86_cosh_ps(x86_pack_ps(v)));
	}
	template<std::size_t N>
	inline void vector_sinh(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = x86_sinh_ps(v.simd);
	}
	template<std::size_t N>
	inline void vector_cosh(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = x86_cosh_ps(v.simd);
	}

	SEK_API __m128 x86_tanh_ps(__m128 v) noexcept;

	template<std::size_t N, storage_policy P>
	inline void vector_tanh(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
	{
		x86_unpack_ps(out, x86_tanh_ps(x86_pack_ps(v)));
	}
	template<std::size_t N>
	inline void vector_tanh(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = x86_tanh_ps(v.simd);
	}

	SEK_API __m128 x86_asin_ps(__m128 v) noexcept;
	SEK_API __m128 x86_acos_ps(__m128 v) noexcept;

	template<std::size_t N, storage_policy P>
	inline void vector_asin(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
	{
		x86_unpack_ps(out, x86_asin_ps(x86_pack_ps(v)));
	}
	template<std::size_t N, storage_policy P>
	inline void vector_acos(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
	{
		x86_unpack_ps(out, x86_acos_ps(x86_pack_ps(v)));
	}
	template<std::size_t N>
	inline void vector_asin(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = x86_asin_ps(v.simd);
	}
	template<std::size_t N>
	inline void vector_acos(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = x86_acos_ps(v.simd);
	}

	SEK_API __m128 x86_atan_ps(__m128 v) noexcept;

	template<std::size_t N, storage_policy P>
	inline void vector_atan(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
	{
		x86_unpack_ps(out, x86_atan_ps(x86_pack_ps(v)));
	}
	template<std::size_t N>
	inline void vector_atan(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = x86_atan_ps(v.simd);
	}

	SEK_API __m128 x86_asinh_ps(__m128 v) noexcept;
	SEK_API __m128 x86_acosh_ps(__m128 v) noexcept;

	template<std::size_t N, storage_policy P>
	inline void vector_asinh(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
	{
		x86_unpack_ps(out, x86_asinh_ps(x86_pack_ps(v)));
	}
	template<std::size_t N, storage_policy P>
	inline void vector_acosh(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
	{
		x86_unpack_ps(out, x86_acosh_ps(x86_pack_ps(v)));
	}
	template<std::size_t N>
	inline void vector_asinh(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = x86_asinh_ps(v.simd);
	}
	template<std::size_t N>
	inline void vector_acosh(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = x86_acosh_ps(v.simd);
	}

	SEK_API __m128 x86_atanh_ps(__m128 v) noexcept;

	template<std::size_t N, storage_policy P>
	inline void vector_atanh(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
	{
		x86_unpack_ps(out, x86_atanh_ps(x86_pack_ps(v)));
	}
	template<std::size_t N>
	inline void vector_atanh(simd_vector<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_vector<float, N>>
	{
		out.simd = x86_atanh_ps(v.simd);
	}

	SEK_API __m128d x86_sin_pd(__m128d v) noexcept;
	SEK_API __m128d x86_cos_pd(__m128d v) noexcept;

	template<storage_policy P>
	inline void vector_sin(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v)
	{
		x86_unpack_pd(out, x86_sin_pd(x86_pack_pd(v)));
	}
	template<storage_policy P>
	inline void vector_cos(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v)
	{
		x86_unpack_pd(out, x86_cos_pd(x86_pack_pd(v)));
	}
	inline void vector_sin(simd_vector<double, 2> &out, const simd_vector<double, 2> &v) noexcept
	{
		out.simd = x86_sin_pd(v.simd);
	}
	inline void vector_cos(simd_vector<double, 2> &out, const simd_vector<double, 2> &v) noexcept
	{
		out.simd = x86_cos_pd(v.simd);
	}

#ifndef SEK_USE_AVX
	template<std::size_t N, storage_policy P>
	inline void vector_sin(vector_data<double, N, P> &out, const vector_data<double, N, P> &v)
	{
		vector_data<double, 2, P> tmp = {v[0], v[1]};
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
	template<std::size_t N, storage_policy P>
	inline void vector_cos(vector_data<double, N, P> &out, const vector_data<double, N, P> &v)
	{
		vector_data<double, 2, P> tmp = {v[0], v[1]};
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

	template<std::size_t N>
	inline void vector_sin(simd_vector<double, N> &out, const simd_vector<double, N> &v) noexcept
		requires simd_enabled<simd_vector<double, N>>
	{
		out.simd[0] = x86_sin_pd(v.simd[0]);
		out.simd[1] = x86_sin_pd(v.simd[1]);
	}
	template<std::size_t N>
	inline void vector_cos(simd_vector<double, N> &out, const simd_vector<double, N> &v) noexcept
		requires simd_enabled<simd_vector<double, N>>
	{
		out.simd[0] = x86_cos_pd(v.simd[0]);
		out.simd[1] = x86_cos_pd(v.simd[1]);
	}
#endif
}	 // namespace sek::math::detail
#endif