//
// Created by switchblade on 2022-04-14.
//

#pragma once

#include <iostream>

#include "../archive_traits.hpp"
#include "../manipulators.hpp"
#include "common.hpp"
#include "sekhmet/detail/bswap.hpp"

namespace sek::serialization::ubj
{
	namespace detail
	{
		using namespace serialization::detail;

		enum token_t : std::int8_t
		{
			INVALID_TYPE = 0,
			DYNAMIC_TYPE = INT8_MAX,

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
	}	 // namespace detail

	typedef int config_flags;

	/** Enables fixed-size container output. */
	constexpr static config_flags fixed_size = 1;
	/** Enables fixed-type containers output. Implies `fixed_size`. */
	constexpr static config_flags fixed_type = 2 | fixed_size;
	/** Enables integer size packing (will use smallest integer size possible to represent any integral value).
	 * @note Decreases performance since every integer's size will need to be checked at runtime.
	 * @note Container sizes will always be packed. */
	constexpr static config_flags pack_integers = 4;

	/** Treat high-precision numbers as input errors. */
	constexpr static config_flags highp_error = 8;
	/** Parse high-precision numbers as strings. */
	constexpr static config_flags highp_as_string = 16;
	/** Skip high-precision numbers (not recommended). */
	constexpr static config_flags highp_skip = 32;

	/** @details Archive used to read UBJson data.
	 *
	 * The archive itself does not do any de-serialization, instead deserialization is done by special archive frames,
	 * which represent a Json object or array. These frames are then passed to deserialization functions
	 * of serializable types.
	 *
	 * @tparam CharType Character type used for Json. */
	template<config_flags Config, typename CharType = char>
	class basic_input_archive : detail::json_input_archive_base<CharType>
	{
		using base_t = serialization::detail::json_input_archive_base<CharType>;

	public:
		typedef typename base_t::read_frame archive_frame;
		typedef typename archive_frame::archive_category archive_category;

	private:
		struct parser_base : base_t::parse_event_handler
		{
			using base_handler = typename base_t::parse_event_handler;

			constexpr static auto eof_msg = "UBJson: Unexpected end of input";
			constexpr static auto data_msg = "UBJson: Invalid input";
			constexpr static auto bad_length_msg = "UBJson: Invalid input, expected integer type";
			constexpr static auto bad_size_msg = "UBJson: Invalid input, expected container size";

			parser_base() = delete;
			constexpr explicit parser_base(basic_input_archive *archive, std::pmr::memory_resource *res) noexcept
				: base_handler(archive, res)
			{
			}

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
			detail::token_t read_token()
			{
				detail::token_t token;
				guarded_read(&token, sizeof(detail::token_t));
				return token;
			}
			detail::token_t peek_token()
			{
				if (auto c = peek(); c == EOF) [[unlikely]]
					throw archive_error(eof_msg);
				else
					return static_cast<detail::token_t>(c);
			}
			void bump_token() { guarded_bump(sizeof(detail::token_t)); }

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
					case detail::token_t::UINT8: return static_cast<std::int64_t>(read_literal<std::uint8_t>());
					case detail::token_t::INT8: return static_cast<std::int64_t>(read_literal<std::int8_t>());
					case detail::token_t::INT16: return static_cast<std::int64_t>(read_literal<std::int16_t>());
					case detail::token_t::INT32: return static_cast<std::int64_t>(read_literal<std::int32_t>());
					case detail::token_t::INT64: return read_literal<std::int64_t>();
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

			[[nodiscard]] std::pair<detail::token_t, std::int64_t> parse_container_attributes()
			{
				std::pair<detail::token_t, std::int64_t> result = {detail::token_t::DYNAMIC_TYPE, -1};

				switch (auto token = peek_token(); token)
				{
					case detail::token_t::CONTAINER_TYPE:
					{
						/* Consume the token & read type. */
						bump_token();
						result.first = read_token();

						/* Container size always follows the type. */
						if ((token = peek_token()) != detail::token_t::CONTAINER_SIZE) [[unlikely]]
							throw archive_error(bad_size_msg);
						[[fallthrough]];
					}
					case detail::token_t::CONTAINER_SIZE:
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
						if (token == detail::token_t::ARRAY_END) [[unlikely]]
							break;
						parse_entry(token);
					}
				}
				else /* Fixed-size array. */
				{
					base_handler::on_array_start(static_cast<std::size_t>(size));
					if (data_type != detail::token_t::DYNAMIC_TYPE) [[likely]] /* Fixed-type array. */
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
						if (token == detail::token_t::OBJECT_END) [[unlikely]]
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
					if (value_type != detail::token_t::DYNAMIC_TYPE) [[likely]] /* Fixed-type object. */
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
					case detail::token_t::CHAR:
						base_handler::on_char(static_cast<CharType>(read_literal<char>()));
						break;
					case detail::token_t::UINT8: base_handler::on_int(read_literal<std::uint8_t>()); break;
					case detail::token_t::INT8: base_handler::on_int(read_literal<std::int8_t>()); break;
					case detail::token_t::INT16: base_handler::on_int(read_literal<std::int16_t>()); break;
					case detail::token_t::INT32: base_handler::on_int(read_literal<std::int32_t>()); break;
					case detail::token_t::INT64: base_handler::on_int(read_literal<std::int64_t>()); break;
					case detail::token_t::FLOAT32: base_handler::on_float(read_literal<float>()); break;
					case detail::token_t::FLOAT64: base_handler::on_float(read_literal<double>()); break;
					case detail::token_t::HIGHP:
					{
						if constexpr (Config & highp_error)
							throw archive_error("High-precision number support is disabled");
						else if constexpr (Config & highp_skip)
							break;
						[[fallthrough]];
					}
					case detail::token_t::STRING:
					{
						auto [str, len] = read_string();
						base_handler::on_string(str, len);
						break;
					}
					case detail::token_t::ARRAY_START: parse_array(); break;
					case detail::token_t::OBJECT_START: parse_object(); break;
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
			constexpr buffer_parser(basic_input_archive *archive, std::pmr::memory_resource *res, const void *buff, std::size_t n) noexcept
				: parser_base(archive, res), curr(static_cast<const std::byte *>(buff)), end(curr + n)
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
			constexpr file_parser(basic_input_archive *archive, std::pmr::memory_resource *res, FILE *file) noexcept
				: parser_base(archive, res), file(file)
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
			constexpr streambuf_parser(basic_input_archive *archive, std::pmr::memory_resource *res, std::streambuf *buff) noexcept
				: parser_base(archive, res), buff(buff)
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
		basic_input_archive() = delete;
		basic_input_archive(const basic_input_archive &) = delete;
		basic_input_archive &operator=(const basic_input_archive &) = delete;

		constexpr basic_input_archive(basic_input_archive &&) noexcept = default;
		constexpr basic_input_archive &operator=(basic_input_archive &&) noexcept = default;

		/** Reads UBJson from a memory buffer.
		 * @param buff Pointer to the memory buffer containing UBJson data.
		 * @param len Size of the memory buffer. */
		basic_input_archive(const void *buff, std::size_t len)
			: basic_input_archive(buff, len, std::pmr::get_default_resource())
		{
		}
		/** @copydoc basic_input_archive
		 * @param res PMR memory resource used for internal allocation. */
		basic_input_archive(const void *buff, std::size_t len, std::pmr::memory_resource *res) : base_t(res)
		{
			parse(buff, len, res);
		}
		/** Reads UBJson from a file.
		 * @param file Pointer to the UBJson file.
		 * @note File must be opened in binary mode. */
		explicit basic_input_archive(FILE *file) : basic_input_archive(file, std::pmr::get_default_resource()) {}
		/** @copydoc basic_input_archive
		 * @param res Memory resource used for internal allocation. */
		basic_input_archive(FILE *file, std::pmr::memory_resource *res) : base_t(res) { parse(file, res); }
		/** Reads UBJson from a stream buffer.
		 * @param buff Pointer to the stream buffer.
		 * @note Stream buffer must be a binary stream buffer. */
		explicit basic_input_archive(std::streambuf *buff) : basic_input_archive(buff, std::pmr::get_default_resource())
		{
		}
		/** @copydoc basic_input_archive
		 * @param res Memory resource used for internal allocation. */
		basic_input_archive(std::streambuf *buff, std::pmr::memory_resource *res) : base_t(res) { parse(buff, res); }
		/** Reads UBJson from an input stream.
		 * @param is Reference to the input stream.
		 * @note Stream must be a binary stream. */
		explicit basic_input_archive(std::istream &is) : basic_input_archive(is.rdbuf()) {}
		/** @copydoc basic_input_archive
		 * @param res Memory resource used for internal allocation. */
		basic_input_archive(std::istream &is, std::pmr::memory_resource *res) : basic_input_archive(is.rdbuf(), res) {}

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
		const basic_input_archive &read(T &&value) const
		{
			base_t::do_read(std::forward<T>(value));
			return *this;
		}
		/** @copydoc read */
		template<typename T>
		const basic_input_archive &operator>>(T &&value) const
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

		constexpr void swap(basic_input_archive &other) noexcept { base_t::swap(other); }
		friend constexpr void swap(basic_input_archive &a, basic_input_archive &b) noexcept { a.swap(b); }

	private:
		void parse(const void *buff, std::size_t len, std::pmr::memory_resource *res)
		{
			buffer_parser parser{this, res, buff, len};
			parser.parse_entry();
		}
		void parse(FILE *file, std::pmr::memory_resource *res)
		{
			file_parser parser{this, res, file};
			parser.parse_entry();
		}
		void parse(std::streambuf *buff, std::pmr::memory_resource *res)
		{
			streambuf_parser parser{this, res, buff};
			parser.parse_entry();
		}
	};

	typedef basic_input_archive<highp_error, char> input_archive;

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
	static_assert(serialization::container_like_archive<input_archive::archive_frame>);

	/** @details Archive used to write UBJson data. */
	template<config_flags Config, typename CharType = char>
	class basic_output_archive
	{
	private:
		using sv_type = std::basic_string_view<CharType>;

		struct entry_t;
		struct member_t;

		union literal_t
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
		struct container_t
		{
			union
			{
				void *data_ptr = nullptr;
				entry_t *array_data;
				member_t *object_data;
			};
			std::size_t size = 0;
			std::size_t capacity = 0;
			detail::token_t value_type = detail::token_t::INVALID_TYPE;
		};
		struct entry_t
		{
			constexpr entry_t() noexcept : container(), type(detail::token_t::INVALID_TYPE) {}

			union
			{
				literal_t literal;
				sv_type string;
				container_t container;
			};
			detail::token_t type;
		};
		struct member_t
		{
			entry_t value;
			sv_type key;
		};

		struct emitter_base
		{
			void write_guarded(const void *src, std::size_t n)
			{
				if (write(this, src, n) != n) [[unlikely]]
					throw archive_error("UBJson: Emitter write failure");
			}
			void write_token(detail::token_t token) { write_guarded(&token, sizeof(token)); }
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
					write_token(detail::token_t::INT64);
					write_literal(length);
				}
				else if (length > std::numeric_limits<std::int16_t>::max())
				{
					write_token(detail::token_t::INT32);
					write_literal(static_cast<std::int32_t>(length));
				}
				else if (length > std::numeric_limits<std::uint8_t>::max())
				{
					write_token(detail::token_t::INT16);
					write_literal(static_cast<std::uint16_t>(length));
				}
				else if (length > std::numeric_limits<std::int8_t>::max())
				{
					write_token(detail::token_t::UINT8);
					write_literal(static_cast<std::uint8_t>(length));
				}
				else
				{
					write_token(detail::token_t::INT8);
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

				if constexpr ((Config & fixed_type) == fixed_type)
					if (container.value_type != detail::token_t::DYNAMIC_TYPE)
					{
						write_token(detail::token_t::CONTAINER_TYPE);
						write_token(container.value_type);
						result.first = true;
					}
				if constexpr (Config & fixed_size)
				{
					write_token(detail::token_t::CONTAINER_SIZE);
					emit_length(static_cast<std::int64_t>(container.size));
					result.second = true;
				}

				return result;
			}
			void emit_array(const container_t &array)
			{
				auto [is_fixed_type, is_fixed_size] = emit_container_attributes(array);

				if (is_fixed_type) [[likely]] /* Expect that arrays are fixed-type. */
					for (std::size_t i = 0; i < array.size; ++i) emit_data(array.array_data[i]);
				else
					for (std::size_t i = 0; i < array.size; ++i) emit_entry(array.array_data[i]);

				if (!is_fixed_size) [[unlikely]]
					write_token(detail::token_t::ARRAY_END);
			}
			void emit_object(const container_t &object)
			{
				auto [is_fixed_type, is_fixed_size] = emit_container_attributes(object);

				if (is_fixed_type) [[unlikely]] /* Expect that objects are dynamic-type. */
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
						emit_entry(member.value);
					}

				if (!is_fixed_size) [[unlikely]]
					write_token(detail::token_t::OBJECT_END);
			}
			void emit_data(const entry_t &entry)
			{
				switch (entry.type)
				{
					case detail::token_t::CHAR: write_literal(entry.literal.character); break;
					case detail::token_t::UINT8: write_literal(entry.literal.uint8); break;
					case detail::token_t::INT8: write_literal(entry.literal.int8); break;
					case detail::token_t::INT16: write_literal(entry.literal.int16); break;
					case detail::token_t::INT32: write_literal(entry.literal.int32); break;
					case detail::token_t::INT64: write_literal(entry.literal.int64); break;
					case detail::token_t::FLOAT32: write_literal(entry.literal.float32); break;
					case detail::token_t::FLOAT64: write_literal(entry.literal.float64); break;

					case detail::token_t::HIGHP: /* TODO: Implement high-precision number output support. */
					case detail::token_t::STRING: emit_string(entry.string); break;

					case detail::token_t::ARRAY_START: emit_array(entry.container); break;
					case detail::token_t::OBJECT_START: emit_object(entry.container); break;

					default: break;
				}
			}
			void emit_entry(const entry_t &entry)
			{
				write_token(entry.type);
				emit_data(entry);
			}

			std::size_t (*write)(void *, const void *, std::size_t);
		};
		struct file_emitter_t final : emitter_base
		{
			constexpr explicit file_emitter_t(FILE *file) noexcept : file(file)
			{
				emitter_base::write = +[](void *ptr, const void *src, std::size_t n) -> std::size_t
				{
					auto emitter = static_cast<file_emitter_t *>(ptr);
					return fwrite(src, 1, n, emitter->file);
				};
			}

			FILE *file = nullptr;
		};
		struct buffer_emitter_t final : emitter_base
		{
			constexpr buffer_emitter_t(void *buff, std::size_t n) noexcept
				: curr(static_cast<std::byte *>(buff)), end(curr + n)
			{
				emitter_base::write = +[](void *ptr, const void *src, std::size_t n) -> std::size_t
				{
					auto emitter = static_cast<buffer_emitter_t *>(ptr);
					if (emitter->curr + n > emitter->end) [[unlikely]]
						n = emitter->end - emitter->curr;
					emitter->curr = std::copy_n(static_cast<const std::byte *>(src), n, emitter->curr);
					return n;
				};
			}

			std::byte *curr = nullptr;
			std::byte *end = nullptr;
		};
		struct streambuf_emitter_t final : emitter_base
		{
			constexpr explicit streambuf_emitter_t(std::streambuf *buff) noexcept : buff(buff)
			{
				emitter_base::write = +[](void *ptr, const void *src, std::size_t n) -> std::size_t
				{
					auto emitter = static_cast<streambuf_emitter_t *>(ptr);
					auto data = static_cast<const std::streambuf::char_type *>(src);
					return static_cast<std::size_t>(emitter->buff->sputn(data, static_cast<std::streamsize>(n)));
				};
			}

			std::streambuf *buff = nullptr;
		};

		class write_frame
		{
			friend class basic_output_archive;

		public:
			typedef output_archive_category archive_category;

		private:
			constexpr write_frame(basic_output_archive &parent, entry_t &entry) noexcept
				: parent(parent), current(entry)
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
			[[nodiscard]] CharType *alloc_string(std::size_t n) const
			{
				auto bytes = (n + 1) * sizeof(CharType);
				auto result = static_cast<CharType *>(parent.string_pool.allocate(bytes));
				if (!result) [[unlikely]]
					throw std::bad_alloc();
				return result;
			}
			[[nodiscard]] sv_type copy_string(sv_type str) const
			{
				auto result = alloc_string(str.size());
				*std::copy_n(str.data(), str.size(), result) = '\0';
				return {result, str.size()};
			}

			[[nodiscard]] sv_type get_next_key(sv_type key) const { return copy_string(key); }
			[[nodiscard]] sv_type get_next_key() const
			{
				constexpr CharType prefix[] = "value_";
				constexpr auto prefix_size = SEK_ARRAY_SIZE(prefix) - 1;

				/* Format the current index into the buffer. */
				CharType buffer[20];
				std::size_t i = 20;
				for (auto idx = current.container.size;;) /* Write index digits to the buffer. */
				{
					buffer[--i] = static_cast<CharType>('0') + static_cast<CharType>(idx % 10);
					if (!(idx = idx / 10)) break;
				}

				auto key_size = SEK_ARRAY_SIZE(buffer) - i + prefix_size;
				auto key_str = alloc_string(key_size);

				std::copy_n(prefix, SEK_ARRAY_SIZE(prefix) - 1, key_str);					   /* Copy prefix. */
				std::copy(buffer + i, buffer + SEK_ARRAY_SIZE(buffer), key_str + prefix_size); /* Copy digits. */
				key_str[key_size] = '\0';

				return {key_str, key_size};
			}

			template<typename T>
			void resize_container(std::size_t n) const
			{
				auto *old_data = current.container.data_ptr;
				auto old_cap = current.container.capacity * sizeof(T), new_cap = n * sizeof(T);

				auto *new_data = parent.entry_pool.reallocate(old_data, old_cap, new_cap);
				if (!new_data) [[unlikely]]
					throw std::bad_alloc();

				current.container.data_ptr = new_data;
				current.container.capacity = n;
			}
			template<typename T>
			[[nodiscard]] T *push_container() const
			{
				auto next_idx = current.container.size;
				if (current.container.capacity == current.container.size++)
					resize_container<T>(current.container.size * 2);
				return static_cast<T *>(current.container.data_ptr) + next_idx;
			}
			[[nodiscard]] entry_t *next_entry() const
			{
				entry_t *entry;
				switch (current.type)
				{
					default:
					{
						current.type = detail::token_t::OBJECT_START;
						[[fallthrough]];
					}
					case detail::token_t::OBJECT_START:
					{
						auto member = push_container<member_t>();
						member->key = next_key;
						entry = &member->value;
						break;
					}
					case detail::token_t::ARRAY_START:
					{
						entry = push_container<entry_t>();
						break;
					}
				}
				return std::construct_at(entry);
			}

			template<typename T>
			void write_value(entry_t &entry, T &&value) const
			{
				write_frame frame{parent, entry};
				detail::invoke_serialize(std::forward<T>(value), frame);
			}

			void write_value(entry_t &entry, std::nullptr_t) const { entry.type = detail::token_t::NULL_ENTRY; }
			void write_value(entry_t &entry, bool b) const
			{
				entry.type = b ? detail::token_t::BOOL_TRUE : detail::token_t::BOOL_FALSE;
			}
			void write_value(entry_t &entry, CharType c) const
			{
				entry.type = detail::token_t::CHAR;
				entry.literal.character = c;
			}

			template<std::unsigned_integral I>
			void write_int_packed(entry_t &entry, I i) const
			{
				if (i > static_cast<I>(std::numeric_limits<std::int32_t>::max()))
				{
					entry.type = detail::token_t::INT64;
					entry.literal.int64 = static_cast<std::int64_t>(i);
				}
				else if (i > static_cast<I>(std::numeric_limits<std::int16_t>::max()))
				{
					entry.type = detail::token_t::INT32;
					entry.literal.int32 = static_cast<std::int32_t>(i);
				}
				else if (i > static_cast<I>(std::numeric_limits<std::uint8_t>::max()))
				{
					entry.type = detail::token_t::INT16;
					entry.literal.int16 = static_cast<std::int16_t>(i);
				}
				else if (i > static_cast<I>(std::numeric_limits<std::int8_t>::max()))
				{
					entry.type = detail::token_t::UINT8;
					entry.literal.uint8 = static_cast<std::uint8_t>(i);
				}
				else
				{
					entry.type = detail::token_t::INT8;
					entry.literal.int8 = static_cast<std::int8_t>(i);
				}
			}
			template<std::signed_integral I>
			void write_int_packed(entry_t &entry, I i) const
			{
				if (i < 0)
				{
					if (i < static_cast<I>(std::numeric_limits<std::int32_t>::min()))
					{
						entry.type = detail::token_t::INT64;
						entry.literal.int64 = static_cast<std::int64_t>(i);
					}
					else if (i < static_cast<I>(std::numeric_limits<std::int16_t>::min()))
					{
						entry.type = detail::token_t::INT32;
						entry.literal.int32 = static_cast<std::int32_t>(i);
					}
					else if (i < static_cast<I>(std::numeric_limits<std::int8_t>::min()))
					{
						entry.type = detail::token_t::INT16;
						entry.literal.int16 = static_cast<std::int16_t>(i);
					}
					else
					{
						entry.type = detail::token_t::INT8;
						entry.literal.int8 = static_cast<std::int8_t>(i);
					}
				}
				else
					write_int_packed(entry, static_cast<std::make_unsigned_t<I>>(i));
			}
			template<std::unsigned_integral I>
			void write_int_fast(entry_t &entry, I i) const
			{
				if constexpr (std::numeric_limits<I>::max() > static_cast<I>(std::numeric_limits<std::int32_t>::max()))
				{
					entry.type = detail::token_t::INT64;
					entry.literal.int64 = static_cast<std::int64_t>(i);
				}
				else if constexpr (std::numeric_limits<I>::max() > static_cast<I>(std::numeric_limits<std::int16_t>::max()))
				{
					entry.type = detail::token_t::INT32;
					entry.literal.int32 = static_cast<std::int32_t>(i);
				}
				else if constexpr (std::numeric_limits<I>::max() > static_cast<I>(std::numeric_limits<std::uint8_t>::max()))
				{
					entry.type = detail::token_t::INT16;
					entry.literal.int16 = static_cast<std::int16_t>(i);
				}
				else if constexpr (std::numeric_limits<I>::max() > static_cast<I>(std::numeric_limits<std::int8_t>::max()))
				{
					entry.type = detail::token_t::UINT8;
					entry.literal.uint8 = static_cast<std::uint8_t>(i);
				}
				else
				{
					entry.type = detail::token_t::INT8;
					entry.literal.int8 = static_cast<std::int8_t>(i);
				}
			}
			template<std::signed_integral I>
			void write_int_fast(entry_t &entry, I i) const
			{
				if constexpr (std::numeric_limits<I>::max() > static_cast<I>(std::numeric_limits<std::int32_t>::max()) ||
							  std::numeric_limits<I>::min() < static_cast<I>(std::numeric_limits<std::int32_t>::min()))
				{
					entry.type = detail::token_t::INT64;
					entry.literal.int64 = static_cast<std::int64_t>(i);
				}
				else if constexpr (std::numeric_limits<I>::max() > static_cast<I>(std::numeric_limits<std::int16_t>::max()) ||
								   std::numeric_limits<I>::min() < static_cast<I>(std::numeric_limits<std::int16_t>::min()))
				{
					entry.type = detail::token_t::INT32;
					entry.literal.int32 = static_cast<std::int32_t>(i);
				}
				else if constexpr (std::numeric_limits<I>::max() > static_cast<I>(std::numeric_limits<std::int8_t>::max()) ||
								   std::numeric_limits<I>::min() < static_cast<I>(std::numeric_limits<std::int8_t>::min()))
				{
					entry.type = detail::token_t::INT16;
					entry.literal.int16 = static_cast<std::int16_t>(i);
				}
				else
				{
					entry.type = detail::token_t::INT8;
					entry.literal.int8 = static_cast<std::int8_t>(i);
				}
			}

			template<typename I>
			void write_value(entry_t &entry, I &&i) const requires std::integral<std::decay_t<I>>
			{
				std::decay_t<I> value = i;

				if constexpr (Config & pack_integers)
					write_int_packed(entry, value);
				else
					write_int_fast(entry, value);
			}

			void write_value(entry_t &entry, float f) const
			{
				entry.type = detail::token_t::FLOAT32;
				entry.literal.float32 = f;
			}
			void write_value(entry_t &entry, double d) const
			{
				entry.type = detail::token_t::FLOAT64;
				entry.literal.float64 = d;
			}
			template<std::floating_point T>
			void write_value(entry_t &entry, T f) const
			{
				if constexpr (std::numeric_limits<T>::max() > static_cast<T>(std::numeric_limits<float>::max()) &&
							  std::numeric_limits<T>::min() < static_cast<T>(std::numeric_limits<float>::min()))
					write_value(entry, static_cast<double>(f));
				else
					write_value(entry, static_cast<float>(f));
			}

			void write_value(entry_t &entry, sv_type sv) const
			{
				entry.type = detail::token_t::STRING;
				entry.string = copy_string(sv);
			}
			void write_value(entry_t &entry, const CharType *str) const { write_value(entry, sv_type{str}); }
			template<typename Traits>
			void write_value(entry_t &entry, std::basic_string<CharType, Traits> &&str) const
			{
				write_value(entry, sv_type{str});
			}
			template<typename Traits>
			void write_value(entry_t &entry, std::basic_string<CharType, Traits> &str) const
			{
				write_value(entry, sv_type{str});
			}
			template<typename Traits>
			void write_value(entry_t &entry, const std::basic_string<CharType, Traits> &str) const
			{
				write_value(entry, sv_type{str});
			}
			template<typename T>
			void write_value(entry_t &entry, T &&str) const requires std::constructible_from<sv_type, T>
			{
				write_value(entry, sv_type{std::forward<T>(str)});
			}
			template<typename T>
			void write_value(entry_t &entry, T &&str) const requires std::convertible_to<T, const CharType *>
			{
				write_value(entry, static_cast<const CharType *>(str));
			}
			template<typename T>
			void write_value(T &&value) const
			{
				auto entry = next_entry();
				SEK_ASSERT(entry != nullptr);
				write_value(*entry, std::forward<T>(value));

				if constexpr ((Config & fixed_type) == fixed_type)
					if (current.container.value_type != entry->type) [[likely]]
					{
						if (current.container.value_type == detail::token_t::INVALID_TYPE)
							current.container.value_type = entry->type;
						else
							current.container.value_type = detail::token_t::DYNAMIC_TYPE;
					}
			}

			template<typename T>
			void write_impl(T &&value)
			{
				next_key = get_next_key();
				write_value(std::forward<T>(value));
			}
			template<typename T>
			void write_impl(named_entry_t<CharType, T> value)
			{
				next_key = get_next_key(value.name);
				write_value(std::forward<T>(value.value));
			}
			template<typename T>
			void write_impl(container_size_t<T> size)
			{
				switch (current.type)
				{
					default:
					{
						current.type = detail::token_t::OBJECT_START;
						[[fallthrough]];
					}
					case detail::token_t::OBJECT_START:
					{
						resize_container<member_t>(static_cast<std::size_t>(size.value));
						break;
					}
					case detail::token_t::ARRAY_START:
					{
						resize_container<entry_t>(static_cast<std::size_t>(size.value));
						break;
					}
				}
			}
			void write_impl(array_mode_t)
			{
				SEK_ASSERT(current.type != detail::token_t::OBJECT_START, "Array modifier modifier applied to object entry");
				current.type = detail::token_t::ARRAY_START;
			}

			basic_output_archive &parent;
			entry_t &current;

			sv_type next_key = {};
		};

	public:
		typedef write_frame archive_frame;
		typedef typename archive_frame::archive_category archive_category;

	public:
		basic_output_archive() = delete;
		basic_output_archive(const basic_output_archive &) = delete;
		basic_output_archive &operator=(const basic_output_archive &) = delete;

		constexpr basic_output_archive(basic_output_archive &&other) noexcept { swap(other); }
		constexpr basic_output_archive &operator=(basic_output_archive &&other) noexcept
		{
			swap(other);
			return *this;
		}

		/** Initialized output archive for buffer writing.
		 * @param buff Memory buffer to write UBJson data to.
		 * @param size Size of the memory buffer. */
		basic_output_archive(void *buff, std::size_t size)
			: basic_output_archive(buff, size, std::pmr::get_default_resource())
		{
		}
		/** @copydoc basic_output_archive
		 * @param res PMR memory resource used for internal state allocation. */
		basic_output_archive(void *buff, std::size_t size, std::pmr::memory_resource *res)
			: buffer_emitter(buff, size), entry_pool(res), string_pool(res)
		{
		}
		/** Initialized output archive for file writing.
		 * @param file File to write UBJson data to.
		 * @note File must be opened in binary mode. */
		explicit basic_output_archive(FILE *file) : basic_output_archive(file, std::pmr::get_default_resource()) {}
		/** @copydoc basic_output_archive
		 * @param res PMR memory resource used for internal state allocation. */
		basic_output_archive(FILE *file, std::pmr::memory_resource *res)
			: file_emitter(file), entry_pool(res), string_pool(res)
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
			: streambuf_emitter(buff), entry_pool(res), string_pool(res)
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

		~basic_output_archive()
		{
			if (top_level) [[likely]]
				emitter.emit_entry(*top_level);
		}

		/** Serialized the forwarded value to UBJson. Flushes previous uncommitted state.
		 * @param value Value to serialize as UBJson.
		 * @return Reference to this archive.
		 * @note Serialized data is kept inside the archive's internal state and will be written to the output once the
		 * archive is destroyed or `flush` is called. */
		template<typename T>
		basic_output_archive &write(T &&value)
		{
			/* Flush uncommitted changes before initializing a new emit tree. */
			flush();

			auto *entry = static_cast<entry_t *>(entry_pool.allocate(sizeof(entry_t)));
			if (!entry) [[unlikely]]
				throw std::bad_alloc();
			top_level = std::construct_at(entry);

			write_frame frame{*this, *entry};
			detail::invoke_serialize(std::forward<T>(value), frame);
			return *this;
		}
		/** @copydoc write */
		template<typename T>
		basic_output_archive &operator<<(T &&value)
		{
			return write(std::forward<T>(value));
		}

		/** Flushes the internal state & writes UBJson to the output. */
		void flush()
		{
			if (top_level) [[likely]]
			{
				emitter.emit_entry(*top_level);
				entry_pool.release();
				string_pool.release();
				top_level = nullptr;
			}
		}

		constexpr void swap(basic_output_archive &other) noexcept
		{
			using std::swap;
			swap(emitter_padding, other.emitter_padding);
			swap(top_level, other.top_level);
			swap(upstream, other.upstream);

			entry_pool.swap(other.entry_pool);
			string_pool.swap(other.string_pool);
		}
		friend constexpr void swap(basic_output_archive &a, basic_output_archive &b) noexcept { a.swap(b); }

	private:
		union
		{
			std::byte emitter_padding[sizeof(buffer_emitter_t)] = {};

			emitter_base emitter;
			file_emitter_t file_emitter;
			buffer_emitter_t buffer_emitter;
			streambuf_emitter_t streambuf_emitter;
		};

		const entry_t *top_level = nullptr; /* Top-level entry of the entry tree. */

		std::pmr::memory_resource *upstream = nullptr;				   /* Upstream allocator used for memory pools. */
		detail::basic_pool_allocator<sizeof(entry_t) * 64> entry_pool; /* Pool used for the entry tree. */
		detail::basic_pool_allocator<SEK_KB(1)> string_pool;		   /* Pool used to buffer output strings. */
	};

	typedef basic_output_archive<fixed_type, char> output_archive;

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