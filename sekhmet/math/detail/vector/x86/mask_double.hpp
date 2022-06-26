/*
 * Created by switchblade on 2022-01-31
 */

#pragma once

#include "common.hpp"

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
	template<>
	union mask_data<double, 2, storage_policy::OPTIMAL>
	{
		using element_t = mask_element<std::uint64_t>;
		using const_element_t = mask_element<const std::uint64_t>;

		constexpr mask_data() noexcept : values{} {}
		constexpr mask_data(bool x, bool y) noexcept
		{
			operator[](0) = x;
			operator[](1) = y;
		}

		template<std::size_t M>
		constexpr mask_data(const bool (&data)[M]) noexcept : values{}
		{
			for (std::size_t i = 0; i < min(2, M); ++i) operator[](i) = data[i];
		}

		constexpr element_t operator[](std::size_t i) noexcept { return {values[i]}; }
		constexpr const_element_t operator[](std::size_t i) const noexcept { return {values[i]}; }

		std::uint64_t values[2];
		__m128d simd;
	};

	template<std::size_t I0, std::size_t I1>
	inline void mask_shuffle(simd_mask<double, 2> &out, const simd_mask<double, 2> &l, std::index_sequence<I0, I1>) noexcept
	{
		constexpr auto mask = x86_128_shuffle2_mask(std::index_sequence<I0, I1>{});
		out.simd = _mm_shuffle_pd(l.simd, l.simd, mask);
	}

	inline void mask_and(simd_mask<double, 2> &out, const simd_mask<double, 2> &l, const simd_mask<double, 2> &r) noexcept
	{
		out.simd = _mm_and_pd(l.simd, r.simd);
	}
	inline void mask_or(simd_mask<double, 2> &out, const simd_mask<double, 2> &l, const simd_mask<double, 2> &r) noexcept
	{
		out.simd = _mm_or_pd(l.simd, r.simd);
	}
	inline void mask_neg(simd_mask<double, 2> &out, const simd_mask<double, 2> &l) noexcept
	{
		constexpr auto mask = std::bit_cast<double>(0xffff'ffff'ffff'ffff);
		out.simd = _mm_xor_pd(l.simd, _mm_set1_pd(mask));
	}

	inline void mask_eq(simd_mask<double, 2> &out, const simd_mask<double, 2> &l, const simd_mask<double, 2> &r) noexcept
	{
		out.simd = _mm_castsi128_pd(_mm_cmpeq_epi32(_mm_castpd_si128(l.simd), _mm_castpd_si128(r.simd)));
	}
	inline void mask_ne(simd_mask<double, 2> &out, const simd_mask<double, 2> &l, const simd_mask<double, 2> &r) noexcept
	{
		out.simd = _mm_xor_pd(l.simd, r.simd);
	}

#ifndef SEK_USE_AVX
	template<>
	union mask_data<double, 3, storage_policy::OPTIMAL>
	{
		using element_t = mask_element<std::uint64_t>;
		using const_element_t = mask_element<const std::uint64_t>;

		constexpr mask_data() noexcept : values{} {}
		constexpr mask_data(bool x, bool y, bool z) noexcept
		{
			operator[](0) = x;
			operator[](1) = y;
			operator[](2) = z;
		}

		template<std::size_t M>
		constexpr mask_data(const bool (&data)[M]) noexcept : values{}
		{
			for (std::size_t i = 0; i < min(3, M); ++i) operator[](i) = data[i];
		}

		constexpr element_t operator[](std::size_t i) noexcept { return {values[i]}; }
		constexpr const_element_t operator[](std::size_t i) const noexcept { return {values[i]}; }

		std::uint64_t values[3];
		__m128d simd[2];
	};
	template<>
	union mask_data<double, 4, storage_policy::OPTIMAL>
	{
		using element_t = mask_element<std::uint64_t>;
		using const_element_t = mask_element<const std::uint64_t>;

		constexpr mask_data() noexcept : values{} {}
		constexpr mask_data(bool x, bool y, bool z, bool w) noexcept
		{
			operator[](0) = x;
			operator[](1) = y;
			operator[](2) = z;
			operator[](3) = w;
		}

		template<std::size_t M>
		constexpr mask_data(const bool (&data)[M]) noexcept : values{}
		{
			for (std::size_t i = 0; i < min(4, M); ++i) operator[](i) = data[i];
		}

		constexpr element_t operator[](std::size_t i) noexcept { return {values[i]}; }
		constexpr const_element_t operator[](std::size_t i) const noexcept { return {values[i]}; }

		std::uint64_t values[4];
		__m128d simd[2];
	};

	template<std::size_t N, std::size_t I0, std::size_t I1, std::size_t... Is>
	inline void mask_shuffle(simd_mask<double, N> &out, const simd_mask<double, 2> &l, std::index_sequence<I0, I1, Is...>) noexcept
		requires(SEK_DETAIL_IS_SIMD(out, l))
	{
		constexpr auto mask0 = x86_128_shuffle2_mask(std::index_sequence<I0, I1>{});
		constexpr auto mask1 = x86_128_shuffle2_mask(std::index_sequence<Is...>{});
		out.simd[0] = _mm_shuffle_pd(l.simd, l.simd, mask0);
		out.simd[1] = _mm_shuffle_pd(l.simd, l.simd, mask1);
	}

	template<std::size_t N>
	inline void mask_and(simd_mask<double, N> &out, const simd_mask<double, N> &l, const simd_mask<double, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_and_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_and_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N>
	inline void mask_or(simd_mask<double, N> &out, const simd_mask<double, N> &l, const simd_mask<double, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_or_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_or_pd(l.simd[1], r.simd[1]);
	}
	template<std::size_t N>
	inline void mask_neg(simd_mask<double, N> &out, const simd_mask<double, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		const auto mask = _mm_set1_pd(std::bit_cast<double>(0xffff'ffff'ffff'ffff));
		out.simd[0] = _mm_xor_pd(l.simd[0], mask);
		out.simd[1] = _mm_xor_pd(l.simd[1], mask);
	}

	template<std::size_t N>
	inline void mask_eq(simd_mask<double, N> &out, const simd_mask<double, N> &l, const simd_mask<double, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_castsi128_pd(_mm_cmpeq_epi32(_mm_castpd_si128(l.simd[0]), _mm_castpd_si128(r.simd[0])));
		out.simd[1] = _mm_castsi128_pd(_mm_cmpeq_epi32(_mm_castpd_si128(l.simd[1]), _mm_castpd_si128(r.simd[1])));
	}
	template<std::size_t N>
	inline void mask_ne(simd_mask<double, N> &out, const simd_mask<double, N> &l, const simd_mask<double, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd[0] = _mm_xor_pd(l.simd[0], r.simd[0]);
		out.simd[1] = _mm_xor_pd(l.simd[1], r.simd[1]);
	}
#endif
}	 // namespace sek::math::detail
#endif
