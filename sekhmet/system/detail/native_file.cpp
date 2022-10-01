/*
 * Created by switchblade on 09/06/22
 */

#include "../native_file.hpp"

#include <cstring>

#if defined(SEK_OS_UNIX)
#include "unix/native_file.cpp"	   // NOLINT
#endif

namespace sek
{
	constexpr std::size_t init_buffer_size = SEK_KB(8);

	void native_file::init_buffer(std::size_t min_size)
	{
		const auto size = std::max(min_size, init_buffer_size);
		m_buffer = new std::byte[size];
		m_buffer_size = size;
	}

	native_file::~native_file()
	{
		if (is_open()) flush();
		delete[] m_buffer;
	}

	void native_file::open(const path_char *path, openmode mode) { return_if(open(std::nothrow, path, mode)); }

	void native_file::close() { return_if(close(std::nothrow)); }
	void native_file::flush() { return_if(flush(std::nothrow)); }
	void native_file::sync() { return_if(sync(std::nothrow)); }

	std::size_t native_file::read(void *dst, std::size_t n) { return return_if(read(std::nothrow, dst, n)); }
	std::size_t native_file::read(asio::mutable_buffer &buff) { return return_if(write(std::nothrow, buff)); }
	std::size_t native_file::write(const void *src, std::size_t n) { return return_if(write(std::nothrow, src, n)); }
	std::size_t native_file::write(const asio::const_buffer &buff) { return return_if(write(std::nothrow, buff)); }

	std::uint64_t native_file::seek(std::int64_t off, seek_basis dir)
	{
		return return_if(seek(std::nothrow, off, dir));
	}
	std::uint64_t native_file::setpos(std::uint64_t pos) { return return_if(setpos(std::nothrow, pos)); }
	std::uint64_t native_file::resize(std::uint64_t size) { return return_if(resize(std::nothrow, size)); }

	std::uint64_t native_file::size() const { return return_if(size(std::nothrow)); }
	std::uint64_t native_file::tell() const { return return_if(tell(std::nothrow)); }

	expected<void, std::error_code> native_file::open(std::nothrow_t, const path_char *path, openmode mode) noexcept
	{
		auto result = m_handle.open(path, mode);
		if (result.has_value()) [[likely]]
			m_mode = mode;
		return result;
	}

	expected<void, std::error_code> native_file::close(std::nothrow_t) noexcept
	{
		if (auto result = flush(std::nothrow); result.has_value()) [[likely]]
			return m_handle.close();
		else
			return result;
	}
	expected<void, std::error_code> native_file::flush(std::nothrow_t) noexcept
	{
		if (std::exchange(m_reading, false)) /* Un-read buffered input. */
		{
			const auto size = std::exchange(m_input_size, 0);
			const auto pos = std::exchange(m_buffer_pos, 0);
			m_reading = false;

			return m_handle.seek(static_cast<std::int64_t>(pos - size), seek_cur);
		}
		else if (std::exchange(m_writing, false)) /* Flush buffered output */
		{
			const auto pos = std::exchange(m_buffer_pos, 0);
			m_writing = false;

			return m_handle.write(m_buffer, static_cast<std::size_t>(pos));
		}
		return {};
	}
	expected<void, std::error_code> native_file::sync(std::nothrow_t) noexcept
	{
		if (auto result = flush(std::nothrow); result.has_value()) [[likely]]
			return m_handle.sync();
		else
			return result;
	}

	expected<std::uint64_t, std::error_code> native_file::seek(std::nothrow_t, std::int64_t off, seek_basis dir) noexcept
	{
		if (dir == seek_cur) /* If the new offset is within the current buffer, update the buffer position. */
		{
			const auto buf_size = static_cast<std::int64_t>(m_buffer_size);
			const auto in_size = static_cast<std::int64_t>(m_input_size);
			const auto new_pos = static_cast<std::int64_t>(m_buffer_pos) + off;
			if (new_pos >= 0 && ((m_reading && new_pos <= in_size) || (m_writing && new_pos <= buf_size)))
			{
				const auto pos = m_buffer_pos = static_cast<std::uint64_t>(new_pos);
				const auto result = m_handle.tell();
				if (!result.has_value()) [[unlikely]]
					return unexpected{result.error()};
				return *result + pos;
			}
		}
		/* Otherwise, flush the buffer & seek the file directly. */
		if (const auto result = flush(std::nothrow); !result.has_value()) [[unlikely]]
			return unexpected{result.error()};
		return m_handle.seek(off, dir);
	}
	expected<std::uint64_t, std::error_code> native_file::setpos(std::nothrow_t, std::uint64_t pos) noexcept
	{
		if (const auto result = flush(std::nothrow); !result.has_value()) [[unlikely]]
			return unexpected{result.error()};
		return m_handle.setpos(pos);
	}

	expected<std::uint64_t, std::error_code> native_file::resize(std::nothrow_t, std::uint64_t size) noexcept
	{
		if (auto result = flush(std::nothrow); !result.has_value()) [[unlikely]]
			return unexpected{result.error()};
		return m_handle.resize(size);
	}

	expected<std::uint64_t, std::error_code> native_file::size(std::nothrow_t) const noexcept
	{
		return m_handle.size();
	}
	expected<std::uint64_t, std::error_code> native_file::tell(std::nothrow_t) const noexcept
	{
		auto result = m_handle.tell();
		if (result.has_value() && (m_reading || m_writing)) [[likely]]
			*result += m_buffer_pos;
		return result;
	}

	expected<std::size_t, std::error_code> native_file::read(std::nothrow_t, void *dst, std::size_t n) noexcept
	{
		if ((m_mode & read_only) || (m_mode & read_write)) [[likely]]
		{
			if (m_writing) /* Flush buffered output. */
			{
				auto result = m_handle.write(m_buffer, static_cast<std::size_t>(m_buffer_pos));
				if (!result.has_value()) [[unlikely]]
					return result;

				m_buffer_pos = 0;
				m_writing = false;
			}
			else if (m_buffer == nullptr) [[unlikely]] /* Allocate new buffer if needed. */
			{
				/* Unbuffered mode. */
				if (m_mode & direct) [[unlikely]]
					return m_handle.read(dst, n);

				init_buffer(init_buffer_size);
			}

			/* Fill the internal buffer & copy bytes to destination. */
			std::size_t total, read_n = 0;
			for (; total < n; total += read_n)
			{
				if (m_buffer_pos == m_input_size) [[unlikely]] /* Read data from file. */
				{
					/* Result might be less than buffer_size. This can happen if the file is smaller than 8kb.
					 * If read non-0 but less than m_buffer_size, we still want to copy buffer contents to the
					 * destination. If read 0 bytes, we are at the end and no more reading is possible. */
					auto result = m_handle.read(m_buffer, static_cast<std::size_t>(m_buffer_size));
					if (!result.has_value()) [[unlikely]]
						return result;

					m_buffer_pos = 0;
					m_input_size = *result;
					if (m_input_size == 0) [[unlikely]]
						break;
				}

				read_n = std::min(n, static_cast<std::size_t>(m_input_size - m_buffer_pos));
				memcpy(dst, m_buffer + m_buffer_pos, read_n);
				m_buffer_pos += static_cast<std::uint64_t>(read_n);
			}
			m_reading = m_input_size > m_buffer_pos;
			return total;
		}
		return 0u;
	}
	expected<std::size_t, std::error_code> native_file::read(std::nothrow_t, asio::mutable_buffer &buff) noexcept
	{
		return read(std::nothrow, buff.data(), buff.size());
	}

	expected<std::size_t, std::error_code> native_file::write(std::nothrow_t, const void *src, std::size_t n) noexcept
	{
		if ((m_mode & write_only) || (m_mode & read_write)) [[likely]]
		{
			if (m_reading) /* Un-read buffered input. */
			{
				auto result = m_handle.seek(static_cast<std::int64_t>(m_buffer_pos - m_input_size), seek_cur);
				if (!result.has_value()) [[unlikely]]
					return result;

				m_input_size = 0;
				m_reading = false;
			}
			else if (m_buffer == nullptr) [[unlikely]] /* Allocate new buffer if needed. */
			{
				/* Unbuffered mode. */
				if (m_mode & direct) [[unlikely]]
					return m_handle.write(src, n);

				init_buffer(init_buffer_size);
			}

			std::size_t total = 0, write_n;
			for (; total < n; total += write_n)
			{
				write_n = std::min(n, static_cast<std::size_t>(m_buffer_size - m_buffer_pos));
				memcpy(m_buffer + m_buffer_pos, src, write_n);

				/* Flush to the file if needed. */
				if ((m_buffer_pos += static_cast<std::uint64_t>(write_n)) == m_buffer_size)
				{
					auto result = m_handle.write(m_buffer, static_cast<std::size_t>(write_n));
					if (!result.has_value()) [[unlikely]]
						return result;
					if (*result != m_buffer_size) [[unlikely]]
						return total;

					m_buffer_pos = 0;
				}
			}
			m_writing = m_buffer_pos != 0;
			return total;
		}
		return 0u;
	}
	expected<std::size_t, std::error_code> native_file::write(std::nothrow_t, const asio::const_buffer &buff) noexcept
	{
		return write(std::nothrow, buff.data(), buff.size());
	}
}	 // namespace sek
