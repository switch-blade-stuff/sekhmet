/*
 * Created by switchblade on 09/06/22
 */

#pragma once

#include <filesystem>
#include <span>

#include "sekhmet/detail/define.h"

namespace sek::system
{
	namespace detail
	{
		typedef int openmode;

		constexpr openmode in = 1;
		constexpr openmode out = 2;
		constexpr openmode trunc = 4;
		constexpr openmode append = 8;
		constexpr openmode atend = 16;
		constexpr openmode create = 32;
		constexpr openmode direct = 64;
		constexpr auto openmode_bits = 8;

		typedef int mapmode;
		constexpr mapmode copy = 1 << openmode_bits;
		constexpr mapmode populate = 2 << openmode_bits;

		typedef int seek_dir;

		constexpr static seek_dir beg = -1;
		constexpr static seek_dir cur = 0;
		constexpr static seek_dir end = 1;

		class native_filemap_handle;
		class native_file_handle;
	}	 // namespace detail

	class native_filemap;
	class native_file;
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
	/** @brief Structure used to preform operations on a native OS file. */
	class native_file
	{
		friend class native_filemap;

	public:
		typedef typename detail::native_file_handle::native_handle_type native_handle_type;
		typedef typename detail::openmode openmode;

		constexpr static openmode in = detail::in;
		constexpr static openmode out = detail::out;
		constexpr static openmode trunc = detail::trunc;
		constexpr static openmode append = detail::append;
		constexpr static openmode atend = detail::atend;
		constexpr static openmode create = detail::create;
		constexpr static openmode direct = detail::direct;

		typedef typename detail::seek_dir seek_dir;

		constexpr static seek_dir beg = detail::beg;
		constexpr static seek_dir cur = detail::cur;
		constexpr static seek_dir end = detail::end;

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

		/** Initializes & opens the file.
		 * @param path Path to the file.
		 * @param mode Mode to open the file with. */
		inline native_file(const typename std::filesystem::path::value_type *path, openmode mode) { open(path, mode); }
		/** @copydoc native_file */
		inline native_file(const std::filesystem::path &path, openmode mode) : native_file(path.c_str(), mode) {}

		/** Opens the file.
		 * @param path Path to the file.
		 * @param mode Mode to open the file with.
		 * @return `true` on success, `false` on failure. */
		inline bool open(const std::filesystem::path &path, openmode mode) { return open(path.c_str(), mode); }
		/** @copydoc open */
		SEK_API bool open(const typename std::filesystem::path::value_type *path, openmode mode);
		/** Flushes & closes the file.
		 * @return `true` on success, `false` on failure.
		 * @note File is always closed, even if the flush fails. */
		SEK_API bool close();

		/** Flushes buffered output to the underlying file and un-reads any buffered input.
		 * @return `true` on success, `false` on failure. If the internal buffer is empty, returns true. */
		SEK_API bool flush();
		/** Seeks the file in the specified direction to the specified offset.
		 * @param off Offset to seek.
		 * @param dir Direction to seek in (`beg`, `curr` or `end`).
		 * @return Resulting within the file or a negative integer on error.
		 * @note If the wile is open in read mode, flushes the output buffer before seeking. */
		SEK_API std::int64_t seek(std::int64_t off, seek_dir dir);

		/** Writes data buffer to the file.
		 * @param src Memory buffer containing source data.
		 * @param n Amount of bytes to write.
		 * @return Amount of bytes written to the output, which might be less than `n` if an error has occurred. */
		SEK_API std::size_t write(const void *src, std::size_t n);
		/** Reads file to a data buffer.
		 * @param dst Memory buffer receiving data.
		 * @param n Amount of bytes to read.
		 * @return Amount of bytes read from the file, which might be less than `n` if an error has occurred. */
		SEK_API std::size_t read(void *dst, std::size_t n);

		/** Returns the current position within the file or a negative integer on error. */
		[[nodiscard]] SEK_API std::int64_t tell() const;
		/** Returns the file open mode. */
		[[nodiscard]] constexpr openmode mode() const noexcept { return m_mode; }
		/** Checks if the file is open. */
		[[nodiscard]] constexpr bool is_open() const noexcept { return m_handle.is_open(); }
		/** Returns the underlying OS file handle. */
		[[nodiscard]] constexpr native_handle_type native_handle() const noexcept { return m_handle.native_handle(); }

		constexpr void swap(native_file &other) noexcept
		{
			m_handle.swap(other.m_handle);
			std::swap(m_buffer, other.m_buffer);
			std::swap(m_buffer_size, other.m_buffer_pos);
			std::swap(m_buffer_pos, other.m_buffer_pos);
			std::swap(m_mode, other.m_mode);
		}
		friend constexpr void swap(native_file &a, native_file &b) noexcept { a.swap(b); }

	private:
		detail::native_file_handle m_handle;

		std::byte *m_buffer = nullptr;	/* Buffer used fore read & write operations. */
		std::int64_t m_buffer_size = 0; /* Total size of the buffer. */
		std::int64_t m_buffer_pos = 0;	/* Current read or write position within the buffer. */

		/* Size of the input buffer, used only for reading. Might be less than buffer_size in case the file
		 * size is less than size of the buffer. */
		std::int64_t m_input_size = 0;

		openmode m_mode = 0;
		bool m_writing = false;
		bool m_reading = false;
	};
	/** @brief Structure used to manage a memory-mapped file. */
	class native_filemap
	{
	public:
		typedef typename detail::native_filemap_handle::native_handle_type native_handle_type;

		typedef detail::mapmode mapmode;
		/** Enable copy-on-write for mapped pages. Any changes will not be committed to the backing file.
		 * @note Source file must be open for reading. */
		constexpr static mapmode copy = detail::copy;
		/** Pre-populate mapped pages. */
		constexpr static mapmode populate = detail::populate;

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

		/** Maps a portion of the file into memory.
		 * @param file File to create a mapping of.
		 * @param off Offset into the file at which to create the mapping.
		 * @param n Amount of bytes to map. Must be less than file size - offset. If set to 0, maps the entire file.
		 * @param mode Mode of the mapping. If set to 0 will use the default mode.
		 * @note After a file has been mapped, the source file can be closed.
		 * @note File should be open with a combination of `in` and `out` modes. */
		inline native_filemap(const native_file &file, std::int64_t off = 0, std::int64_t n = 0, mapmode mode = 0)
		{
			map(file, off, n, mode);
		}
		/** @copydoc native_filemap
		 * @return `true` on success, `false` on error. */
		inline bool map(const native_file &file, std::int64_t off = 0, std::int64_t n = 0, mapmode mode = 0)
		{
			return m_handle.map(file.m_handle, off, n, mode | file.m_mode);
		}
		/** Unmaps the file mapping.
		 * @return `true` on success, `false` on error.
		 * @note File is always unmapped, even when an error has occured. */
		inline bool unmap() { return m_handle.unmap(); }

		/** Returns size of the mapping. */
		[[nodiscard]] constexpr std::int64_t size() const noexcept { return m_handle.size(); }
		/** Returns pointer to the mapped data. */
		[[nodiscard]] constexpr void *data() const noexcept { return m_handle.data(); }
		/** Returns span of bytes to the mapped data. */
		[[nodiscard]] constexpr std::span<std::byte> bytes() const noexcept
		{
			return {static_cast<std::byte *>(data()), static_cast<std::size_t>(size())};
		}

		/** @brief Checks if the mapping is valid. */
		[[nodiscard]] constexpr bool is_mapped() const noexcept { return m_handle.is_mapped(); }

		/** Returns underlying OS handle of the file mapping. */
		[[nodiscard]] constexpr native_handle_type native_handle() const noexcept { return m_handle.native_handle(); }

		constexpr void swap(native_filemap &other) noexcept { m_handle.swap(other.m_handle); }
		friend constexpr void swap(native_filemap &a, native_filemap &b) noexcept { a.swap(b); }

	private:
		detail::native_filemap_handle m_handle;
	};
}	 // namespace sek::system
