//
// Created by switchblade on 2021-11-26.
//

#pragma once

#include <concepts>
#include <cstdint>
#include <limits>

namespace sek
{
	/** @brief Helper structure used to store an integer and a flag in one of it's bits. */
	template<std::integral IntType>
	class flagged_integer_t
	{
		constexpr static IntType mask = std::numeric_limits<IntType>::max() << 1;

	public:
		constexpr flagged_integer_t() noexcept = default;
		constexpr explicit flagged_integer_t(IntType v, bool f = false) noexcept
		{
			set_value(v);
			set_flag(f);
		}

		[[nodiscard]] constexpr IntType get_value() const noexcept { return data / 2; }
		constexpr IntType set_value(IntType value) noexcept
		{
			data = (value * 2) | (data & 1);
			return value;
		}
		[[nodiscard]] constexpr bool get_flag() const noexcept { return data & 1; }
		constexpr bool set_flag(bool value) noexcept
		{
			data = (data & mask) | value;
			return value;
		}
		constexpr void toggle_flag() noexcept { data ^= 1; }

		[[nodiscard]] constexpr bool operator==(const flagged_integer_t &) const noexcept = default;

	private:
		IntType data = 0;
	};

	template<std::unsigned_integral I>
	flagged_integer_t(I) -> flagged_integer_t<I>;
	template<std::unsigned_integral I>
	flagged_integer_t(I, bool) -> flagged_integer_t<I>;
}	 // namespace sek