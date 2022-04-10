//
// Created by switchblade on 2022-04-08.
//

#include "ubj_archive.hpp"

#include <bit>

#ifdef SEK_OS_LINUX

#include <byteswap.h>

#else
#ifdef _MSC_VER

#include <cstdlib>
#define bswap_32(x) _byteswap_ulong(x)
#define bswap_64(x) _byteswap_uint64(x)

#elif SEK_OS_APPLE

#include <libkern/OSByteOrder.h>
#define bswap_32(x) OSSwapInt32(x)
#define bswap_64(x) OSSwapInt64(x)

#elif defined(__sun) || defined(sun)

#include <sys/byteorder.h>
#define bswap_32(x) BSWAP_32(x)
#define bswap_64(x) BSWAP_64(x)

#elif defined(__FreeBSD__)

#include <sys/endian.h>
#define bswap_32(x) bswap32(x)
#define bswap_64(x) bswap64(x)

#elif defined(__OpenBSD__)

#include <sys/types.h>
#define bswap_32(x) swap32(x)
#define bswap_64(x) swap64(x)

#elif defined(__NetBSD__)

#include <machine/bswap.h>
#include <sys/types.h>
#if defined(__BSWAP_RENAME) && !defined(__bswap_32)
#define bswap_32(x) bswap32(x)
#define bswap_64(x) bswap64(x)
#endif

#endif

#ifndef bswap_16
constexpr static uint16_t bswap_16(uint16_t value)
{
	auto *data = std::bit_cast<uint8_t *>(&value);
	std::swap(data[0], data[1]);
	return value;
}
#endif
#endif

#include "ubj_spec12_types.h"

#define EOF_ERROR_MSG "UBJson: Premature EOF"
#define HIGHP_ERROR_MSG "UBJson: High-precision number support disabled"
#define BAD_DATA_MSG "UBJson: Invalid input, expected value or container data"
#define BAD_LENGTH_MSG "UBJson: Invalid input, expected length"

namespace sek::adt
{
#ifndef SEK_ARCH_BIG_ENDIAN

	template<typename T>
	constexpr static T fix_endianness_read(T value)
	{
		if constexpr (sizeof(T) == sizeof(std::uint16_t))
			return std::bit_cast<T>(bswap_16(std::bit_cast<std::uint16_t>(value)));
		else if constexpr (sizeof(T) == sizeof(std::uint32_t))
			return std::bit_cast<T>(bswap_32(std::bit_cast<std::uint32_t>(value)));
		else if constexpr (sizeof(T) == sizeof(std::uint64_t))
			return std::bit_cast<T>(bswap_64(std::bit_cast<std::uint64_t>(value)));
	}
	template<typename T>
	constexpr static T fix_endianness_write(T value)
	{
		return value;
	}

#else

	template<typename T>
	constexpr static T fix_endianness_read(T value)
	{
		return value;
	}
	template<typename T>
	constexpr static T fix_endianness_write(T value)
	{
		if constexpr (sizeof(T) == sizeof(std::uint16_t))
			return std::bit_cast<T>(bswap_16(std::bit_cast<std::uint16_t>(value)));
		else if constexpr (sizeof(T) == sizeof(std::uint32_t))
			return std::bit_cast<T>(bswap_32(std::bit_cast<std::uint32_t>(value)));
		else if constexpr (sizeof(T) == sizeof(std::uint64_t))
			return std::bit_cast<T>(bswap_64(std::bit_cast<std::uint64_t>(value)));
	}

#endif

	void ubj_input_archive::basic_parser::read_guarded(void *dest, std::size_t n) const
	{
		if (reader->read(dest, n) != n) [[unlikely]]
			throw archive_error(EOF_ERROR_MSG);
	}
	void ubj_input_archive::basic_parser::bump_guarded(std::size_t n) const
	{
		if (reader->bump(n) != n) [[unlikely]]
			throw archive_error(EOF_ERROR_MSG);
	}
	char ubj_input_archive::basic_parser::read_token() const
	{
		char token;
		read_guarded(&token, sizeof(token));
		return token;
	}
	char ubj_input_archive::basic_parser::peek_token() const
	{
		if (int token = reader->peek(); token == std::istream::traits_type::eof()) [[unlikely]]
			throw archive_error(EOF_ERROR_MSG);
		else
			return static_cast<char>(token);
	}

	struct ubj_input_archive::parser_spec12 : basic_parser
	{
		static ubj_type_t assert_type_token(char token)
		{
			if (auto type = ubj_spec12_type_table[token]; token == UBJ_INVALID) [[unlikely]]
				throw archive_error(BAD_DATA_MSG);
			else
				return type;
		}

		template<typename T>
		[[nodiscard]] T read_literal() const
		{
			T result;
			read_guarded(&result, sizeof(T));
			return result;
		}
		[[nodiscard]] ubj_type_t read_type_token() const { return assert_type_token(read_token()); }

		[[nodiscard]] double parse_float(ubj_type_t type) const
		{
			switch (type)
			{
				case UBJ_FLOAT32: return fix_endianness_read(read_literal<float>());
				case UBJ_FLOAT64: return fix_endianness_read(read_literal<double>());
			}
			/* Other cases are handled upstream. */
		}
		[[nodiscard]] std::int64_t parse_int(ubj_type_t type) const
		{
			switch (type)
			{
				case UBJ_UINT8: return static_cast<std::int64_t>(read_literal<std::uint8_t>());
				case UBJ_INT8: return static_cast<std::int64_t>(read_literal<std::int8_t>());
				case UBJ_INT16: return static_cast<std::int64_t>(fix_endianness_read(read_literal<std::int16_t>()));
				case UBJ_INT32: return static_cast<std::int64_t>(fix_endianness_read(read_literal<std::int32_t>()));
				case UBJ_INT64: return fix_endianness_read(read_literal<std::int64_t>());
			}
			/* Other cases are handled upstream. */
		}
		[[nodiscard]] std::int64_t parse_length() const
		{
			if (auto token = ubj_spec12_type_table[read_token()]; !(token & UBJ_INT_MASK)) [[unlikely]]
				throw archive_error(BAD_LENGTH_MSG);
			else
				return parse_int(token);
		}
		[[nodiscard]] std::string parse_string() const
		{
			/* Allocate a string of 0s & read its characters from input. */
			std::string result(static_cast<std::size_t>(parse_length()), '\0');
			read_guarded(result.data(), result.size());
			return result;
		}
		[[nodiscard]] node parse_value(ubj_type_t type) const
		{
			if (type & UBJ_BOOL_MASK)
				return node{static_cast<bool>(type & UBJ_BOOL_TRUE)};
			else if (type & UBJ_INT_MASK)
				return node{parse_int(type)};
			else if (type & UBJ_FLOAT_MASK)
				return node{parse_float(type)};
			else if (type & UBJ_STRING_MASK)
			{
				if (type == UBJ_HIGHP) [[unlikely]]
				{
					if (hp_mode == highp_mode::THROW)
						throw archive_error(HIGHP_ERROR_MSG);
					else if (hp_mode == highp_mode::SKIP)
						return {};
				}
				return node{parse_string()};
			}
			return {};
		}

		[[nodiscard]] node parse_array(std::int64_t length, ubj_type_t data_type) const
		{
			node::sequence_type result;

			if (length < 0) /* Dynamic-size array. */
				for (;;)
				{
					auto token = read_token();
					if (token == ']') [[unlikely]]
						break;
					result.push_back(parse_node(assert_type_token(token)));
				}
			else
			{
				result.reserve(static_cast<std::size_t>(length));
				if (data_type == UBJ_INVALID) /* Dynamic-type array. */
					while (length-- > 0) result.push_back(parse_node());
				else
					while (length-- > 0) result.push_back(parse_node(data_type));
			}

			return node{std::move(result)};
		};
		[[nodiscard]] node parse_object(std::int64_t length, ubj_type_t data_type) const
		{
			node::table_type result;
			auto parse_item_dynamic = [&]()
			{
				auto key = parse_string();
				auto value = parse_node();
				result.emplace(std::move(key), std::move(value));
			};

			if (length < 0) /* Dynamic-size object. */
				for (;;)
				{
					auto token = peek_token();
					if (token == '}') [[unlikely]]
					{
						bump_guarded(1);
						break;
					}
					parse_item_dynamic();
				}
			else
			{
				result.reserve(static_cast<std::size_t>(length));
				if (data_type == UBJ_INVALID) /* Dynamic-type object. */
					while (length-- > 0) parse_item_dynamic();
				else
					while (length-- > 0)
					{
						auto key = parse_string();
						auto value = parse_node(data_type);
						result.emplace(std::move(key), std::move(value));
					}
			}

			return node{std::move(result)};
		}
		[[nodiscard]] node parse_container(ubj_type_t type) const
		{
			/* Read length & data type if available. */
			std::int64_t length = -1;
			ubj_type_t data_type = UBJ_INVALID;
			switch (peek_token())
			{
				case '$':
				{
					bump_guarded(1);
					data_type = read_type_token();

					/* Always expect length after a type. */
					if (peek_token() != '#') [[unlikely]]
						throw archive_error(BAD_LENGTH_MSG);
				}
				case '#':
				{
					bump_guarded(1);
					length = parse_length();
				}
				default: break;
			}

			/* Parse the appropriate container. */
			if (type == UBJ_ARRAY)
				return parse_array(length, data_type);
			else
				return parse_object(length, data_type);
		}

		[[nodiscard]] node parse_node(ubj_type_t type) const
		{
			if (type & ubj_type_t::UBJ_CONTAINER_MASK)
				return parse_container(type);
			else
				return parse_value(type);
		}
		[[nodiscard]] node parse_node() const { return parse_node(read_type_token()); }

		void parse() const { result = parse_node(); }
	};

	void ubj_input_archive::init(highp_mode hpm, syntax stx) noexcept
	{
		constinit static const parse_func parse_table[] = {
			/* Spec 12. */
			+[](basic_parser *parser) { static_cast<parser_spec12 *>(parser)->parse(); },
		};

		parse = parse_table[static_cast<int>(stx)];
		hp_mode = hpm;
	}
	void ubj_input_archive::do_read(node &n)
	{
		basic_parser state = {
			.reader = reader,
			.hp_mode = hp_mode,
			.result = n,
		};
		parse(&state);
	}
}	 // namespace sek::adt
