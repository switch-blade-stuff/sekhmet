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

		typedef std::vector<std::byte, Alloc<std::byte>> binary_type;
		typedef std::basic_string<C, T, Alloc<C>> string_type;
		typedef std::basic_string_view<C, T> string_view_type;
		typedef string_type key_type;

		typedef basic_json_value value_type;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

	private:
		/* TODO: Use a better way to represent 32 & 64-bit floats. */
		using float32_t = float;
		using float64_t = double;

		template<typename U>
		[[nodiscard]] constexpr static bool compatible_bool() noexcept
		{
			return std::is_convertible_v<U, bool> && !std::integral<U>;
		}
		template<typename U>
		[[nodiscard]] constexpr static bool compatible_int() noexcept
		{
			return std::integral<U>;
		}
		template<typename U>
		[[nodiscard]] constexpr static bool compatible_float() noexcept
		{
			return std::floating_point<U>;
		}
		template<typename U>
		[[nodiscard]] constexpr static bool compatible_string() noexcept
		{
			return std::is_constructible_v<string_type, U>;
		}
		template<typename U>
		[[nodiscard]] constexpr static bool compatible_binary() noexcept
		{
			return std::is_constructible_v<binary_type, U>;
		}

		template<detail::pair_like U, typename... Args>
		[[nodiscard]] constexpr static bool compatible_object_pair() noexcept
		{
			using first_t = std::remove_cvref_t<decltype(std::declval<U>().first)>;
			using second_t = std::remove_cvref_t<decltype(std::declval<U>().second)>;

			return compatible_string<first_t>() && std::constructible_from<value_type, second_t, Args...>;
		}
		template<typename U, typename... Args>
		[[nodiscard]] constexpr static bool compatible_object_pair() noexcept
		{
			return false;
		}
		template<typename U, typename... Args>
		[[nodiscard]] constexpr static bool compatible_object() noexcept
		{
			return std::ranges::range<U> && compatible_object_pair<std::ranges::range_value_t<U>, Args...>();
		}
		template<typename U, typename... Args>
		[[nodiscard]] constexpr static bool compatible_array() noexcept
		{
			return std::ranges::range<U> && std::constructible_from<value_type, U, Args...> &&
				   !compatible_object_pair<std::ranges::range_value_t<U>, Args...>();
		}

		template<typename U, typename... Args>
		[[nodiscard]] constexpr static bool is_serializable() noexcept
		{
			return serializable<U, basic_json_value, Args...>;
		}

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

		using object_type = ordered_map<key_type, basic_json_value, Alloc<std::pair<const key_type, basic_json_value>>>;
		using array_type = std::vector<basic_json_value, Alloc<basic_json_value>>;

	public:
		/** Initializes an empty object value. */
		constexpr basic_json_value() noexcept : m_object() {}

		/** Initializes a null value. */
		constexpr basic_json_value(std::nullptr_t) noexcept : m_padding(), m_type(json_type::NULL_VALUE) {}

		// clang-format off
		/** Initializes a boolean value. */
		template<typename U>
		constexpr basic_json_value(U &&value) noexcept requires(compatible_bool<std::remove_cvref_t<U>>())
			: m_bool(value), m_type(json_type::BOOL) {}
		/** Initializes an integer number value. */
		template<typename U>
		constexpr basic_json_value(U &&value) noexcept requires(compatible_int<std::remove_cvref_t<U>>())
			: m_type(int_type<std::remove_cvref_t<U>>())
		{
			if constexpr (!std::is_signed_v<std::remove_cvref_t<U>>)
				m_uint = static_cast<std::uintmax_t>(value);
			else
				m_int = static_cast<std::intmax_t>(value);
		}
		/** Initializes a floating-point number value. */
		template<typename U>
		constexpr basic_json_value(U &&value) noexcept requires(compatible_float<std::remove_cvref_t<U>>())
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
			requires(compatible_string<std::remove_cvref_t<U>>())
			: m_string(std::forward<U>(value)), m_type(json_type::STRING) {}
		/** Initializes a binary value. */
		template<typename U>
		constexpr basic_json_value(U &&value) noexcept(noexcept(binary_type{std::forward<U>(value)}))
			requires(compatible_binary<std::remove_cvref_t<U>>())
			: m_binary(std::forward<U>(value)), m_type(json_type::BINARY) {}

		/** Initializes a Json array. */
		template<typename U>
		constexpr basic_json_value(U &&value) requires(compatible_array<std::remove_cvref_t<U>>());
		/** Initializes a Json object. */
		template<typename U>
		constexpr basic_json_value(U &&value) requires(compatible_object<std::remove_cvref_t<U>>());

		// clang-format on

		constexpr ~basic_json_value() { destroy_impl(); }

	private:
		constexpr void destroy_impl()
		{
			if (const auto container = m_type & json_type::CONTAINER_MASK; container == json_type::OBJECT)
				std::destroy_at(&m_object);
			else if (container == json_type::ARRAY)
				std::destroy_at(&m_array);
			else if (m_type == json_type::STRING)
				std::destroy_at(&m_string);
			else if (m_type == json_type::BINARY)
				std::destroy_at(&m_binary);
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
			binary_type m_binary;
			object_type m_object;
			array_type m_array;
		};

		/* Every json value starts as a dynamic object of size 0. */
		json_type m_type = json_type::OBJECT | json_type::DYNAMIC;
	};

	using json_value = basic_json_value<char>;
}	 // namespace sek::serialization