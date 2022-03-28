//
// Created by switchblade on 2021-12-31.
//

#pragma once

#include <concepts>
#include <cstdint>
#include <limits>

namespace sek
{
	/** @brief Helper structure used to store a pointer and a flag in it's bottom bit.
	 * @note Type must have alignment greater than 1. */
	template<typename T>
	class flagged_ptr_t
	{
		constexpr static std::uintptr_t mask = std::numeric_limits<std::uintptr_t>::max() << 1;

		union data_t
		{
			constexpr data_t() noexcept = default;
			constexpr explicit data_t(T *ptr) noexcept : ptr_value(ptr) {}
			constexpr explicit data_t(std::uintptr_t i) noexcept : int_value(i) {}

			std::uintptr_t int_value = 0;
			T *ptr_value;
		};

	public:
		constexpr flagged_ptr_t() noexcept = default;
		constexpr explicit flagged_ptr_t(T *ptr, bool flag = false) noexcept
		{
			set_pointer(ptr);
			set_flag(flag);
		}

		[[nodiscard]] constexpr T *get_pointer() const noexcept { return data_t{data.int_value & mask}.ptr_value; }
		constexpr T *set_pointer(T *new_ptr) noexcept
		{
			data.int_value = data_t{new_ptr}.int_value | get_flag();
			return new_ptr;
		}
		[[nodiscard]] constexpr bool get_flag() const noexcept { return data.int_value & 1; }
		constexpr bool set_flag(bool flag) noexcept
		{
			data.int_value = (data.int_value & mask) | flag;
			return flag;
		}
		constexpr void toggle_flag() noexcept { data.int_value ^= 1; }

		[[nodiscard]] constexpr bool operator==(const flagged_ptr_t &) const noexcept = default;

	private:
		data_t data;
	};

	template<typename T>
	flagged_ptr_t(T *) -> flagged_ptr_t<T>;
	template<typename T>
	flagged_ptr_t(T *, bool) -> flagged_ptr_t<T>;
}	 // namespace sek