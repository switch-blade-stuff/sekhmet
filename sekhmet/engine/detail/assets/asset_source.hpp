/*
 * Created by switchblade on 10/08/22
 */

#pragma once

#include "sekhmet/uri.hpp"

#include "asset_io.hpp"
#include "fwd.hpp"

namespace sek
{
	/** @brief Structure providing a read-only access to data of an asset.
	 *
	 * Since assets may be either loose or compressed and archived, a special structure is needed to read asset data.
	 * In addition, to allow for implementation of storage optimization techniques (such as DirectStorage),
	 * streams cannot be used directly either, as access to the underlying file or data buffer is needed. */
	class asset_source
	{
		friend class detail::package_info;

	public:
		typedef typename native_file::seek_basis seek_basis;

		constexpr static seek_basis seek_set = native_file::seek_set;
		constexpr static seek_basis seek_cur = native_file::seek_cur;
		constexpr static seek_basis seek_end = native_file::seek_end;

	private:
		constexpr asset_source(detail::asset_io_data &&data, std::uint64_t offset, std::uint64_t size) noexcept
			: m_data(std::move(data)), m_offset(offset), m_size(size)
		{
		}

				template<typename T>
				inline static T return_if(expected<T, std::error_code> &&exp)
				{
					if (!exp.has_value()) [[unlikely]]
						throw std::system_error(exp.error());
					return std::move(exp.value());
				}
	public:
		asset_source(const asset_source &) = delete;
		asset_source &operator=(const asset_source &) = delete;

		/** Initializes asset source from a native file.
		 * @param file File containing the asset.
		 * @throw std::system_error On implementation-defined file errors. */
		explicit asset_source(native_file &&file) : m_data(detail::asset_io_data(std::move(file)))
		{
			m_offset = m_data.file()->tell();
			m_size = m_data.file()->size();
		}
		/** @copydoc asset_source
		 * @param offset Offset from the start of the file at which the asset's data starts.
		 * @note File will be seeked to the specified offset. */
		asset_source(native_file &&file, std::uint64_t offset)
			: m_data(detail::asset_io_data(std::move(file))), m_offset(offset)
		{
			m_size = m_data.file()->size() - offset;
			m_data.file()->setpos(offset);
		}
		/** @copydoc asset_source
		 * @param size Size of the asset. */
		asset_source(native_file &&file, std::uint64_t offset, std::uint64_t size)
			: m_data(detail::asset_io_data(std::move(file))), m_offset(offset), m_size(size)
		{
			m_data.file()->setpos(offset);
		}

		/** Initializes asset source from an asset buffer.
		 * @param buff Buffer containing the asset. */
		explicit asset_source(asset_buffer &&buff) noexcept : m_data(detail::asset_io_data(std::move(buff)))
		{
			m_size = return_if(m_data.size());
		}
		/** @copydoc asset_source
		 * @param offset Offset from the start of the buffer at which the asset's data starts.
		 * @note Buffer will be seeked to the specified offset. */
		asset_source(asset_buffer &&buff, std::uint64_t offset) noexcept
			: m_data(detail::asset_io_data(std::move(buff))), m_offset(offset)
		{
			m_size = return_if(m_data.size()) - offset;
			return_if(m_data.setpos(offset));
		}
		/** @copydoc asset_source
		 * @param size Size of the asset. */
		asset_source(asset_buffer &&buff, std::uint64_t offset, std::uint64_t size) noexcept
			: m_data(detail::asset_io_data(std::move(buff))), m_offset(offset), m_size(size)
		{
			return_if(m_data.setpos(offset));
		}

		/** Initializes an empty asset source. */
		constexpr asset_source() noexcept = default;
		constexpr asset_source(asset_source &&other) noexcept
			: m_data(std::move(other.m_data)),
			  m_offset(std::exchange(other.m_offset, 0)),
			  m_size(std::exchange(other.m_size, 0)),
			  m_read_pos(std::exchange(other.m_read_pos, 0))
		{
		}
		constexpr asset_source &operator=(asset_source &&other) noexcept
		{
			m_data = std::move(other.m_data);
			m_size = std::exchange(other.m_size, 0);
			m_offset = std::exchange(other.m_offset, 0);
			m_read_pos = std::exchange(other.m_read_pos, 0);
			return *this;
		}

		/** Checks if the asset source is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_data.empty(); }

		/** Returns the base offset of the asset source. */
		[[nodiscard]] constexpr std::uint64_t base_offset() const noexcept { return m_offset; }
		/** Returns the total size of the asset source. */
		[[nodiscard]] constexpr std::uint64_t size() const noexcept { return m_size; }
		/** Returns the current read position of the asset source. */
		[[nodiscard]] constexpr std::uint64_t tell() const noexcept { return m_read_pos; }

		/** @brief Reads `n` bytes from the asset source and advances the read position.
		 * @param dst Memory buffer receiving data.
		 * @param n Amount of bytes to read.
		 * @return Amount of bytes read from the file.
		 * @throw std::system_error On implementation-defined system errors. */
		SEK_API std::size_t read(void *dst, std::size_t n);
		/** @copybrief read
		 * @param dst Memory buffer receiving data.
		 * @param n Amount of bytes to read.
		 * @return Amount of bytes read from the file or an error code. */
		SEK_API expected<std::size_t, std::error_code> read(std::nothrow_t, void *dst, std::size_t n) noexcept;

		/** @brief Reads `n` bytes from the asset source and advances the read position.
		 * @param dst ASIO mutable buffer receiving data.
		 * @return Amount of bytes read from the file.
		 * @throw std::system_error On implementation-defined system errors. */
		SEK_API std::size_t read(asio::mutable_buffer &dst);
		/** @copybrief read
		 * @param dst ASIO mutable buffer receiving data.
		 * @return Amount of bytes read from the file or an error code. */
		SEK_API expected<std::size_t, std::error_code> read(std::nothrow_t, asio::mutable_buffer &dst) noexcept;

		/** @brief Seeks the asset source to the specified offset.
		 * @param off Offset to seek to.
		 * @param dir Direction in which to seek.
		 * @return Resulting position within the asset source.
		 * @throw std::system_error On implementation-defined system errors. */
		SEK_API std::uint64_t seek(std::int64_t off, seek_basis dir);
		/** @copybrief seek
		 * @param off Offset to seek to.
		 * @param dir Direction in which to seek.
		 * @return Resulting position within the asset source or an error code. */
		SEK_API expected<std::uint64_t, std::error_code> seek(std::nothrow_t, std::int64_t off, seek_basis dir) noexcept;

		/** @brief Sets position within the asset source to the specified offset from the start.
		 * Equivalent to `seek(static_cast<std::int64_t>(pos), seek_set)`.
		 * @param pos New position within the asset source.
		 * @return Resulting position within the asset source.
		 * @throw std::system_error On implementation-defined system errors. */
		SEK_API std::uint64_t setpos(std::uint64_t pos);
		/** @copybrief setpos
		 * @param pos New position within the asset source.
		 * @return Resulting position within the asset source or an error code. */
		SEK_API expected<std::uint64_t, std::error_code> setpos(std::nothrow_t, std::uint64_t pos) noexcept;

		/** If the asset source is backed by a file, returns pointer to the file. Otherwise returns `nullptr`. */
		[[nodiscard]] constexpr native_file *file() noexcept { return m_data.file(); }
		/** @copydoc file */
		[[nodiscard]] constexpr const native_file *file() const noexcept { return m_data.file(); }

		constexpr void swap(asset_source &other) noexcept
		{
			m_data.swap(other.m_data);
			std::swap(m_size, other.m_size);
			std::swap(m_offset, other.m_offset);
			std::swap(m_read_pos, other.m_read_pos);
		}
		friend constexpr void swap(asset_source &a, asset_source &b) noexcept { a.swap(b); }

	private:
		detail::asset_io_data m_data = {};

		std::uint64_t m_offset = 0;	  /* Base offset within the source data. */
		std::uint64_t m_size = 0;	  /* Total (accessible) size of the data. */
		std::uint64_t m_read_pos = 0; /* Current read position with the base offset applied. */
	};
}	 // namespace sek