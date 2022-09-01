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
		/** Mask used to extract container type flags (either `OBJECT` or `ARRAY`). */
		CONTAINER_MASK = 0b1100'0000,
		/** Flag used to identify an object container type. Object element types are OR'ed with this flag. */
		OBJECT = 0b0100'0000,
		/** Flag used to identify an array container type. Array element types are OR'ed with this flag. */
		ARRAY = 0b1000'0000,

		/** Mask used to extract literal type bits. */
		LITERAL_MASK = 0b11'1111,

		/** Dynamic-type value entry.
		 * @note This type is mainly used by containers to indicate a non-fixed element type. */
		DYNAMIC = LITERAL_MASK,
		/** `null` Json value. */
		NULL_VALUE = 0,

		/** Boolean value. */
		BOOL = 0b000'0001,
		/** String value. */
		STRING = 0b000'0010,

		/** Mask used to extract value of the number type flags (either `NUMBER_FLOAT` or `NUMBER_INT`). */
		NUMBER_MASK = 0b11'0000,
		/** Floating-point type. Width of the floating-point number is OR'ed with this flag. */
		NUMBER_FLOAT = 0b010'0000,
		/** Integer type. Sign and width of the integer number are OR'ed with this flag. */
		NUMBER_INT = 0b001'0000,
		/** Flag indicating a signed integer. Must be OR'ed with `INT_FLAG` and width of the integer type. */
		SIGN_FLAG = 0b000'1000,

		/** Mask used to extract value of the integer of floating-point bit width value. */
		BITS_MASK = 0b000'0111,
		/** 8-bit numeric type. */
		BITS_8 = 0b000,
		/** 16-bit numeric type. */
		BITS_16 = 0b001,
		/** 32-bit numeric type. */
		BITS_32 = 0b010,
		/** 64-bit numeric type. */
		BITS_64 = 0b011,
#if 0
		/** 128-bit numeric type. */
		BITS_128 = 0b100,
		/** 256-bit numeric type. */
		BITS_256 = 0b101,
		/** 512-bit numeric type. */
		BITS_512 = 0b110,
#endif

		/** 8-bit signed integer type. */
		INT_8 = NUMBER_INT | SIGN_FLAG | BITS_8,
		/** 16-bit signed integer type. */
		INT_16 = NUMBER_INT | SIGN_FLAG | BITS_16,
		/** 32-bit signed integer type. */
		INT_32 = NUMBER_INT | SIGN_FLAG | BITS_32,
		/** 64-bit signed integer type. */
		INT_64 = NUMBER_INT | SIGN_FLAG | BITS_64,
		/** 8-bit unsigned integer type. */
		UINT_8 = NUMBER_INT | BITS_8,
		/** 16-bit unsigned integer type. */
		UINT_16 = NUMBER_INT | BITS_16,
		/** 32-bit unsigned integer type. */
		UINT_32 = NUMBER_INT | BITS_32,
		/** 64-bit unsigned integer type. */
		UINT_64 = NUMBER_INT | BITS_64,

		/** 32-bit floating-point type. */
		FLOAT_32 = NUMBER_FLOAT | BITS_32,
		/** 64-bit floating-point type. */
		FLOAT_64 = NUMBER_FLOAT | BITS_64,

#if 0 /* High-precision numbers are not supported. */
		/** High-precision floating-point type.
		 * @note If supported by the underlying Json-like format, high-precision floats are stored as strings. */
		FLOAT_HIGHP = NUMBER_FLOAT | BITS_HIGHP,
		/** High-precision (no bit width limit) numeric type. */
		BITS_HIGHP = 0b111,
#endif
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