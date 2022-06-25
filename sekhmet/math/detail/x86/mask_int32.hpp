/*
 * Created by switchblade on 2022-01-31
 */

#pragma once

#include "../util.hpp"
#include "common.hpp"

#ifdef SEK_USE_SSE2
namespace sek::math::detail
{
	template<integral_of_size<4> T>
	union mask_data<T, 3, storage_policy::OPTIMAL>
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
		template<typename... Args>
		constexpr mask_data(Args &&...args) noexcept : mask_data({std::forward<Args>(args)...})
		{
		}

		constexpr element_t operator[](std::size_t i) noexcept { return {values[i]}; }
		constexpr const_element_t operator[](std::size_t i) const noexcept { return {values[i]}; }

		std::uint32_t values[3];
		__m128i simd;
	};
	template<integral_of_size<4> T>
	union mask_data<T, 4, storage_policy::OPTIMAL>
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
		template<typename... Args>
		constexpr mask_data(Args &&...args) noexcept : mask_data({std::forward<Args>(args)...})
		{
		}

		constexpr element_t operator[](std::size_t i) noexcept { return {values[i]}; }
		constexpr const_element_t operator[](std::size_t i) const noexcept { return {values[i]}; }

		std::uint32_t values[4];
		__m128i simd;
	};

	template<integral_of_size<4> T, std::size_t N, std::size_t M, std::size_t... Is>
	inline void mask_shuffle(simd_mask<T, N> &out, const simd_mask<double, N> &l, std::index_sequence<Is...> s) noexcept
		requires(SEK_DETAIL_IS_SIMD(out, l))
	{
		constexpr auto mask = x86_128_shuffle4_mask(s);
		out.simd = _mm_shuffle_epi32(l.simd, mask);
	}

	template<integral_of_size<4> T, std::size_t N>
	inline void mask_and(simd_mask<T, N> &out, const simd_mask<T, N> &l, const simd_mask<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_and_si128(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void mask_or(simd_mask<T, N> &out, const simd_mask<T, N> &l, const simd_mask<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_or_si128(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void mask_neg(simd_mask<T, N> &out, const simd_mask<T, N> &l) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_xor_si128(l.simd, _mm_set1_epi32(-1));
	}

	template<integral_of_size<4> T, std::size_t N>
	inline void mask_eq(simd_mask<T, N> &out, const simd_mask<T, N> &l, const simd_mask<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_cmpeq_epi32(l.simd, r.simd);
	}
	template<integral_of_size<4> T, std::size_t N>
	inline void mask_ne(simd_mask<T, N> &out, const simd_mask<T, N> &l, const simd_mask<T, N> &r) noexcept
		requires(SEK_DETAIL_IS_SIMD(out))
	{
		out.simd = _mm_xor_si128(l.simd, r.simd);
	}
}	 // namespace sek::math::detail
#endif
