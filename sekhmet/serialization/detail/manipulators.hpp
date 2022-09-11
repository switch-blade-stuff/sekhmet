/*
 * Created by switchblade on 2022-04-14
 */

#pragma once

#include <type_traits>
#include <string_view>

namespace sek::serialization
{
	namespace detail
	{
		template<typename T>
		constexpr static bool noexcept_fwd = noexcept(T(std::forward<T>(std::declval<T &&>())));
	}

	/** @brief Archive manipulator used to read or write an entry with explicit key. */
	template<typename C, typename T>
	struct keyed_entry_t
	{
		std::basic_string_view<C> key;
		T value;
	};
	/** Reads or writes an entry with an explicit key.
	 * @param key Key of the entry.
	 * @param value Value to be read or written, forwarded by the manipulator.
	 * @note An archive is allowed to ignore this manipulator.
	 * @note Names consisting of one or multiple underscores followed by decimal digits (`_+[0-9]+`) are reserved. */
	template<typename C, typename T>
	constexpr keyed_entry_t<C, T> keyed_entry(std::basic_string_view<C> key, T &&value) noexcept(detail::noexcept_fwd<T>)
	{
		return keyed_entry_t<C, T>{key, std::forward<T>(value)};
	}
	/** @copydoc keyed_entry */
	template<typename K, typename T, typename C = typename std::remove_reference_t<K>::traits_type::char_type>
	constexpr keyed_entry_t<C, T> keyed_entry(K &&key, T &&value) noexcept(detail::noexcept_fwd<T>)
		requires std::constructible_from<std::basic_string_view<C>, K>
	{
		return keyed_entry_t<C, T>{std::basic_string_view<C>{key}, std::forward<T>(value)};
	}
	/** @copydoc keyed_entry */
	template<typename C, typename T>
	constexpr keyed_entry_t<C, T> keyed_entry(const C *key, T &&value) noexcept(detail::noexcept_fwd<T>)
	{
		return keyed_entry_t<C, T>{key, std::forward<T>(value)};
	}

	/** @brief Archive manipulator used to read or write an entry attribute. */
	template<typename T>
	struct entry_attribute_t
	{
		T value;
	};
	/** @copydoc T */
	template<typename T>
	constexpr entry_attribute_t<T> entry_attribute(T &&value) noexcept(detail::noexcept_fwd<T>)
	{
		return entry_attribute_t{std::forward<T>(value)};
	}

	/** @brief Archive manipulator used read or write size of the current container.
	 *
	 * By default, archives should infer container size during serialization. This manipulator is used to specify
	 * an explicit container size.
	 *
	 * @note An archive is allowed to ignore this manipulator. */
	template<typename T>
	struct container_size_t
	{
		T value;
	};
	/** Reads or writes size of the current container entry.
	 * @param size Size of the container, forwarded by the manipulator.
	 * @note An archive is allowed to ignore this manipulator. */
	template<typename T>
	constexpr container_size_t<T> container_size(T &&size) noexcept
		requires std::integral<std::remove_reference_t<T>>
	{
		return container_size_t<T>{std::forward<T>(size)};
	}

	/** @brief Archive manipulator used to switch the archive to array output mode.
	 *
	 * By default archives serialize types as table-like "object" entries. This manipulator is used to switch an
	 * archive to array output mode.
	 *
	 * @note An archive is allowed to ignore this manipulator.
	 * @warning Entries written to an array will not be accessible via a key. Doing so will result in serialization errors.
	 * @warning Switching an archive to array output mode after multiple entries have already been written or after the
	 * container size was specified, will result in serialization errors. */
	struct array_mode_t
	{
	};
	/** Switches archive to array output mode.
	 * @copydetails array_mode_t */
	constexpr array_mode_t array_mode() noexcept { return {}; }

	/** @brief Archive manipulator used to explicitly switch an archive to dynamic-type mode.
	 *
	 * By default, type-aware archives would either use a dynamic datatype or determine the data type during
	 * serialization. Sometimes it may be desirable to force an archive into dynamic-type mode, which is what
	 * this manipulator accomplishes.
	 *
	 * @note Archives may ignore this manipulator.
	 * @note If type information is supported, an archive may be switched to dynamic mode at any point in time, even
	 * after multiple entries have been serialized. */
	struct dynamic_type_t
	{
	};
	/** Switches archive to dynamic type mode.
	 * @copydetails array_mode_t */
	constexpr dynamic_type_t dynamic_type() noexcept { return {}; }
	/** @brief Archive manipulator used to explicitly specify datatype of an archive.
	 *
	 * By default, type-aware archives would either use a dynamic datatype or determine the data type during
	 * serialization. Sometimes it may be desirable to explicitly specify a datatype for an archive (for example
	 * to reduce encoding size).
	 *
	 * @note Archives may ignore this manipulator.
	 * @warning Forcing an explicit data type may require explicit conversions during serialization and may result in
	 * serialization errors if an archive does not support the desired type conversion.
	 * @warning Switching an archive type after multiple entries have already been serialized will result in
	 * serialization errors. */
	template<typename T>
	struct explicit_type_t
	{
		typedef T type;
	};
	/** Switches archive to an explicit type.
	 * @copydetails explicit_type_t */
	template<typename T>
	constexpr explicit_type_t<T> explicit_type() noexcept
	{
		return {};
	}
}	 // namespace sek::serialization