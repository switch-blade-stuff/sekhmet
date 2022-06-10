/*
 * ============================================================================
 * Sekhmet - C++20 game engine & editor
 * Copyright (C) 2022 switchblade
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 * ============================================================================
 *
 * Created by switchblade on 09/06/22
 */

#pragma once

#include <filesystem>

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

		typedef int seek_dir;

		constexpr static seek_dir beg = -1;
		constexpr static seek_dir cur = 0;
		constexpr static seek_dir end = 1;

		class native_file_handle;
	}	 // namespace detail

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
		[[nodiscard]] constexpr openmode mode() const noexcept { return file_mode; }
		/** Checks if the file is open. */
		[[nodiscard]] constexpr bool is_open() const noexcept { return handle.is_open(); }
		/** Returns the underlying OS file handle. */
		[[nodiscard]] constexpr native_handle_type native_handle() const noexcept { return handle.native_handle(); }

		constexpr void swap(native_file &other) noexcept
		{
			using std::swap;
			swap(handle, other.handle);
			swap(buffer, other.buffer);
			swap(buffer_size, other.buffer_size);
			swap(buffer_pos, other.buffer_pos);
			swap(file_mode, other.file_mode);
		}
		friend constexpr void swap(native_file &a, native_file &b) noexcept { a.swap(b); }

	private:
		detail::native_file_handle handle;

		std::byte *buffer = nullptr;  /* Buffer used fore read & write operations. */
		std::int64_t buffer_size = 0; /* Total size of the buffer. */
		std::int64_t buffer_pos = 0;  /* Current read or write position within the buffer. */

		/* Size of the input buffer, used only for reading. Might be less than buffer_size in case the file
		 * size is less than size of the buffer. */
		std::int64_t input_size = 0;

		openmode file_mode = 0;
	};
}	 // namespace sek::system
