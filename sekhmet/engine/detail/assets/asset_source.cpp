/*
 * Created by switchblade on 10/08/22
 */

#include "asset_source.hpp"

namespace sek::engine
{
	template<typename T>
	inline static T return_if(expected<T, std::error_code> &&exp)
	{
		if (!exp.has_value()) [[unlikely]]
			throw std::system_error(exp.error());
		return std::move(exp.value());
	}

	std::size_t asset_source::read(void *dst, std::size_t n) { return return_if(read(std::nothrow, dst, n)); }
	std::size_t asset_source::read(asio::mutable_buffer &buff) { return return_if(read(std::nothrow, buff)); }
	expected<std::size_t, std::error_code> asset_source::read(std::nothrow_t, void *dst, std::size_t n) noexcept
	{
		auto new_pos = m_read_pos + static_cast<std::uint64_t>(n);
		if (new_pos > m_size) [[unlikely]]
		{
			n = static_cast<std::size_t>(m_size - m_read_pos);
			new_pos = m_size;
		}

		auto result = m_data.read(dst, n);
		if (result.has_value()) [[likely]]
			m_read_pos += *result;
		return result;
	}
	expected<std::size_t, std::error_code> asset_source::read(std::nothrow_t, asio::mutable_buffer &buff) noexcept
	{
		return read(buff.data(), buff.size());
	}

	std::uint64_t asset_source::seek(std::int64_t off, seek_basis dir)
	{
		return return_if(seek(std::nothrow, off, dir));
	}
	std::uint64_t asset_source::setpos(std::uint64_t pos) { return return_if(setpos(std::nothrow, pos)); }
	expected<std::uint64_t, std::error_code> asset_source::seek(std::nothrow_t, std::int64_t off, asset_source::seek_basis dir) noexcept
	{
		if (!empty()) [[likely]]
			return m_data.seek(off + static_cast<std::int64_t>(m_offset), dir);
		return unexpected{std::make_error_code(std::errc::invalid_argument)};
	}
	expected<std::uint64_t, std::error_code> asset_source::setpos(std::nothrow_t, std::uint64_t pos) noexcept
	{
		if (!empty()) [[likely]]
			return m_data.setpos(m_offset + pos);
		return unexpected{std::make_error_code(std::errc::invalid_argument)};
	}
}