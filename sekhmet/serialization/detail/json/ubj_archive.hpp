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
	template<typename CharType = char>
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
				return value;
			}
			[[nodiscard]] std::int64_t read_length()
			{
				switch (read_token())
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
			using handler_t = typename base_t::parse_event_handler;

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
			using handler_t = typename base_t::parse_event_handler;

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
			using handler_t = typename base_t::parse_event_handler;

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
		bool try_read(T &&value)
		{
			return base_t::do_try_read(std::forward<T>(value));
		}
		/** Deserializes the top-level Json entry of the archive.
		 * @param value Value to deserialize from the Json entry.
		 * @return Reference to this archive.
		 * @throw archive_error On deserialization errors. */
		template<typename T>
		basic_ubj_input_archive &read(T &&value)
		{
			base_t::do_read(std::forward<T>(value));
			return *this;
		}
		/** @copydoc read */
		template<typename T>
		basic_ubj_input_archive &operator>>(T &&value)
		{
			return read(std::forward<T>(value));
		}
		/** Deserializes an instance of `T` from the top-level Json entry of the archive.
		 * @return Deserialized instance of `T`.
		 * @throw archive_error On deserialization errors. */
		template<std::default_initializable T>
		T read()
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
	class ubj_output_archive
	{
	};

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