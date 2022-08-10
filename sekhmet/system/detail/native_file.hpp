/*
 * Created by switchblade on 09/06/22
 */

#pragma once

#include <filesystem>
#include <span>

#include "sekhmet/detail/expected.hpp"

#include <asio/basic_file.hpp>
#include <system_error>

namespace sek::system
{
#ifdef ASIO_HAS_FILE /* If ASIO is available, openmode & seek_basis are defined via asio::file_base for compatibility */
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
#error "Implement `sek::system::openmode` and `sek::system::seek_basis`"
#endif

	typedef int mapmode;
	constexpr mapmode map_copy = 1;
	constexpr mapmode map_populate = 2;

	class native_file;
	class native_filemap;
}	 // namespace sek::system

#if defined(SEK_OS_UNIX)
#include "unix/native_file.hpp"
#elif defined(SEK_OS_WIN)
#include "win/native_file.hpp"
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

	private:
		using path_char = typename std::filesystem::path::value_type;

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
		 * @throw std::system_error On any system errors. */
		native_file(const path_char *path, openmode mode) { open(path, mode); }
		/** @copydoc native_file */
		native_file(const std::filesystem::path &path, openmode mode) : native_file(path.c_str(), mode) {}

		/** Opens an existing native file handle.
		 * @param handle Native handle to open. */
		constexpr void open(native_handle_type handle) noexcept { m_handle.open(handle); }

		/** @brief Opens the file.
		 * @param path Path to the file.
		 * @param mode Mode to open the file with.
		 * @throw std::system_error On any system errors. */
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
		 * @throw std::system_error On any system errors. */
		SEK_API void close();
		/** @copybrief close
		 * @return `void` or an error code. */
		SEK_API expected<void, std::error_code> close(std::nothrow_t) noexcept;

		/** @brief Flushes buffered output to the underlying file and un-reads any buffered input.
		 * @throw std::system_error On any system errors. */
		SEK_API void flush();
		/** @brief Synchronizes file to disk. If the internal buffers are not empty, flushes their contents as well.
		 * @throw std::system_error On any system errors. */
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
		 * @throw std::system_error On any system errors. */
		SEK_API std::size_t read(void *dst, std::size_t n);
		/** @brief Writes data buffer to the file.
		 * @param src Memory buffer containing source data.
		 * @param n Amount of bytes to write.
		 * @return Amount of bytes written to the file.
		 * @throw std::system_error On any system errors. */
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

		// clang-format off
		/** @brief Reads file to ASIO data buffers.
		 * @param buff ASIO mutable buffer sequence receiving data.
		 * @return Amount of bytes read from the file.
		 * @throw std::system_error On any system errors. */
		SEK_API std::size_t read(asio::mutable_buffer &buff);
		/** @brief Writes data buffers to the file.
		 * @param buff ASIO const buffer sequence containing source data.
		 * @return Amount of bytes written to the file.
		 * @throw std::system_error On any system errors. */
		SEK_API std::size_t write(const asio::const_buffer &buff);
		// clang-format on

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
		 * @throw std::system_error On any system errors. */
		SEK_API std::uint64_t seek(std::int64_t off, seek_basis dir);
		/** @brief Sets position within the file to the specified offset from the start.
		 * Equivalent to `seek(static_cast<std::int64_t>(pos), seek_set)`.
		 * @param pos New position within the file.
		 * @return Resulting position within the file.
		 * @note If the wile is open in read mode, flushes the output buffer before seeking.
		 * @throw std::system_error On any system errors. */
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
		 * @throw std::system_error On any system errors. */
		SEK_API std::uint64_t resize(std::uint64_t size);
		/** @copybrief resize
		 * @param size New size of the file.
		 * @return Updated size of the file pr an error code. */
		SEK_API expected<std::uint64_t, std::error_code> resize(std::nothrow_t, std::uint64_t size) noexcept;

		/** @brief Returns total size of the file.
		 * @throw std::system_error On any system errors. */
		[[nodiscard]] SEK_API std::uint64_t size() const;
		/** @brief Returns the current position within the file.
		 * @throw std::system_error On any system errors. */
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

	private:
		detail::native_file_handle m_handle;

		std::byte *m_buffer = nullptr;	 /* Buffer used fore read & write operations. */
		std::uint64_t m_buffer_size = 0; /* Total size of the buffer. */
		std::uint64_t m_buffer_pos = 0;	 /* Current read or write position within the buffer. */

		/* Size of the input buffer, used only for reading. Might be less than buffer_size in case the file
		 * size is less than size of the buffer. */
		std::uint64_t m_input_size = 0;

		openmode m_mode = {};
		bool m_writing = false;
		bool m_reading = false;
	};

	/** @brief Structure used to manage a memory-mapped file. */
	class native_filemap
	{
	public:
		typedef typename detail::native_filemap_handle::native_handle_type native_handle_type;

		typedef system::mapmode mapmode;
		/** Enable copy-on-write for mapped pages. Any changes will not be committed to the backing file.
		 * @note Source file must be open for reading. */
		constexpr static mapmode map_copy = system::map_copy;
		/** Pre-populate mapped pages. */
		constexpr static mapmode map_populate = system::map_populate;

	public:
		native_filemap(const native_filemap &) = delete;
		native_filemap &operator=(const native_filemap &) = delete;

		/** Initializes an invalid (not mapped) filemap. */
		constexpr native_filemap() noexcept = default;
		inline ~native_filemap() { unmap(); }

		constexpr native_filemap(native_filemap &&other) noexcept { swap(other); }
		constexpr native_filemap &operator=(native_filemap &&other) noexcept
		{
			swap(other);
			return *this;
		}

		/** @brief Maps a portion of the file into memory.
		 * @param file File to create a mapping of.
		 * @param off Offset into the file at which to create the mapping.
		 * @param n Amount of bytes to map. Must be less than file size - offset. If set to 0, maps the entire file.
		 * @param mode Mode of the mapping. If set to 0 will use the default mode.
		 * @note After a file has been mapped, the source file can be closed.
		 * @note File should be open with a combination of `in` and `out` modes.
		 * @throw std::system_error On any system errors. */
		explicit native_filemap(const native_file &file, std::uint64_t off = 0, std::uint64_t n = 0, mapmode mode = 0)
		{
			map(file, off, n, mode);
		}

		// clang-format off
		/** @copydoc native_filemap */
		SEK_API void map(const native_file &file, std::uint64_t off = 0, std::uint64_t n = 0, mapmode mode = 0);
		/** @copybrief map
		 * @param file File to create a mapping of.
		 * @param off Offset into the file at which to create the mapping.
		 * @param n Amount of bytes to map. Must be less than file size - offset. If set to 0, maps the entire file.
		 * @param mode Mode of the mapping. If set to 0 will use the default mode.
		 * @return `void` or an error code.
		 * @note After a file has been mapped, the source file can be closed.
		 * @note File should be open with a combination of `in` and `out` modes. */
		expected<void, std::error_code> map(std::nothrow_t, const native_file &file, std::uint64_t off = 0, std::uint64_t n = 0, mapmode mode = 0) noexcept
		{
			return m_handle.map(file.m_handle, off, n, file.mode(), mode);
		}
		// clang-format on

		/** @brief Unmaps the file mapped file from memory.
		 * @throw std::system_error On any system errors. */
		SEK_API void unmap();
		/** @copybrief unmap
		 * @return `void` or an error code. */
		inline expected<void, std::error_code> unmap(std::nothrow_t) noexcept { return m_handle.unmap(); }

		/** Returns size of the mapping. */
		[[nodiscard]] constexpr std::uint64_t size() const noexcept { return m_handle.size(); }
		/** Returns pointer to the mapped data. */
		[[nodiscard]] constexpr void *data() const noexcept { return m_handle.data(); }
		/** Returns span of bytes to the mapped data. */
		[[nodiscard]] constexpr std::span<std::byte> bytes() const noexcept
		{
			return {static_cast<std::byte *>(data()), static_cast<std::size_t>(size())};
		}

		/** @brief Checks if the mapping is valid. */
		[[nodiscard]] constexpr bool is_mapped() const noexcept { return m_handle.is_mapped(); }

		/** Releases and returns the underlying OS handle of the file mapping. */
		[[nodiscard]] constexpr native_handle_type release() noexcept { return m_handle.release(); }
		/** Returns underlying OS handle of the file mapping. */
		[[nodiscard]] constexpr native_handle_type native_handle() const noexcept { return m_handle.native_handle(); }

		constexpr void swap(native_filemap &other) noexcept { m_handle.swap(other.m_handle); }
		friend constexpr void swap(native_filemap &a, native_filemap &b) noexcept { a.swap(b); }

	private:
		detail::native_filemap_handle m_handle;
	};
}	 // namespace sek::system
