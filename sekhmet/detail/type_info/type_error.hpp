//
// Created by switch_blade on 2022-10-03.
//

#pragma once

#include "fwd.hpp"
#include <system_error>

namespace sek
{
	/** @brief Exception thrown by the type reflection system on runtime errors. */
	class SEK_API type_error : public std::system_error
	{
	public:
		type_error(const type_error &) noexcept = default;

		type_error(std::error_code ec) : std::system_error(ec) {}
		type_error(std::error_code ec, const char *msg) : std::system_error(ec, msg) {}
		type_error(std::error_code ec, const std::string &msg) : std::system_error(ec, msg) {}

		type_error(int ev, const std::error_category &c) : std::system_error(ev, c) {}
		type_error(int ev, const std::error_category &c, const char *msg) : std::system_error(ev, c, msg) {}
		type_error(int ev, const std::error_category &c, const std::string &msg) : std::system_error(ev, c, msg) {}

		~type_error() override;
	};

	/** @brief Error code used to specify reflection errors. */
	enum class type_errc : int
	{
		/** Mask used to obtain `INVALID_PARAM` argument index. */
		PARAM_MASK = 0xff,
		/** Incorrect argument to a function. Index (`std::uint8_t`) of the invalid argument is OR'ed with the error code. */
		INVALID_PARAM = 0x100,

		/** Unexpected/invalid type. */
		INVALID_TYPE = 0x200,
		/** Unexpected/invalid type qualifier (ex. expected non-const but got const). */
		INVALID_QUALIFIER = INVALID_TYPE | 0x400,
		/** Requested attribute does not exist. */
		INVALID_ATTRIBUTE = INVALID_TYPE | 0x800,

		/** Requested member of a type does not exist. */
		INVALID_MEMBER = INVALID_TYPE | 0xa00,
		/** Requested member property of a type does not exist. */
		INVALID_PROPERTY = INVALID_MEMBER | 0x1000,
		/** Requested member function of a type does not exist. */
		INVALID_FUNCTION = INVALID_MEMBER | 0x2000,
		/** Requested constructor of a type does not exist. */
		INVALID_CONSTRUCTOR = INVALID_MEMBER | 0x4000,
		/** Requested enumeration of a type does not exist. */
		INVALID_ENUMERATION = INVALID_MEMBER | 0x8000,

		/** Provided `any` instance is not a reference. */
		EXPECTED_REF_ANY = 0xa000,
		/** Unexpected empty `any` instance. */
		UNEXPECTED_EMPTY_ANY = 0x1'0000,
	};

	[[nodiscard]] constexpr type_errc operator&(type_errc a, type_errc b)
	{
		return static_cast<type_errc>(static_cast<int>(a) & static_cast<int>(b));
	}
	[[nodiscard]] constexpr type_errc operator|(type_errc a, type_errc b)
	{
		return static_cast<type_errc>(static_cast<int>(a) | static_cast<int>(b));
	}
	[[nodiscard]] constexpr type_errc operator^(type_errc a, type_errc b)
	{
		return static_cast<type_errc>(static_cast<int>(a) ^ static_cast<int>(b));
	}
	[[nodiscard]] constexpr type_errc &operator&=(type_errc &a, type_errc b) { return a = a & b; }
	[[nodiscard]] constexpr type_errc &operator|=(type_errc &a, type_errc b) { return a = a | b; }
	[[nodiscard]] constexpr type_errc &operator^=(type_errc &a, type_errc b) { return a = a ^ b; }
	[[nodiscard]] constexpr type_errc operator~(type_errc e) { return static_cast<type_errc>(~static_cast<int>(e)); }

	[[nodiscard]] constexpr type_errc operator&(type_errc a, std::uint8_t b)
	{
		return static_cast<type_errc>(static_cast<int>(a) & static_cast<int>(b));
	}
	[[nodiscard]] constexpr type_errc operator|(type_errc a, std::uint8_t b)
	{
		return static_cast<type_errc>(static_cast<int>(a) | static_cast<int>(b));
	}
	[[nodiscard]] constexpr type_errc operator^(type_errc a, std::uint8_t b)
	{
		return static_cast<type_errc>(static_cast<int>(a) ^ static_cast<int>(b));
	}
	[[nodiscard]] constexpr type_errc &operator&=(type_errc &a, std::uint8_t b) { return a = a & b; }
	[[nodiscard]] constexpr type_errc &operator|=(type_errc &a, std::uint8_t b) { return a = a | b; }
	[[nodiscard]] constexpr type_errc &operator^=(type_errc &a, std::uint8_t b) { return a = a ^ b; }

	/** Returns a reference to `std::error_category` used for reflection errors. */
	[[nodiscard]] SEK_API const std::error_category &type_category() noexcept;

	/** Creates an instance of `std::error_code` from the specified `type_errc` value.
	 * Equivalent to `std::error_code{static_cast<int>(e), type_category()}`. */
	[[nodiscard]] std::error_code make_error_code(type_errc e) noexcept
	{
		return std::error_code{static_cast<int>(e), type_category()};
	}
}	 // namespace sek