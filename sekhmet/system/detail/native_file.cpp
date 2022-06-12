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
		delete[] m_buffer;
	}

	bool native_file::open(const typename std::filesystem::path::value_type *path, openmode mode)
	{
		bool success = m_handle.open(path, mode);
		if (success) [[likely]]
		{
			m_mode = mode;
			return true;
		}
		return false;
	}
	bool native_file::close()
	{
		const auto result = flush();
		return m_handle.close() && result;
	}

	bool native_file::flush()
	{
		if (m_input_size != 0) /* Un-read any buffered input. */
			return m_handle.seek(m_buffer_pos - std::exchange(m_input_size, 0), cur) >= 0;
		else if (m_buffer_pos != 0) /* Write any buffered output */
			return m_handle.write(m_buffer, static_cast<std::size_t>(m_buffer_pos)) == std::exchange(m_buffer_pos, 0);
		return true;
	}
	std::int64_t native_file::seek(std::int64_t off, seek_dir dir)
	{
		if (dir == cur)
		{
			/* If the new offset is within the current buffer, update the buffer position. */
			const auto new_pos = m_buffer_pos + off;
			if (((m_input_size && new_pos <= m_input_size) || new_pos <= m_buffer_size) && new_pos >= 0)
				return m_handle.tell() + (m_buffer_pos = new_pos);
		}
		if (!flush()) [[unlikely]]
			return -1;
		return m_handle.seek(off, dir);
	}
	std::int64_t native_file::tell() const { return m_handle.tell(); }

	std::size_t native_file::write(const void *src, std::size_t n)
	{
		if (m_mode & out) [[likely]]
		{
			if (m_input_size != 0) /* Un-read the buffered input. */
			{
				if (m_handle.seek(m_buffer_pos - m_input_size, cur) < 0) [[unlikely]]
					return 0;
				m_input_size = 0;
			}
			else if (m_buffer == nullptr) [[unlikely]] /* Allocate new buffer if needed. */
			{
				/* Unbuffered mode. */
				if (m_mode & direct) [[unlikely]]
				{
					const auto result = m_handle.write(src, n);
					return result > 0 ? static_cast<std::size_t>(result) : 0;
				}

				m_buffer = new std::byte[init_buffer_size];
				m_buffer_size = init_buffer_size;
			}

			for (std::size_t total = 0, write_n; total < n; total += write_n)
			{
				write_n = math::min(n, static_cast<std::size_t>(m_buffer_size - m_buffer_pos));
				memcpy(m_buffer + m_buffer_pos, src, write_n);

				/* Flush to the file if needed. */
				if ((m_buffer_pos += static_cast<std::int64_t>(write_n)) == m_buffer_size)
				{
					auto flush_n = m_handle.write(m_buffer, static_cast<std::size_t>(write_n));
					if (flush_n != m_buffer_size) [[unlikely]]
						return static_cast<std::size_t>(total);
					m_buffer_pos = 0;
				}
			}
			return n;
		}
		return 0;
	}
	std::size_t native_file::read(void *dst, std::size_t n)
	{
		if (m_mode & in) [[likely]]
		{
			if (m_input_size == 0 && m_buffer_pos != 0) /* Flush any pending output. */
			{
				if (m_handle.write(m_buffer, static_cast<std::size_t>(m_buffer_pos)) != m_buffer_pos) [[unlikely]]
					return 0;
				m_buffer_pos = 0;
			}
			else if (m_buffer == nullptr) [[unlikely]] /* Allocate new buffer if needed. */
			{
				/* Unbuffered mode. */
				if (m_mode & direct) [[unlikely]]
				{
					const auto result = m_handle.read(dst, n);
					return result > 0 ? static_cast<std::size_t>(result) : 0;
				}

				m_buffer = new std::byte[init_buffer_size];
				m_buffer_size = init_buffer_size;
			}

			for (std::size_t total = 0, read_n; total < n; total += read_n)
			{
				if (m_buffer_pos == m_input_size) [[unlikely]] /* Buffer in data from file. */
				{
					/* Result might be less than buffer_size. This can happen if the file is smaller than 8kb. */
					auto res = m_handle.read(m_buffer, static_cast<std::size_t>(m_buffer_size));
					if (res <= 0) [[unlikely]]
						return total;
					m_input_size = res;
					m_buffer_pos = 0;
				}

				read_n = math::min(n, static_cast<std::size_t>(m_input_size - m_buffer_pos));
				memcpy(dst, m_buffer + m_buffer_pos, read_n);
				m_buffer_pos += static_cast<std::int64_t>(read_n);
			}
			return n;
		}
		return 0;
	}
}	 // namespace sek::system
