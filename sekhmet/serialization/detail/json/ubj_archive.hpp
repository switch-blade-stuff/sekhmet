//
// Created by switchblade on 2022-04-14.
//

#pragma once

#include <iostream>

#include "../archive_traits.hpp"
#include "../manipulators.hpp"
#include "common.hpp"
#include "sekhmet/detail/bswap.hpp"

namespace sek::serialization
{
	namespace detail
	{
		enum ubj_token : std::int8_t
		{
			UBJ_INVALID_TOKEN = 0,
			UBJ_NULL = 'Z',
			UBJ_NOOP = 'N',
			UBJ_BOOL_TRUE = 'T',
			UBJ_BOOL_FALSE = 'F',
			UBJ_CHAR = 'C',

			UBJ_UINT8 = 'U',
			UBJ_INT8 = 'i',
			UBJ_INT16 = 'I',
			UBJ_INT32 = 'l',
			UBJ_INT64 = 'L',

			UBJ_FLOAT32 = 'd',
			UBJ_FLOAT64 = 'D',

			UBJ_STRING = 'S',
			UBJ_HIGHP = 'H',

			UBJ_CONTAINER_TYPE = '$',
			UBJ_CONTAINER_SIZE = '#',
			UBJ_OBJECT_START = '{',
			UBJ_OBJECT_END = '}',
			UBJ_ARRAY_START = '[',
			UBJ_ARRAY_END = ']',
		};
		enum ubj_version : int
		{
			UBJ_SPEC12 = 0,
		};
		enum ubj_highp_mode : int
		{
			UBJ_HIGHP_STRING = 0,
			UBJ_HIGHP_THROW = 1,
			UBJ_HIGHP_SKIP = 2,
		};
	}	 // namespace detail

	/** @details Archive used to read UBJson data.
	 *
	 * The archive itself does not do any de-serialization, instead deserialization is done by special archive frames,
	 * which represent a Json object or array. These frames are then passed to deserialization functions
	 * of serializable types.
	 *
	 * @tparam CharType Character type used for Json. */
	template<typename CharType>
	class basic_ubj_input_archive : detail::json_input_archive_base<CharType>
	{
		using base_t = detail::json_input_archive_base<CharType>;

	public:
		typedef typename base_t::read_frame archive_frame;

	private:
		struct parser_base : base_t::parse_event_handler
		{
			using base_handler = typename base_t::parse_event_handler;

			constexpr static auto eof_msg = "UBJson: Unexpected end of input";
			constexpr static auto data_msg = "UBJson: Invalid input";
			constexpr static auto bad_length_msg = "UBJson: Invalid input, expected integer type";
			constexpr static auto bad_size_msg = "UBJson: Invalid input, expected container size";

			parser_base() = delete;
			constexpr explicit parser_base(basic_ubj_input_archive *archive) noexcept : base_handler(archive) {}

			void guarded_read(void *dest, std::size_t n)
			{
				if (read(dest, n) != n) [[unlikely]]
					throw archive_error(eof_msg);
			}
			void guarded_bump(std::size_t n)
			{
				if (bump(n) != n) [[unlikely]]
					throw archive_error(eof_msg);
			}
			detail::ubj_token read_token()
			{
				detail::ubj_token token;
				guarded_read(&token, sizeof(detail::ubj_token));
				return token;
			}
			detail::ubj_token peek_token()
			{
				if (auto c = peek(); c == EOF) [[unlikely]]
					throw archive_error(eof_msg);
				else
					return static_cast<detail::ubj_token>(c);
			}
			void bump_token() { guarded_bump(sizeof(detail::ubj_token)); }

			template<typename T>
			[[nodiscard]] T read_literal()
			{
				T value;
				guarded_read(&value, sizeof(value));

				/* Fix endianness from big endian to machine endianness.
				 * TODO: Only do this for Spec12 */
#ifndef SEK_ARCH_BIG_ENDIAN
				if constexpr (sizeof(T) == sizeof(std::uint16_t))
					return std::bit_cast<T>(bswap_16(std::bit_cast<std::uint16_t>(value)));
				else if constexpr (sizeof(T) == sizeof(std::uint32_t))
					return std::bit_cast<T>(bswap_32(std::bit_cast<std::uint32_t>(value)));
				else if constexpr (sizeof(T) == sizeof(std::uint64_t))
					return std::bit_cast<T>(bswap_64(std::bit_cast<std::uint64_t>(value)));
#endif
				return value;
			}
			[[nodiscard]] std::int64_t read_length()
			{
				auto token = read_token();
				switch (token)
				{
					case detail::ubj_token::UBJ_UINT8: return static_cast<std::int64_t>(read_literal<std::uint8_t>());
					case detail::ubj_token::UBJ_INT8: return static_cast<std::int64_t>(read_literal<std::int8_t>());
					case detail::ubj_token::UBJ_INT16: return static_cast<std::int64_t>(read_literal<std::int16_t>());
					case detail::ubj_token::UBJ_INT32: return static_cast<std::int64_t>(read_literal<std::int32_t>());
					case detail::ubj_token::UBJ_INT64: return read_literal<std::int64_t>();
					default: throw archive_error(bad_length_msg);
				}
			}
			[[nodiscard]] std::pair<const CharType *, std::size_t> read_string()
			{
				auto len = static_cast<std::size_t>(read_length());
				auto *str = base_handler::on_string_alloc(len);
				guarded_read(str, len * sizeof(CharType));
				str[len] = '\0';
				return {str, len};
			}

			[[nodiscard]] std::pair<detail::ubj_token, std::int64_t> parse_container_attributes()
			{
				std::pair<detail::ubj_token, std::int64_t> result = {detail::ubj_token::UBJ_INVALID_TOKEN, -1};

				switch (auto token = peek_token(); token)
				{
					case detail::ubj_token::UBJ_CONTAINER_TYPE:
					{
						/* Consume the token & read type. */
						bump_token();
						result.first = read_token();

						/* Container size always follows the type. */
						if ((token = peek_token()) != detail::ubj_token::UBJ_CONTAINER_SIZE) [[unlikely]]
							throw archive_error(bad_size_msg);
						[[fallthrough]];
					}
					case detail::ubj_token::UBJ_CONTAINER_SIZE:
					{
						bump_token();
						result.second = read_length();
						[[fallthrough]];
					}
					default: break;
				}

				return result;
			}
			void parse_array()
			{
				auto [data_type, size] = parse_container_attributes();

				if (size == -1) [[unlikely]] /* Fully dynamic array. */
				{
					base_handler::on_array_start();
					for (size = 0;; ++size)
					{
						auto token = read_token();
						if (token == detail::ubj_token::UBJ_ARRAY_END) [[unlikely]]
							break;
						parse_entry(token);
					}
				}
				else /* Fixed-size array. */
				{
					base_handler::on_array_start(static_cast<std::size_t>(size));
					if (data_type != detail::ubj_token::UBJ_INVALID_TOKEN) [[likely]] /* Fixed-type array. */
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
					auto [key_str, key_len] = read_string();
					base_handler::on_object_key(key_str, key_len);
				};
				auto [value_type, size] = parse_container_attributes();

				if (size == -1) [[unlikely]] /* Fully dynamic object. */
				{
					base_handler::on_object_start();
					for (size = 0;; ++size)
					{
						auto token = peek_token();
						if (token == detail::ubj_token::UBJ_OBJECT_END) [[unlikely]]
						{
							bump_token();
							break;
						}

						read_key();
						parse_entry();
					}
				}
				else /* Fixed-size object. */
				{
					base_handler::on_object_start(static_cast<std::size_t>(size));
					if (value_type != detail::ubj_token::UBJ_INVALID_TOKEN) [[likely]] /* Fixed-type object. */
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
			void parse_entry(detail::ubj_token token)
			{
				switch (token)
				{
					case detail::ubj_token::UBJ_NOOP: break;
					case detail::ubj_token::UBJ_NULL: base_handler::on_null(); break;
					case detail::ubj_token::UBJ_BOOL_TRUE: base_handler::on_true(); break;
					case detail::ubj_token::UBJ_BOOL_FALSE: base_handler::on_false(); break;
					case detail::ubj_token::UBJ_CHAR:
						base_handler::on_char(static_cast<CharType>(read_literal<char>()));
						break;
					case detail::ubj_token::UBJ_UINT8: base_handler::on_int(read_literal<std::uint8_t>()); break;
					case detail::ubj_token::UBJ_INT8: base_handler::on_int(read_literal<std::int8_t>()); break;
					case detail::ubj_token::UBJ_INT16: base_handler::on_int(read_literal<std::int16_t>()); break;
					case detail::ubj_token::UBJ_INT32: base_handler::on_int(read_literal<std::int32_t>()); break;
					case detail::ubj_token::UBJ_INT64: base_handler::on_int(read_literal<std::int64_t>()); break;
					case detail::ubj_token::UBJ_FLOAT32: base_handler::on_float(read_literal<float>()); break;
					case detail::ubj_token::UBJ_FLOAT64: base_handler::on_float(read_literal<double>()); break;
					case detail::ubj_token::UBJ_HIGHP:
						/* TODO: Handle different high-precision modes. */
					case detail::ubj_token::UBJ_STRING:
					{
						auto [str, len] = read_string();
						base_handler::on_string(str, len);
						break;
					}
					case detail::ubj_token::UBJ_ARRAY_START: parse_array(); break;
					case detail::ubj_token::UBJ_OBJECT_START: parse_object(); break;
					default: throw archive_error(data_msg);
				}
			}
			void parse_entry() { parse_entry(read_token()); }

			virtual std::size_t read(void *, std::size_t) = 0;
			virtual std::size_t bump(std::size_t) = 0;
			virtual int peek() = 0;
		};

		struct buffer_parser final : parser_base
		{
			constexpr buffer_parser(basic_ubj_input_archive *archive, const void *buff, std::size_t n) noexcept
				: parser_base(archive), curr(static_cast<const std::byte *>(buff)), end(curr + n)
			{
			}

			std::size_t read(void *dest, std::size_t n) noexcept final
			{
				auto new_curr = curr + n;
				if (new_curr >= end) [[unlikely]]
					new_curr = end;
				std::copy(curr, new_curr, static_cast<std::byte *>(dest));
				return static_cast<std::size_t>(new_curr - std::exchange(curr, new_curr));
			}
			std::size_t bump(std::size_t n) noexcept final
			{
				auto new_curr = curr + n;
				if (new_curr >= end) [[unlikely]]
					new_curr = end;
				return static_cast<std::size_t>(new_curr - std::exchange(curr, new_curr));
			}
			int peek() noexcept final
			{
				if (curr == end) [[unlikely]]
					return EOF;
				else
					return static_cast<int>(*curr);
			}

			const std::byte *curr;
			const std::byte *end;
		};
		struct file_parser final : parser_base
		{
			constexpr file_parser(basic_ubj_input_archive *archive, FILE *file) noexcept
				: parser_base(archive), file(file)
			{
			}

			std::size_t read(void *dest, std::size_t n) noexcept final { return fread(dest, 1, n, file); }
			std::size_t bump(std::size_t n) noexcept final
			{
#if defined(_POSIX_C_SOURCE)
#if _FILE_OFFSET_BITS < 64
				auto err = fseeko64(file, static_cast<off64_t>(n), SEEK_CUR);
#else
				auto err = fseeko(file, static_cast<off_t>(n), SEEK_CUR);
#endif
#elif defined(SEK_OS_WIN)
				auto err = _fseeki64(file, static_cast<__int64>(n), SEEK_CUR);
#else
				auto err = fseek(file, static_cast<long>(n), SEEK_CUR);
#endif

				if (!err) [[likely]]
					return n;
				else
					return 0;
			}
			int peek() noexcept final { return ungetc(getc(file), file); }

			FILE *file;
		};
		struct streambuf_parser final : parser_base
		{
			constexpr streambuf_parser(basic_ubj_input_archive *archive, std::streambuf *buff) noexcept
				: parser_base(archive), buff(buff)
			{
			}

			std::size_t read(void *dest, std::size_t n) noexcept final
			{
				return static_cast<std::size_t>(buff->sgetn(static_cast<char *>(dest), static_cast<std::streamsize>(n)));
			}
			std::size_t bump(std::size_t n) noexcept final
			{
				auto off = static_cast<std::streamoff>(n);
				if (buff->seekoff(off, std::ios::cur, std::ios::in) == std::streambuf::pos_type{off}) [[likely]]
					return n;
				else
					return 0;
			}
			int peek() noexcept final { return static_cast<int>(buff->sgetc()); }

			std::streambuf *buff;
		};

	public:
		basic_ubj_input_archive() = delete;
		basic_ubj_input_archive(const basic_ubj_input_archive &) = delete;
		basic_ubj_input_archive &operator=(const basic_ubj_input_archive &) = delete;

		constexpr basic_ubj_input_archive(basic_ubj_input_archive &&) noexcept = default;
		constexpr basic_ubj_input_archive &operator=(basic_ubj_input_archive &&) noexcept = default;

		/** Reads UBJson from a memory buffer.
		 * @param buff Pointer to the memory buffer containing UBJson data.
		 * @param len Size of the memory buffer. */
		basic_ubj_input_archive(const void *buff, std::size_t len) : base_t() { parse(buff, len); }
		/** @copydoc basic_ubj_input_archive
		 * @param res Memory resource used for internal allocation. */
		basic_ubj_input_archive(const void *buff, std::size_t len, std::pmr::memory_resource *res) : base_t(res)
		{
			parse(buff, len);
		}
		/** Reads UBJson from a file.
		 * @param file Pointer to the UBJson file.
		 * @note File must be opened in binary mode. */
		explicit basic_ubj_input_archive(FILE *file) : base_t() { parse(file); }
		/** @copydoc basic_ubj_input_archive
		 * @param res Memory resource used for internal allocation. */
		basic_ubj_input_archive(FILE *file, std::pmr::memory_resource *res) : base_t(res) { parse(file); }
		/** Reads UBJson from a stream buffer.
		 * @param buff Pointer to the stream buffer.
		 * @note Stream buffer must be a binary stream buffer. */
		explicit basic_ubj_input_archive(std::streambuf *buff) : base_t() { parse(buff); }
		/** @copydoc basic_ubj_input_archive
		 * @param res Memory resource used for internal allocation. */
		basic_ubj_input_archive(std::streambuf *buff, std::pmr::memory_resource *res) : base_t(res) { parse(buff); }
		/** Reads UBJson from an input stream.
		 * @param is Reference to the input stream.
		 * @note Stream must be a binary stream. */
		explicit basic_ubj_input_archive(std::istream &is) : basic_ubj_input_archive(is.rdbuf()) {}
		/** @copydoc basic_ubj_input_archive
		 * @param res Memory resource used for internal allocation. */
		basic_ubj_input_archive(std::istream &is, std::pmr::memory_resource *res)
			: basic_ubj_input_archive(is.rdbuf(), res)
		{
		}

		/** Attempts to deserialize the top-level Json entry of the archive.
		 * @param value Value to deserialize from the Json entry.
		 * @return true if deserialization was successful, false otherwise. */
		template<typename T>
		bool try_read(T &&value) const
		{
			return base_t::do_try_read(std::forward<T>(value));
		}
		/** Deserializes the top-level Json entry of the archive.
		 * @param value Value to deserialize from the Json entry.
		 * @return Reference to this archive.
		 * @throw archive_error On deserialization errors. */
		template<typename T>
		const basic_ubj_input_archive &read(T &&value) const
		{
			base_t::do_read(std::forward<T>(value));
			return *this;
		}
		/** @copydoc read */
		template<typename T>
		const basic_ubj_input_archive &operator>>(T &&value) const
		{
			return read(std::forward<T>(value));
		}
		/** Deserializes an instance of `T` from the top-level Json entry of the archive.
		 * @return Deserialized instance of `T`.
		 * @throw archive_error On deserialization errors. */
		template<std::default_initializable T>
		T read() const
		{
			T result;
			read(result);
			return result;
		}

		constexpr void swap(basic_ubj_input_archive &other) noexcept { base_t::swap(other); }
		friend constexpr void swap(basic_ubj_input_archive &a, basic_ubj_input_archive &b) noexcept { a.swap(b); }

	private:
		void parse(const void *buff, std::size_t len)
		{
			buffer_parser parser{this, buff, len};
			parser.parse_entry();
		}
		void parse(FILE *file)
		{
			file_parser parser{this, file};
			parser.parse_entry();
		}
		void parse(std::streambuf *buff)
		{
			streambuf_parser parser{this, buff};
			parser.parse_entry();
		}
	};

	typedef basic_ubj_input_archive<char> ubj_input_archive;

	static_assert(input_archive<ubj_input_archive::archive_frame, bool>);
	static_assert(input_archive<ubj_input_archive::archive_frame, char>);
	static_assert(input_archive<ubj_input_archive::archive_frame, std::uint8_t>);
	static_assert(input_archive<ubj_input_archive::archive_frame, std::int8_t>);
	static_assert(input_archive<ubj_input_archive::archive_frame, std::int16_t>);
	static_assert(input_archive<ubj_input_archive::archive_frame, std::int32_t>);
	static_assert(input_archive<ubj_input_archive::archive_frame, std::int64_t>);
	static_assert(input_archive<ubj_input_archive::archive_frame, float>);
	static_assert(input_archive<ubj_input_archive::archive_frame, double>);
	static_assert(input_archive<ubj_input_archive::archive_frame, std::string>);
	static_assert(container_like_archive<ubj_input_archive::archive_frame>);

	/** @details Archive used to write UBJson data. */
	template<typename CharType>
	class basic_ubj_output_archive
	{
	private:
		using mem_res_type = std::pmr::memory_resource;
		using sv_type = std::basic_string_view<CharType>;

		struct node_t;
		struct member_t;

		struct literal_t
		{
			union
			{
				CharType character;

				std::uint8_t uint8;
				std::int8_t int8;
				std::int16_t int16;
				std::int32_t int32;
				std::int64_t int64;

				float float32;
				double float64;
			};
		};
		struct container_t
		{
			union
			{
				void *data_ptr;
				node_t *array_data;
				member_t *object_data;
			};
			std::size_t size;
			std::size_t capacity;
			detail::ubj_token value_type;
		};
		struct node_t
		{
			union
			{
				literal_t literal;
				container_t container;
				sv_type string;
			};
			detail::ubj_token type;
		};
		struct member_t
		{
			node_t value;
			sv_type key;
		};

		struct emitter_base
		{
			emitter_base() = delete;

			void write_guarded(const void *src, std::size_t n)
			{
				if (write(src, n) != n) [[unlikely]]
					throw archive_error("UBJson: Emitter write failure");
			}
			void write_token(detail::ubj_token token) { write_guarded(&token, sizeof(token)); }
			template<typename T>
			void write_literal(T value)
			{
				/* Fix endianness from machine endianness to big endian.
				 * TODO: Only do this for Spec12 */
#ifndef SEK_ARCH_BIG_ENDIAN
				if constexpr (sizeof(T) == sizeof(std::uint16_t))
				{
					auto temp = bswap_16(std::bit_cast<std::uint16_t>(value));
					write_guarded(static_cast<const void *>(&temp), sizeof(temp));
				}
				else if constexpr (sizeof(T) == sizeof(std::uint32_t))
				{
					auto temp = bswap_32(std::bit_cast<std::uint32_t>(value));
					write_guarded(static_cast<const void *>(&temp), sizeof(temp));
				}
				else if constexpr (sizeof(T) == sizeof(std::uint64_t))
				{
					auto temp = bswap_64(std::bit_cast<std::uint64_t>(value));
					write_guarded(static_cast<const void *>(&temp), sizeof(temp));
				}
				else
#endif
					write_guarded(static_cast<const void *>(&value), sizeof(value));
			}

			void emit_length(std::int64_t length)
			{
				if (length > std::numeric_limits<std::int32_t>::max()) [[unlikely]]
				{
					write_token(detail::ubj_token::UBJ_INT64);
					write_literal(length);
				}
				else if (length > std::numeric_limits<std::int16_t>::max())
				{
					write_token(detail::ubj_token::UBJ_INT32);
					write_literal(static_cast<std::int32_t>(length));
				}
				else if (length > std::numeric_limits<std::uint8_t>::max())
				{
					write_token(detail::ubj_token::UBJ_INT16);
					write_literal(static_cast<std::uint16_t>(length));
				}
				else if (length > std::numeric_limits<std::int8_t>::max())
				{
					write_token(detail::ubj_token::UBJ_UINT8);
					write_literal(static_cast<std::uint8_t>(length));
				}
				else
				{
					write_token(detail::ubj_token::UBJ_INT8);
					write_literal(static_cast<std::int8_t>(length));
				}
			}
			void emit_string(sv_type str)
			{
				emit_length(static_cast<std::int64_t>(str.size()));
				write_guarded(static_cast<const void *>(str.data()), str.size() * sizeof(CharType));
			}

			std::pair<bool, bool> emit_container_attributes(const container_t &container)
			{
				std::pair<bool, bool> result = {};

				/* TODO: Add toggle for dynamic-type preference. */
				if (container.value_type != detail::ubj_token::UBJ_INVALID_TOKEN)
				{
					write_token(detail::ubj_token::UBJ_CONTAINER_TYPE);
					write_token(container.value_type);
					result.first = true;
				}

				/* TODO: Add toggle for dynamic-size preference. */
				write_token(detail::ubj_token::UBJ_CONTAINER_SIZE);
				emit_length(static_cast<std::int64_t>(container.size));
				result.second = true;

				return result;
			}
			void emit_array(const container_t &array)
			{
				auto [fixed_type, fixed_size] = emit_container_attributes(array);

				if (fixed_type) [[likely]] /* Expect that arrays are fixed-type. */
					for (std::size_t i = 0; i < array.size; ++i) emit_data(array.array_data[i]);
				else
					for (std::size_t i = 0; i < array.size; ++i) emit_node(array.array_data[i]);

				if (!fixed_size) [[unlikely]]
					write_token(detail::ubj_token::UBJ_ARRAY_END);
			}
			void emit_object(const container_t &object)
			{
				auto [fixed_type, fixed_size] = emit_container_attributes(object);

				if (fixed_type) [[unlikely]] /* Expect that objects are dynamic-type. */
					for (std::size_t i = 0; i < object.size; ++i)
					{
						auto &member = object.object_data[i];
						emit_string(member.key);
						emit_data(member.value);
					}
				else
					for (std::size_t i = 0; i < object.size; ++i)
					{
						auto &member = object.object_data[i];
						emit_string(member.key);
						emit_node(member.value);
					}

				if (!fixed_size) [[unlikely]]
					write_token(detail::ubj_token::UBJ_OBJECT_END);
			}
			void emit_data(const node_t &node)
			{
				switch (node.type)
				{
					case detail::ubj_token::UBJ_CHAR: write_literal(node.literal.character); break;
					case detail::ubj_token::UBJ_UINT8: write_literal(node.literal.uint8); break;
					case detail::ubj_token::UBJ_INT8: write_literal(node.literal.int8); break;
					case detail::ubj_token::UBJ_INT16: write_literal(node.literal.int16); break;
					case detail::ubj_token::UBJ_INT32: write_literal(node.literal.int32); break;
					case detail::ubj_token::UBJ_INT64: write_literal(node.literal.int64); break;
					case detail::ubj_token::UBJ_FLOAT32: write_literal(node.literal.float32); break;
					case detail::ubj_token::UBJ_FLOAT64: write_literal(node.literal.float64); break;

					case detail::ubj_token::UBJ_HIGHP: /* TODO: Implement high-precision number writing via a manipulator. */
					case detail::ubj_token::UBJ_STRING: emit_string(node.string); break;

					case detail::ubj_token::UBJ_ARRAY_START: emit_array(node.container); break;
					case detail::ubj_token::UBJ_OBJECT_START: emit_object(node.container); break;

					default: break;
				}
			}
			void emit_node(const node_t &node)
			{
				write_token(node.type);
				emit_data(node);
			}

			[[nodiscard]] virtual std::size_t write(const void *, std::size_t) = 0;
			[[nodiscard]] virtual std::size_t size_of() const noexcept = 0;
		};
		struct file_emitter final : emitter_base
		{
			constexpr explicit file_emitter(FILE *file) noexcept : file(file) {}

			[[nodiscard]] std::size_t write(const void *src, std::size_t n) final { return fwrite(src, 1, n, file); }
			[[nodiscard]] std::size_t size_of() const noexcept final { return sizeof(file_emitter); }

			FILE *file;
		};
		struct buffer_emitter final : emitter_base
		{
			constexpr buffer_emitter(void *buff, std::size_t n) noexcept
				: curr(static_cast<std::byte *>(buff)), end(curr + n)
			{
			}

			[[nodiscard]] std::size_t write(const void *src, std::size_t n) noexcept final
			{
				if (curr + n > end) [[unlikely]]
					n = end - curr;
				curr = std::copy_n(static_cast<const std::byte *>(src), n, curr);
				return n;
			}
			[[nodiscard]] std::size_t size_of() const noexcept final { return sizeof(buffer_emitter); }

			std::byte *curr;
			std::byte *end;
		};
		struct streambuf_emitter final : emitter_base
		{
			constexpr explicit streambuf_emitter(std::streambuf *buff) noexcept : buff(buff) {}

			[[nodiscard]] std::size_t write(const void *src, std::size_t n) noexcept final
			{
				auto ptr = static_cast<const std::streambuf::char_type *>(src);
				return static_cast<std::size_t>(buff->sputn(ptr, static_cast<std::streamsize>(n)));
			}
			[[nodiscard]] std::size_t size_of() const noexcept final { return sizeof(streambuf_emitter); }

			std::streambuf *buff;
		};

		class write_frame
		{
			friend class basic_ubj_output_archive;

			constexpr write_frame(basic_ubj_output_archive *parent, node_t *node) noexcept
				: parent(parent), current(node)
			{
			}

		public:
			write_frame() = delete;
			write_frame(const write_frame &) = delete;
			write_frame &operator=(const write_frame &) = delete;
			write_frame(write_frame &&) = delete;
			write_frame &operator=(write_frame &&) = delete;

			/** Serialized the forwarded value to UBJson.
			 * @param value Value to serialize as UBJson.
			 * @return Reference to this frame. */
			template<typename T>
			write_frame &write(T &&value)
			{
				write_impl(std::forward<T>(value));
				return *this;
			}
			/** @copydoc write */
			template<typename T>
			write_frame &operator<<(T &&value)
			{
				return write(std::forward<T>(value));
			}

		private:
			template<typename T>
			void write_impl(T &&value)
			{
			}

			basic_ubj_output_archive *parent;
			node_t *current;
		};

	public:
		basic_ubj_output_archive() = delete;
		basic_ubj_output_archive(const basic_ubj_output_archive &) = delete;
		basic_ubj_output_archive &operator=(const basic_ubj_output_archive &) = delete;

		constexpr basic_ubj_output_archive(basic_ubj_output_archive &&other) noexcept
			: emitter(nullptr), upstream(nullptr)
		{
			swap(other);
		}
		constexpr basic_ubj_output_archive &operator=(basic_ubj_output_archive &&other) noexcept
		{
			swap(other);
			return *this;
		}

		explicit basic_ubj_output_archive(FILE *file) : basic_ubj_output_archive(file, std::pmr::get_default_resource())
		{
		}
		basic_ubj_output_archive(FILE *file, mem_res_type *upstream) : upstream(upstream)
		{
			init_emitter<file_emitter>(file);
		}

		basic_ubj_output_archive(void *buff, std::size_t size)
			: basic_ubj_output_archive(buff, size, std::pmr::get_default_resource())
		{
		}
		basic_ubj_output_archive(void *buff, std::size_t size, mem_res_type *upstream) : upstream(upstream)
		{
			init_emitter<buffer_emitter>(buff, size);
		}

		explicit basic_ubj_output_archive(std::streambuf *buff)
			: basic_ubj_output_archive(buff, std::pmr::get_default_resource())
		{
		}
		basic_ubj_output_archive(std::streambuf *buff, mem_res_type *upstream) : upstream(upstream)
		{
			init_emitter<streambuf_emitter>(buff);
		}

		~basic_ubj_output_archive()
		{
			if (flush_impl()) [[likely]]
				upstream->deallocate(emitter, emitter->size_of());
		}

		/** Serialized the forwarded value to UBJson. Flushes previous uncommitted state.
		 * @param value Value to serialize as UBJson.
		 * @return Reference to this archive.
		 * @note Serialized data is kept inside the archive's internal state and will be written to the output once the
		 * archive is destroyed or `flush` is called. */
		template<typename T>
		basic_ubj_output_archive &write(T &&value)
		{
			auto top_frame = init_write_frame();
			top_frame.write(std::forward<T>(value));
			return *this;
		}
		/** @copydoc write */
		template<typename T>
		basic_ubj_output_archive &operator<<(T &&value)
		{
			return write(std::forward<T>(value));
		}

		/** Flushes the internal state & writes UBJson to the output. */
		void flush()
		{
			flush_impl();
			top_level = nullptr;
		}

		constexpr void swap(basic_ubj_output_archive &other) noexcept
		{
			using std::swap;
			swap(emitter, other.emitter);
			swap(top_level, other.top_level);
			swap(upstream, other.upstream);

			node_pool.swap(other.node_pool);
			string_pool.swap(other.string_pool);
		}
		friend constexpr void swap(basic_ubj_output_archive &a, basic_ubj_output_archive &b) noexcept { a.swap(b); }

	private:
		template<typename T, typename... Args>
		void init_emitter(Args &&...args)
		{
			auto *new_emitter = static_cast<T *>(upstream->allocate(sizeof(T)));
			if (!new_emitter) [[unlikely]]
				throw std::bad_cast();
			std::construct_at(new_emitter, std::forward<Args>(args)...);
			emitter = static_cast<emitter_base *>(new_emitter);
		}
		bool flush_impl()
		{
			if (emitter && top_level) [[likely]]
			{
				emitter->emit_node(*top_level);
				node_pool.release(upstream);
				string_pool.release(upstream);
				return true;
			}
			return false;
		}
		[[nodiscard]] write_frame init_write_frame()
		{
			/* Flush uncommitted changes before initializing a new emit tree. */
			flush_impl();

			auto *node = static_cast<node_t *>(node_pool.allocate(upstream, sizeof(node_t)));
			if (!node) [[unlikely]]
				throw std::bad_alloc();
			top_level = node;
			return write_frame{this, node};
		}

		emitter_base *emitter;			   /* Emitter used for writing. Allocated from the upstream allocator. */
		const node_t *top_level = nullptr; /* Top-level node of the node tree. */

		mem_res_type *upstream; /* Upstream allocator used for memory pools. */

		detail::basic_pool_allocator<sizeof(node_t) * 64> node_pool; /* Pool used for the node tree. */
		detail::basic_pool_allocator<SEK_KB(1)> string_pool; /* Pool used to allocate copies of output strings. */
	};

	typedef basic_ubj_output_archive<char> ubj_output_archive;

	//	static_assert(output_archive<ubj_output_archive, bool>);
	//	static_assert(output_archive<ubj_output_archive, char>);
	//	static_assert(output_archive<ubj_output_archive, std::uint8_t>);
	//	static_assert(output_archive<ubj_output_archive, std::int8_t>);
	//	static_assert(output_archive<ubj_output_archive, std::int16_t>);
	//	static_assert(output_archive<ubj_output_archive, std::int32_t>);
	//	static_assert(output_archive<ubj_output_archive, std::int64_t>);
	//	static_assert(output_archive<ubj_output_archive, float>);
	//	static_assert(output_archive<ubj_output_archive, double>);
	//	static_assert(output_archive<ubj_output_archive, std::string>);
}	 // namespace sek::serialization