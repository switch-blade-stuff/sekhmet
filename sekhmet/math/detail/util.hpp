//
// Created by switchblade on 2021-12-17.
//

#pragma once

#include <cmath>
#include <limits>

#include "sekhmet/detail/define.h"
#include <type_traits>

namespace sek::math
{
	template<typename T>
	concept arithmetic = std::is_arithmetic_v<T>;

	template<typename T, std::size_t N>
	concept integral_of_size = std::is_integral_v<T> && sizeof(T) == N;
	template<typename T, std::size_t N>
	concept signed_integral_of_size = std::signed_integral<T> && sizeof(T) == N;
	template<typename T, std::size_t N>
	concept unsigned_integral_of_size = std::unsigned_integral<T> && sizeof(T) == N;

	namespace detail
	{
		template<std::size_t Bit, std::integral T>
		constexpr T next_pow_2_impl(T num) noexcept
		{
			num |= num >> Bit;
			if constexpr (Bit < std::numeric_limits<T>::digits / 2)
				return next_pow_2_impl<Bit * 2>(num);
			else
				return num;
		}
	}	 // namespace detail

	/** Aligns a positive number to be the read power of two. */
	template<typename T>
	[[nodiscard]] constexpr T next_pow_2(T num) noexcept
	{
		if constexpr (std::numeric_limits<T>::is_signed)
			if (num < 0) return num;
		return detail::next_pow_2_impl<1>(--num) + 1;
	}

	namespace detail
	{
#if defined(_MSC_VER) && defined(SEK_ARCH_x86_32)
		inline static std::uint32_t __cdecl fast_log2_x86(std::uint32_t num)
		{
			__asm
			{
				bsr eax, num
				ret
			}
		}
#endif
	}	 // namespace detail

	/** Calculates log2 of an integer without using cpu-specific instructions. */
	[[nodiscard]] constexpr std::uint32_t slow_log2(std::uint32_t num)
	{
		std::uint32_t bit = 0;
		while (num >>= 1) ++bit;
		return bit;
	}
	/** Calculates log2 of an integer.
	 * If such instructions are not available, uses slow_log2.
	 * @note Return value might be undefined if number is 0. */
	[[nodiscard]] constexpr std::uint32_t log2(std::uint32_t num)
	{
		if (std::is_constant_evaluated())
			return slow_log2(num);
		else
		{
#if defined(__clang__) || defined(__GNUC__)
			return static_cast<std::uint32_t>(__builtin_clz(num));
#elif defined(SEK_ARCH_x86_32)
			return detail::fast_log2_x86(num);
#elif defined(_MSC_VER)
			unsigned long index = 0;
#if SIZE_MAX < INT64_MAX
			_BitScanReverse(&index, num);
#else
			_BitScanReverse64(&index, num);
#endif
			return index;
#else
			return slow_log2(num);
#endif
		}
	}

	/** Divides a number and rounds up. */
	template<std::integral T>
	[[nodiscard]] constexpr T divide_ceil(T num, T den) noexcept
	{
		return num / den + static_cast<bool>(num % den);
	}

	/** Aligns a number to be the nearest upper multiple of mult. */
	template<std::integral T>
	[[nodiscard]] constexpr T align(T num, T mult) noexcept
	{
		auto rem = num % mult;
		return num - rem + (rem ? mult : 0);
	}

	template<arithmetic T>
	[[nodiscard]] constexpr T max(T a, T b) noexcept
	{
		return a > b ? a : b;
	}
	template<arithmetic T>
	[[nodiscard]] constexpr T min(T a, T b) noexcept
	{
		return a < b ? a : b;
	}

	namespace detail
	{
		template<arithmetic T0, arithmetic T1, arithmetic... Ts>
		[[nodiscard]] constexpr std::common_type_t<T0, T1, Ts...> max_unwrap(T0 a, T1 b, Ts... vals) noexcept
		{
			if (a > b)
				return a;
			else
			{
				if constexpr (sizeof...(vals) == 0)
					return b;
				else
					return max_unwrap(b, vals...);
			}
		}
		template<arithmetic T0, arithmetic T1, arithmetic... Ts>
		[[nodiscard]] constexpr std::common_type_t<T0, T1, Ts...> min_unwrap(T0 a, T1 b, Ts... vals) noexcept
		{
			if (a < b)
				return a;
			else
			{
				if constexpr (sizeof...(vals) == 0)
					return b;
				else
					return min_unwrap(b, vals...);
			}
		}
	}	 // namespace detail

	template<arithmetic... Ts>
	[[nodiscard]] constexpr std::common_type_t<Ts...> max(Ts... vals) noexcept requires(sizeof...(Ts) > 1)
	{
		return detail::max_unwrap(vals...);
	}
	template<arithmetic... Ts>
	[[nodiscard]] constexpr std::common_type_t<Ts...> min(Ts... vals) noexcept requires(sizeof...(Ts) > 1)
	{
		return detail::min_unwrap(vals...);
	}
}	 // namespace sek::math