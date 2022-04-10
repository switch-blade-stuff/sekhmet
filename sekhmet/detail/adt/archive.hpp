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
		struct basic_reader
		{
			constexpr std::size_t read(void *dest, std::size_t n) { return invoke_read(this, dest, n); }
			constexpr std::size_t bump(std::size_t n) { return invoke_bump(this, n); }
			constexpr int peek() { return invoke_peek(this); }
			constexpr void destroy() { invoke_destroy(this); }

		protected:
			std::size_t (*invoke_read)(basic_reader *, void *, std::size_t) = nullptr;
			std::size_t (*invoke_bump)(basic_reader *, std::size_t) = nullptr;
			int (*invoke_peek)(basic_reader *) = nullptr;
			void (*invoke_destroy)(basic_reader *) = nullptr;
		};

	private:
		struct file_reader : basic_reader
		{
			constexpr explicit file_reader(FILE *file) noexcept : file(file)
			{
				invoke_read = [](basic_reader *base, void *dest, std::size_t n) -> std::size_t
				{
					auto *file = static_cast<file_reader *>(base)->file;
					return fread(dest, 1, n, file);
				};
				invoke_bump = [](basic_reader *base, std::size_t n) -> std::size_t
				{
					auto *file = static_cast<file_reader *>(base)->file;
#ifdef SEK_OS_WIN
					if (!_fseeki64(file, n, SEEK_CUR)) [[unlikely]]
						return 0;
#else
					if (!fseeko(file, static_cast<off_t>(n), SEEK_CUR)) [[unlikely]]
						return 0;
#endif
					return n;
				};
				invoke_peek = [](basic_reader *base) -> int
				{
					auto *file = static_cast<file_reader *>(base)->file;
					return ungetc(fgetc(file), file);
				};
				invoke_destroy = [](basic_reader *base) { delete static_cast<file_reader *>(base); };
			}

			FILE *file;
		};
		struct buffer_reader : basic_reader
		{
			constexpr buffer_reader(const void *data, std::size_t n) noexcept
				: data_pos(static_cast<const std::byte *>(data)), data_end(data_pos + n)
			{
				invoke_read = [](basic_reader *base, void *dest, std::size_t n) -> std::size_t
				{
					auto *buffer = static_cast<buffer_reader *>(base);
					auto new_pos = buffer->data_pos + n;
					if (new_pos > buffer->data_end) [[unlikely]]
						new_pos = buffer->data_end;
					std::copy(buffer->data_pos, new_pos, static_cast<std::byte *>(dest));
					return buffer->set_pos(new_pos);
				};
				invoke_bump = [](basic_reader *base, std::size_t n) -> std::size_t
				{
					auto *buffer = static_cast<buffer_reader *>(base);
					auto new_pos = buffer->data_pos + n;
					if (new_pos > buffer->data_end) [[unlikely]]
						new_pos = buffer->data_end;
					return buffer->set_pos(new_pos);
				};
				invoke_peek = [](basic_reader *base) -> int
				{
					auto *buffer = static_cast<buffer_reader *>(base);
					return buffer->data_pos != buffer->data_end ? static_cast<int>(*buffer->data_pos) : -1;
				};
				invoke_destroy = [](basic_reader *base) { delete static_cast<buffer_reader *>(base); };
			}

			[[nodiscard]] constexpr std::size_t set_pos(const std::byte *new_pos) noexcept
			{
				return static_cast<std::size_t>(std::distance(std::exchange(data_pos, new_pos), new_pos));
			}

			const std::byte *data_pos;
			const std::byte *data_end;
		};
		struct streambuf_reader : basic_reader
		{
			constexpr explicit streambuf_reader(std::streambuf *buf) noexcept : buf(buf)
			{
				invoke_read = [](basic_reader *base, void *dest, std::size_t n) -> std::size_t
				{
					auto *buf = static_cast<streambuf_reader *>(base)->buf;
					return static_cast<std::size_t>(buf->sgetn(static_cast<char *>(dest), static_cast<std::streamsize>(n)));
				};
				invoke_bump = [](basic_reader *base, std::size_t n) -> std::size_t
				{
					auto *buf = static_cast<streambuf_reader *>(base)->buf;
					auto old_pos = buf->pubseekoff(0, std::ios::cur, std::ios::in);
					auto new_pos = buf->pubseekoff(static_cast<std::streamoff>(n), std::ios::cur, std::ios::in);
					return static_cast<std::size_t>(new_pos - old_pos);
				};
				invoke_peek = [](basic_reader *base) { return static_cast<streambuf_reader *>(base)->buf->sgetc(); };
				invoke_destroy = [](basic_reader *base) { delete static_cast<streambuf_reader *>(base); };
			}

			std::streambuf *buf;
		};

	public:
		basic_input_archive(const basic_input_archive &) = delete;
		basic_input_archive &operator=(const basic_input_archive &) = delete;

		constexpr basic_input_archive() noexcept = default;
		constexpr basic_input_archive(basic_input_archive &&other) noexcept : reader(std::exchange(other.reader, {})) {}
		constexpr basic_input_archive &operator=(basic_input_archive &&other) noexcept
		{
			swap(other);
			return *this;
		}
		virtual ~basic_input_archive()
		{
			if (reader) [[likely]]
				reader->destroy();
		}

		/** Returns true if the archive was initialized. */
		[[nodiscard]] constexpr bool initialized() const noexcept { return reader != nullptr; }
		/** @copydoc initialized */
		[[nodiscard]] constexpr operator bool() const noexcept { return initialized(); }

		/** Returns the next node read from the input.
		 * @throw archive_error On read failure. */
		[[nodiscard]] node read()
		{
			node result;
			read(result);
			return result;
		}
		/** Reads the next node from the input.
		 * @param n Node to read.
		 * @throw archive_error On read failure.
		 * @note On any exception, `n` is left in undefined state. */
		void read(node &n)
		{
			check_initialized();
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
		basic_input_archive(const void *buf, std::size_t n) noexcept : reader(new buffer_reader(buf, n)) {}
		explicit basic_input_archive(FILE *file) noexcept : reader(new file_reader(file)) {}
		explicit basic_input_archive(std::streambuf *buf) noexcept : reader(new streambuf_reader(buf)) {}
		explicit basic_input_archive(std::istream &is) noexcept : basic_input_archive(is.rdbuf()) {}

		virtual void do_read(node &n) = 0;

		basic_reader *reader = nullptr;

	private:
		constexpr void check_initialized() const
		{
			if (!initialized()) [[unlikely]]
				throw archive_error("Archive was not initialized");
		}
	};
	/** @brief Base interface for ADT output archives. */
	class basic_output_archive
	{
	protected:
		struct basic_writer
		{
			std::size_t write(const void *src, std::size_t n) { return invoke_write(this, src, n); }
			void destroy() { invoke_destroy(this); }

		protected:
			std::size_t (*invoke_write)(basic_writer *, const void *, std::size_t) = nullptr;
			void (*invoke_destroy)(basic_writer *) = nullptr;
		};

	private:
		struct file_writer : basic_writer
		{
			constexpr explicit file_writer(FILE *file) noexcept : file(file)
			{
				invoke_write = [](basic_writer *base, const void *src, std::size_t n) -> std::size_t
				{
					auto file = static_cast<file_writer *>(base)->file;
					return fwrite(src, 1, n, file);
				};
				invoke_destroy = [](basic_writer *base) { delete static_cast<file_writer *>(base); };
			}

			FILE *file;
		};
		struct buffer_writer : basic_writer
		{
			constexpr buffer_writer(void *data, std::size_t n) noexcept
				: data_pos(static_cast<std::byte *>(data)), data_end(data_pos + n)
			{
				invoke_write = [](basic_writer *base, const void *src, std::size_t n) -> std::size_t
				{
					auto buffer = static_cast<buffer_writer *>(base);
					if (buffer->data_pos + n > buffer->data_end) [[unlikely]]
						n = static_cast<std::size_t>(std::distance(buffer->data_pos, buffer->data_end));
					buffer->data_pos = std::copy_n(static_cast<const std::byte *>(src), n, buffer->data_pos);
					return n;
				};
				invoke_destroy = [](basic_writer *base) { delete static_cast<buffer_writer *>(base); };
			}

			std::byte *data_pos;
			std::byte *data_end;
		};
		struct streambuf_writer : basic_writer
		{
			constexpr explicit streambuf_writer(std::streambuf *buf) noexcept : buf(buf)
			{
				invoke_write = [](basic_writer *base, const void *src, std::size_t n) -> std::size_t
				{
					auto buf = static_cast<streambuf_writer *>(base)->buf;
					return static_cast<std::size_t>(buf->sputn(static_cast<const char *>(src), static_cast<std::ptrdiff_t>(n)));
				};
				invoke_destroy = [](basic_writer *base) { delete static_cast<streambuf_writer *>(base); };
			}

			std::streambuf *buf;
		};

	public:
		basic_output_archive(const basic_output_archive &) = delete;
		basic_output_archive &operator=(const basic_output_archive &) = delete;

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
		constexpr ~basic_output_archive()
		{
			if (writer) [[likely]]
				writer->destroy();
		}

		/** Returns true if the archive was initialized. */
		[[nodiscard]] constexpr bool initialized() const noexcept { return writer != nullptr; }
		/** @copydoc initialized */
		[[nodiscard]] constexpr operator bool() const noexcept { return initialized(); }

		/** Writes the next node to the output.
		 * @param n Node to write.
		 * @throw archive_error On write failure. */
		void write(const node &n)
		{
			check_initialized();
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
		basic_output_archive(void *buf, std::size_t n) noexcept : writer(new buffer_writer(buf, n)) {}
		explicit basic_output_archive(FILE *file) noexcept : writer(new file_writer(file)) {}
		explicit basic_output_archive(std::streambuf *buf) noexcept : writer(new streambuf_writer(buf)) {}
		explicit basic_output_archive(std::ostream &os) noexcept : basic_output_archive(os.rdbuf()) {}

		virtual void do_write(const node &n) = 0;

		basic_writer *writer = nullptr;

	private:
		constexpr void check_initialized() const
		{
			if (!initialized()) [[unlikely]]
				throw archive_error("Archive was not initialized");
		}
	};
}	 // namespace sek::adt