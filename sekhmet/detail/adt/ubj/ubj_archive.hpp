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
		enum class highp_mode : int
		{
			/** Treat high-precision numbers as errors and throw `archive_error`. */
			THROW,
			/** Parse high-precision numbers as strings. */
			AS_STRING,
			/** Skip high-precision numbers (not recommended). */
			SKIP,
		};

	private:
		struct basic_parser
		{
			void read_guarded(void *, std::size_t) const;
			void bump_guarded(std::size_t) const;

			[[nodiscard]] char read_token() const;
			[[nodiscard]] char peek_token() const;

			basic_reader *reader;
			highp_mode hp_mode;
			node &result;
		};
		struct parser_spec12;

		typedef void (*parse_func)(basic_parser *);

	public:
		constexpr ubj_input_archive(ubj_input_archive &&other) noexcept
			: basic_input_archive(std::move(other)), parse(other.parse)
		{
		}
		constexpr ubj_input_archive &operator=(ubj_input_archive &&other) noexcept
		{
			parse = other.parse;
			basic_input_archive::operator=(std::move(other));
			return *this;
		}

		/** Initializes an empty UBJson archive. */
		constexpr ubj_input_archive() noexcept = default;

		/** Initializes a UBJson archive from a raw memory buffer.
		 * @param buf Buffer containing UBJson data.
		 * @param n Size of the data buffer.
		 * @param hp_mode Mode used to parse high-precision numbers.
		 * @param stx Version of UBJson syntax to use. */
		ubj_input_archive(const void *buf, std::size_t n, highp_mode hp_mode = highp_mode::THROW, syntax stx = syntax::SPEC_12)
			: basic_input_archive(buf, n)
		{
			init(hp_mode, stx);
		}
		/** Initializes a UBJson archive from a `FILE *`.
		 * @param file File containing UBJson data.
		 * @param hp_mode Mode used to parse high-precision numbers.
		 * @param stx Version of UBJson syntax to use.
		 * @note File must be opened in binary mode. */
		explicit ubj_input_archive(FILE *file, highp_mode hp_mode = highp_mode::THROW, syntax stx = syntax::SPEC_12) noexcept
			: basic_input_archive(file)
		{
			init(hp_mode, stx);
		}
		/** Initializes a UBJson archive from an input buffer.
		 * @param buf `std::streambuf` used to read UBJson data.
		 * @param hp_mode Mode used to parse high-precision numbers.
		 * @param stx Version of UBJson syntax to use.
		 * @note Buffer must be a binary buffer. */
		explicit ubj_input_archive(std::streambuf *buf, highp_mode hp_mode = highp_mode::THROW, syntax stx = syntax::SPEC_12) noexcept
			: basic_input_archive(buf)
		{
			init(hp_mode, stx);
		}
		/** Initializes a UBJson archive from an input stream.
		 * @param is Stream used to read UBJson data.
		 * @param hp_mode Mode used to parse high-precision numbers.
		 * @param stx Version of UBJson syntax to use.
		 * @note Stream must be a binary stream. */
		explicit ubj_input_archive(std::istream &is, highp_mode hp_mode = highp_mode::THROW, syntax stx = syntax::SPEC_12) noexcept
			: ubj_input_archive(is.rdbuf(), hp_mode, stx)
		{
		}

		constexpr void swap(ubj_input_archive &other) noexcept { basic_input_archive::swap(other); }
		friend constexpr void swap(ubj_input_archive &a, ubj_input_archive &b) noexcept { a.swap(b); }

	private:
		SEK_API void init(highp_mode, syntax) noexcept;
		SEK_API void do_read(node &) final;

		highp_mode hp_mode;
		parse_func parse;
	};
}	 // namespace sek::adt
