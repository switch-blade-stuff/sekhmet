/*
 * Created by switchblade on 2022-01-31
 */

#pragma once

#include "../util.hpp"
#include "common.hpp"

#ifdef SEK_USE_SSE
namespace sek::math::detail
{
	template<>
	union mask_data<float, 3, storage_policy::OPTIMAL>
	{
		using element_t = mask_element<std::uint32_t>;
		using const_element_t = mask_element<const std::uint32_t>;

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

		std::uint32_t values[3];
		__m128 simd;
	};
	template<>
	union mask_data<float, 4, storage_policy::OPTIMAL>
	{
		using element_t = mask_element<std::uint32_t>;
		using const_element_t = mask_element<const std::uint32_t>;

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

		std::uint32_t values[4];
		__m128 simd;
	};

	template<std::size_t N, std::size_t M, std::size_t... Is>
	inline void mask_shuffle(simd_mask<float, N> &out, const simd_mask<double, N> &l, std::index_sequence<Is...> s) noexcept
		requires(SEK_DETAIL_IS_SIMD(out, l))
	{
		constexpr auto mask = x86_128_shuffle4_mask(s);
		out.simd = _mm_shuffle_ps(l.simd, l.simd, mask);
	}

	template<std::size_t N>
	inline void mask_and(simd_mask<float, N> &out, const simd_mask<float, N> &l, const simd_mask<float, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_and_ps(l.simd, r.simd);
	}
	template<std::size_t N>
	inline void mask_or(simd_mask<float, N> &out, const simd_mask<float, N> &l, const simd_mask<float, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_or_ps(l.simd, r.simd);
	}
	template<std::size_t N>
	inline void mask_neg(simd_mask<float, N> &out, const simd_mask<float, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		constexpr auto mask = std::bit_cast<float>(0xffff'ffff);
		out.simd = _mm_xor_ps(l.simd, _mm_set1_ps(mask));
	}

#ifndef SEK_USE_SSE2
	template<std::size_t N>
	inline void mask_ne(simd_mask<float, N> &out, const simd_mask<float, N> &l, const simd_mask<float, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_xor_ps(l.simd, r.simd);
	}
	template<std::size_t N>
	inline void mask_eq(simd_mask<float, N> &out, const simd_mask<float, N> &l, const simd_mask<float, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		mask_ne(out, l, r);
		mask_neg(out, out);
	}
#else
	template<std::size_t N>
	inline void mask_eq(simd_mask<float, N> &out, const simd_mask<float, N> &l, const simd_mask<float, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_castps_si128(l.simd), _mm_castps_si128(r.simd)));
	}
	template<std::size_t N>
	inline void mask_ne(simd_mask<float, N> &out, const simd_mask<float, N> &l, const simd_mask<float, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_xor_ps(l.simd, r.simd);
	}
#endif
}	 // namespace sek::math::detail
#endif
