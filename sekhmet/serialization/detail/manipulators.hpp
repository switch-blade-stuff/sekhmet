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

	/** @brief Archive manipulator used to specify an explicit name for an entry. */
	template<typename T>
	struct named_entry
	{
	public:
		named_entry() = delete;

		/** Constructs a named entry manipulator from a name and a perfectly-forwarded value.
		 * @param name Name of the entry.
		 * @param value Value forwarded by the manipulator. */
		constexpr named_entry(std::string_view name, T &&value) noexcept(detail::noexcept_fwd<T>)
			: name(name), value(std::forward<T>(value))
		{
		}
		/** @copydoc named_entry */
		constexpr named_entry(const char *name, T &&value) noexcept(detail::noexcept_fwd<T>)
			: name(name), value(std::forward<T>(value))
		{
		}

		std::string_view name;
		T value;
	};

	template<typename T>
	named_entry(std::string_view, T &&) -> named_entry<T>;
	template<typename T>
	named_entry(const char *, T &&) -> named_entry<T>;

	/** @brief Constant used as a dynamic size value for array & object entry manipulators. */
	constexpr auto dynamic_size = std::numeric_limits<std::size_t>::max();

	/** @brief Archive manipulator used to switch an archive to array IO mode and read/write array size.
	 * @note If the archive does not support fixed-size arrays, size will be left unmodified. */
	template<typename T>
	requires std::integral<std::decay_t<T>>
	struct array_entry
	{
		/** Constructs an array entry manipulator from a perfectly-forwarded array size.
		 * @param value Size of the array forwarded by the manipulator. */
		constexpr explicit array_entry(T &&value) noexcept : value(std::forward<T>(value)) {}

		T value;
	};
	template<typename T>
	array_entry(T &&) -> array_entry<T>;

	/** @brief Archive manipulator used to switch an archive to array IO mode and read/write object size.
	 * @note If the archive does not support fixed-size objects, size will be left unmodified. */
	template<typename T>
	requires std::integral<std::decay_t<T>>
	struct object_entry
	{
		/** Constructs an array entry manipulator from a perfectly-forwarded array size.
		 * @param value Size of the array forwarded by the manipulator. */
		constexpr explicit object_entry(T &&value) noexcept : value(std::forward<T>(value)) {}

		T value;
	};
	template<typename T>
	object_entry(T &&) -> object_entry<T>;

	/** @brief Archive manipulator used to read & write binary data. */
	template<typename T>
	struct binary_entry
	{
		constexpr explicit binary_entry(T &&data) noexcept(detail::noexcept_fwd<T>) : data(std::forward<T>(data)) {}

		T data;
	};

	template<typename T>
	binary_entry(T &&) -> binary_entry<T>;
}	 // namespace sek::serialization