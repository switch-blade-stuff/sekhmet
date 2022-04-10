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
#define LENGTH_OVERFLOW_MSG "UBJson: Data length out of int64 range"
#define WRITE_FAIL_MSG "UBJson: Failed to write serialized data"

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
			if (type & UBJ_FLOAT_MASK)
				return node{parse_float(type)};
			else if (type & UBJ_INT_MASK)
				return node{parse_int(type)};
			else if (type & UBJ_STRING_MASK)
			{
				if (type == UBJ_HIGHP) [[unlikely]]
				{
					if (auto hp_mode = mode & highp_mask; hp_mode == highp_throw)
						throw archive_error(HIGHP_ERROR_MSG);
					else if (hp_mode == highp_skip)
						return {};
				}
				return node{parse_string()};
			}
			else if (type & UBJ_BOOL_MASK)
				return node{static_cast<bool>(type & UBJ_BOOL_TRUE)};
			return {};
		}

		[[nodiscard]] node parse_binary(std::int64_t length) const
		{
			node::binary_type result(static_cast<std::size_t>(length));
			read_guarded(result.data(), result.size());
			return node{std::move(result)};
		}
		[[nodiscard]] node parse_array(std::int64_t length, ubj_type_t data_type) const
		{
			/* Interpret array of unsigned int as `binary_type` if `uint8_binary` mode is set. */
			if (data_type == UBJ_UINT8 && (mode & uint8_binary)) [[unlikely]]
				return parse_binary(length);

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

	void ubj_input_archive::init(parse_mode m, syntax stx) noexcept
	{
		constinit static const parse_func parse_table[] = {
			/* Spec 12. */
			+[](basic_parser *parser) { static_cast<parser_spec12 *>(parser)->parse(); },
		};

		parse = parse_table[static_cast<int>(stx)];
		mode = m;
	}
	void ubj_input_archive::do_read(node &n)
	{
		basic_parser state = {
			.reader = reader,
			.mode = mode,
			.result = n,
		};
		parse(&state);
	}

	void ubj_output_archive::basic_emitter::write_guarded(const void *src, std::size_t n) const
	{
		if (writer->write(src, n) != n) [[unlikely]]
			throw archive_error(WRITE_FAIL_MSG);
	}
	void ubj_output_archive::basic_emitter::write_token(char c) const { write_guarded(&c, sizeof(c)); }

	struct ubj_output_archive::emitter_spec12 : basic_emitter
	{
		[[nodiscard]] static ubj_type_t get_int_type(std::int64_t i)
		{
			if (i <= static_cast<node::int_type>(std::numeric_limits<std::int8_t>::max()))
				return UBJ_INT8;
			else if (i <= static_cast<node::int_type>(std::numeric_limits<std::uint8_t>::max()))
				return UBJ_UINT8;
			else if (i <= static_cast<node::int_type>(std::numeric_limits<std::uint16_t>::max()))
				return UBJ_INT16;
			else if (i <= static_cast<node::int_type>(std::numeric_limits<std::uint32_t>::max()))
				return UBJ_INT32;
			else
				return UBJ_INT64;
		}
		[[nodiscard]] static ubj_type_t get_float_type(double f)
		{
			if (f <= static_cast<node::float_type>(std::numeric_limits<float>::max()))
				return UBJ_FLOAT32;
			else
				return UBJ_FLOAT64;
		}
		[[nodiscard]] ubj_type_t get_node_type(const node &n) const
		{
			switch (n.state())
			{
				case node::state_type::EMPTY: return UBJ_NULL;
				case node::state_type::CHAR: return UBJ_CHAR;
				case node::state_type::BOOL: return n.as_bool() ? UBJ_BOOL_TRUE : UBJ_BOOL_FALSE;
				/*case node::state_type::NUMBER:*/
				case node::state_type::INT: return mode & best_fit ? get_int_type(n.as_int()) : UBJ_INT64;
				case node::state_type::FLOAT: return mode & best_fit ? get_float_type(n.as_float()) : UBJ_FLOAT64;
				case node::state_type::STRING: return UBJ_STRING;
				case node::state_type::BINARY: /* Array of int */
				case node::state_type::ARRAY: return UBJ_ARRAY;
				case node::state_type::TABLE: return UBJ_OBJECT;
			}
		}

		[[nodiscard]] constexpr bool do_fix_type() const noexcept { return (mode & fix_type) == fix_type; }
		[[nodiscard]] ubj_type_t get_array_type(const node::sequence_type &s) const
		{
			ubj_type_t type = UBJ_INVALID;
			if (std::all_of(s.begin(),
							s.end(),
							[&](auto &n)
							{
								auto node_type = get_node_type(n);
								if (type == UBJ_INVALID) [[unlikely]]
								{
									type = node_type;
									return true;
								}
								else
									return type == node_type;
							}))
				return type;
			else
				return UBJ_INVALID;
		}
		[[nodiscard]] ubj_type_t get_object_type(const node::table_type &t) const
		{
			ubj_type_t type = UBJ_INVALID;
			if (std::all_of(t.begin(),
							t.end(),
							[&](auto &p)
							{
								auto node_type = get_node_type(p.second);
								if (type == UBJ_INVALID) [[unlikely]]
								{
									type = node_type;
									return true;
								}
								else
									return type == node_type;
							}))
				return type;
			else
				return UBJ_INVALID;
		}

		template<typename T>
		void emit_literal(T value) const
		{
			write_guarded(&value, sizeof(T));
		}
		void emit_type_token(ubj_type_t type) const { write_token(ubj_spec12_token_table[type]); }

		void emit_int(std::int64_t i, ubj_type_t type) const
		{
			switch (type)
			{
				case UBJ_UINT8: emit_literal(static_cast<std::uint8_t>(i)); break;
				case UBJ_INT8: emit_literal(static_cast<std::int8_t>(i)); break;
				case UBJ_INT16: emit_literal(fix_endianness_write(static_cast<std::int16_t>(i))); break;
				case UBJ_INT32: emit_literal(fix_endianness_write(static_cast<std::int32_t>(i))); break;
				case UBJ_INT64: emit_literal(fix_endianness_write(i)); break;
			}
		}
		void emit_float(double f, ubj_type_t type) const
		{
			switch (type)
			{
				case UBJ_FLOAT32: emit_literal(fix_endianness_write(static_cast<float>(f))); break;
				case UBJ_FLOAT64: emit_literal(fix_endianness_write(f)); break;
			}
		}
		void emit_length(std::int64_t l) const
		{
			if (l < 0) [[unlikely]]
				throw archive_error(LENGTH_OVERFLOW_MSG);
			auto int_type = mode & best_fit ? get_int_type(l) : UBJ_INT64;
			emit_type_token(int_type);
			emit_int(l, int_type);
		}
		void emit_string(std::string_view s) const
		{
			emit_length(static_cast<std::int64_t>(s.size()));
			write_guarded(s.data(), s.size());
		}
		void emit_value(const node &n, ubj_type_t type) const
		{
			if (type == UBJ_CHAR)
				emit_literal(n.as_char());
			else if (type & UBJ_STRING_MASK)
				emit_string(n.as_string());
			else if (type & UBJ_FLOAT_MASK)
				emit_float(n.as_float(), type);
			else if (type & UBJ_INT_MASK)
				emit_int(n.as_int(), type);
		}

		void emit_fixed_type(ubj_type_t type) const
		{
			write_token('$');
			emit_type_token(type);
		}
		void emit_fixed_length(std::int64_t l) const
		{
			write_token('#');
			emit_length(l);
		}

		void emit_binary(const node::binary_type &b) const
		{
			emit_fixed_type(UBJ_UINT8);
			emit_fixed_length(static_cast<std::int64_t>(b.size()));
			write_guarded(b.data(), b.size());
		}
		void emit_array(const node::sequence_type &s) const
		{
			if (ubj_type_t type; do_fix_type() && (type = get_array_type(s)) != UBJ_INVALID) [[unlikely]]
			{
				/* Fixed-type & fixed-size array. */
				emit_fixed_type(type);
				emit_fixed_length(static_cast<std::int64_t>(s.size()));
				for (auto &item : s) emit_node(item, type);
			}
			else if (mode & fix_size) [[likely]]
			{
				/* Fixed-size array. */
				emit_fixed_length(static_cast<std::int64_t>(s.size()));
				for (auto &item : s) emit_node(item);
			}
			else
			{
				/* Fully dynamic array. */
				for (auto &item : s) emit_node(item);
				write_token(']');
			}
		}
		void emit_object(const node::table_type &t) const
		{
			if (ubj_type_t type; do_fix_type() && (type = get_object_type(t)) != UBJ_INVALID) [[unlikely]]
			{
				/* Fixed-type & fixed-size object. */
				emit_fixed_type(type);
				emit_fixed_length(static_cast<std::int64_t>(t.size()));
				for (auto &item : t)
				{
					emit_string(item.first);
					emit_node(item.second, type);
				}
			}
			else if (mode & fix_size) [[likely]]
			{
				/* Fixed-size object. */
				emit_fixed_length(static_cast<std::int64_t>(t.size()));
				for (auto &item : t)
				{
					emit_string(item.first);
					emit_node(item.second);
				}
			}
			else
			{
				/* Fully dynamic object. */
				for (auto &item : t)
				{
					emit_string(item.first);
					emit_node(item.second);
				}
				write_token('}');
			}
		}

		void emit_node(const node &n, ubj_type_t type) const
		{
			if (type == UBJ_ARRAY)
			{
				if (n.is_binary()) [[unlikely]]
					emit_binary(n.as_binary());
				else
					emit_array(n.as_sequence());
			}
			else if (type == UBJ_OBJECT)
				emit_object(n.as_table());
			else
				emit_value(n, type);
		}
		void emit_node(const node &n) const
		{
			auto type = get_node_type(n);
			emit_type_token(type);
			emit_node(n, type);
		}
		void emit() const { emit_node(data); }
	};

	void ubj_output_archive::init(emit_mode m, syntax stx) noexcept
	{
		constinit static const emit_func emit_table[] = {
			/* Spec 12. */
			+[](basic_emitter *emitter) { static_cast<emitter_spec12 *>(emitter)->emit(); },
		};

		emit = emit_table[static_cast<int>(stx)];
		mode = m;
	}
	void ubj_output_archive::do_write(const node &n)
	{
		basic_emitter state = {
			.writer = writer,
			.mode = mode,
			.data = n,
		};
		emit(&state);
	}
}	 // namespace sek::adt
