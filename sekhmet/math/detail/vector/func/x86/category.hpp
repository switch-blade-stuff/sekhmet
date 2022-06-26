/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "common.hpp"

#ifdef SEK_USE_SSE
namespace sek::math::detail
{
	template<std::size_t N>
	inline void vector_is_nan(simd_mask<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_mask<float, N>> && simd_enabled<simd_vector<float, N>>
	{
		out.simd = _mm_cmpunord_ps(v.simd, v.simd);
	}
	template<std::size_t N>
	inline void vector_is_inf(simd_mask<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_mask<float, N>> && simd_enabled<simd_vector<float, N>>
	{
		const auto mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));
		const auto inf = _mm_set1_ps(std::bit_cast<float>(0x7f80'0000));
		out.simd = _mm_cmpeq_ps(_mm_and_ps(v.simd, mask), inf);
	}
	template<std::size_t N>
	inline void vector_is_fin(simd_mask<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_mask<float, N>> && simd_enabled<simd_vector<float, N>>
	{
		const auto mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));
		const auto inf = _mm_set1_ps(std::bit_cast<float>(0x7f80'0000));
		out.simd = _mm_cmplt_ps(_mm_and_ps(v.simd, mask), inf);
	}
	template<std::size_t N>
	inline void vector_is_neg(simd_mask<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_mask<float, N>> && simd_enabled<simd_vector<float, N>>
	{
		const auto mask = _mm_set1_ps(std::bit_cast<float>(0x8000'0000));
		out.simd = _mm_and_ps(v.simd, mask);
	}
	template<std::size_t N>
	inline void vector_is_norm(simd_mask<float, N> &out, const simd_vector<float, N> &v) noexcept
		requires simd_enabled<simd_mask<float, N>> && simd_enabled<simd_vector<float, N>>
	{
		const auto mask = _mm_set1_ps(std::bit_cast<float>(0x7f80'0000));
		const auto a = _mm_and_ps(v.simd, mask);
		const auto b = _mm_cmpneq_ps(a, _mm_setzero_ps());
		const auto c = _mm_cmplt_ps(a, mask);
		out.simd = _mm_and_ps(b, c);
	}

#ifdef SEK_USE_SSE2
	inline void vector_is_nan(simd_mask<double, 2> &out, const simd_vector<double, 2> &v) noexcept
	{
		out.simd = _mm_cmpunord_pd(v.simd, v.simd);
	}
	inline void vector_is_inf(simd_mask<double, 2> &out, const simd_vector<double, 2> &v) noexcept
	{
		const auto mask = _mm_set1_pd(std::bit_cast<double>(0x7fff'ffff'ffff'ffff));
		const auto inf = _mm_set1_pd(std::bit_cast<double>(0x7ff0'0000'0000'0000));
		out.simd = _mm_cmpeq_pd(_mm_and_pd(v.simd, mask), inf);
	}
	inline void vector_is_fin(simd_mask<double, 2> &out, const simd_vector<double, 2> &v) noexcept
	{
		const auto mask = _mm_set1_pd(std::bit_cast<double>(0x7fff'ffff'ffff'ffff));
		const auto inf = _mm_set1_pd(std::bit_cast<double>(0x7ff0'0000'0000'0000));
		out.simd = _mm_cmplt_pd(_mm_and_pd(v.simd, mask), inf);
	}
	inline void vector_is_neg(simd_mask<double, 2> &out, const simd_vector<double, 2> &v) noexcept
	{
		const auto mask = _mm_set1_pd(std::bit_cast<double>(0x8000'0000'0000'0000));
		out.simd = _mm_and_pd(v.simd, mask);
	}
	inline void vector_is_norm(simd_mask<double, 2> &out, const simd_vector<double, 2> &v) noexcept
	{
		const auto mask = _mm_set1_pd(std::bit_cast<double>(0x7ff0'0000'0000'0000));
		const auto a = _mm_and_pd(v.simd, mask);
		const auto b = _mm_cmpneq_pd(a, _mm_setzero_pd());
		const auto c = _mm_cmplt_pd(a, mask);
		out.simd = _mm_and_pd(b, c);
	}

#ifndef SEK_USE_AVX
	template<std::size_t N>
	inline void vector_is_nan(simd_mask<double, N> &out, const simd_vector<double, N> &v) noexcept
		requires simd_enabled<simd_mask<double, N>> && simd_enabled<simd_vector<double, N>>
	{
		out.simd[0] = _mm_cmpunord_pd(v.simd[0], v.simd[0]);
		out.simd[1] = _mm_cmpunord_pd(v.simd[1], v.simd[1]);
	}
	template<std::size_t N>
	inline void vector_is_inf(simd_mask<double, N> &out, const simd_vector<double, N> &v) noexcept
		requires simd_enabled<simd_mask<double, N>> && simd_enabled<simd_vector<double, N>>
	{
		const auto mask = _mm_set1_pd(std::bit_cast<double>(0x7fff'ffff'ffff'ffff));
		const auto inf = _mm_set1_pd(std::bit_cast<double>(0x7ff0'0000'0000'0000));
		out.simd[0] = _mm_cmpeq_pd(_mm_and_pd(v.simd[0], mask), inf);
		out.simd[1] = _mm_cmpeq_pd(_mm_and_pd(v.simd[1], mask), inf);
	}
	template<std::size_t N>
	inline void vector_is_fin(simd_mask<double, N> &out, const simd_vector<double, N> &v) noexcept
		requires simd_enabled<simd_mask<double, N>> && simd_enabled<simd_vector<double, N>>
	{
		const auto mask = _mm_set1_pd(std::bit_cast<double>(0x7fff'ffff'ffff'ffff));
		const auto inf = _mm_set1_pd(std::bit_cast<double>(0x7ff0'0000'0000'0000));
		out.simd[0] = _mm_cmplt_pd(_mm_and_pd(v.simd[0], mask), inf);
		out.simd[1] = _mm_cmplt_pd(_mm_and_pd(v.simd[1], mask), inf);
	}
	template<std::size_t N>
	inline void vector_is_neg(simd_mask<double, N> &out, const simd_vector<double, N> &v) noexcept
		requires simd_enabled<simd_mask<double, N>> && simd_enabled<simd_vector<double, N>>
	{
		const auto mask = _mm_set1_pd(std::bit_cast<double>(0x8000'0000'0000'0000));
		out.simd[0] = _mm_and_pd(v.simd[0], mask);
		out.simd[1] = _mm_and_pd(v.simd[1], mask);
	}
	template<std::size_t N>
	inline void vector_is_norm(simd_mask<double, N> &out, const simd_vector<double, N> &v) noexcept
		requires simd_enabled<simd_mask<double, N>> && simd_enabled<simd_vector<double, N>>
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