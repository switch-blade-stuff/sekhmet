/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "common.hpp"

#ifdef SEK_USE_SSE
namespace sek::math::detail
{
	template<std::size_t N, storage_policy P>
	inline void vector_is_nan(mask_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires simd_enabled<mask_data<float, N, P>> && simd_enabled<vector_data<float, N, P>>
	{
		out.simd = _mm_cmpunord_ps(v.simd, v.simd);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_is_inf(mask_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires simd_enabled<mask_data<float, N, P>> && simd_enabled<vector_data<float, N, P>>
	{
		const auto mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));
		const auto inf = _mm_set1_ps(std::bit_cast<float>(0x7f80'0000));
		out.simd = _mm_cmpeq_ps(_mm_and_ps(v.simd, mask), inf);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_is_fin(mask_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires simd_enabled<mask_data<float, N, P>> && simd_enabled<vector_data<float, N, P>>
	{
		const auto mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));
		const auto inf = _mm_set1_ps(std::bit_cast<float>(0x7f80'0000));
		out.simd = _mm_cmplt_ps(_mm_and_ps(v.simd, mask), inf);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_is_neg(mask_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires simd_enabled<mask_data<float, N, P>> && simd_enabled<vector_data<float, N, P>>
	{
		const auto mask = _mm_set1_ps(std::bit_cast<float>(0x8000'0000));
		out.simd = _mm_and_ps(v.simd, mask);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_is_norm(mask_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
		requires simd_enabled<mask_data<float, N, P>> && simd_enabled<vector_data<float, N, P>>
	{
		const auto mask = _mm_set1_ps(std::bit_cast<float>(0x7f80'0000));
		const auto a = _mm_and_ps(v.simd, mask);
		const auto b = _mm_cmpneq_ps(a, _mm_setzero_ps());
		const auto c = _mm_cmplt_ps(a, mask);
		out.simd = _mm_and_ps(b, c);
	}

#ifdef SEK_USE_SSE2
	template<storage_policy P>
	inline void vector_is_nan(mask_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires simd_enabled<mask_data<double, 2, P>> && simd_enabled<vector_data<double, 2, P>>
	{
		out.simd = _mm_cmpunord_pd(v.simd, v.simd);
	}
	template<storage_policy P>
	inline void vector_is_inf(mask_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires simd_enabled<mask_data<double, 2, P>> && simd_enabled<vector_data<double, 2, P>>
	{
		const auto mask = _mm_set1_pd(std::bit_cast<double>(0x7fff'ffff'ffff'ffff));
		const auto inf = _mm_set1_pd(std::bit_cast<double>(0x7ff0'0000'0000'0000));
		out.simd = _mm_cmpeq_pd(_mm_and_pd(v.simd, mask), inf);
	}
	template<storage_policy P>
	inline void vector_is_fin(mask_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires simd_enabled<mask_data<double, 2, P>> && simd_enabled<vector_data<double, 2, P>>
	{
		const auto mask = _mm_set1_pd(std::bit_cast<double>(0x7fff'ffff'ffff'ffff));
		const auto inf = _mm_set1_pd(std::bit_cast<double>(0x7ff0'0000'0000'0000));
		out.simd = _mm_cmplt_pd(_mm_and_pd(v.simd, mask), inf);
	}
	template<storage_policy P>
	inline void vector_is_neg(mask_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires simd_enabled<mask_data<double, 2, P>> && simd_enabled<vector_data<double, 2, P>>
	{
		const auto mask = _mm_set1_pd(std::bit_cast<double>(0x8000'0000'0000'0000));
		out.simd = _mm_and_pd(v.simd, mask);
	}
	template<storage_policy P>
	inline void vector_is_norm(mask_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires simd_enabled<mask_data<double, 2, P>> && simd_enabled<vector_data<double, 2, P>>
	{
		const auto mask = _mm_set1_pd(std::bit_cast<double>(0x7ff0'0000'0000'0000));
		const auto a = _mm_and_pd(v.simd, mask);
		const auto b = _mm_cmpneq_pd(a, _mm_setzero_pd());
		const auto c = _mm_cmplt_pd(a, mask);
		out.simd = _mm_and_pd(b, c);
	}

#ifndef SEK_USE_AVX
	template<std::size_t N, storage_policy P>
	inline void vector_is_nan(mask_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires simd_enabled<mask_data<double, N, P>> && simd_enabled<vector_data<double, N, P>>
	{
		out.simd[0] = _mm_cmpunord_pd(v.simd[0], v.simd[0]);
		out.simd[1] = _mm_cmpunord_pd(v.simd[1], v.simd[1]);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_is_inf(mask_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires simd_enabled<mask_data<double, N, P>> && simd_enabled<vector_data<double, N, P>>
	{
		const auto mask = _mm_set1_pd(std::bit_cast<double>(0x7fff'ffff'ffff'ffff));
		const auto inf = _mm_set1_pd(std::bit_cast<double>(0x7ff0'0000'0000'0000));
		out.simd[0] = _mm_cmpeq_pd(_mm_and_pd(v.simd[0], mask), inf);
		out.simd[1] = _mm_cmpeq_pd(_mm_and_pd(v.simd[1], mask), inf);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_is_fin(mask_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires simd_enabled<mask_data<double, N, P>> && simd_enabled<vector_data<double, N, P>>
	{
		const auto mask = _mm_set1_pd(std::bit_cast<double>(0x7fff'ffff'ffff'ffff));
		const auto inf = _mm_set1_pd(std::bit_cast<double>(0x7ff0'0000'0000'0000));
		out.simd[0] = _mm_cmplt_pd(_mm_and_pd(v.simd[0], mask), inf);
		out.simd[1] = _mm_cmplt_pd(_mm_and_pd(v.simd[1], mask), inf);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_is_neg(mask_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires simd_enabled<mask_data<double, N, P>> && simd_enabled<vector_data<double, N, P>>
	{
		const auto mask = _mm_set1_pd(std::bit_cast<double>(0x8000'0000'0000'0000));
		out.simd[0] = _mm_and_pd(v.simd[0], mask);
		out.simd[1] = _mm_and_pd(v.simd[1], mask);
	}
	template<std::size_t N, storage_policy P>
	inline void vector_is_norm(mask_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires simd_enabled<mask_data<double, N, P>> && simd_enabled<vector_data<double, N, P>>
	{
		const auto mask = _mm_set1_pd(std::bit_cast<double>(0x7ff0'0000'0000'0000));
		const auto zero = _mm_setzero_pd();

		const __m128d a[2] = {_mm_and_pd(v.simd[0], mask), _mm_and_pd(v.simd[1], mask)};
		const __m128d b[2] = {_mm_cmpneq_pd(a[0], zero), _mm_cmpneq_pd(a[1], zero)};
		const __m128d c[2] = {_mm_cmplt_pd(a[0], mask), _mm_cmplt_pd(a[1], mask)};
		out.simd[0] = _mm_and_pd(b[0], c[0]);
		out.simd[1] = _mm_and_pd(b[1], c[1]);
	}
#endif
#endif
}	 // namespace sek::math::detail
#endif