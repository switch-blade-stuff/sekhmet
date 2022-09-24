/*
 * Created by switchblade on 09/06/22
 */

#pragma once

#include <filesystem>
#include <span>

#include "sekhmet/expected.hpp"

#include <asio/basic_file.hpp>
#include <system_error>

namespace sek::system
{
#ifdef ASIO_HAS_FILE /* If ASIO file is available, openmode & seek_basis are defined via asio::file_base for compatibility */
	using openmode = asio::file_base::flags;
	using seek_basis = asio::file_base::seek_basis;

	constexpr openmode read_only = openmode::read_only;
	constexpr openmode write_only = openmode::write_only;
	constexpr openmode read_write = openmode::read_write;
	constexpr openmode append = openmode::append;
	constexpr openmode create = openmode::create;
	constexpr openmode exclusive = openmode::exclusive;
	constexpr openmode truncate = openmode::truncate;
	constexpr openmode sync_all_on_write = openmode::sync_all_on_write;

	constexpr seek_basis seek_cur = seek_basis::seek_cur;
	constexpr seek_basis seek_end = seek_basis::seek_end;
	constexpr seek_basis seek_set = seek_basis::seek_set;
#else

#ifdef SEK_OS_WIN
	using openmode = int;
	constexpr openmode read_only = 1;
	constexpr openmode write_only = 2;
	constexpr openmode read_write = 4;
	constexpr openmode append = 8;
	constexpr openmode create = 16;
	constexpr openmode exclusive = 32;
	constexpr openmode truncate = 64;
	constexpr openmode sync_all_on_write = 128;
#else
	using openmode = int;
	constexpr openmode read_only = O_RDONLY;
	constexpr openmode write_only = O_WRONLY;
	constexpr openmode read_write = O_RDWR;
	constexpr openmode append = O_APPEND;
	constexpr openmode create = O_CREAT;
	constexpr openmode exclusive = O_EXCL;
	constexpr openmode truncate = O_TRUNC;
	constexpr openmode sync_all_on_write = O_SYNC;
#endif

	using seek_basis = int;
	constexpr seek_basis seek_cur = SEEK_SET;
	constexpr seek_basis seek_end = SEEK_CUR;
	constexpr seek_basis seek_set = SEEK_END;
#endif

	typedef int mapmode;
	constexpr mapmode map_copy = 1;
	constexpr mapmode map_populate = 2;
}	 // namespace sek::system

#if defined(SEK_OS_UNIX)
#include "detail/unix/native_file.hpp"
#else
#error "Native file not implemented"
#endif

namespace sek::system
{
	/** @brief Structure used to preform buffered IO operations on a native OS file.
	 * Provides both a C-like read & write API and an ASIO buffer compatible API. */
	class native_file
	{
		friend class native_filemap;

		using handle_t = detail::native_file_handle;

	public:
		typedef typename handle_t::native_handle_type native_handle_type;
		typedef system::seek_basis seek_basis;
		typedef system::openmode openmode;

		constexpr static openmode read_only = system::read_only;
		constexpr static openmode write_only = system::write_only;
		constexpr static openmode read_write = system::read_write;
		constexpr static openmode append = system::append;
		constexpr static openmode create = system::create;
		constexpr static openmode exclusive = system::exclusive;
		constexpr static openmode truncate = system::truncate;
		constexpr static openmode sync_all_on_write = system::sync_all_on_write;
		constexpr static openmode direct = system::sync_all_on_write;

		constexpr static seek_basis seek_cur = system::seek_cur;
		constexpr static seek_basis seek_end = system::seek_end;
		constexpr static seek_basis seek_set = system::seek_set;

	protected:
		using path_char = typename std::filesystem::path::value_type;

		template<typename T>
		inline static T return_if(expected<T, std::error_code> &&exp)
		{
			if (!exp.has_value()) [[unlikely]]
				throw std::system_error(exp.error());

			if constexpr (!std::is_void_v<T>) return std::move(exp.value());
		}

	public:
		native_file(const native_file &) = delete;
		native_file &operator=(const native_file &) = delete;

		/** Initializes an invalid (closed) file. */
		constexpr native_file() noexcept = default;
		SEK_API ~native_file();

		constexpr native_file(native_file &&other) noexcept { swap(other); }
		constexpr native_file &operator=(native_file &&other) noexcept
		{
			swap(other);
			return *this;
		}

		/** Initializes a native file from an existing native handle. */
		constexpr explicit native_file(native_handle_type handle) { open(handle); }
		/** Initializes & opens the file.
		 * @param path Path to the file.
		 * @param mode Mode to open the file with.
		 * @throw std::system_error On implementation-defined system errors. */
		native_file(const path_char *path, openmode mode) { open(path, mode); }
		/** @copydoc native_file */
		native_file(const std::filesystem::path &path, openmode mode) : native_file(path.c_str(), mode) {}

		/** Opens an existing native file handle.
		 * @param handle Native handle to open. */
		constexpr void open(native_handle_type handle) noexcept { m_handle.open(handle); }

		/** @brief Opens the file.
		 * @param path Path to the file.
		 * @param mode Mode to open the file with.
		 * @throw std::system_error On implementation-defined system errors. */
		void open(const std::filesystem::path &path, openmode mode) { open(path.c_str(), mode); }
		/** @copydoc open */
		SEK_API void open(const path_char *path, openmode mode);

		/** @copybrief open
		 * @param path Path to the file.
		 * @param mode Mode to open the file with.
		 * @return `void` or an error code. */
		expected<void, std::error_code> open(std::nothrow_t, const std::filesystem::path &path, openmode mode) noexcept
		{
			return open(std::nothrow, path.c_str(), mode);
		}
		/** @copydoc open */
		SEK_API expected<void, std::error_code> open(std::nothrow_t, const path_char *path, openmode mode) noexcept;

		/** @brief Flushes & closes the file.
		 * @throw std::system_error On implementation-defined system errors. */
		SEK_API void close();
		/** @copybrief close
		 * @return `void` or an error code. */
		SEK_API expected<void, std::error_code> close(std::nothrow_t) noexcept;

		/** @brief Flushes buffered output to the underlying file and un-reads any buffered input.
		 * @throw std::system_error On implementation-defined system errors. */
		SEK_API void flush();
		/** @brief Synchronizes file to disk. If the internal buffers are not empty, flushes their contents as well.
		 * @throw std::system_error On implementation-defined system errors. */
		SEK_API void sync();

		/** @copybrief sync
		 * @return `void` or an error code. */
		SEK_API expected<void, std::error_code> sync(std::nothrow_t) noexcept;
		/** @copybrief flush
		 * @return `void` or an error code. */
		SEK_API expected<void, std::error_code> flush(std::nothrow_t) noexcept;

		/** @brief Reads file to a data buffer.
		 * @param dst Memory buffer receiving data.
		 * @param n Amount of bytes to read.
		 * @return Amount of bytes read from the file.
		 * @throw std::system_error On implementation-defined system errors. */
		SEK_API std::size_t read(void *dst, std::size_t n);
		/** @brief Writes data buffer to the file.
		 * @param src Memory buffer containing source data.
		 * @param n Amount of bytes to write.
		 * @return Amount of bytes written to the file.
		 * @throw std::system_error On implementation-defined system errors. */
		SEK_API std::size_t write(const void *src, std::size_t n);

		/** @copybrief write
		 * @param dst Memory buffer receiving data.
		 * @param n Amount of bytes to read.
		 * @return Amount of bytes read from the file or an error code. */
		SEK_API expected<std::size_t, std::error_code> read(std::nothrow_t, void *dst, std::size_t n) noexcept;
		/** @copybrief write
		 * @param src Memory buffer containing source data.
		 * @param n Amount of bytes to write.
		 * @return Amount of bytes written to the file or an error code. */
		SEK_API expected<std::size_t, std::error_code> write(std::nothrow_t, const void *src, std::size_t n) noexcept;

		/** @brief Reads file to ASIO data buffers.
		 * @param buff ASIO mutable buffer sequence receiving data.
		 * @return Amount of bytes read from the file.
		 * @throw std::system_error On implementation-defined system errors. */
		SEK_API std::size_t read(asio::mutable_buffer &buff);
		/** @brief Writes data buffers to the file.
		 * @param buff ASIO const buffer sequence containing source data.
		 * @return Amount of bytes written to the file.
		 * @throw std::system_error On implementation-defined system errors. */
		SEK_API std::size_t write(const asio::const_buffer &buff);

		/** @copybrief read
		 * @param buff ASIO mutable buffer receiving data.
		 * @return Amount of bytes read from the file or an error code. */
		SEK_API expected<std::size_t, std::error_code> read(std::nothrow_t, asio::mutable_buffer &buff) noexcept;
		/** @copybrief write
		 * @return Amount of bytes written to the file or an error code. */
		SEK_API expected<std::size_t, std::error_code> write(std::nothrow_t, const asio::const_buffer &buff) noexcept;

		/** @brief Seeks the file in the specified direction to the specified offset.
		 * @param off Offset to seek.
		 * @param dir Direction to seek in (`beg`, `curr` or `end`).
		 * @return Resulting position within the file.
		 * @note If the wile is open in read mode, flushes the output buffer before seeking.
		 * @throw std::system_error On implementation-defined system errors. */
		SEK_API std::uint64_t seek(std::int64_t off, seek_basis dir);
		/** @brief Sets position within the file to the specified offset from the start.
		 * Equivalent to `seek(static_cast<std::int64_t>(pos), seek_set)`.
		 * @param pos New position within the file.
		 * @return Resulting position within the file.
		 * @note If the wile is open in read mode, flushes the output buffer before seeking.
		 * @throw std::system_error On implementation-defined system errors. */
		SEK_API std::uint64_t setpos(std::uint64_t pos);

		/** @copybrief seek
		 * @param off Offset to seek.
		 * @param dir Direction to seek in (`beg`, `curr` or `end`).
		 * @return Resulting position within the file or an error code.
		 * @note If the wile is open in read mode, flushes the output buffer before seeking. */
		SEK_API expected<std::uint64_t, std::error_code> seek(std::nothrow_t, std::int64_t off, seek_basis dir) noexcept;
		/** @copybrief setpos
		 * @param pos New position within the file.
		 * @return Resulting position within the file or an error code.
		 * @note If the wile is open in read mode, flushes the output buffer before seeking. */
		SEK_API expected<std::uint64_t, std::error_code> setpos(std::nothrow_t, std::uint64_t pos) noexcept;

		/** @brief Modifies size of the file.
		 * @param size New size of the file.
		 * @return Updated size of the file.
		 * @throw std::system_error On implementation-defined system errors. */
		SEK_API std::uint64_t resize(std::uint64_t size);
		/** @copybrief resize
		 * @param size New size of the file.
		 * @return Updated size of the file pr an error code. */
		SEK_API expected<std::uint64_t, std::error_code> resize(std::nothrow_t, std::uint64_t size) noexcept;

		/** @brief Returns total size of the file.
		 * @throw std::system_error On implementation-defined system errors. */
		[[nodiscard]] SEK_API std::uint64_t size() const;
		/** @brief Returns the current position within the file.
		 * @throw std::system_error On implementation-defined system errors. */
		[[nodiscard]] SEK_API std::uint64_t tell() const;

		/** @copybrief size
		 * @return Total size of the file or an error code. */
		[[nodiscard]] SEK_API expected<std::uint64_t, std::error_code> size(std::nothrow_t) const noexcept;
		/** @copybrief tell
		 * @return Current position within the file or an error code. */
		[[nodiscard]] SEK_API expected<std::uint64_t, std::error_code> tell(std::nothrow_t) const noexcept;

		/** Releases and returns the underlying native file handle. */
		[[nodiscard]] constexpr native_handle_type release() noexcept { return m_handle.release(); }
		/** Returns the underlying native file handle. */
		[[nodiscard]] constexpr native_handle_type native_handle() const noexcept { return m_handle.native_handle(); }

		/** Returns the file open mode. */
		[[nodiscard]] constexpr openmode mode() const noexcept { return m_mode; }
		/** Checks if the file is open. */
		[[nodiscard]] constexpr bool is_open() const noexcept { return m_handle.is_open(); }

		constexpr void swap(native_file &other) noexcept
		{
			m_handle.swap(other.m_handle);

			using std::swap;
			swap(m_buffer, other.m_buffer);
			swap(m_buffer_size, other.m_buffer_pos);
			swap(m_buffer_pos, other.m_buffer_pos);
			swap(m_mode, other.m_mode);
			swap(m_writing, other.m_writing);
			swap(m_reading, other.m_reading);
		}
		friend constexpr void swap(native_file &a, native_file &b) noexcept { a.swap(b); }

	protected:
		SEK_API void init_buffer(std::size_t min_size);

		detail::native_file_handle m_handle;

		std::byte *m_buffer = nullptr;	 /* Buffer used fore read & write operations. */
		std::uint64_t m_buffer_size = 0; /* Total size of the buffer. */
		std::uint64_t m_buffer_pos = 0;	 /* Current read or write position within the buffer. */

		/* Size of the input buffer, used only for reading. Might be less than buffer_size. */
		std::uint64_t m_input_size = 0;

		openmode m_mode = {};
		bool m_writing = false;
		bool m_reading = false;
	};
}	 // namespace sek::system
