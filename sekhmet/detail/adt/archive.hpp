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

	/** @brief Base interface for ADT input archives. */
	class basic_input_archive
	{
	protected:
		struct reader_base
		{
			virtual ~reader_base() = default;
			virtual std::size_t read(void *, std::size_t) = 0;
			virtual int peek() = 0;
			virtual std::size_t bump(std::size_t) = 0;
		};
		struct file_reader final : reader_base
		{
			constexpr explicit file_reader(FILE *file) noexcept : file(file) {}
			~file_reader() final { fclose(file); }

			std::size_t read(void *dest, std::size_t n) final { return fread(dest, 1, n, file); }
			int peek() final { return ungetc(getc(file), file); }
			std::size_t bump(std::size_t n) final
			{
#ifdef SEK_OS_WIN
				return _fseeki64(file, n, SEEK_CUR) ? 0 : n;
#else
				return fseeko(file, static_cast<off_t>(n), SEEK_CUR) ? 0 : n;
#endif
			}

			FILE *file;
		};
		struct buffer_reader final : reader_base
		{
			constexpr buffer_reader(const void *data, std::size_t n) noexcept
				: curr(static_cast<const std::byte *>(data)), last(curr + n), size(n)
			{
			}
			~buffer_reader() final = default;

			std::size_t read(void *dest, std::size_t n) final
			{
				auto next = curr + n;
				if (next > last) [[unlikely]]
					next = last;
				std::copy(curr, next, static_cast<std::byte *>(dest));
				return static_cast<std::size_t>(next - std::exchange(curr, next));
			}
			int peek() final
			{
				if (curr == last) [[unlikely]]
					return EOF;
				else
					return static_cast<int>(*curr);
			}
			std::size_t bump(std::size_t n) final
			{
				auto next = curr + n;
				if (next > last) [[unlikely]]
					next = last;
				return static_cast<std::size_t>(next - std::exchange(curr, next));
			}

			const std::byte *curr;
			const std::byte *last;
			std::size_t size;
		};
		struct streambuf_reader final : reader_base
		{
			constexpr explicit streambuf_reader(std::streambuf *streambuf) noexcept : streambuf(streambuf) {}
			~streambuf_reader() final = default;

			std::size_t read(void *dest, std::size_t n) final
			{
				return static_cast<std::size_t>(streambuf->sgetn(static_cast<char *>(dest), static_cast<std::streamsize>(n)));
			}
			int peek() final { return streambuf->sgetc(); }
			std::size_t bump(std::size_t n) final
			{
				auto old = streambuf->pubseekoff(0, std::ios::cur, std::ios::in);
				auto abs = streambuf->pubseekpos(old + static_cast<std::streamoff>(n), std::ios::in);
				return static_cast<std::size_t>(abs - old);
			}

			std::streambuf *streambuf;
		};

	public:
		basic_input_archive(const basic_input_archive &) = delete;
		basic_input_archive &operator=(const basic_input_archive &) = delete;

		/** Initializes an empty archive. */
		constexpr basic_input_archive() noexcept = default;
		constexpr basic_input_archive(basic_input_archive &&other) noexcept : reader(std::exchange(other.reader, {})) {}
		constexpr basic_input_archive &operator=(basic_input_archive &&other) noexcept
		{
			swap(other);
			return *this;
		}
		constexpr ~basic_input_archive() { delete reader; }

		/** Initializes an archive from a raw memory buffer. */
		basic_input_archive(const void *buf, std::size_t n) noexcept : reader(new buffer_reader(buf, n)) {}
		/** Initializes an archive from a `FILE *`. */
		explicit basic_input_archive(FILE *file) noexcept : reader(new file_reader(file)) {}
		/** Initializes an archive from an input buffer. */
		explicit basic_input_archive(std::streambuf *buf) noexcept : reader(new streambuf_reader(buf)) {}
		/** Initializes an archive from an input stream. */
		explicit basic_input_archive(std::istream &is) noexcept : basic_input_archive(is.rdbuf()) {}

		/** Checks if the archive is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return reader == nullptr; }

		/** Returns the next node read from the input.
		 * @throw archive_error If failed to read input. */
		[[nodiscard]] node read()
		{
			check_reader();
			return do_read();
		}
		/** Reads the next node from the input.
		 * @param n Node to read.
		 * @throw archive_error If failed to read input. */
		void read(node &n)
		{
			check_reader();
			do_read(n);
		}
		/** @copydoc read */
		basic_input_archive &operator>>(node &n)
		{
			read(n);
			return *this;
		}

		constexpr void swap(basic_input_archive &other) noexcept { std::swap(reader, other.reader); }
		friend constexpr void swap(basic_input_archive &a, basic_input_archive &b) noexcept { a.swap(b); }

	protected:
		[[nodiscard]] virtual node do_read()
		{
			node result;
			read(result);
			return result;
		}
		virtual void do_read(node &n) = 0;

		reader_base *reader = nullptr;

	private:
		constexpr void check_reader() const
		{
			if (!reader) [[unlikely]]
				throw archive_error("Archive reader is empty");
		}
	};
	/** @brief Base interface for ADT output archives. */
	class basic_output_archive
	{
	protected:
		struct writer_base
		{
			virtual ~writer_base() = default;
			virtual std::size_t write(const void *, std::size_t) = 0;
		};
		struct file_writer final : writer_base
		{
			constexpr explicit file_writer(FILE *file) noexcept : file(file) {}
			~file_writer() final { fclose(file); }

			std::size_t write(const void *src, std::size_t n) final { return fwrite(src, 1, n, file); }

			FILE *file;
		};
		struct buffer_writer final : writer_base
		{
			constexpr buffer_writer(void *data, std::size_t n) noexcept
				: curr(static_cast<std::byte *>(data)), last(curr + n), size(n)
			{
			}
			~buffer_writer() final = default;

			std::size_t write(const void *src, std::size_t n) final
			{
				if (curr + n > last) [[unlikely]]
					n = static_cast<std::size_t>(last - curr);
				curr = std::copy_n(static_cast<const std::byte *>(src), n, curr);
				return n;
			}

			std::byte *curr;
			std::byte *last;
			std::size_t size;
		};
		struct streambuf_writer final : writer_base
		{
			constexpr explicit streambuf_writer(std::streambuf *streambuf) noexcept : streambuf(streambuf) {}
			~streambuf_writer() final = default;

			std::size_t write(const void *src, std::size_t n) final
			{
				return static_cast<std::size_t>(streambuf->sputn(static_cast<const std::streambuf::char_type *>(src),
																 static_cast<std::ptrdiff_t>(n)));
			}

			std::streambuf *streambuf;
		};

	public:
		/** Initializes an empty archive. */
		constexpr basic_output_archive() noexcept = default;
		constexpr basic_output_archive(basic_output_archive &&other) noexcept : writer(std::exchange(other.writer, {}))
		{
		}
		constexpr basic_output_archive &operator=(basic_output_archive &&other) noexcept
		{
			swap(other);
			return *this;
		}
		constexpr ~basic_output_archive() { delete writer; }

		/** Initializes an archive from a `FILE *`. */
		explicit basic_output_archive(FILE *file) noexcept : writer(new file_writer(file)) {}
		/** Initializes an archive from a raw memory buffer. */
		basic_output_archive(void *buf, std::size_t n) noexcept : writer(new buffer_writer(buf, n)) {}
		/** Initializes an archive from an output buffer. */
		explicit basic_output_archive(std::streambuf *buf) noexcept : writer(new streambuf_writer(buf)) {}
		/** Initializes an archive from an output stream. */
		explicit basic_output_archive(std::ostream &os) noexcept : basic_output_archive(os.rdbuf()) {}

		/** Checks if the archive is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return writer == nullptr; }

		/** Writes the next node to the output.
		 * @param n Node to write.
		 * @throw archive_error If failed to write output. */
		void write(const node &n)
		{
			check_writer();
			do_write(n);
		}
		/** @copydoc write */
		basic_output_archive &operator<<(const node &n)
		{
			write(n);
			return *this;
		}

		constexpr void swap(basic_output_archive &other) noexcept { std::swap(writer, other.writer); }
		friend constexpr void swap(basic_output_archive &a, basic_output_archive &b) noexcept { a.swap(b); }

	protected:
		virtual void do_write(const node &n) = 0;

		writer_base *writer = nullptr;

	private:
		constexpr void check_writer() const
		{
			if (!writer) [[unlikely]]
				throw archive_error("Archive writer is empty");
		}
	};
}	 // namespace sek::adt