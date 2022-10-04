//
// Created by switch_blade on 2022-10-03.
//

#pragma once

#include "any.hpp"
#include "any_range.hpp"
#include "any_string.hpp"
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

			result.any_funcs = any_funcs_t::make_instance<T>();
			if constexpr (tuple_like<T>) result.tuple_data = &tuple_type_data::instance<T>;
			if constexpr (std::ranges::range<T>)
			{
				result.range_data = &range_type_data::instance<T>;
				if constexpr (table_range_type<T>) result.table_data = &table_type_data::instance<T>;
			}
			if constexpr (string_like_type<T>) result.string_data = &string_type_data::instance<T>;

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
		friend class any_string;
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
		/** Checks if the referenced type has a parent of type `T`.
		 * @return `true` if `other` is parent of `this`. `false` if types are unrelated or the same. */
		template<typename T>
		[[nodiscard]] bool has_parent() const noexcept
		{
			return has_parent(get<T>());
		}

		/** Checks if the referenced type has a constructor invocable with the specified arguments.
		 * @return `true` if the type has such constructor, `false` otherwise. */
		[[nodiscard]] bool has_constructor(std::span<const any> args) const noexcept;
		/** @copydoc has_constructor */
		[[nodiscard]] bool has_constructor(std::initializer_list<any> args) const noexcept
		{
			return has_constructor(std::span{args.begin(), args.end()});
		}
		/** @copydoc has_constructor */
		template<detail::allowed_types<any, any_ref>... Ts>
		[[nodiscard]] bool has_constructor(Ts &&...args) const noexcept
		{
			return has_constructor({args.ref()...});
		}

		/** Checks if the referenced type has a constructor that takes the specified argument types.
		 * @return `true` if the type has such constructor, `false` otherwise. */
		[[nodiscard]] bool has_constructor(std::span<const type_info> args) const noexcept;
		/** @copydoc has_constructor */
		[[nodiscard]] bool has_constructor(std::initializer_list<type_info> args) const noexcept
		{
			return has_constructor(std::span{args.begin(), args.end()});
		}
		/** @copydoc has_constructor */
		template<detail::allowed_types<type_info>... Ts>
		[[nodiscard]] bool has_constructor(Ts &&...args) const noexcept
		{
			return has_constructor({args...});
		}
		/** @copydoc has_constructor */
		template<typename... Ts>
		[[nodiscard]] bool has_constructor() const noexcept
		{
			return has_constructor(type_info::get<Ts>()...);
		}

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

	any::any(type_info type, void *ptr) noexcept
	{
		m_storage = base_t::storage_t{ptr, false};
		m_type = type.m_data;
	}
	any::any(type_info type, const void *ptr) noexcept
	{
		m_storage = base_t::storage_t{ptr, true};
		m_type = type.m_data;
	}

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

	constexpr type_info any_string::char_type() const noexcept { return type_info{m_data->char_type}; }
	constexpr type_info any_string::value_type() const noexcept { return type_info{m_data->char_type}; }
	constexpr type_info any_string::traits_type() const noexcept { return type_info{m_data->traits_type}; }

	template<typename C>
	C *any_string::chars()
	{
		if (char_type() != type_info::get<C>()) [[unlikely]]
			return nullptr;
		return static_cast<C *>(data());
	}
	template<typename C>
	const C *any_string::chars() const
	{
		if (char_type() != type_info::get<C>()) [[unlikely]]
			return nullptr;
		return static_cast<C *>(data());
	}

	template<typename Sc, typename C, typename T, typename A>
	bool any_string::convert_with(std::basic_string<C, T, A> &dst, const std::locale &l, const A &a) const
	{
		/* Ignore this overload if the type is different. */
		if (char_type() != type_info::get<Sc>()) [[likely]]
			return false;

		if constexpr (std::same_as<Sc, C>)
			dst.assign(static_cast<const Sc *>(data()), size());
		else
		{
			using tmp_alloc_t = typename std::allocator_traits<A>::template rebind_alloc<char>;
			using tmp_traits_t = std::conditional_t<std::same_as<C, char>, T, std::char_traits<char>>;
			using tmp_string_t = std::basic_string<char, tmp_traits_t, tmp_alloc_t>;
			using conv_from_t = std::codecvt<Sc, char, std::mbstate_t>;
			using conv_to_t = std::codecvt<C, char, std::mbstate_t>;

			/* If `Sc` is not `char`, use codecvt to convert to `char`. Otherwise, directly copy the string. */
			auto tmp_buffer = tmp_string_t{tmp_alloc_t{a}};

			if constexpr (std::same_as<Sc, char>)
				tmp_buffer.assign(static_cast<const Sc *>(data()), size());
			else
			{
				auto &conv = std::use_facet<conv_from_t>(l);
				const auto *src_start = static_cast<const Sc *>(data());
				const auto *src_end = src_start + size();
				do_convert(src_start, src_end, tmp_buffer, conv);
			}
			/* If `C` is not `char`, preform a second conversion. Otherwise, use the temporary buffer. */
			if constexpr (std::same_as<C, char>)
				dst = std::move(std::basic_string<C, T, A>{tmp_buffer});
			else
			{
				auto &conv = std::use_facet<conv_to_t>(l);
				const auto *src_end = tmp_buffer.data() + tmp_buffer.size();
				const auto *src_start = tmp_buffer.data();
				do_convert(src_start, src_end, dst, conv);
			}
		}
		return true;
	}
	template<typename C, typename T, typename A>
	expected<std::basic_string<C, T, A>, std::error_code> any_string::as_str(std::nothrow_t, const std::locale &l, const A &a) const
	{
		/* Check if the requested type it is one of the standard character types.
		 * If so, convert using `std::codecvt`. Otherwise, return type error. */
		expected<std::basic_string<C, T, A>, std::error_code> result;
		if (convert_with<char>(*result, l, a) || convert_with<wchar_t>(*result, l, a) || convert_with<char8_t>(*result, l, a) ||
			convert_with<char16_t>(*result, l, a) || convert_with<char32_t>(*result, l, a)) [[likely]]
			return result;

		/* Characters are incompatible and cannot be converted via codecvt. */
		return unexpected{make_error_code(type_errc::INVALID_TYPE)};
	}
	template<typename C, typename T, typename A>
	std::basic_string<C, T, A> any_string::as_str(const std::locale &l, const A &a) const
	{
		/* Check if the requested type it is one of the standard character types.
		 * If so, convert using `std::codecvt`. Otherwise throw. */
		std::basic_string<C, T, A> result;
		if (convert_with<char>(result, l, a) || convert_with<wchar_t>(result, l, a) || convert_with<char8_t>(result, l, a) ||
			convert_with<char16_t>(result, l, a) || convert_with<char32_t>(result, l, a)) [[likely]]
			return result;

		/* Characters are incompatible and cannot be converted via codecvt. */
		throw type_error(make_error_code(type_errc::INVALID_TYPE), "Cannot convert to requested string type");
	}

	// clang-format off
	extern template SEK_API_IMPORT expected<std::basic_string<char>, std::error_code> any_string::as_str(
		std::nothrow_t, const std::locale &, const typename std::basic_string<char>::allocator_type &) const;
	extern template SEK_API_IMPORT expected<std::basic_string<wchar_t>, std::error_code> any_string::as_str(
		std::nothrow_t, const std::locale &, const typename std::basic_string<wchar_t>::allocator_type &) const;

	extern template SEK_API_IMPORT std::basic_string<char> any_string::as_str(
		const std::locale &, const typename std::basic_string<char>::allocator_type &) const;
	extern template SEK_API_IMPORT std::basic_string<wchar_t> any_string::as_str(
		const std::locale &, const typename std::basic_string<wchar_t>::allocator_type &) const;
	// clang-format on

	/* Type names for reflection types. */
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
	[[nodiscard]] constexpr std::string_view type_name<type_info>() noexcept
	{
		return "sek::type_info";
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
#define SEK_EXTERN_TYPE(T)                                                                                             \
	extern template SEK_API_IMPORT sek::detail::type_data *sek::detail::type_data::instance<T>() noexcept;

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
#define SEK_EXPORT_TYPE(T)                                                                                             \
	template SEK_API_EXPORT sek::detail::type_data *sek::detail::type_data::instance<T>() noexcept;

/* Type exports for reflection types */
SEK_EXTERN_TYPE(sek::any);
SEK_EXTERN_TYPE(sek::any_ref);
SEK_EXTERN_TYPE(sek::type_info);
