//
// Created by switchblade on 2022-04-08.
//

#pragma once

#include "sekhmet/detail/adt/archive.hpp"

namespace sek::adt
{
	namespace detail
	{
		enum class ubj_syntax : int
		{
			SPEC_12,
		};
	}	 // namespace detail

	class ubj_input_archive final : public basic_input_archive
	{
	public:
		typedef detail::ubj_syntax syntax;
		typedef int parse_mode;

		/** Treat high-precision numbers as errors and throw `archive_error`. */
		constexpr static parse_mode highp_throw = 0;
		/** Parse high-precision numbers as strings. */
		constexpr static parse_mode highp_string = 1;
		/** Skip high-precision numbers (not recommended). */
		constexpr static parse_mode highp_skip = 2;

		/** Treat arrays of unsigned 8-bit integers as binary array. */
		constexpr static parse_mode uint8_binary = 4;

	private:
		struct basic_parser
		{
			void read_guarded(void *, std::size_t) const;
			void bump_guarded(std::size_t) const;

			[[nodiscard]] char read_token() const;
			[[nodiscard]] char peek_token() const;

			basic_reader *reader;
			parse_mode mode;
			node &result;
		};
		struct parser_spec12;

		typedef void (*parse_func)(basic_parser *);

		constexpr static parse_mode highp_mask = 3;

	public:
		constexpr ubj_input_archive(ubj_input_archive &&other) noexcept
			: basic_input_archive(std::move(other)), mode(other.mode), parse(other.parse)
		{
		}
		constexpr ubj_input_archive &operator=(ubj_input_archive &&other) noexcept
		{
			mode = other.mode;
			parse = other.parse;
			basic_input_archive::operator=(std::move(other));
			return *this;
		}

		/** Initializes an empty UBJson archive. */
		constexpr ubj_input_archive() noexcept = default;

		/** Initializes a UBJson archive from a raw memory buffer.
		 * @param buf Buffer containing UBJson data.
		 * @param n Size of the data buffer.
		 * @param mode Mode used to parse high-precision numbers & binary arrays.
		 * @param stx Version of UBJson syntax to use. */
		ubj_input_archive(const void *buf, std::size_t n, parse_mode mode = highp_throw | uint8_binary, syntax stx = syntax::SPEC_12)
			: basic_input_archive(buf, n)
		{
			init(mode, stx);
		}
		/** Initializes a UBJson archive from a `FILE *`.
		 * @param file File containing UBJson data.
		 * @param mode Mode used to parse high-precision numbers & binary arrays.
		 * @param stx Version of UBJson syntax to use.
		 * @note File must be opened in binary mode. */
		explicit ubj_input_archive(FILE *file, parse_mode mode = highp_throw | uint8_binary, syntax stx = syntax::SPEC_12) noexcept
			: basic_input_archive(file)
		{
			init(mode, stx);
		}
		/** Initializes a UBJson archive from an input buffer.
		 * @param buf `std::streambuf` used to read UBJson data.
		 * @param mode Mode used to parse high-precision numbers & binary arrays.
		 * @param stx Version of UBJson syntax to use.
		 * @note Buffer must be a binary buffer. */
		explicit ubj_input_archive(std::streambuf *buf, parse_mode mode = highp_throw | uint8_binary, syntax stx = syntax::SPEC_12) noexcept
			: basic_input_archive(buf)
		{
			init(mode, stx);
		}
		/** Initializes a UBJson archive from an input stream.
		 * @param is Stream used to read UBJson data.
		 * @param mode Mode used to parse high-precision numbers & binary arrays.
		 * @param stx Version of UBJson syntax to use.
		 * @note Stream must be a binary stream. */
		explicit ubj_input_archive(std::istream &is, parse_mode mode = highp_throw | uint8_binary, syntax stx = syntax::SPEC_12) noexcept
			: ubj_input_archive(is.rdbuf(), mode, stx)
		{
		}

		constexpr void swap(ubj_input_archive &other) noexcept
		{
			using std::swap;
			swap(mode, other.mode);
			swap(parse, other.parse);

			basic_input_archive::swap(other);
		}
		friend constexpr void swap(ubj_input_archive &a, ubj_input_archive &b) noexcept { a.swap(b); }

	private:
		SEK_API void init(parse_mode, syntax) noexcept;
		SEK_API void do_read(node &) final;

		parse_mode mode;
		parse_func parse;
	};

	class ubj_output_archive final : public basic_output_archive
	{
	public:
		typedef detail::ubj_syntax syntax;
		typedef int emit_mode;

		/** Emit fixed-size containers (recommended). */
		constexpr static emit_mode fix_size = 1;
		/** Emit fixed-type containers when possible
		 * @warning Will decrease performance, as every container element will need to be inspected. */
		constexpr static emit_mode fix_type = 3;
		/** Use best-fit value types (recommended). If not set will use int64 for integers & float64 for floats. */
		constexpr static emit_mode best_fit = 4;

	private:
		struct basic_emitter
		{
			void write_guarded(const void *, std::size_t) const;
			void write_token(char) const;

			basic_writer *writer;
			emit_mode mode;
			const node &data;
		};
		struct emitter_spec12;

		typedef void (*emit_func)(basic_emitter *);

	public:
		constexpr ubj_output_archive(ubj_output_archive &&other) noexcept
			: basic_output_archive(std::move(other)), emit(other.emit)
		{
		}
		constexpr ubj_output_archive &operator=(ubj_output_archive &&other) noexcept
		{
			emit = other.emit;
			basic_output_archive::operator=(std::move(other));
			return *this;
		}

		/** Initializes an empty UBJson archive. */
		constexpr ubj_output_archive() noexcept = default;

		/** Initializes a UBJson archive from a raw memory buffer.
		 * @param buf Buffer receiving UBJson data.
		 * @param n Size of the data buffer.
		 * @param mode Mode flags used to control emitter behavior.
		 * @param stx Version of UBJson syntax to use. */
		ubj_output_archive(void *buf, std::size_t n, emit_mode mode = fix_size | best_fit, syntax stx = syntax::SPEC_12)
			: basic_output_archive(buf, n)
		{
			init(mode, stx);
		}
		/** Initializes a UBJson archive from a `FILE *`.
		 * @param file File receiving UBJson data.
		 * @param mode Mode flags used to control emitter behavior.
		 * @param stx Version of UBJson syntax to use.
		 * @note File must be opened in binary mode. */
		explicit ubj_output_archive(FILE *file, emit_mode mode = fix_size | best_fit, syntax stx = syntax::SPEC_12) noexcept
			: basic_output_archive(file)
		{
			init(mode, stx);
		}
		/** Initializes a UBJson archive from an output buffer.
		 * @param buf `std::streambuf` used to write UBJson data.
		 * @param mode Mode flags used to control emitter behavior.
		 * @param stx Version of UBJson syntax to use.
		 * @note Buffer must be a binary buffer. */
		explicit ubj_output_archive(std::streambuf *buf, emit_mode mode = fix_size | best_fit, syntax stx = syntax::SPEC_12) noexcept
			: basic_output_archive(buf)
		{
			init(mode, stx);
		}
		/** Initializes a UBJson archive from an output stream.
		 * @param os Stream used to write UBJson data.
		 * @param mode Mode flags used to control emitter behavior.
		 * @param stx Version of UBJson syntax to use.
		 * @note Stream must be a binary stream. */
		explicit ubj_output_archive(std::ostream &os, emit_mode mode = fix_size | best_fit, syntax stx = syntax::SPEC_12) noexcept
			: ubj_output_archive(os.rdbuf(), mode, stx)
		{
		}

		constexpr void swap(ubj_output_archive &other) noexcept
		{
			using std::swap;
			swap(emit, other.emit);
			basic_output_archive::swap(other);
		}
		friend constexpr void swap(ubj_output_archive &a, ubj_output_archive &b) noexcept { a.swap(b); }

	private:
		SEK_API void init(emit_mode, syntax) noexcept;
		SEK_API void do_write(const node &) final;

		emit_mode mode;
		emit_func emit;
	};
}	 // namespace sek::adt
