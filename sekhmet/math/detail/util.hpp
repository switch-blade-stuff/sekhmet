/*
 * Created by switchblade on 2021-12-17
 */

#pragma once

#include <cmath>
#include <limits>
#include <numbers>

#include "sekhmet/detail/define.h"

#include <type_traits>

namespace sek::math
{
	template<typename T>
	concept arithmetic = std::is_arithmetic_v<T>;

	// clang-format off
	template<typename T, std::size_t N>
	concept integral_of_size = std::is_integral_v<T> && sizeof(T) == N;
	template<typename T, std::size_t N>
	concept signed_integral_of_size = std::signed_integral<T> && sizeof(T) == N;
	template<typename T, std::size_t N>
	concept unsigned_integral_of_size = std::unsigned_integral<T> && sizeof(T) == N;
	// clang-format on

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

	/** Aligns an integer to the next power of two. */
	template<std::integral T>
	[[nodiscard]] constexpr T next_pow_2(T num) noexcept
	{
		if constexpr (std::numeric_limits<T>::is_signed)
			if (num < 0) return num;
		return detail::next_pow_2_impl<1>(--num) + 1;
	}

	namespace detail
	{
		template<std::integral I>
		[[nodiscard]] constexpr std::size_t slow_msb(I i) noexcept
		{
			std::size_t bit = 0;
			while (i >> bit) ++bit;
			return bit;
		}
		template<std::integral I>
		[[nodiscard]] constexpr std::size_t slow_lsb(I i) noexcept
		{
			std::size_t bit = 0;
			while (!((i >> bit) & 1)) ++bit;
			return bit;
		}
	}	 // namespace detail

	/** Finds the MSB of the passed integer. */
	template<std::integral I>
	[[nodiscard]] constexpr std::size_t msb(I i) noexcept
	{
		if (std::is_constant_evaluated())
			return detail::slow_msb(i);
		else
		{
			i |= 1;
#if defined(__clang__) || defined(__GNUC__)
			if constexpr (sizeof(I) <= sizeof(unsigned int))
				return static_cast<std::size_t>(__builtin_clz(static_cast<unsigned int>(i)));
			else if constexpr (sizeof(I) <= sizeof(unsigned long))
				return static_cast<std::size_t>(__builtin_clzl(static_cast<unsigned long>(i)));
			else if constexpr (sizeof(I) <= sizeof(unsigned long long))
				return static_cast<std::size_t>(__builtin_clzll(static_cast<unsigned long long>(i)));
#elif defined(_MSC_VER)
			if constexpr (sizeof(I) <= sizeof(unsigned long))
			{
				unsigned long index = 0;
				_BitScanReverse(&index, static_cast<unsigned long>(i));
				return index;
			}
			else if constexpr (sizeof(I) <= sizeof(__int64))
			{
				unsigned long index = 0;
				_BitScanReverse(&index, static_cast<__int64>(i));
				return index;
			}
#else
			return detail::slow_msb(num);
#endif
		}
	}
	/** Finds the LSB of the passed integer. */
	template<std::integral I>
	[[nodiscard]] constexpr std::size_t lsb(I i) noexcept
	{
		if (std::is_constant_evaluated())
			return detail::slow_lsb(i);
		else
		{
			i |= 1;
#if defined(__clang__) || defined(__GNUC__)
			if constexpr (sizeof(I) <= sizeof(unsigned int))
				return static_cast<std::size_t>(__builtin_ctz(static_cast<unsigned int>(i)));
			else if constexpr (sizeof(I) <= sizeof(unsigned long))
				return static_cast<std::size_t>(__builtin_ctzl(static_cast<unsigned long>(i)));
			else if constexpr (sizeof(I) <= sizeof(unsigned long long))
				return static_cast<std::size_t>(__builtin_ctzll(static_cast<unsigned long long>(i)));
#elif defined(_MSC_VER)
			if constexpr (sizeof(I) <= sizeof(unsigned long))
			{
				unsigned long index = 0;
				_BitScanForward(&index, static_cast<unsigned long>(i));
				return index;
			}
			else if constexpr (sizeof(I) <= sizeof(__int64))
			{
				unsigned long index = 0;
				_BitScanForward64(&index, static_cast<__int64>(i));
				return index;
			}
#else
			return detail::slow_lsb(num);
#endif
		}
	}

	/** Calculates log2 of an integer. */
	template<std::integral I>
	[[nodiscard]] constexpr I log2(I i) noexcept
	{
		return static_cast<I>(msb(i));
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
		const auto rem = num % mult;
		return num - rem + (rem ? mult : 0);
	}

	/** Converts degrees to radians. */
	template<std::floating_point T>
	[[nodiscard]] constexpr T rad(T d) noexcept
	{
		return d * std::numbers::pi_v<T> / static_cast<T>(180.0);
	}
	/** Converts radians to degrees. */
	template<std::floating_point T>
	[[nodiscard]] constexpr T deg(T r) noexcept
	{
		return r * static_cast<T>(180.0) / std::numbers::pi_v<T>;
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
	[[nodiscard]] constexpr std::common_type_t<Ts...> max(Ts... vals) noexcept
		requires(sizeof...(Ts) > 1)
	{
		return detail::max_unwrap(vals...);
	}
	template<arithmetic... Ts>
	[[nodiscard]] constexpr std::common_type_t<Ts...> min(Ts... vals) noexcept
		requires(sizeof...(Ts) > 1)
	{
		return detail::min_unwrap(vals...);
	}

	template<typename T>
	[[nodiscard]] constexpr T clamp(T value, T min_val, T max_val) noexcept
	{
		return max(min_val, min(max_val, value));
	}
}	 // namespace sek::math