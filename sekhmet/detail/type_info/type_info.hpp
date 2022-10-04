//
// Created by switch_blade on 2022-10-03.
//

#pragma once

#include "any.hpp"
#include "any_range.hpp"
#include "any_table.hpp"
#include "any_tuple.hpp"
#include "type_factory.hpp"

namespace sek
{
	namespace detail
	{
		template<typename T>
		constexpr type_data type_data::make_instance() noexcept
		{
			type_data result;
			result.name = type_name<T>();
			result.is_empty = std::is_empty_v<T>;
			result.is_nullptr = std::same_as<T, std::nullptr_t>;

			if constexpr (tuple_like<T>) result.tuple_data = &tuple_type_data::instance<T>;
			if constexpr (std::ranges::range<T>)
			{
				result.range_data = &range_type_data::instance<T>;
				if constexpr (table_range_type<T>) result.table_data = &table_type_data::instance<T>;
			}

			return result;
		}
		template<typename T>
		type_data *type_data::instance() noexcept
		{
			constinit static auto value = make_instance<T>();
			return &value;
		}
	}	 // namespace detail

	/** @brief Handle to information about a reflected type. */
	class type_info
	{
		friend class detail::type_handle;

		friend class any;
		friend class any_ref;
		friend class any_range;
		friend class any_table;
		friend class any_tuple;
		friend class type_database;

		using data_t = detail::type_data;
		using handle_t = detail::type_handle;

		template<typename T>
		[[nodiscard]] constexpr static handle_t handle() noexcept
		{
			return handle_t{type_selector<std::remove_cvref_t<T>>};
		}

		constexpr explicit type_info(const data_t *data) noexcept : m_data(data) {}
		constexpr explicit type_info(handle_t handle) noexcept : m_data(handle.get ? handle.get() : nullptr) {}

	public:
		/** Returns type info for type `T`.
		 * @note Removes any const & volatile qualifiers and decays references.
		 * @note The returned type info is generated at compile time and may not be present within the type database. */
		template<typename T>
		[[nodiscard]] constexpr static type_info get()
		{
			return type_info{handle<T>()};
		}
		/** Searches for a reflected type in the type database.
		 * @return Type info of the type, or an invalid type info if such type is not found. */
		[[nodiscard]] inline static type_info get(std::string_view name);

		/** Reflects type `T`, making it available for runtime lookup by name.
		 * @return Type factory for type `T`, which can be used to specify additional information about the type.
		 * @note Removes any const & volatile qualifiers and decays references.
		 * @note Modification of type information (via type factory) is not thread-safe. */
		template<typename T>
		inline static type_factory<T> reflect();
		/** Resets a reflected type, removing it from the type database.
		 * @note The type will no longer be available for runtime lookup. */
		inline static void reset(std::string_view name);
		/** @copydoc reset */
		template<typename T>
		static void reset() noexcept
		{
			reset(type_name<std::remove_cvref_t<T>>());
		}

	public:
		/** Initializes an invalid type info handle. */
		constexpr type_info() noexcept = default;

		/** Checks if the type info references a valid type. */
		[[nodiscard]] constexpr bool valid() const noexcept { return m_data != nullptr; }
		/** If the type info references a valid type, returns it's name. Otherwise, returns an empty string view. */
		[[nodiscard]] constexpr std::string_view name() const noexcept
		{
			return valid() ? m_data->name : std::string_view{};
		}

		/** Checks if the referenced type is `void`. */
		[[nodiscard]] constexpr bool is_void() const noexcept { return valid() && m_data->is_void; }
		/** Checks if the referenced type is empty (as if via `std::is_empty_v`). */
		[[nodiscard]] constexpr bool is_empty() const noexcept { return valid() && m_data->is_empty; }
		/** Checks if the referenced type is `std::nullptr_t`, or can be implicitly converted to `std::nullptr_t`. */
		[[nodiscard]] constexpr bool is_nullptr() const noexcept { return valid() && m_data->is_nullptr; }

		/** Checks if the referenced type is an enum (as if via `std::is_enum_v`). */
		[[nodiscard]] constexpr bool is_enum() const noexcept { return valid() && m_data->enum_type.get; }
		/** Checks if the referenced type is a signed integral type or can be implicitly converted to one. */
		[[nodiscard]] constexpr bool is_signed() const noexcept { return valid() && m_data->signed_conv; }
		/** Checks if the referenced type is an unsigned integral type or can be implicitly converted to one. */
		[[nodiscard]] constexpr bool is_unsigned() const noexcept { return valid() && m_data->unsigned_conv; }
		/** Checks if the referenced type is a floating-point type or can be implicitly converted to one. */
		[[nodiscard]] constexpr bool is_floating() const noexcept { return valid() && m_data->floating_conv; }

		/** Checks if the referenced type is a range. */
		[[nodiscard]] constexpr bool is_range() const noexcept { return valid() && m_data->range_data; }
		/** Checks if the referenced type is a table (range who's value type is a key-value pair). */
		[[nodiscard]] constexpr bool is_table() const noexcept { return valid() && m_data->table_data; }
		/** Checks if the referenced type is tuple-like. */
		[[nodiscard]] constexpr bool is_tuple() const noexcept { return valid() && m_data->tuple_data; }
		/** Checks if the referenced type is string-like. */
		[[nodiscard]] constexpr bool is_string() const noexcept { return valid() && m_data->string_data; }

		/** Returns the referenced type of an enum (as if via `std::underlying_type_t`), or an invalid type info it the type is not an enum. */
		[[nodiscard]] constexpr type_info enum_type() const noexcept
		{
			return valid() ? type_info{m_data->enum_type} : type_info{};
		}
		/** Returns the size of the tuple, or `0` if the type is not a tuple. */
		[[nodiscard]] constexpr std::size_t tuple_size() const noexcept
		{
			return is_tuple() ? m_data->tuple_data->types.size() : 0;
		}
		/** Returns the `i`th element type of the tuple, or an invalid type info if the type is not a tuple or `i` is out of range. */
		[[nodiscard]] constexpr type_info tuple_element(std::size_t i) const noexcept
		{
			return i < tuple_size() ? type_info{m_data->tuple_data->types[i]} : type_info{};
		}

		/** Checks if the referenced type has a parent of type `other`.
		 * @return `true` if `other` is parent of `this`. `false` if types are unrelated or the same. */
		[[nodiscard]] bool has_parent(type_info other) const noexcept;

		// clang-format off
		/** Checks if the referenced type has a parent of type `T`.
		 * @return `true` if `other` is parent of `this`. `false` if types are unrelated or the same. */
		template<typename T>
		[[nodiscard]] bool has_parent() const noexcept { return has_parent(get<T>()); }
		// clang-format on

		[[nodiscard]] constexpr bool operator==(const type_info &other) const noexcept
		{
			/* If data is different, types might still be the same, but declared in different binaries. */
			return m_data == other.m_data || name() == other.name();
		}

		constexpr void swap(type_info &other) noexcept { std::swap(m_data, other.m_data); }
		friend constexpr void swap(type_info &a, type_info &b) noexcept { a.swap(b); }

	private:
		const data_t *m_data = nullptr;
	};

	constexpr type_info any::type() const noexcept { return type_info{m_type}; }
	constexpr type_info any_ref::type() const noexcept { return type_info{m_type}; }
	constexpr type_info any_range::value_type() const noexcept { return type_info{m_data->value_type}; }
	constexpr type_info any_table::value_type() const noexcept { return type_info{m_data->value_type}; }
	constexpr type_info any_table::key_type() const noexcept { return type_info{m_data->key_type}; }
	constexpr type_info any_table::mapped_type() const noexcept { return type_info{m_data->mapped_type}; }
	constexpr type_info any_tuple::element(std::size_t i) const noexcept
	{
		return i < size() ? type_info{m_data->types[i]} : type_info{};
	}

	/* Type names for reflection types. */
	template<>
	[[nodiscard]] constexpr std::string_view type_name<type_info>() noexcept
	{
		return "sek::type_info";
	}
	template<>
	[[nodiscard]] constexpr std::string_view type_name<any>() noexcept
	{
		return "sek::any";
	}
	template<>
	[[nodiscard]] constexpr std::string_view type_name<any_ref>() noexcept
	{
		return "sek::any_ref";
	}
	template<>
	[[nodiscard]] constexpr std::string_view type_name<any_range>() noexcept
	{
		return "sek::any_range";
	}
	template<>
	[[nodiscard]] constexpr std::string_view type_name<any_table>() noexcept
	{
		return "sek::any_table";
	}
	template<>
	[[nodiscard]] constexpr std::string_view type_name<any_tuple>() noexcept
	{
		return "sek::any_table";
	}
	template<>
	[[nodiscard]] constexpr std::string_view type_name<any_string>() noexcept
	{
		return "sek::any_string";
	}

	/** Returns the type info of an object's type. Equivalent to `type_info::get<T>()`. */
	template<typename T>
	[[nodiscard]] constexpr type_info type_of(T &&obj) noexcept
	{
		return type_info::get<T>();
	}

	namespace literals
	{
		/** Retrieves a reflected type from the runtime database. */
		[[nodiscard]] inline type_info operator""_type(const char *str, std::size_t n)
		{
			return type_info::get({str, n});
		}
	}	 // namespace literals
}	 // namespace sek

/** Macro used to declare an instance of type info for type `T` as extern.
 * @note Type must be exported via `SEK_EXPORT_TYPE`.
 *
 * @example
 * @code{.cpp}
 * // my_type.hpp
 * struct my_type {};
 * SEK_EXTERN_TYPE(my_type)
 *
 * // my_type.cpp
 * SEK_EXPORT_TYPE(my_type)
 * @endcode*/
#define SEK_EXTERN_TYPE(T) extern template SEK_API_IMPORT sek::type_info::data_t *sek::type_info::get_data<T>();

/** Macro used to export instance of type info for type `T`.
 * @note Type must be declared as `extern` via `SEK_EXTERN_TYPE`.
 *
 * @example
 * @code{.cpp}
 * // my_type.hpp
 * struct my_type {};
 * SEK_EXTERN_TYPE(my_type)
 *
 * // my_type.cpp
 * SEK_EXPORT_TYPE(my_type)
 * @endcode */
#define SEK_EXPORT_TYPE(T) template SEK_API_EXPORT sek::type_info::data_t *sek::type_info::get_data<T>();