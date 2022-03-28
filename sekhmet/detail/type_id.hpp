//
// Created by switchblade on 2022-01-23.
//

#pragma once

#include <cstddef>

#include "define.h"
#include "hash.hpp"
#include "static_string.hpp"
#include <string_view>

namespace sek::detail
{
	template<basic_static_string NameString>
	consteval auto generate_type_name() noexcept;

	template<typename T>
	[[nodiscard]] constexpr auto generate_type_name() noexcept
	{
		return generate_type_name<SEK_PRETTY_FUNC>();
	}

	template<basic_static_string Src, std::size_t J, std::size_t I, std::size_t Last, std::size_t N, typename Fmt>
	consteval auto format_type_name(Fmt f, basic_static_string<char, N> result) noexcept
	{
		if constexpr (I == Last)
		{
			result[J] = '\0';
			return result;
		}
		else if constexpr (constexpr auto Skip = f(I); Skip != 0)
			return format_type_name<Src, J, I + Skip, Last>(f, result);
		else
		{
			result[J] = static_cast<typename basic_static_string<char, N>::value_type>(Src[I]);
			return format_type_name<Src, J + 1, I + 1, Last>(f, result);
		}
	}
	template<basic_static_string Src, std::size_t J, std::size_t I, std::size_t Last, std::size_t N, typename Fmt>
	consteval auto format_type_name(Fmt f) noexcept
	{
		return format_type_name<Src, J, I, Last, N>(f, {});
	}
	template<basic_static_string Src, std::size_t J, std::size_t I, std::size_t Last, std::size_t N>
	consteval auto format_type_name() noexcept
	{
		return format_type_name<Src, J, I, Last, N>([](std::size_t) -> std::size_t { return 0; });
	}
}	 // namespace sek::detail

#if defined(__clang__) || defined(__GNUC__)

#include "gcc/type_name.hpp"

#elif defined(_MSC_VER)

#include "msvc/type_name.hpp"

#else

#error "Implement type name generation for this compiler"

#endif

namespace sek::detail
{
	/** Returns name of the specified type.
	 * @note If the type was not declared using `SEK_DECLARE_TYPE`, type name will be generated using compiler-specific method.
	 * @warning Consistency of generated type names across different compilers is not guaranteed. */
	template<typename T>
	[[nodiscard]] constexpr std::string_view type_name() noexcept
	{
		constexpr auto &name = auto_constant<generate_type_name<T>()>::value;
		return std::string_view{name.begin(), name.end()};
	}
	/** Returns hash of the specified type's name. */
	template<typename T>
	[[nodiscard]] constexpr std::size_t type_hash() noexcept
	{
		constexpr auto name = type_name<T>();
		return fnv1a(name.data(), name.size());
	}

	/** @brief Structure used to identify a type. */
	class type_id
	{
	public:
		/** Returns a type id instance for the specified type. Equivalent to `type_id{type_name<T>()}`.
		 * @tparam T Type to create id for. */
		template<typename T>
		[[nodiscard]] constexpr static type_id identify() noexcept
		{
			return type_id{type_name<T>(), type_hash<T>()};
		}

	private:
		constexpr explicit type_id(std::string_view n, std::size_t h) noexcept : name_value(n), hash_value(h) {}

	public:
		type_id() = delete;
		/** Initializes a type id from a type name.
		 * @param sv String view containing the type name. */
		constexpr type_id(std::string_view sv) noexcept : type_id(sv, detail::fnv1a(sv.data(), sv.size())) {}

		/** Returns name of the type. */
		[[nodiscard]] constexpr std::string_view name() const noexcept { return name_value; }
		/** Returns hash of the type. */
		[[nodiscard]] constexpr std::size_t hash() const noexcept { return hash_value; }

		[[nodiscard]] constexpr auto operator<=>(const type_id &other) const noexcept
		{
			return name_value <=> other.name_value;
		}
		[[nodiscard]] constexpr bool operator==(const type_id &other) const noexcept
		{
			return name_value == other.name_value;
		}

	private:
		std::string_view name_value;
		std::size_t hash_value;
	};

	[[nodiscard]] constexpr hash_t hash(const type_id &tid) noexcept { return tid.hash(); }
}	 // namespace sek::detail

template<>
struct std::hash<sek::detail::type_id>
{
	[[nodiscard]] constexpr sek::detail::hash_t operator()(const sek::detail::type_id &tid) const noexcept
	{
		return sek::detail::hash(tid);
	}
};
