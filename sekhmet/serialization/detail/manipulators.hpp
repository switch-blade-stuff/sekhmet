//
// Created by switchblade on 2022-04-14.
//

#pragma once

#include "archive_traits.hpp"

namespace sek::serialization
{
	namespace detail
	{
		template<typename T>
		constexpr static bool noexcept_fwd = noexcept(T(std::forward<T>(std::declval<T &&>())));
	}

	/** @brief Archive manipulator used to read or write an entry with explicit name. */
	template<typename C, typename T>
	struct named_entry_t
	{
		std::basic_string_view<C> name;
		T value;
	};
	/** Reads or writes an entry with an explicit name.
	 * @param name Name of the entry.
	 * @param value Value to be read or written, forwarded by the manipulator.
	 * @note If the current entry (entry of the object being deserialized) is an array entry,
	 * specifying an explicit entry name will have no effect.
	 * @note Names consisting of an underscore followed by decimal digits (`_[0-9]+`) are reserved. */
	template<typename C, typename T>
	constexpr named_entry_t<C, T> named_entry(std::basic_string_view<C> name, T &&value) noexcept(detail::noexcept_fwd<T>)
	{
		return named_entry_t<C, T>{name, std::forward<T>(value)};
	}
	/** @copydoc named_entry */
	template<typename C, typename T>
	constexpr named_entry_t<C, T> named_entry(const C *name, T &&value) noexcept(detail::noexcept_fwd<T>)
	{
		return named_entry_t<C, T>{name, std::forward<T>(value)};
	}

	/** @brief Archive manipulator used read or write size of the current container.
	 * @note If the archive does not support fixed-size containers, size will be left unmodified. */
	template<typename T>
	struct container_size_t
	{
		T value;
	};
	/** Reads or writes size of the current container entry.
	 * @param size Size of the container, forwarded by the manipulator.
	 * @note If the archive does not support fixed-size containers, size will be left unmodified.*/
	template<typename T>
	constexpr container_size_t<T> container_size(T &&size) noexcept requires std::integral<std::decay_t<T>>
	{
		return container_size_t<T>{std::forward<T>(size)};
	}

	/** @brief Archive manipulator used to switch the archive to array output mode.
	 *
	 * By default archives serialize types as table-like entries.
	 * This manipulator is used to switch an archive to array output mode.
	 *
	 * @note Entries written to an array will not be accessible via a name.
	 * @warning Switching an archive to array output mode after multiple entries have already been
	 * written or after the container size was specified, leads to undefined behavior. */
	struct array_mode_t
	{
	};
	/** Switches archive to array output mode.
	 * @copydetails array_mode_t */
	constexpr array_mode_t array_mode() noexcept { return {}; }
}	 // namespace sek::serialization