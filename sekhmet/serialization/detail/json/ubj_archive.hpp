/*
 * Created by switchblade on 2022-04-14
 */

#pragma once

#include <iostream>

#include "sekhmet/detail/bswap.hpp"

#include "common.hpp"

namespace sek::serialization::ubj
{
	typedef int config_flags;

	namespace detail
	{
		using namespace sek::serialization::detail;

		using base_archive = json_archive_base<char, std::char_traits<char>, container_types | char_value>;

		enum token_t : std::int8_t
		{
			INVALID = 0,

			NULL_ENTRY = 'Z',
			NOOP = 'N',
			BOOL_TRUE = 'T',
			BOOL_FALSE = 'F',
			CHAR = 'C',

			UINT8 = 'U',
			INT8 = 'i',
			INT16 = 'I',
			INT32 = 'l',
			INT64 = 'L',

			FLOAT32 = 'd',
			FLOAT64 = 'D',

			STRING = 'S',
			HIGHP = 'H',

			CONTAINER_TYPE = '$',
			CONTAINER_SIZE = '#',
			OBJECT_START = '{',
			OBJECT_END = '}',
			ARRAY_START = '[',
			ARRAY_END = ']',
		};

		constexpr static config_flags highp_mask = 12;
	}	 // namespace detail

	/** Enables fixed-size container output. */
	constexpr static config_flags fixed_size = 1;
	/** Enables fixed-type containers output. Implies `fixed_size`. */
	constexpr static config_flags fixed_type = 2 | fixed_size;

	/** Treat high-precision numbers as input errors (this is the default). */
	constexpr static config_flags highp_error = 0;
	/** Parse high-precision numbers as strings. */
	constexpr static config_flags highp_as_string = 4;
	/** Skip high-precision numbers (not recommended). */
	constexpr static config_flags highp_skip = 8;
	constexpr config_flags no_flags = 0;

	/** @details Archive used to read UBJson data.
	 *
	 * The archive itself does not do any deserialization, instead deserialization is done by archive frames,
	 * which represent a Json object or array. These frames are then passed to deserialization functions
	 * of serializable types.
	 *
	 * @tparam Config Configuration flags used for the archive.
	 * @note UBJson input archives can outlive the source they were initialized from, and can thus be used
	 * to cache json data to be deserialized later. */
	template<config_flags Config>
	class basic_input_archive : detail::base_archive
	{
		using base_t = detail::base_archive;

	public:
		typedef typename base_t::tree_type tree_type;
		typedef typename base_t::read_frame archive_frame;
		typedef typename archive_frame::archive_category archive_category;
		typedef typename archive_frame::char_type char_type;
		typedef typename archive_frame::size_type size_type;
		typedef archive_reader<char_type> reader_type;

	private:
		constexpr static auto eof_msg = "UBJson: Unexpected end of input";
		constexpr static auto data_msg = "UBJson: Invalid input";
		constexpr static auto bad_length_msg = "UBJson: Invalid input, expected integer type";
		constexpr static auto bad_size_msg = "UBJson: Invalid input, expected container size";

		class ubj_reader : reader_type
		{
			using base_t = reader_type;

		public:
			constexpr explicit ubj_reader(base_t &&reader) : base_t(std::move(reader)) {}

			void guarded_read(void *dest, std::size_t n)
			{
				const auto chars = n / sizeof(char_type);
				if (base_t::getn(static_cast<char_type *>(dest), chars) != chars) [[unlikely]]
					throw archive_error(eof_msg);
			}
			void guarded_bump(std::size_t n)
			{
				const auto chars = n / sizeof(char_type);
				if (base_t::bump(chars) != chars) [[unlikely]]
					throw archive_error(eof_msg);
			}
			detail::token_t read_token()
			{
				detail::token_t token;
				guarded_read(&token, sizeof(detail::token_t));
				return token;
			}
			detail::token_t peek_token()
			{
				if (auto c = base_t::peek(); c == traits_type::not_eof(c)) [[likely]]
					return static_cast<detail::token_t>(traits_type::to_char_type(c));
				else
					throw archive_error(eof_msg);
			}
			void bump_token() { guarded_bump(sizeof(detail::token_t)); }
		};

		struct parser_spec12 : base_t::parser_base
		{
			using base_handler = typename base_t::parser_base;

			constexpr parser_spec12(basic_input_archive &archive, ubj_reader &&reader) noexcept
				: base_handler(archive), m_reader(std::move(reader))
			{
			}

			template<typename T>
			[[nodiscard]] T parse_literal()
			{
				T value;
				m_reader.guarded_read(&value, sizeof(value));

				/* Fix endianness from big endian to machine endianness. */
#ifndef SEK_ARCH_BIG_ENDIAN
				if constexpr (sizeof(T) == sizeof(std::uint16_t))
					return std::bit_cast<T>(bswap_16(std::bit_cast<std::uint16_t>(value)));
				else if constexpr (sizeof(T) == sizeof(std::uint32_t))
					return std::bit_cast<T>(bswap_32(std::bit_cast<std::uint32_t>(value)));
				else if constexpr (sizeof(T) == sizeof(std::uint64_t))
					return std::bit_cast<T>(bswap_64(std::bit_cast<std::uint64_t>(value)));
				else
#endif
					return value;
			}
			[[nodiscard]] std::int64_t parse_length()
			{
				auto token = m_reader.read_token();
				switch (token)
				{
					case detail::token_t::UINT8: return static_cast<std::int64_t>(parse_literal<std::uint8_t>());
					case detail::token_t::INT8: return static_cast<std::int64_t>(parse_literal<std::int8_t>());
					case detail::token_t::INT16: return static_cast<std::int64_t>(parse_literal<std::int16_t>());
					case detail::token_t::INT32: return static_cast<std::int64_t>(parse_literal<std::int32_t>());
					case detail::token_t::INT64: return parse_literal<std::int64_t>();
					default: [[unlikely]] throw archive_error(bad_length_msg);
				}
			}
			[[nodiscard]] std::pair<const char *, std::size_t> parse_string()
			{
				auto len = static_cast<std::size_t>(parse_length());
				auto *str = base_handler::on_string_alloc(len);
				m_reader.guarded_read(str, len * sizeof(char));
				str[len] = '\0';
				return {str, len};
			}
			[[nodiscard]] std::pair<detail::token_t, std::int64_t> parse_container()
			{
				std::pair<detail::token_t, std::int64_t> result = {detail::token_t::INVALID, -1};

				switch (auto token = m_reader.peek_token(); token)
				{
					case detail::token_t::CONTAINER_TYPE:
					{
						/* Consume the token & read type. */
						m_reader.bump_token();
						result.first = m_reader.read_token();

						/* Container size always follows the type. */
						if ((token = m_reader.peek_token()) != detail::token_t::CONTAINER_SIZE) [[unlikely]]
							throw archive_error(bad_size_msg);
						[[fallthrough]];
					}
					case detail::token_t::CONTAINER_SIZE:
					{
						m_reader.bump_token();
						result.second = parse_length();
						[[fallthrough]];
					}
					default: break;
				}

				return result;
			}
			void parse_array()
			{
				auto [data_type, size] = parse_container();

				if (size == -1) [[unlikely]] /* Fully dynamic array. */
				{
					base_handler::on_array_start();
					for (size = 0;; ++size)
					{
						auto token = m_reader.read_token();
						if (token == detail::token_t::ARRAY_END) [[unlikely]]
							break;
						parse_entry(token);
					}
				}
				else /* Fixed-size array. */
				{
					base_handler::on_array_start(static_cast<std::size_t>(size));
					if (data_type != detail::token_t::INVALID) [[likely]] /* Fixed-type array. */
						for (auto i = size; i != 0; --i) parse_entry(data_type);
					else /* Dynamic-type array. */
						for (auto i = size; i != 0; --i) parse_entry();
				}

				base_handler::on_array_end(static_cast<std::size_t>(size));
			}
			void parse_object()
			{
				auto read_key = [&]()
				{
					auto [key_str, key_len] = parse_string();
					base_handler::on_object_key(key_str, key_len);
				};
				auto [value_type, size] = parse_container();

				if (size == -1) [[unlikely]] /* Fully dynamic object. */
				{
					base_handler::on_object_start();
					for (size = 0;; ++size)
					{
						auto token = m_reader.peek_token();
						if (token == detail::token_t::OBJECT_END) [[unlikely]]
						{
							m_reader.bump_token();
							break;
						}

						read_key();
						parse_entry();
					}
				}
				else /* Fixed-size object. */
				{
					base_handler::on_object_start(static_cast<std::size_t>(size));
					if (value_type != detail::token_t::INVALID) [[likely]] /* Fixed-type object. */
						for (auto i = size; i != 0; --i)
						{
							read_key();
							parse_entry(value_type);
						}
					else /* Dynamic-type object. */
						for (auto i = size; i != 0; --i)
						{
							read_key();
							parse_entry();
						}
				}
				base_handler::on_object_end(static_cast<std::size_t>(size));
			}
			void parse_entry(detail::token_t token)
			{
				switch (token)
				{
					case detail::token_t::NOOP: break;
					case detail::token_t::NULL_ENTRY: base_handler::on_null(); break;
					case detail::token_t::BOOL_TRUE: base_handler::on_true(); break;
					case detail::token_t::BOOL_FALSE: base_handler::on_false(); break;
					case detail::token_t::CHAR: base_handler::on_char(parse_literal<char>()); break;
					case detail::token_t::UINT8: base_handler::on_int(parse_literal<std::uint8_t>()); break;
					case detail::token_t::INT8: base_handler::on_int(parse_literal<std::int8_t>()); break;
					case detail::token_t::INT16: base_handler::on_int(parse_literal<std::int16_t>()); break;
					case detail::token_t::INT32: base_handler::on_int(parse_literal<std::int32_t>()); break;
					case detail::token_t::INT64: base_handler::on_int(parse_literal<std::int64_t>()); break;
					case detail::token_t::FLOAT32: base_handler::on_float(parse_literal<float>()); break;
					case detail::token_t::FLOAT64: base_handler::on_float(parse_literal<double>()); break;
					case detail::token_t::HIGHP:
					{
						if constexpr ((Config & detail::highp_mask) == highp_error)
							throw archive_error("High-precision number support is disabled");
						else if constexpr ((Config & detail::highp_mask) == highp_skip)
							break;
						[[fallthrough]];
					}
					case detail::token_t::STRING:
					{
						auto [str, len] = parse_string();
						base_handler::on_string(str, len);
						break;
					}
					case detail::token_t::ARRAY_START: parse_array(); break;
					case detail::token_t::OBJECT_START: parse_object(); break;
					default: [[unlikely]] throw archive_error(data_msg);
				}
			}
			void parse_entry() { parse_entry(m_reader.read_token()); }

			ubj_reader m_reader;
		};

	public:
		basic_input_archive() = delete;
		basic_input_archive(const basic_input_archive &) = delete;
		basic_input_archive &operator=(const basic_input_archive &) = delete;

		constexpr basic_input_archive(basic_input_archive &&) noexcept = default;
		constexpr basic_input_archive &operator=(basic_input_archive &&other) noexcept
		{
			base_t::operator=(std::forward<basic_input_archive>(other));
			return *this;
		}

		/** Initializes input archive from a json node tree.
		 * @param tree Json node tree containing source data. */
		explicit basic_input_archive(tree_type &tree) : basic_input_archive(tree, std::pmr::get_default_resource()) {}
		/** @copydoc basic_input_archive */
		explicit basic_input_archive(tree_type &&tree)
			: basic_input_archive(std::move(tree), std::pmr::get_default_resource())
		{
		}
		/** @copydoc basic_input_archive
		 * @param res Memory resource used for internal allocation. */
		basic_input_archive(tree_type &tree, std::pmr::memory_resource *res) : base_t(tree, res) {}
		/** @copydoc basic_input_archive */
		basic_input_archive(tree_type &&tree, std::pmr::memory_resource *res) : base_t(std::move(tree), res) {}

		/** Reads UBJson using the provided archive reader.
		 * @param reader Reader used to read UBJson data. */
		explicit basic_input_archive(reader_type reader)
			: basic_input_archive(std::move(reader), std::pmr::get_default_resource())
		{
		}
		/** @copydoc basic_input_archive
		 * @param res Memory resource used for internal allocation. */
		basic_input_archive(reader_type reader, std::pmr::memory_resource *res) : base_t(res)
		{
			parse(ubj_reader{std::move(reader)});
		}
		/** Reads UBJson from a memory buffer.
		 * @param buff Pointer to the memory buffer containing UBJson data.
		 * @param len Size of the memory buffer. */
		basic_input_archive(const void *buff, std::size_t len)
			: basic_input_archive(buff, len, std::pmr::get_default_resource())
		{
		}
		/** @copydoc basic_input_archive
		 * @param res PMR memory resource used for internal allocation. */
		basic_input_archive(const void *buff, std::size_t len, std::pmr::memory_resource *res)
			: basic_input_archive(reader_type{buff, len}, res)
		{
		}
		/** Reads UBJson from a file.
		 * @param file Native file containing UBJson data. */
		explicit basic_input_archive(system::native_file &file)
			: basic_input_archive(file, std::pmr::get_default_resource())
		{
		}
		/** @copydoc basic_input_archive
		 * @param res Memory resource used for internal allocation. */
		basic_input_archive(system::native_file &file, std::pmr::memory_resource *res)
			: basic_input_archive(reader_type{file}, res)
		{
		}
		/** Reads UBJson from a file.
		 * @param file Pointer to the UBJson file.
		 * @note File must be opened in binary mode. */
		explicit basic_input_archive(FILE *file) : basic_input_archive(file, std::pmr::get_default_resource()) {}
		/** @copydoc basic_input_archive
		 * @param res Memory resource used for internal allocation. */
		basic_input_archive(FILE *file, std::pmr::memory_resource *res) : basic_input_archive(reader_type{file}, res) {}
		/** Reads UBJson from a stream buffer.
		 * @param buff Pointer to the stream buffer.
		 * @note Stream buffer must be a binary stream buffer. */
		explicit basic_input_archive(std::streambuf *buff) : basic_input_archive(buff, std::pmr::get_default_resource())
		{
		}
		/** @copydoc basic_input_archive
		 * @param res Memory resource used for internal allocation. */
		basic_input_archive(std::streambuf *buff, std::pmr::memory_resource *res)
			: basic_input_archive(reader_type{buff}, res)
		{
		}
		/** Reads UBJson from an input stream.
		 * @param is Reference to the input stream.
		 * @note Stream must be a binary stream. */
		explicit basic_input_archive(std::istream &is) : basic_input_archive(is.rdbuf()) {}
		/** @copydoc basic_input_archive
		 * @param res Memory resource used for internal allocation. */
		basic_input_archive(std::istream &is, std::pmr::memory_resource *res) : basic_input_archive(is.rdbuf(), res) {}

		/** Attempts to deserialize the top-level Json entry of the archive.
		 * @param value Value to deserialize from the Json entry.
		 * @param args Arguments forwarded to the deserialization function.
		 * @return true if deserialization was successful, false otherwise. */
		template<typename T, typename... Args>
		bool try_read(T &&value, Args &&...args)
		{
			return base_t::do_try_read(std::forward<T>(value), std::forward<Args>(args)...);
		}
		/** Deserializes the top-level Json entry of the archive.
		 * @param value Value to deserialize from the Json entry.
		 * @return Reference to this archive.
		 * @throw archive_error On deserialization errors. */
		template<typename T>
		basic_input_archive &operator>>(T &&value)
		{
			return read(std::forward<T>(value));
		}
		/** @copydoc operator>>
		 * @param args Arguments forwarded to the deserialization function. */
		template<typename T, typename... Args>
		basic_input_archive &read(T &&value, Args &&...args)
		{
			base_t::do_read(std::forward<T>(value), std::forward<Args>(args)...);
			return *this;
		}
		/** @brief Deserializes an instance of `T` from the top-level Json entry of the archive in-place.
		 * Uses the in-place `deserialize` overload (taking `std::in_place_type_t<T>`)
		 * or constructor accepting the archive frame as one of it's arguments if available.
		 * Otherwise, default-constructs & deserializes using `read(T &&)`.
		 * @param args Arguments forwarded to the deserialization function.
		 * @return Deserialized instance of `T`.
		 * @throw archive_error On deserialization errors. */
		template<typename T, typename... Args>
		T read(std::in_place_type_t<T>, Args &&...args)
		{
			return base_t::do_read(std::in_place_type<T>, std::forward<Args>(args)...);
		}

		constexpr void swap(basic_input_archive &other) noexcept { base_t::swap(other); }
		friend constexpr void swap(basic_input_archive &a, basic_input_archive &b) noexcept { a.swap(b); }

		using base_t::tree;

	private:
		void parse(ubj_reader reader)
		{
			parser_spec12 parser{*this, std::move(reader)};
			parser.parse_entry();
		}
	};

	typedef basic_input_archive<highp_error> input_archive;

	static_assert(serialization::input_archive<input_archive::archive_frame, bool>);
	static_assert(serialization::input_archive<input_archive::archive_frame, char>);
	static_assert(serialization::input_archive<input_archive::archive_frame, std::uint8_t>);
	static_assert(serialization::input_archive<input_archive::archive_frame, std::int8_t>);
	static_assert(serialization::input_archive<input_archive::archive_frame, std::int16_t>);
	static_assert(serialization::input_archive<input_archive::archive_frame, std::int32_t>);
	static_assert(serialization::input_archive<input_archive::archive_frame, std::int64_t>);
	static_assert(serialization::input_archive<input_archive::archive_frame, float>);
	static_assert(serialization::input_archive<input_archive::archive_frame, double>);
	static_assert(serialization::input_archive<input_archive::archive_frame, std::string>);
	static_assert(serialization::structured_data_archive<input_archive::archive_frame>);

	/** @details Archive used to write UBJson data.
	 *
	 * The archive itself does not do any serialization, instead serialization is done by archive frames,
	 * which represent a Json object or array. These frames are then passed to serialization functions
	 * of serializable types.
	 *
	 * @tparam Config Configuration flags used for the archive.
	 * @note UBJson output archives may not outlive the destination stream, buffer, file or archive writer they were initialized from. */
	template<config_flags Config>
	class basic_output_archive : detail::base_archive
	{
		using base_t = detail::base_archive;
		using entry_type = typename base_t::tree_type::type_selector;

	public:
		typedef typename base_t::tree_type tree_type;
		typedef typename base_t::write_frame archive_frame;
		typedef typename archive_frame::archive_category archive_category;
		typedef typename archive_frame::char_type char_type;
		typedef typename archive_frame::size_type size_type;
		typedef archive_writer<char_type> writer_type;

	private:
		class ubj_writer : writer_type
		{
			using base_t = writer_type;

		public:
			constexpr explicit ubj_writer(base_t &&writer) : base_t(std::move(writer)) {}

			void write_guarded(const void *src, std::size_t n)
			{
				const auto chars = n / sizeof(char_type);
				if (base_t::putn(static_cast<const char_type *>(src), chars) != chars) [[unlikely]]
					throw archive_error("UBJson: Emitter write failure");
			}
			void write_token(detail::token_t token) { write_guarded(&token, sizeof(token)); }
		};
		struct emitter_spec12
		{
			struct frame_t
			{
				void (*emit_type_token)(ubj_writer *, detail::token_t);
				entry_type value_type = {};
			};

			static void emit_fixed_type(ubj_writer *, detail::token_t) {}
			static void emit_dynamic_type(ubj_writer *writer, detail::token_t token) { writer->write_token(token); }

			constexpr static detail::token_t get_type_token(entry_type type) noexcept
			{
				if (type.storage == entry_type::ARRAY)
					return detail::token_t::ARRAY_START;
				else if (type.storage == entry_type::TABLE)
					return detail::token_t::OBJECT_START;
				else if (type.storage == entry_type::VALUE)
					switch (type.value)
					{
						case detail::json_type::NULL_VALUE: return detail::token_t::NULL_ENTRY;
						case detail::json_type::BOOL_FALSE: return detail::token_t::BOOL_FALSE;
						case detail::json_type::BOOL_TRUE: return detail::token_t::BOOL_TRUE;
						case detail::json_type::CHAR: return detail::token_t::CHAR;

						case detail::json_type::INT_S8: return detail::token_t::INT8;
						case detail::json_type::INT_U8: return detail::token_t::UINT8;
						case detail::json_type::INT_S16: [[fallthrough]];
						case detail::json_type::INT_U16: return detail::token_t::INT16;
						case detail::json_type::INT_S32: [[fallthrough]];
						case detail::json_type::INT_U32: return detail::token_t::INT32;
						case detail::json_type::INT_S64: [[fallthrough]];
						case detail::json_type::INT_U64: return detail::token_t::INT64;

						case detail::json_type::FLOAT32: return detail::token_t::FLOAT32;
						case detail::json_type::FLOAT64: return detail::token_t::FLOAT64;

						case detail::json_type::STRING: return detail::token_t::STRING;
						default: break;
					}
				return detail::token_t::INVALID;
			}

			constexpr emitter_spec12() noexcept = default;
			constexpr explicit emitter_spec12(ubj_writer *writer) noexcept : m_writer(writer) {}

			template<typename T>
			void emit_literal(T value)
			{
				/* Fix endianness from machine endianness to big endian. */
#ifndef SEK_ARCH_BIG_ENDIAN
				if constexpr (sizeof(T) == sizeof(std::uint16_t))
				{
					auto temp = bswap_16(std::bit_cast<std::uint16_t>(value));
					m_writer->write_guarded(static_cast<const void *>(&temp), sizeof(temp));
				}
				else if constexpr (sizeof(T) == sizeof(std::uint32_t))
				{
					auto temp = bswap_32(std::bit_cast<std::uint32_t>(value));
					m_writer->write_guarded(static_cast<const void *>(&temp), sizeof(temp));
				}
				else if constexpr (sizeof(T) == sizeof(std::uint64_t))
				{
					auto temp = bswap_64(std::bit_cast<std::uint64_t>(value));
					m_writer->write_guarded(static_cast<const void *>(&temp), sizeof(temp));
				}
				else
#endif
					m_writer->write_guarded(static_cast<const void *>(&value), sizeof(value));
			}
			void emit_type(detail::token_t type) { m_frame.emit_type_token(m_writer, type); }
			void emit_length(std::size_t value)
			{
				/* << 1 is used to make sure sizes are within signed range. */
				switch (detail::int_size_category(value << 1))
				{
					case 0:
						m_writer->write_token(detail::token_t::UINT8);
						emit_literal(static_cast<std::uint8_t>(value));
						break;
					case 1:
						m_writer->write_token(detail::token_t::INT16);
						emit_literal(static_cast<std::uint16_t>(value));
						break;
					case 2:
						m_writer->write_token(detail::token_t::INT32);
						emit_literal(static_cast<std::uint32_t>(value));
						break;
					case 3:
					default:
						m_writer->write_token(detail::token_t::INT64);
						emit_literal(static_cast<std::uint64_t>(value));
						break;
				}
			}
			void emit_string(const char *str, std::size_t size)
			{
				emit_length(size);
				m_writer->write_guarded(str, size * sizeof(char));
			}
			void emit_container(std::size_t size, entry_type value_type)
			{
				m_frame.emit_type_token = emit_dynamic_type;
				if constexpr ((Config & fixed_type) == fixed_type)
					if ((m_frame.value_type = value_type).storage != entry_type::DYNAMIC)
					{
						m_writer->write_token(detail::token_t::CONTAINER_TYPE);
						m_writer->write_token(get_type_token(value_type));
						m_frame.emit_type_token = emit_fixed_type;
					}
				if constexpr ((Config & fixed_size) == fixed_size)
				{
					m_writer->write_token(detail::token_t::CONTAINER_SIZE);
					emit_length(size);
				}
			}

			void on_null() { emit_type(detail::token_t::NULL_ENTRY); }
			void on_true() { emit_type(detail::token_t::BOOL_TRUE); }
			void on_false() { emit_type(detail::token_t::BOOL_FALSE); }
			void on_char(char value)
			{
				emit_type(detail::token_t::CHAR);
				emit_literal(value);
			}

			[[nodiscard]] detail::json_type current_int_type(detail::json_type type) const noexcept
			{
				if constexpr ((Config & fixed_type) == fixed_type)
					return m_frame.value_type.value & detail::json_type::INT_TYPE ? m_frame.value_type.value : type;
				else
					return type;
			}
			void on_int(detail::json_type type, std::intmax_t value)
			{
				on_uint(type, static_cast<std::uintmax_t>(value));
			}
			void on_uint(detail::json_type type, std::uintmax_t value)
			{
				switch (current_int_type(type))
				{
					case detail::json_type::INT_U8:
						emit_type(detail::token_t::UINT8);
						emit_literal(static_cast<std::uint8_t>(value));
						break;
					case detail::json_type::INT_S8:
						emit_type(detail::token_t::INT8);
						emit_literal(static_cast<std::int8_t>(value));
						break;
					case detail::json_type::INT_U16:
					case detail::json_type::INT_S16:
						emit_type(detail::token_t::INT16);
						emit_literal(static_cast<std::int16_t>(value));
						break;
					case detail::json_type::INT_U32:
					case detail::json_type::INT_S32:
						emit_type(detail::token_t::INT32);
						emit_literal(static_cast<std::int32_t>(value));
						break;
					case detail::json_type::INT_U64:
					case detail::json_type::INT_S64:
						emit_type(detail::token_t::INT64);
						emit_literal(static_cast<std::int64_t>(value));
						break;
					default: [[unlikely]] SEK_NEVER_REACHED;
				}
			}

			void on_float32(float value)
			{
				emit_type(detail::token_t::FLOAT32);
				emit_literal(value);
			}
			void on_float64(double value)
			{
				emit_type(detail::token_t::FLOAT64);
				emit_literal(value);
			}

			void on_string(const char_type *str, std::size_t size)
			{
				emit_type(detail::token_t::STRING);
				emit_string(str, size);
			}

			void on_array_start(std::size_t size, entry_type value_type)
			{
				emit_type(detail::token_t::ARRAY_START);
				emit_container(size, value_type);
			}
			void on_array_end()
			{
				// clang-format off
				if constexpr ((Config & fixed_size) != fixed_size)
					m_writer->write_token(detail::token_t::ARRAY_END);
				// clang-format on
			}
			void on_object_start(std::size_t size, entry_type value_type)
			{
				emit_type(detail::token_t::OBJECT_START);
				emit_container(size, value_type);
			}
			void on_object_key(const char_type *str, std::size_t size) { emit_string(str, size); }
			void on_object_end()
			{
				// clang-format off
				if constexpr ((Config & fixed_size) != fixed_size)
					m_writer->write_token(detail::token_t::OBJECT_END);
				// clang-format on
			}

			constexpr frame_t enter_frame() { return m_frame; }
			constexpr void exit_frame(frame_t old) { m_frame = old; }

			frame_t m_frame = {emit_dynamic_type};
			ubj_writer *m_writer = nullptr;
		};

	public:
		basic_output_archive() = delete;
		basic_output_archive(const basic_output_archive &) = delete;
		basic_output_archive &operator=(const basic_output_archive &) = delete;

		constexpr basic_output_archive(basic_output_archive &&other) noexcept
			: base_t(std::forward<base_t>(other)), m_writer(std::move(other.m_writer))
		{
		}
		constexpr basic_output_archive &operator=(basic_output_archive &&other) noexcept
		{
			base_t::operator=(std::forward<base_t>(other));
			std::swap(m_writer, other.m_writer);
			return *this;
		}

		/** Initializes output archive from a json node tree.
		 * @param tree Json node tree containing source data. */
		explicit basic_output_archive(tree_type &tree) : basic_output_archive(tree, std::pmr::get_default_resource()) {}
		/** @copydoc basic_input_archive */
		explicit basic_output_archive(tree_type &&tree)
			: basic_output_archive(std::move(tree), std::pmr::get_default_resource())
		{
		}
		/** @copydoc basic_input_archive
		 * @param res Memory resource used for internal allocation. */
		basic_output_archive(tree_type &tree, std::pmr::memory_resource *res) : base_t(tree, res), m_can_flush(false) {}
		/** @copydoc basic_input_archive */
		basic_output_archive(tree_type &&tree, std::pmr::memory_resource *res)
			: base_t(std::move(tree), res), m_can_flush(false)
		{
		}

		/** Initializes output archive for writing using the provided writer.
		 * @param writer Writer used to write UBJson data. */
		explicit basic_output_archive(writer_type writer)
			: basic_output_archive(std::move(writer), std::pmr::get_default_resource())
		{
		}
		/** @copydoc basic_input_archive
		 * @param res Memory resource used for internal allocation. */
		basic_output_archive(writer_type writer, std::pmr::memory_resource *res)
			: base_t(res), m_writer(std::move(writer))
		{
		}
		/** Initializes output archive for file writing.
		 * @param file Native file to write UBJson data to. */
		explicit basic_output_archive(system::native_file &file)
			: basic_output_archive(file, std::pmr::get_default_resource())
		{
		}
		/** @copydoc basic_input_archive
		 * @param res Memory resource used for internal allocation. */
		basic_output_archive(system::native_file &file, std::pmr::memory_resource *res)
			: basic_output_archive(writer_type{file}, res)
		{
		}
		/** Initialized output archive for file writing.
		 * @param file File to write UBJson data to.
		 * @note File must be opened in binary mode. */
		explicit basic_output_archive(FILE *file) : basic_output_archive(file, std::pmr::get_default_resource()) {}
		/** @copydoc basic_output_archive
		 * @param res PMR memory resource used for internal state allocation. */
		basic_output_archive(FILE *file, std::pmr::memory_resource *res) : basic_output_archive(writer_type{file}, res)
		{
		}
		/** Initialized output archive for stream buffer writing.
		 * @param buff Stream buffer to write UBJson data to.
		 * @note Stream buffer must be a binary stream buffer. */
		explicit basic_output_archive(std::streambuf *buff)
			: basic_output_archive(buff, std::pmr::get_default_resource())
		{
		}
		/** @copydoc basic_output_archive
		 * @param res PMR memory resource used for internal state allocation. */
		basic_output_archive(std::streambuf *buff, std::pmr::memory_resource *res)
			: basic_output_archive(writer_type{buff}, res)
		{
		}
		/** Initialized output archive for stream writing.
		 * @param os Output stream to write UBJson data to.
		 * @note Stream must be a binary stream. */
		explicit basic_output_archive(std::ostream &os) : basic_output_archive(os.rdbuf()) {}
		/** @copydoc basic_output_archive
		 * @param res PMR memory resource used for internal state allocation. */
		basic_output_archive(std::ostream &os, std::pmr::memory_resource *res) : basic_output_archive(os.rdbuf(), res)
		{
		}

		~basic_output_archive() { flush_impl(); }

		/** Serializes the forwarded value to UBJson. Flushes previous uncommitted state.
		 * @param value Value to serialize as UBJson.
		 * @return Reference to this archive.
		 * @note Serialized data is kept inside the archive's internal state and will be written to the output once the
		 * archive is destroyed or `flush` is called. */
		template<typename T>
		basic_output_archive &operator<<(T &&value)
		{
			return write(std::forward<T>(value));
		}
		/** @copydoc operator<<
		 * @param args Arguments forwarded to the serialization function. */
		template<typename T, typename... Args>
		basic_output_archive &write(T &&value, Args &&...args)
		{
			/* Flush uncommitted changes before initializing a new emit tree. */
			flush();
			base_t::do_write(std::forward<T>(value), std::forward<Args>(args)...);
			return *this;
		}

		/** Flushes the internal state & writes UBJson to the output. */
		void flush()
		{
			flush_impl();
			base_t::reset();
		}

		/** Replaces the internal node tree with the specified one and returns pointer to the old tree. */
		constexpr tree_type *reset(tree_type *new_tree = nullptr) noexcept
		{
			m_can_flush = m_can_flush || new_tree != nullptr;
			return std::exchange(base_t::tree, new_tree);
		}

		constexpr void swap(basic_output_archive &other) noexcept
		{
			base_t::swap(other);
			std::swap(m_writer, other.m_writer);
		}
		friend constexpr void swap(basic_output_archive &a, basic_output_archive &b) noexcept { a.swap(b); }

	private:
		void flush_impl()
		{
			if (m_can_flush) [[likely]]
			{
				emitter_spec12 emitter{&m_writer};
				base_t::do_flush(emitter);
			}
		}

		union
		{
			std::byte m_padding[sizeof(ubj_writer)] = {};
			ubj_writer m_writer;
		};

		bool m_can_flush = true;
	};

	typedef basic_output_archive<fixed_type> output_archive;

	static_assert(serialization::output_archive<output_archive, std::nullptr_t>);
	static_assert(serialization::output_archive<output_archive, bool>);
	static_assert(serialization::output_archive<output_archive, char>);
	static_assert(serialization::output_archive<output_archive, std::uint8_t>);
	static_assert(serialization::output_archive<output_archive, std::int8_t>);
	static_assert(serialization::output_archive<output_archive, std::int16_t>);
	static_assert(serialization::output_archive<output_archive, std::int32_t>);
	static_assert(serialization::output_archive<output_archive, std::int64_t>);
	static_assert(serialization::output_archive<output_archive, float>);
	static_assert(serialization::output_archive<output_archive, double>);
	static_assert(serialization::output_archive<output_archive, std::string>);
}	 // namespace sek::serialization::ubj