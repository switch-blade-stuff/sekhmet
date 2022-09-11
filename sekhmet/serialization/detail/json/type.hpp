/*
 * Created by switchblade on 22/08/22
 */

#pragma once

#include "fwd.hpp"
#include <fmt/format.h>

namespace sek::serialization
{
	/** @brief Enumeration used to identify types of a Json-like format. */
	enum class json_type : std::uint16_t
	{
		/** `null` (empty) Json value type. */
		NULL_VALUE = 0,
		/** Boolean value type. */
		BOOL = 0b0001,
		/** String value type. */
		STRING = 0b0010,

		/** Flag used to identify a numeric value type. */
		NUMBER_FLAG = 0b1000,
		/** Signed integer number type. */
		INT = NUMBER_FLAG | 0b0010,
		/** Unsigned integer number type. */
		UINT = NUMBER_FLAG | 0b0011,
		/** Floating-point number type. */
		FLOAT = NUMBER_FLAG | 0b0100,

		/** Flag used to identify Json container types. */
		CONTAINER_FLAG = 0b1'0000,
		/** Json object container type. */
		TABLE = CONTAINER_FLAG | 0b0001,
		/** Json array container type. */
		ARRAY = CONTAINER_FLAG | 0b0010,
	};

	[[nodiscard]] constexpr json_type operator~(json_type lhs) noexcept
	{
		return static_cast<json_type>(~static_cast<std::uint16_t>(lhs));
	}
	[[nodiscard]] constexpr json_type operator|(json_type lhs, json_type rhs) noexcept
	{
		return static_cast<json_type>(static_cast<std::uint16_t>(lhs) | static_cast<std::uint16_t>(rhs));
	}
	[[nodiscard]] constexpr json_type operator&(json_type lhs, json_type rhs) noexcept
	{
		return static_cast<json_type>(static_cast<std::uint16_t>(lhs) & static_cast<std::uint16_t>(rhs));
	}
	[[nodiscard]] constexpr json_type operator^(json_type lhs, json_type rhs) noexcept
	{
		return static_cast<json_type>(static_cast<std::uint16_t>(lhs) ^ static_cast<std::uint16_t>(rhs));
	}
	constexpr json_type &operator|=(json_type &lhs, json_type rhs) noexcept
	{
		lhs = lhs | rhs;
		return lhs;
	}
	constexpr json_type &operator&=(json_type &lhs, json_type rhs) noexcept
	{
		lhs = lhs & rhs;
		return lhs;
	}
	constexpr json_type &operator^=(json_type &lhs, json_type rhs) noexcept
	{
		lhs = lhs ^ rhs;
		return lhs;
	}
}	 // namespace sek::serialization