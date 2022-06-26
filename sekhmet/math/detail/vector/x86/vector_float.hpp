/*
 * Created by switchblade on 2022-01-31
 */

#pragma once

#include "common.hpp"
#include "mask_float.hpp"

#ifdef SEK_USE_SSE
namespace sek::math::detail
{
	template<>
	union vector_data<float, 3, storage_policy::OPTIMAL>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(float x, float y, float z) noexcept : values{x, y, z} {}

		template<std::size_t M>
		constexpr explicit vector_data(const float (&data)[M]) noexcept
		{
			std::copy_n(data, min<std::size_t>(3, M), values);
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		float values[3];
		__m128 simd;
	};
	template<>
	union vector_data<float, 4, storage_policy::OPTIMAL>
	{
		constexpr vector_data() noexcept : values{} {}
		constexpr vector_data(float x, float y, float z, float w) noexcept : values{x, y, z, w} {}

		template<std::size_t M>
		constexpr explicit vector_data(const float (&data)[M]) noexcept
		{
			std::copy_n(data, min<std::size_t>(4, M), values);
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		float values[4];
		__m128 simd;
	};

	template<std::size_t N>
	inline void vector_add(simd_vector<float, N> &out, const simd_vector<float, N> &l, const simd_vector<float, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_add_ps(l.simd, r.simd);
	}
	template<std::size_t N>
	inline void vector_sub(simd_vector<float, N> &out, const simd_vector<float, N> &l, const simd_vector<float, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_sub_ps(l.simd, r.simd);
	}
	template<std::size_t N>
	inline void vector_mul(simd_vector<float, N> &out, const simd_vector<float, N> &l, const simd_vector<float, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_mul_ps(l.simd, r.simd);
	}
	template<std::size_t N>
	inline void vector_div(simd_vector<float, N> &out, const simd_vector<float, N> &l, const simd_vector<float, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_div_ps(l.simd, r.simd);
	}

	template<std::size_t N>
	inline void vector_neg(simd_vector<float, N> &out, const simd_vector<float, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_sub_ps(_mm_setzero_ps(), l.simd);
	}
	template<std::size_t N>
	inline void vector_abs(simd_vector<float, N> &out, const simd_vector<float, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		constexpr auto mask = std::bit_cast<float>(0x7fff'ffff);
		out.simd = _mm_and_ps(_mm_set1_ps(mask), l.simd);
	}
	template<std::size_t N>
	inline void vector_max(simd_vector<float, N> &out, const simd_vector<float, N> &l, const simd_vector<float, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_max_ps(l.simd, r.simd);
	}
	template<std::size_t N>
	inline void vector_min(simd_vector<float, N> &out, const simd_vector<float, N> &l, const simd_vector<float, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_min_ps(l.simd, r.simd);
	}

	template<std::size_t N>
	inline void vector_sqrt(simd_vector<float, N> &out, const simd_vector<float, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_sqrt_ps(l.simd);
	}
	template<std::size_t N>
	inline void vector_rsqrt(simd_vector<float, N> &out, const simd_vector<float, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_rsqrt_ps(l.simd);
	}

	template<std::size_t N>
	inline void vector_eq(simd_mask<float, N> &out, const simd_vector<float, N> &l, const simd_vector<float, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_cmpeq_ps(l.simd, r.simd);
	}
	template<std::size_t N>
	inline void vector_ne(simd_mask<float, N> &out, const simd_vector<float, N> &l, const simd_vector<float, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_cmpne_ps(l.simd, r.simd);
	}
	template<std::size_t N>
	inline void vector_lt(simd_mask<float, N> &out, const simd_vector<float, N> &l, const simd_vector<float, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_cmplt_ps(l.simd, r.simd);
	}
	template<std::size_t N>
	inline void vector_le(simd_mask<float, N> &out, const simd_vector<float, N> &l, const simd_vector<float, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_cmple_ps(l.simd, r.simd);
	}
	template<std::size_t N>
	inline void vector_gt(simd_mask<float, N> &out, const simd_vector<float, N> &l, const simd_vector<float, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_cmpgt_ps(l.simd, r.simd);
	}
	template<std::size_t N>
	inline void vector_ge(simd_mask<float, N> &out, const simd_vector<float, N> &l, const simd_vector<float, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_cmpge_ps(l.simd, r.simd);
	}

	template<std::size_t N, std::size_t M, std::size_t... Is>
	inline void vector_shuffle(simd_vector<float, N> &out, const simd_vector<float, M> &l, std::index_sequence<Is...> s) noexcept
		requires(SEK_DETAIL_IS_SIMD(out, l))
	{
		constexpr auto mask = x86_128_shuffle4_mask(s);
		out.simd = _mm_shuffle_ps(l.simd, l.simd, mask);
	}
	template<std::size_t N>
	inline void vector_interleave(simd_vector<float, N> &out,
								  const simd_vector<float, N> &l,
								  const simd_vector<float, N> &r,
								  simd_mask<float, N> &m) noexcept
		requires(SEK_DETAIL_IS_SIMD(out, m))
	{
#ifdef SEK_USE_SSE4_1
		out.simd = _mm_blendv_ps(r.simd, l.simd, m.simd);
#else
		out.simd = _mm_or_ps(_mm_and_ps(m.simd, l.simd), _mm_andnot_ps(m.simd, r.simd));
#endif
	}

	inline void vector_cross(simd_vector<float, 3> &out, const simd_vector<float, 3> &l, const simd_vector<float, 3> &r) noexcept
	{
		const auto a = _mm_shuffle_ps(l.simd, l.simd, _MM_SHUFFLE(3, 0, 2, 1));
		const auto b = _mm_shuffle_ps(r.simd, r.simd, _MM_SHUFFLE(3, 1, 0, 2));
		const auto c = _mm_mul_ps(a, r.simd);
		out.simd = _mm_sub_ps(_mm_mul_ps(a, b), _mm_shuffle_ps(c, c, _MM_SHUFFLE(3, 0, 2, 1)));
	}

#ifdef SEK_USE_SSE4_1
	template<std::size_t N>
	inline void vector_round(simd_vector<float, N> &out, const simd_vector<float, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_round_ps(l.simd, _MM_FROUND_RINT);
	}
	template<std::size_t N>
	inline void vector_floor(simd_vector<float, N> &out, const simd_vector<float, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_floor_ps(l.simd);
	}
	template<std::size_t N>
	inline void vector_ceil(simd_vector<float, N> &out, const simd_vector<float, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_ceil_ps(l.simd);
	}
	template<std::size_t N>
	inline void vector_trunc(simd_vector<float, N> &out, const simd_vector<float, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_round_ps(l.simd, _MM_FROUND_TRUNC);
	}

	inline float vector_dot(const simd_vector<float, 3> &l, const simd_vector<float, 3> &r) noexcept
	{
		return _mm_cvtss_f32(_mm_dp_ps(l.simd, r.simd, 0x71));
	}
	inline void vector_norm(simd_vector<float, 3> &out, const simd_vector<float, 3> &l) noexcept
	{
		out.simd = _mm_div_ps(l.simd, _mm_sqrt_ps(_mm_dp_ps(l.simd, l.simd, 0x7f)));
	}
	inline float vector_dot(const simd_vector<float, 4> &l, const simd_vector<float, 4> &r) noexcept
	{
		return _mm_cvtss_f32(_mm_dp_ps(l.simd, r.simd, 0xf1));
	}
	inline void vector_norm(simd_vector<float, 4> &out, const simd_vector<float, 4> &l) noexcept
	{
		out.simd = _mm_div_ps(l.simd, _mm_sqrt_ps(_mm_dp_ps(l.simd, l.simd, 0xff)));
	}
#else
	template<std::size_t N>
	inline float vector_dot(const simd_vector<float, N> &l, const simd_vector<float, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(l, r))
	{
		const auto a = _mm_mul_ps(r.simd, l.simd);
		const auto b = _mm_shuffle_ps(a, a, _MM_SHUFFLE(2, 3, 0, 1));
		const auto c = _mm_add_ps(a, b);
		return _mm_cvtss_f32(_mm_add_ss(c, _mm_movehl_ps(b, c)));
	}
	template<std::size_t N>
	inline void vector_norm(simd_vector<float, N> &out, const simd_vector<float, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out, r))
	{
		out.simd = _mm_div_ps(l.simd, _mm_sqrt_ps(_mm_set1_ps(vector_dot(l, l))));
	}
#endif

	template<std::size_t N>
	inline void vector_is_nan(simd_mask<float, N> &out, const simd_vector<float, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out, l))
	{
		out.simd = _mm_cmpunord_ps(l.simd, l.simd);
	}
	template<std::size_t N>
	inline void vector_is_inf(simd_mask<float, N> &out, const simd_vector<float, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out, l))
	{
		const auto mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));
		const auto inf = _mm_set1_ps(std::bit_cast<float>(0x7f80'0000));
		out.simd = _mm_cmpeq_ps(_mm_and_ps(l.simd, mask), inf);
	}
	template<std::size_t N>
	inline void vector_is_fin(simd_mask<float, N> &out, const simd_vector<float, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out, l))
	{
		const auto mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));
		const auto inf = _mm_set1_ps(std::bit_cast<float>(0x7f80'0000));
		out.simd = _mm_cmplt_ps(_mm_and_ps(l.simd, mask), inf);
	}
	template<std::size_t N>
	inline void vector_is_neg(simd_mask<float, N> &out, const simd_vector<float, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out, l))
	{
		const auto mask = _mm_set1_ps(std::bit_cast<float>(0x8000'0000));
		out.simd = _mm_and_ps(l.simd, mask);
	}
	template<std::size_t N>
	inline void vector_is_norm(simd_mask<float, N> &out, const simd_vector<float, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out, l))
	{
		const auto mask = _mm_set1_ps(std::bit_cast<float>(0x7f80'0000));
		const auto a = _mm_and_ps(l.simd, mask);
		const auto b = _mm_cmpneq_ps(a, _mm_setzero_ps());
		const auto c = _mm_cmplt_ps(a, mask);
		out.simd = _mm_and_ps(b, c);
	}
}	 // namespace sek::math::detail
#endif
