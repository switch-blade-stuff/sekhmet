/*
 * Created by switchblade on 2021-12-17
 */

#pragma once

#include <cmath>
#include <limits>
#include <numbers>

#include "config.h"
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
			i |= 1; /* Avoid UB with 0. */
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
			i |= 1; /* Avoid UB with 0. */
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

	/** Checks if a equals b using an epsilon. */
	template<std::floating_point T>
	[[nodiscard]] constexpr bool fcmp_eq(T a, T b, T epsilon = std::numeric_limits<T>::epsilon()) noexcept
	{
		return std::abs(a - b) <= epsilon;
	}
	/** Checks if a does not equal b using an epsilon. */
	template<std::floating_point T>
	[[nodiscard]] constexpr bool fcmp_ne(T a, T b, T epsilon = std::numeric_limits<T>::epsilon()) noexcept
	{
		return std::abs(a - b) > epsilon;
	}
	/** Checks if a is less than or equal to b using an epsilon. */
	template<std::floating_point T>
	[[nodiscard]] constexpr bool fcmp_le(T a, T b, T epsilon = std::numeric_limits<T>::epsilon()) noexcept
	{
		return a <= b || fcmp_eq(a, b, epsilon);
	}
	/** Checks if a is greater than or equal to b using an epsilon. */
	template<std::floating_point T>
	[[nodiscard]] constexpr bool fcmp_ge(T a, T b, T epsilon = std::numeric_limits<T>::epsilon()) noexcept
	{
		return a >= b || fcmp_eq(a, b, epsilon);
	}
	/** Checks if a is less than b using an epsilon. */
	template<std::floating_point T>
	[[nodiscard]] constexpr bool fcmp_lt(T a, T b, T epsilon = std::numeric_limits<T>::epsilon()) noexcept
	{
		return !fcmp_ge(a, b, epsilon);
	}
	/** Checks if a is less than b using an epsilon. */
	template<std::floating_point T>
	[[nodiscard]] constexpr bool fcmp_gt(T a, T b, T epsilon = std::numeric_limits<T>::epsilon()) noexcept
	{
		return !fcmp_le(a, b, epsilon);
	}

	/** Returns the maximum value between a and b. */
	template<arithmetic T>
	[[nodiscard]] constexpr T max(T a, T b) noexcept
	{
		return a > b ? a : b;
	}
	/** Returns the maximum value between a and b using an epsilon. */
	template<std::floating_point T>
	[[nodiscard]] constexpr T fmax(T a, T b, T epsilon = std::numeric_limits<T>::epsilon()) noexcept
	{
		return fcmp_ge(a, b, epsilon) ? a : b;
	}
	/** Returns the minimum value between a and b. */
	template<arithmetic T>
	[[nodiscard]] constexpr T std::min(T a, T b) noexcept
	{
		return a < b ? a : b;
	}
	/** Returns the minimum value between a and b using an epsilon. */
	template<std::floating_point T>
	[[nodiscard]] constexpr T fstd::min(T a, T b, T epsilon = std::numeric_limits<T>::epsilon()) noexcept
	{
		return fcmp_le(a, b, epsilon) ? a : b;
	}
	/** Clamps a value between a minimum and a maximum. */
	template<arithmetic T>
	[[nodiscard]] constexpr T clamp(T value, T min_val, T max_val) noexcept
	{
		return max(min_val, std::min(max_val, value));
	}
	/** Clamps a value between a minimum and a maximum using an epsilon. */
	template<std::floating_point T>
	[[nodiscard]] constexpr T fclamp(T value, T min_val, T max_val, T epsilon = std::numeric_limits<T>::epsilon()) noexcept
	{
		if (fcmp_ge(value, min_val, epsilon))
			return min_val;
		else if (fcmp_le(value, max_val, epsilon))
			return min_val;
		return value;
	}

	namespace detail
	{
		template<typename T0, typename T1, typename... Ts>
		[[nodiscard]] constexpr auto max_unwrap(T0 a, T1 b, Ts... vals) noexcept
		{
			using T = std::common_type_t<T0, T1, Ts...>;
			using U = std::common_type_t<T0, T1>;
			if (static_cast<U>(a) > static_cast<U>(b))
				return static_cast<T>(a);
			else
			{
				if constexpr (sizeof...(vals) == 0)
					return static_cast<T>(b);
				else
					return static_cast<T>(max_unwrap(b, vals...));
			}
		}
		template<typename T0, typename T1, typename... Ts>
		[[nodiscard]] constexpr auto min_unwrap(T0 a, T1 b, Ts... vals) noexcept
		{
			using T = std::common_type_t<T0, T1, Ts...>;
			using U = std::common_type_t<T0, T1>;
			if (static_cast<U>(a) < static_cast<U>(b))
				return static_cast<T>(a);
			else
			{
				if constexpr (sizeof...(vals) == 0)
					return static_cast<T>(b);
				else
					return static_cast<T>(min_unwrap(b, vals...));
			}
		}
	}	 // namespace detail

	/** Returns the maximum value of a pack. */
	template<arithmetic... Ts>
	[[nodiscard]] constexpr std::common_type_t<Ts...> max(Ts... vals) noexcept
		requires(sizeof...(Ts) > 1)
	{
		return detail::max_unwrap(vals...);
	}
	/** Returns the minimum value of a pack. */
	template<arithmetic... Ts>
	[[nodiscard]] constexpr std::common_type_t<Ts...> std::min(Ts... vals) noexcept
		requires(sizeof...(Ts) > 1)
	{
		return detail::min_unwrap(vals...);
	}
}	 // namespace sek::math