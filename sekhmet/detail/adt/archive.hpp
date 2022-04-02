//
// Created by switchblade on 2022-04-01.
//

#pragma once

#include <iostream>

#include "node.hpp"

namespace sek::adt
{
	/** @brief Exception thrown whenever an archive encounters an error. */
	class archive_error : std::runtime_error
	{
	public:
		archive_error() : std::runtime_error("Unknown archive error") {}
		explicit archive_error(const char *msg) : std::runtime_error(msg) {}
		~archive_error() override = default;
	};

	namespace detail
	{
		struct archive_base
		{
			static void check_buffer(std::streambuf *buf)
			{
				if (!buf) [[unlikely]]
					throw archive_error("Archive is empty");
			}
		};
	}	 // namespace detail

	/** @brief Base interface for ADT input archives. */
	class basic_input_archive : detail::archive_base
	{
	public:
		/** Initializes an empty archive. */
		constexpr basic_input_archive() noexcept = default;
		/** Initializes an archive from an input buffer. */
		constexpr explicit basic_input_archive(std::streambuf *buf) noexcept : input_buf(buf) {}
		/** Initializes an archive from an input stream. */
		explicit basic_input_archive(std::istream &is) noexcept : basic_input_archive(is.rdbuf()) {}

		/** Checks if the archive is empty (does not contain an input buffer). */
		[[nodiscard]] constexpr bool empty() const noexcept { return input_buf == nullptr; }

		/** Returns the stored input buffer. */
		[[nodiscard]] constexpr std::streambuf *rdbuf() const noexcept { return input_buf; }
		/** Returns the stored input buffer and sets a new input buffer. */
		[[nodiscard]] constexpr std::streambuf *rdbuf(std::streambuf *buf) noexcept
		{
			return std::exchange(input_buf, buf);
		}

		/** Returns the next node read from the input.
		 * @throw archive_error If failed to read input. */
		[[nodiscard]] node read()
		{
			check_buffer(rdbuf());
			return do_read();
		}
		/** Reads the next node from the input.
		 * @param n Node to read.
		 * @throw archive_error If failed to read input. */
		void read(node &n)
		{
			check_buffer(rdbuf());
			do_read(n);
		}
		/** @copydoc read */
		basic_input_archive &operator>>(node &n)
		{
			read(n);
			return *this;
		}

	protected:
		[[nodiscard]] virtual node do_read()
		{
			node result;
			read(result);
			return result;
		}
		virtual void do_read(node &n) = 0;

	private:
		std::streambuf *input_buf = nullptr;
	};
	/** @brief Base interface for ADT output archives. */
	class basic_output_archive : detail::archive_base
	{
	public:
		/** Initializes an empty archive. */
		constexpr basic_output_archive() noexcept = default;
		/** Initializes an archive from an output buffer. */
		constexpr explicit basic_output_archive(std::streambuf *buf) noexcept : output_buf(buf) {}
		/** Initializes an archive from an output stream. */
		explicit basic_output_archive(std::ostream &os) noexcept : basic_output_archive(os.rdbuf()) {}

		/** Checks if the archive is empty (does not contain an input buffer). */
		[[nodiscard]] constexpr bool empty() const noexcept { return output_buf == nullptr; }

		/** Returns the stored output buffer. */
		[[nodiscard]] constexpr std::streambuf *rdbuf() const noexcept { return output_buf; }
		/** Returns the stored output buffer and sets a new output buffer. */
		[[nodiscard]] constexpr std::streambuf *rdbuf(std::streambuf *buf) noexcept
		{
			return std::exchange(output_buf, buf);
		}

		/** Writes the next node to the output.
		 * @param n Node to write.
		 * @throw archive_error If failed to write output. */
		void write(const node &n)
		{
			check_buffer(rdbuf());
			do_write(n);
		}
		/** @copydoc write */
		basic_output_archive &operator<<(const node &n)
		{
			write(n);
			return *this;
		}

	protected:
		virtual void do_write(const node &n) = 0;

	private:
		std::streambuf *output_buf = nullptr;
	};
}	 // namespace sek::adt