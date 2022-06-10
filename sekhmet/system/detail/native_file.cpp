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

#include "native_file.hpp"

#include <cstring>

#include "sekhmet/math/utility.hpp"

#if defined(SEK_OS_UNIX)
#include "unix/native_file.cpp"	   // NOLINT
#elif defined(SEK_OS_WIN)
#include "win/native_file.cpp"	  // NOLINT
#endif

namespace sek::system
{
	constexpr std::size_t init_buffer_size = SEK_KB(8);

	native_file::~native_file()
	{
		close();
		delete[] buffer;
	}

	bool native_file::open(const typename std::filesystem::path::value_type *path, openmode mode)
	{
		bool success = handle.open(path, mode);
		if (success) [[likely]]
		{
			file_mode = mode;
			return true;
		}
		return false;
	}
	bool native_file::close()
	{
		const auto result = flush();
		return handle.close() && result;
	}

	bool native_file::flush()
	{
		if (input_size != 0) /* Un-read any buffered input. */
			return handle.seek(buffer_pos - std::exchange(input_size, 0), cur) >= 0;
		else if (buffer_pos != 0) /* Write any buffered output */
			return handle.write(buffer, static_cast<std::size_t>(buffer_pos)) == std::exchange(buffer_pos, 0);
		return true;
	}
	ssize_t native_file::seek(ssize_t off, native_file::seek_dir dir)
	{
		if (off == 0 && dir != cur && !flush()) [[unlikely]]
			return -1;
		return handle.seek(off, dir);
	}
	ssize_t native_file::tell() const { return handle.tell(); }

	std::size_t native_file::write(const void *src, std::size_t n)
	{
		if (file_mode & out) [[likely]]
		{
			if (input_size != 0) /* Un-read the buffered input. */
			{
				if (handle.seek(buffer_pos - input_size, cur) < 0) [[unlikely]]
					return 0;
				input_size = 0;
			}
			else if (buffer == nullptr) [[unlikely]] /* Allocate new buffer if needed. */
			{
				/* Unbuffered mode. */
				if (file_mode & direct) [[unlikely]]
				{
					const auto result = handle.write(src, n);
					return result > 0 ? static_cast<std::size_t>(result) : 0;
				}

				buffer = new std::byte[init_buffer_size];
				buffer_size = init_buffer_size;
			}

			for (std::size_t total = 0, write_n; total < n; total += write_n)
			{
				write_n = math::min(n, static_cast<std::size_t>(buffer_size - buffer_pos));
				memcpy(buffer + buffer_pos, src, write_n);

				/* Flush to the file if needed. */
				if ((buffer_pos += static_cast<ssize_t>(write_n)) == buffer_size)
				{
					auto flush_n = handle.write(buffer, static_cast<std::size_t>(write_n));
					if (flush_n != buffer_size) [[unlikely]]
						return static_cast<std::size_t>(total);
					buffer_pos = 0;
				}
			}
			return n;
		}
		return 0;
	}
	std::size_t native_file::read(void *dst, std::size_t n)
	{
		if (file_mode & in) [[likely]]
		{
			if (input_size == 0 && buffer_pos != 0) /* Flush any pending output. */
			{
				if (handle.write(buffer, static_cast<std::size_t>(buffer_pos)) != buffer_pos) [[unlikely]]
					return 0;
				buffer_pos = 0;
			}
			else if (buffer == nullptr) [[unlikely]] /* Allocate new buffer if needed. */
			{
				/* Unbuffered mode. */
				if (file_mode & direct) [[unlikely]]
				{
					const auto result = handle.read(dst, n);
					return result > 0 ? static_cast<std::size_t>(result) : 0;
				}

				buffer = new std::byte[init_buffer_size];
				buffer_size = init_buffer_size;
			}

			for (std::size_t total = 0, read_n; total < n; total += read_n)
			{
				if (buffer_pos == input_size) [[unlikely]] /* Buffer in data from file. */
				{
					/* Result might be less than buffer_size. This can happen if the file is smaller than 8kb. */
					auto res = handle.read(buffer, static_cast<std::size_t>(buffer_size));
					if (res <= 0) [[unlikely]]
						return total;
					input_size = res;
					buffer_pos = 0;
				}

				read_n = math::min(n, static_cast<std::size_t>(input_size - buffer_pos));
				memcpy(dst, buffer + buffer_pos, read_n);
				buffer_pos += static_cast<ssize_t>(read_n);
			}
			return n;
		}
		return 0;
	}
}	 // namespace sek::system
