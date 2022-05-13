//
// Created by switchblade on 2022-01-23.
//

#pragma once

#include "hash.hpp"
#include "type_util.hpp"

namespace sek
{
	/** @brief Structure used to identify a type. */
	class type_id
	{
	public:
		/** Returns a type id instance for the specified type. Equivalent to `type_id{type_name<T>()}`.
		 * @tparam T Type to create id for. */
		template<typename T>
		[[nodiscard]] constexpr static type_id get() noexcept
		{
			return type_id{type_name<T>()};
		}

	private:
		constexpr explicit type_id(std::string_view n, std::size_t h) noexcept : name_value(n), hash_value(h) {}

	public:
		type_id() = delete;
		/** Initializes a type id from a type name.
		 * @param sv String view containing the type name. */
		constexpr type_id(std::string_view sv) noexcept : type_id(sv, fnv1a(sv.data(), sv.size())) {}

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

	namespace literals
	{
		[[nodiscard]] constexpr type_id operator""_tid(const char *str, std::size_t len) noexcept
		{
			return type_id{{str, len}};
		}
	}	 // namespace literals
}	 // namespace sek

template<>
struct std::hash<sek::type_id>
{
	[[nodiscard]] constexpr sek::hash_t operator()(const sek::type_id &tid) const noexcept { return sek::hash(tid); }
};
