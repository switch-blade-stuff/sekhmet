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

	void ubj_input_archive::parse_state::read_guarded(void *dest, std::size_t n) const
	{
		if (reader->read(dest, n) != n) [[unlikely]]
			throw archive_error(EOF_ERROR_MSG);
	}
	void ubj_input_archive::parse_state::bump_guarded(std::size_t n) const
	{
		if (reader->bump(n) != n) [[unlikely]]
			throw archive_error(EOF_ERROR_MSG);
	}
	char ubj_input_archive::parse_state::read_token() const
	{
		char token;
		read_guarded(&token, sizeof(token));
		return token;
	}
	char ubj_input_archive::parse_state::peek_token() const
	{
		if (int token = reader->peek(); token == std::istream::traits_type::eof()) [[unlikely]]
			throw archive_error(EOF_ERROR_MSG);
		else
			return static_cast<char>(token);
	}

	struct ubj_input_archive::parser_spec12
	{
		static ubj_type_t assert_type_token(char token)
		{
			if (auto type = ubj_spec12_type_table[token]; token == UBJ_INVALID) [[unlikely]]
				throw archive_error(BAD_DATA_MSG);
			else
				return type;
		}

		template<typename T>
		[[nodiscard]] static T read_literal(const parse_state &s)
		{
			T result;
			s.read_guarded(&result, sizeof(T));
			return result;
		}
		[[nodiscard]] static ubj_type_t read_type_token(const parse_state &s)
		{
			return assert_type_token(s.read_token());
		}

		[[nodiscard]] static double parse_float(const parse_state &s, ubj_type_t type)
		{
			switch (type)
			{
				case UBJ_FLOAT32: return fix_endianness_read(read_literal<float>(s));
				case UBJ_FLOAT64: return fix_endianness_read(read_literal<double>(s));
			}
			/* Other cases are handled upstream. */
		}
		[[nodiscard]] static std::int64_t parse_int(const parse_state &s, ubj_type_t type)
		{
			switch (type)
			{
				case UBJ_UINT8: return static_cast<std::int64_t>(read_literal<std::uint8_t>(s));
				case UBJ_INT8: return static_cast<std::int64_t>(read_literal<std::int8_t>(s));
				case UBJ_INT16: return static_cast<std::int64_t>(fix_endianness_read(read_literal<std::int16_t>(s)));
				case UBJ_INT32: return static_cast<std::int64_t>(fix_endianness_read(read_literal<std::int32_t>(s)));
				case UBJ_INT64: return fix_endianness_read(read_literal<std::int64_t>(s));
			}
			/* Other cases are handled upstream. */
		}
		[[nodiscard]] static std::int64_t parse_length(const parse_state &s)
		{
			if (auto token = ubj_spec12_type_table[s.read_token()]; !(token & UBJ_INT_MASK)) [[unlikely]]
				throw archive_error(BAD_LENGTH_MSG);
			else
				return parse_int(s, token);
		}
		[[nodiscard]] static std::string parse_string(const parse_state &s)
		{
			/* Allocate a string of 0s & read its characters from input. */
			std::string result(static_cast<std::size_t>(parse_length(s)), '\0');
			s.read_guarded(result.data(), result.size());
			return result;
		}
		[[nodiscard]] static node parse_value(const parse_state &s, ubj_type_t type)
		{
			if (type & UBJ_FLOAT_MASK)
				return node{parse_float(s, type)};
			else if (type & UBJ_INT_MASK)
				return node{parse_int(s, type)};
			else if (type & UBJ_STRING_MASK)
			{
				if (type == UBJ_HIGHP) [[unlikely]]
				{
					if (auto hp_mode = s.mode & highp_mask; hp_mode == highp_throw)
						throw archive_error(HIGHP_ERROR_MSG);
					else if (hp_mode == highp_skip)
						return {};
				}
				return node{parse_string(s)};
			}
			else if (type & UBJ_BOOL_MASK)
				return node{static_cast<bool>(type & UBJ_BOOL_TRUE)};
			return {};
		}

		[[nodiscard]] static node parse_binary(const parse_state &s, std::int64_t length)
		{
			node::binary_type result(static_cast<std::size_t>(length));
			s.read_guarded(result.data(), result.size());
			return node{std::move(result)};
		}
		[[nodiscard]] static node parse_array(const parse_state &s, std::int64_t length, ubj_type_t data_type)
		{
			/* Interpret array of unsigned int as `binary_type` if `uint8_binary` mode is set. */
			if (data_type == UBJ_UINT8 && (s.mode & uint8_binary)) [[unlikely]]
				return parse_binary(s, length);

			node::sequence_type result;
			if (length < 0) /* Dynamic-size array. */
				for (;;)
				{
					auto token = s.read_token();
					if (token == ']') [[unlikely]]
						break;
					result.push_back(parse_node(s, assert_type_token(token)));
				}
			else
			{
				result.reserve(static_cast<std::size_t>(length));
				if (data_type == UBJ_INVALID) /* Dynamic-type array. */
					while (length-- > 0) result.push_back(parse_node(s));
				else
					while (length-- > 0) result.push_back(parse_node(s, data_type));
			}

			return node{std::move(result)};
		};
		[[nodiscard]] static node parse_object(const parse_state &s, std::int64_t length, ubj_type_t data_type)
		{
			node::table_type result;
			auto parse_item_dynamic = [&]()
			{
				auto key = parse_string(s);
				auto value = parse_node(s);
				result.emplace(std::move(key), std::move(value));
			};

			if (length < 0) /* Dynamic-size object. */
				for (;;)
				{
					auto token = s.peek_token();
					if (token == '}') [[unlikely]]
					{
						s.bump_guarded(1);
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
						auto key = parse_string(s);
						auto value = parse_node(s, data_type);
						result.emplace(std::move(key), std::move(value));
					}
			}

			return node{std::move(result)};
		}
		[[nodiscard]] static node parse_container(const parse_state &s, ubj_type_t type)
		{
			/* Read length & data type if available. */
			std::int64_t length = -1;
			ubj_type_t data_type = UBJ_INVALID;
			switch (s.peek_token())
			{
				case '$':
				{
					s.bump_guarded(1);
					data_type = read_type_token(s);

					/* Always expect length after a type. */
					if (s.peek_token() != '#') [[unlikely]]
						throw archive_error(BAD_LENGTH_MSG);
				}
				case '#':
				{
					s.bump_guarded(1);
					length = parse_length(s);
				}
				default: break;
			}

			/* Parse the appropriate container. */
			if (type == UBJ_ARRAY)
				return parse_array(s, length, data_type);
			else
				return parse_object(s, length, data_type);
		}

		[[nodiscard]] static node parse_node(const parse_state &s, ubj_type_t type)
		{
			if (type & ubj_type_t::UBJ_CONTAINER_MASK)
				return parse_container(s, type);
			else
				return parse_value(s, type);
		}
		[[nodiscard]] static node parse_node(const parse_state &s) { return parse_node(s, read_type_token(s)); }

		static void parse(const parse_state &s) { s.result = parse_node(s); }
	};

	void ubj_input_archive::init(parse_mode m, syntax stx) noexcept
	{
		constinit static const parse_func parse_table[] = {
			parser_spec12::parse,
		};

		parse = parse_table[static_cast<int>(stx)];
		mode = m;
	}
	void ubj_input_archive::do_read(node &n)
	{
		parse_state state = {
			.reader = reader,
			.mode = mode,
			.result = n,
		};
		parse(state);
	}

	void ubj_output_archive::emitter_state::write_guarded(const void *src, std::size_t n) const
	{
		if (writer->write(src, n) != n) [[unlikely]]
			throw archive_error(WRITE_FAIL_MSG);
	}
	void ubj_output_archive::emitter_state::write_token(char c) const { write_guarded(&c, sizeof(c)); }

	struct ubj_output_archive::emitter_spec12
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
		[[nodiscard]] static ubj_type_t get_node_type(const emitter_state &s, const node &n)
		{
			switch (n.state())
			{
				case node::state_type::EMPTY: return UBJ_NULL;
				case node::state_type::CHAR: return UBJ_CHAR;
				case node::state_type::BOOL: return n.as_bool() ? UBJ_BOOL_TRUE : UBJ_BOOL_FALSE;
				/*case node::state_type::NUMBER:*/
				case node::state_type::INT: return s.mode & best_fit ? get_int_type(n.as_int()) : UBJ_INT64;
				case node::state_type::FLOAT: return s.mode & best_fit ? get_float_type(n.as_float()) : UBJ_FLOAT64;
				case node::state_type::STRING: return UBJ_STRING;
				case node::state_type::BINARY: /* Array of int */
				case node::state_type::ARRAY: return UBJ_ARRAY;
				case node::state_type::TABLE: return UBJ_OBJECT;
			}
		}

		[[nodiscard]] constexpr static bool do_fix_type(const emitter_state &s) noexcept
		{
			return (s.mode & fixed_type) == fixed_type;
		}
		[[nodiscard]] static ubj_type_t get_array_type(const emitter_state &s, const node::sequence_type &seq)
		{
			ubj_type_t type = UBJ_INVALID;
			if (std::all_of(seq.begin(),
							seq.end(),
							[&](auto &n)
							{
								auto node_type = get_node_type(s, n);
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
		[[nodiscard]] static ubj_type_t get_object_type(const emitter_state &s, const node::table_type &t)
		{
			ubj_type_t type = UBJ_INVALID;
			if (std::all_of(t.begin(),
							t.end(),
							[&](auto &p)
							{
								auto node_type = get_node_type(s, p.second);
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
		static void emit_literal(const emitter_state &s, T value)
		{
			s.write_guarded(&value, sizeof(T));
		}
		static void emit_type_token(const emitter_state &s, ubj_type_t type)
		{
			s.write_token(ubj_spec12_token_table[type]);
		}

		static void emit_int(const emitter_state &s, std::int64_t i, ubj_type_t type)
		{
			switch (type)
			{
				case UBJ_UINT8: emit_literal(s, static_cast<std::uint8_t>(i)); break;
				case UBJ_INT8: emit_literal(s, static_cast<std::int8_t>(i)); break;
				case UBJ_INT16: emit_literal(s, fix_endianness_write(static_cast<std::int16_t>(i))); break;
				case UBJ_INT32: emit_literal(s, fix_endianness_write(static_cast<std::int32_t>(i))); break;
				case UBJ_INT64: emit_literal(s, fix_endianness_write(i)); break;
			}
		}
		static void emit_float(const emitter_state &s, double f, ubj_type_t type)
		{
			switch (type)
			{
				case UBJ_FLOAT32: emit_literal(s, fix_endianness_write(static_cast<float>(f))); break;
				case UBJ_FLOAT64: emit_literal(s, fix_endianness_write(f)); break;
			}
		}
		static void emit_length(const emitter_state &s, std::int64_t l)
		{
			if (l < 0) [[unlikely]]
				throw archive_error(LENGTH_OVERFLOW_MSG);
			auto int_type = s.mode & best_fit ? get_int_type(l) : UBJ_INT64;
			emit_type_token(s, int_type);
			emit_int(s, l, int_type);
		}
		static void emit_string(const emitter_state &s, std::string_view sv)
		{
			emit_length(s, static_cast<std::int64_t>(sv.size()));
			s.write_guarded(sv.data(), sv.size());
		}
		static void emit_value(const emitter_state &s, const node &n, ubj_type_t type)
		{
			if (type == UBJ_CHAR)
				emit_literal(s, n.as_char());
			else if (type & UBJ_STRING_MASK)
				emit_string(s, n.as_string());
			else if (type & UBJ_FLOAT_MASK)
				emit_float(s, n.as_float(), type);
			else if (type & UBJ_INT_MASK)
				emit_int(s, n.as_int(), type);
		}

		static void emit_fixed_type(const emitter_state &s, ubj_type_t type)
		{
			s.write_token('$');
			emit_type_token(s, type);
		}
		static void emit_fixed_length(const emitter_state &s, std::int64_t l)
		{
			s.write_token('#');
			emit_length(s, l);
		}

		static void emit_binary(const emitter_state &s, const node::binary_type &b)
		{
			emit_fixed_type(s, UBJ_UINT8);
			emit_fixed_length(s, static_cast<std::int64_t>(b.size()));
			s.write_guarded(b.data(), b.size());
		}
		static void emit_array(const emitter_state &s, const node::sequence_type &seq)
		{
			if (ubj_type_t type; do_fix_type(s) && (type = get_array_type(s, seq)) != UBJ_INVALID) [[unlikely]]
			{
				/* Fixed-type & fixed-size array. */
				emit_fixed_type(s, type);
				emit_fixed_length(s, static_cast<std::int64_t>(seq.size()));
				for (auto &item : seq) emit_node(s, item, type);
			}
			else if (s.mode & fixed_size) [[likely]]
			{
				/* Fixed-size array. */
				emit_fixed_length(s, static_cast<std::int64_t>(seq.size()));
				for (auto &item : seq) emit_node(s, item);
			}
			else
			{
				/* Fully dynamic array. */
				for (auto &item : seq) emit_node(s, item);
				s.write_token(']');
			}
		}
		static void emit_object(const emitter_state &s, const node::table_type &t)
		{
			if (ubj_type_t type; do_fix_type(s) && (type = get_object_type(s, t)) != UBJ_INVALID) [[unlikely]]
			{
				/* Fixed-type & fixed-size object. */
				emit_fixed_type(s, type);
				emit_fixed_length(s, static_cast<std::int64_t>(t.size()));
				for (auto &item : t)
				{
					emit_string(s, item.first);
					emit_node(s, item.second, type);
				}
			}
			else if (s.mode & fixed_size) [[likely]]
			{
				/* Fixed-size object. */
				emit_fixed_length(s, static_cast<std::int64_t>(t.size()));
				for (auto &item : t)
				{
					emit_string(s, item.first);
					emit_node(s, item.second);
				}
			}
			else
			{
				/* Fully dynamic object. */
				for (auto &item : t)
				{
					emit_string(s, item.first);
					emit_node(s, item.second);
				}
				s.write_token('}');
			}
		}

		static void emit_node(const emitter_state &s, const node &n, ubj_type_t type)
		{
			if (type == UBJ_ARRAY)
			{
				if (n.is_binary()) [[unlikely]]
					emit_binary(s, n.as_binary());
				else
					emit_array(s, n.as_sequence());
			}
			else if (type == UBJ_OBJECT)
				emit_object(s, n.as_table());
			else
				emit_value(s, n, type);
		}
		static void emit_node(const emitter_state &s, const node &n)
		{
			auto type = get_node_type(s, n);
			emit_type_token(s, type);
			emit_node(s, n, type);
		}
		static void emit(const emitter_state &s) { emit_node(s, s.data); }
	};

	void ubj_output_archive::init(emit_mode m, syntax stx) noexcept
	{
		constinit static const emit_func emit_table[] = {
			emitter_spec12::emit,
		};

		emit = emit_table[static_cast<int>(stx)];
		mode = m;
	}
	void ubj_output_archive::do_write(const node &n)
	{
		emitter_state state = {
			.writer = writer,
			.mode = mode,
			.data = n,
		};
		emit(state);
	}
}	 // namespace sek::adt
