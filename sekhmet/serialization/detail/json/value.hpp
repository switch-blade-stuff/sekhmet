/*
 * Created by switchblade on 07/07/22
 */

#pragma once

#include <vector>

#include "sekhmet/ordered_map.hpp"

#include "../manipulators.hpp"
#include "../util.hpp"
#include "type.hpp"

namespace sek::serialization
{
	/** @brief Structure representing a Json value (object, array or literal) and providing serialization archive operations.
	 *
	 * Json values can either be used on their own as a way to store Json data or as a base type to implement
	 * Json-like serialization archives. Json values provide both a serialization archive functionality and
	 * a general Json container functionality, allowing to create arbitrary Json structures.
	 *
	 * @tparam C Character type used for Json strings.
	 * @tparam T Traits type of `C`.
	 * @tparam Alloc Allocator template used to allocate memory of the Json value. */
	template<typename C, typename T = std::char_traits<C>, template<typename...> typename Alloc = std::allocator>
	class basic_json_value
	{
	public:
		typedef inout_archive_category archive_category;

		typedef Alloc<C> string_allocator;

		typedef std::basic_string<C, T, string_allocator> string_type;
		typedef std::basic_string_view<C, T> string_view_type;
		typedef string_type key_type;

		typedef Alloc<std::pair<const key_type, basic_json_value>> object_allocator;
		typedef ordered_map<key_type, basic_json_value, object_allocator> object_type;

		typedef Alloc<basic_json_value> array_allocator;
		typedef std::vector<basic_json_value, array_allocator> array_type;

		typedef basic_json_value value_type;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

	private:
		/* TODO: Use a better way to represent 32 & 64-bit floats. */
		using float32_t = float;
		using float64_t = double;

		template<typename U>
		constexpr static bool is_compatible_int = std::integral<U>;
		template<typename U>
		constexpr static bool is_compatible_float = std::floating_point<U>;
		template<typename U>
		constexpr static bool is_compatible_bool = std::is_convertible_v<U, bool> && !std::integral<U>;
		template<typename U>
		constexpr static bool is_compatible_string = std::is_constructible_v<string_type, U> && !std::integral<U>;

		template<detail::pair_like U, typename... Args>
		[[nodiscard]] constexpr static bool is_compatible_object_pair() noexcept
		{
			using first_t = std::remove_cvref_t<decltype(std::declval<U>().first)>;
			using second_t = std::remove_cvref_t<decltype(std::declval<U>().second)>;
			return is_compatible_string<first_t> && std::constructible_from<basic_json_value, second_t, Args...>;
		}
		template<typename U, typename... Args>
		[[nodiscard]] constexpr static bool is_compatible_object_pair() noexcept
		{
			return false;
		}

		// clang-format off
		template<typename U, typename... Args>
		constexpr static bool is_compatible_object = std::ranges::range<U> && is_compatible_object_pair<std::ranges::range_value_t<U>, Args...>();
		template<typename U, typename... Args>
		constexpr static bool is_compatible_array = std::ranges::range<U> && std::constructible_from<basic_json_value, U, Args...> &&
				   !is_compatible_object_pair<std::ranges::range_value_t<U>, Args...>();
		// clang-format on

		template<typename U, typename... Args>
		constexpr static bool is_literal = is_compatible_string<U> || is_compatible_int<U> ||
										   is_compatible_float<U> || is_compatible_bool<U> ||
										   is_compatible_object<U, Args...> || is_compatible_array<U, Args...>;
		template<typename U, typename... Args>
		constexpr static bool is_serializable = serializable<U, basic_json_value, Args...>;

		template<typename U>
		[[nodiscard]] constexpr static json_type int_type() noexcept
		{
			static_assert(sizeof(U) <= sizeof(std::uint64_t), "Only integers up to 64-bits are supported.");

			json_type result = json_type::NUMBER_INT;

			/* Sign flag. */
			if constexpr (std::signed_integral<U>) result |= json_type::SIGN_FLAG;

			/* Bit width flags. */
			if constexpr (sizeof(U) <= sizeof(std::uint8_t))
				result |= json_type::BITS_8;
			else if constexpr (sizeof(U) <= sizeof(std::uint16_t))
				result |= json_type::BITS_16;
			else if constexpr (sizeof(U) <= sizeof(std::uint32_t))
				result |= json_type::BITS_32;
			else if constexpr (sizeof(U) <= sizeof(std::uint64_t))
				result |= json_type::BITS_64;
			return result;
		}
		template<typename U>
		[[nodiscard]] constexpr static json_type float_type() noexcept
		{
			static_assert(sizeof(U) <= sizeof(float64_t), "Only 32- & 64-bit floats are supported.");
			if constexpr (sizeof(U) <= sizeof(float32_t))
				return json_type::FLOAT_32;
			else
				return json_type::FLOAT_64;
		}

	public:
		/** Initializes an empty object value. */
		constexpr basic_json_value() noexcept : m_object() {}

		/** Initializes a null value. */
		constexpr basic_json_value(std::nullptr_t) noexcept : m_padding(), m_type(json_type::NULL_VALUE) {}

		// clang-format off
		/** Initializes a boolean value. */
		template<typename U>
		constexpr basic_json_value(U &&value) noexcept requires(is_compatible_bool<std::remove_cvref_t<U>>)
			: m_bool(value), m_type(json_type::BOOL) {}
		/** Initializes an integer number value. */
		template<typename U>
		constexpr basic_json_value(U &&value) noexcept requires(is_compatible_int<std::remove_cvref_t<U>>)
			: m_type(int_type<std::remove_cvref_t<U>>())
		{
			if constexpr (!std::is_signed_v<std::remove_cvref_t<U>>)
				m_uint = static_cast<std::uintmax_t>(value);
			else
				m_int = static_cast<std::intmax_t>(value);
		}
		/** Initializes a floating-point number value. */
		template<typename U>
		constexpr basic_json_value(U &&value) noexcept requires(is_compatible_float<std::remove_cvref_t<U>>)
			: m_type(float_type<std::remove_cvref_t<U>>())
		{
			if constexpr (sizeof(std::remove_cvref_t<U>) <= sizeof(float32_t))
				m_float32 = static_cast<float32_t>(value);
			else
				m_float64 = static_cast<float64_t>(value);
		}

		/** Initializes a string value. */
		template<typename U>
		constexpr basic_json_value(U &&value) noexcept(noexcept(string_type{std::forward<U>(value)}))
			requires(is_compatible_string<std::remove_cvref_t<U>>)
			: m_string(std::forward<U>(value)), m_type(json_type::STRING) {}

		/** Initializes a Json array.
		 * @param value Value passed to the array's constructor. */
		template<typename U>
		constexpr basic_json_value(U &&value) requires(is_compatible_array<std::remove_cvref_t<U>>)
			: m_array(std::forward<U>(value)), m_type(json_type::ARRAY)
		{
		}
		/** @copydoc basic_json_value
		 * @param alloc Allocator used for the array. */
		template<typename U>
		constexpr basic_json_value(U &&value, const array_allocator &alloc) requires(is_compatible_array<std::remove_cvref_t<U>>)
			: m_array(std::forward<U>(value), alloc), m_type(json_type::ARRAY)
		{
		}

		/** Initializes a Json object.
		 * @param value Value passed to the object's constructor. */
		template<typename U>
		constexpr basic_json_value(U &&value) requires(is_compatible_object<std::remove_cvref_t<U>>)
			: m_object(std::forward<U>(value)), m_type(json_type::OBJECT)
		{
		}
		/** @copydoc basic_json_value
		 * @param alloc Allocator used for the object. */
		template<typename U>
		constexpr basic_json_value(U &&value, const object_allocator &alloc) requires(is_compatible_object<std::remove_cvref_t<U>>)
			: m_object(std::forward<U>(value), alloc), m_type(json_type::OBJECT)
		{
		}

		/** Serialized an object into a Json value using either the member or ADL-selected `serialize` function. */
		template<typename U, typename... Args>
		constexpr basic_json_value(U &&value, Args &&...args)
			requires(is_serializable<std::remove_cvref_t<U>, Args...> &&
					 !is_literal<std::remove_cvref_t<U>, Args...>);
		// clang-format on

		/** Initializes a Json array from an initializer list of values.
		 * @param il Initializer list containing values of the array.
		 * @param alloc Allocator used for the array. */
		constexpr basic_json_value(std::initializer_list<typename array_type::value_type> il, const array_allocator &alloc = {})
			: m_array(il, alloc), m_type(json_type::ARRAY)
		{
		}
		/** Initializes a Json object from an initializer list of key-value pairs.
		 * @param il Initializer list containing key-value pairs of the object.
		 * @param alloc Allocator used for the object. */
		constexpr basic_json_value(std::initializer_list<typename object_type::value_type> il,
								   const object_allocator &alloc = {})
			: m_object(il, alloc), m_type(json_type::OBJECT)
		{
		}

		constexpr ~basic_json_value() { destroy_impl(); }

		/** Returns Json type of the contained value. If the value is a Json container (array or object),
		 * element type will be OR'ed with the container type. */
		[[nodiscard]] constexpr json_type type() const noexcept { return m_type; }

		/** Checks if the contained value is null. */
		[[nodiscard]] constexpr bool is_null() const noexcept { return m_type == json_type::NULL_VALUE; }
		/** Checks if the contained value is a boolean. */
		[[nodiscard]] constexpr bool is_bool() const noexcept { return m_type == json_type::BOOL; }
		/** Checks if the contained value is a string. */
		[[nodiscard]] constexpr bool is_string() const noexcept { return m_type == json_type::STRING; }

		/** Checks if the contained value is a number (integer or floating-point). */
		[[nodiscard]] constexpr bool is_number() const noexcept
		{
			return (m_type & json_type::NUMBER_MASK) != json_type{0} && !is_container();
		}

		/** Checks if the contained value is a signed integer. */
		[[nodiscard]] constexpr bool is_int() const noexcept
		{
			return (m_type & (json_type::NUMBER_MASK | json_type::SIGN_FLAG | json_type::CONTAINER_MASK)) ==
				   (json_type::SIGN_FLAG | json_type::NUMBER_INT);
		}
		/** Checks if the contained value is a signed 8-bit integer. */
		[[nodiscard]] constexpr bool is_int8() const noexcept { return m_type == json_type::INT_8; }
		/** Checks if the contained value is a signed 16-bit integer. */
		[[nodiscard]] constexpr bool is_int16() const noexcept { return m_type == json_type::INT_16; }
		/** Checks if the contained value is a signed 32-bit integer. */
		[[nodiscard]] constexpr bool is_int32() const noexcept { return m_type == json_type::INT_32; }
		/** Checks if the contained value is a signed 64-bit integer. */
		[[nodiscard]] constexpr bool is_int64() const noexcept { return m_type == json_type::INT_64; }

		/** Checks if the contained value is an unsigned integer. */
		[[nodiscard]] constexpr bool is_uint() const noexcept
		{
			return (m_type & (json_type::NUMBER_MASK | json_type::SIGN_FLAG | json_type::CONTAINER_MASK)) == json_type::NUMBER_INT;
		}
		/** Checks if the contained value is an unsigned 8-bit integer. */
		[[nodiscard]] constexpr bool is_uint8() const noexcept { return m_type == json_type::UINT_8; }
		/** Checks if the contained value is an unsigned 16-bit integer. */
		[[nodiscard]] constexpr bool is_uint16() const noexcept { return m_type == json_type::UINT_16; }
		/** Checks if the contained value is an unsigned 32-bit integer. */
		[[nodiscard]] constexpr bool is_uint32() const noexcept { return m_type == json_type::UINT_32; }
		/** Checks if the contained value is an unsigned 64-bit integer. */
		[[nodiscard]] constexpr bool is_uint64() const noexcept { return m_type == json_type::UINT_64; }

		/** Checks if the contained value is a floating-point number. */
		[[nodiscard]] constexpr bool is_float() const noexcept
		{
			return (m_type & (json_type::NUMBER_MASK | json_type::CONTAINER_MASK)) == json_type::NUMBER_FLOAT;
		}
		/** Checks if the contained value is a 32-bit floating-point number. */
		[[nodiscard]] constexpr bool is_float32() const noexcept { return m_type == json_type::FLOAT_32; }
		/** Checks if the contained value is a 64-bit floating-point number. */
		[[nodiscard]] constexpr bool is_float64() const noexcept { return m_type == json_type::FLOAT_64; }

		/** Checks if the contained value is a Json container (array or object). */
		[[nodiscard]] constexpr bool is_container() const noexcept
		{
			return (m_type & json_type::CONTAINER_MASK) != json_type{0};
		}
		/** Checks if the contained value is a Json array. */
		[[nodiscard]] constexpr bool is_array() const noexcept
		{
			return (m_type & json_type::CONTAINER_MASK) == json_type::ARRAY;
		}
		/** Checks if the contained value is a Json object. */
		[[nodiscard]] constexpr bool is_object() const noexcept
		{
			return (m_type & json_type::CONTAINER_MASK) == json_type::OBJECT;
		}

		/** If the contained value is a Json container (array or object), checks if the container is empty.
		 * If the contained value is not a Json container, returns `false`. */
		[[nodiscard]] constexpr bool empty() const noexcept
		{
			return is_array() ? m_array.empty() : is_object() && m_object.empty();
		}
		/** If the contained value is a Json container (array or object), returns it's size.
		 * If the contained value is not a Json container, returns `0`. */
		[[nodiscard]] constexpr size_type size() const noexcept
		{
			return is_array() ? m_array.size() : is_object() ? m_object.size() : 0;
		}

	private:
		constexpr void destroy_impl()
		{
			if (const auto container = m_type & json_type::CONTAINER_MASK; container == json_type::OBJECT)
				std::destroy_at(&m_object);
			else if (container == json_type::ARRAY)
				std::destroy_at(&m_array);
			else if (m_type == json_type::STRING)
				std::destroy_at(&m_string);
		}

		union
		{
			std::byte m_padding[sizeof(object_type)] = {};

			bool m_bool;

			std::intmax_t m_int;
			std::uintmax_t m_uint;

			float32_t m_float32;
			float64_t m_float64;

			string_type m_string;
			object_type m_object;
			array_type m_array;
		};

		/* Every json value starts as a dynamic object of size 0. */
		json_type m_type = json_type::OBJECT | json_type::DYNAMIC;
	};

	using json_value = basic_json_value<char>;
}	 // namespace sek::serialization