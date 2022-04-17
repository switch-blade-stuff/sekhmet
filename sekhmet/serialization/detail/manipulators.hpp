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
	 * specifying an explicit entry names will have no effect.  */
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

	/** @brief Archive manipulator used to read & write binary data. */
	struct binary_entry_t
	{
		union
		{
			const void *data_in;
			void *data_out;
		};
		std::size_t size;
	};

	/** Reads a binary entry into a memory buffer.
	 * @param buff Pointer to the output memory buffer.
	 * @param size Size of the memory buffer. */
	constexpr binary_entry_t read_binary(void *buff, std::size_t size) noexcept { return {{buff}, size}; }
	/** Writes a memory buffer to a binary entry.
	 * @param buff Pointer to the input memory buffer.
	 * @param size Size of the memory buffer. */
	constexpr binary_entry_t write_binary(const void *buff, std::size_t size) noexcept { return {{buff}, size}; }
	/** Reads an object from a binary entry.
	 * @param value Reference to the value to be read from a binary entry. */
	template<typename T>
	constexpr binary_entry_t read_binary(T &value) noexcept requires std::is_trivially_copyable_v<T>
	{
		return {{static_cast<void *>(&value)}, sizeof(T)};
	}
	/** Writes an object to a binary entry.
	 * @param value Reference to the value to be written to a binary entry. */
	template<typename T>
	constexpr binary_entry_t write_binary(T &value) noexcept requires std::is_trivially_copyable_v<T>
	{
		return {{static_cast<const void *>(&value)}, sizeof(T)};
	}
	/** Writes an object to a binary entry.
	 * @param value Value to be written to a binary entry. */
	template<typename T>
	constexpr binary_entry_t write_binary(T value) noexcept requires std::is_trivially_copyable_v<T>
	{
		return {{static_cast<const void *>(&value)}, sizeof(T)};
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
	 * @note If the archive does not support fixed-size containers, size will be left unmodified. */
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
	 * written leads to undefined behavior. */
	struct array_mode_t
	{
	};
	/** Switches archive to array output mode.
	 * @copydetails array_mode_t */
	constexpr array_mode_t array_mode() noexcept { return {}; }
}	 // namespace sek::serialization