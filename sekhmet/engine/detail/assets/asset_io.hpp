/*
 * Created by switchblade on 10/08/22
 */

#pragma once

#include "sekhmet/system/native_file.hpp"

#include "fwd.hpp"

namespace sek
{
	/** @brief Utility structure used to represent buffer containing data of an asset. */
	class asset_buffer
	{
		static std::byte *allocate(std::size_t n)
		{
			const auto result = malloc(n);
			if (result == nullptr) [[unlikely]]
				throw std::bad_alloc();
			return static_cast<std::byte *>(result);
		}
		static std::byte *reallocate(std::byte *ptr, std::size_t n)
		{
			const auto result = realloc(ptr, n);
			if (result == nullptr) [[unlikely]]
				throw std::bad_alloc();
			return static_cast<std::byte *>(result);
		}

	public:
		/** Initializes an empty asset buffer. */
		constexpr asset_buffer() noexcept = default;

		asset_buffer(const asset_buffer &other) : asset_buffer(other.m_size)
		{
			std::copy_n(other.m_data, other.m_size, m_owned);
		}
		asset_buffer &operator=(const asset_buffer &other)
		{
			if (this != &other)
			{
				resize(other.size());
				std::copy_n(other.m_data, other.m_size, m_owned);
			}
			return *this;
		}

		constexpr asset_buffer(asset_buffer &&other) noexcept { swap(other); }
		constexpr asset_buffer &operator=(asset_buffer &&other) noexcept
		{
			swap(other);
			return *this;
		}

		~asset_buffer() { free(m_owned); }

		/** Initializes an owning asset buffer with the specified size. */
		explicit asset_buffer(std::size_t n) : m_owned(allocate(n)), m_data(m_owned), m_size(n) {}
		/** Initializes a non-owning asset buffer with the specified data and size. */
		constexpr asset_buffer(const void *data, std::size_t n) noexcept
			: m_data(static_cast<const std::byte *>(data)), m_size(n)
		{
		}

		/** Converts ASIO constant buffer to a non-owning asset buffer. */
		asset_buffer(const asio::const_buffer &buff) noexcept : asset_buffer(buff.data(), buff.size()) {}

		/** If the buffer is non-owning, takes ownership (makes a copy copies) of the data.
		 * @return Reference to this asset buffer. */
		asset_buffer &take_ownership()
		{
			if (m_owned == nullptr && m_size != 0) [[likely]]
			{
				m_owned = allocate(m_size);
				std::copy_n(m_data, m_size, m_owned);
			}
			return *this;
		}
		/** If the buffer owns the lifetime of it's data, resizes the data.
		 * Otherwise, copies and takes ownership of the data with the new size.
		 * @return Reference to this asset buffer. */
		asset_buffer &resize(std::size_t n)
		{
			const auto new_data = reallocate(m_owned, n);
			if (m_data != m_owned) [[likely]]
				std::copy_n(m_data, m_size, new_data);
			m_data = m_owned = new_data;
			m_size = n;
			return *this;
		}

		/** Returns pointer to the owned byte buffer. */
		[[nodiscard]] constexpr std::byte *owned_bytes() noexcept { return m_owned; }

		/** Returns pointer to the data of the buffer. */
		[[nodiscard]] constexpr const void *data() const noexcept { return static_cast<const void *>(m_data); }
		/** Returns the total size of the buffer. */
		[[nodiscard]] constexpr std::uint64_t size() const noexcept { return m_size; }
		/** Returns current read position within the buffer. */
		[[nodiscard]] constexpr std::uint64_t tell() const noexcept { return m_pos; }
		/** Checks if the asset buffer is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_size == 0; }
		/** Checks if the asset buffer owns the lifetime of it's data. */
		[[nodiscard]] constexpr bool is_owning() const noexcept { return m_owned == m_data; }

		/** Reads `n` bytes from the buffer and advances the read position. */
		[[nodiscard]] constexpr std::size_t read(void *dst, std::size_t n) noexcept
		{
			auto new_pos = m_pos + n;
			if (new_pos > m_size) [[unlikely]]
			{
				new_pos = m_size;
				n = m_size - new_pos;
			}
			std::copy_n(m_data + std::exchange(m_pos, new_pos), n, static_cast<std::byte *>(dst));
			return n;
		}
		/** Reads data from the buffer into the ASIO mutable buffer and advances the read position. */
		[[nodiscard]] std::size_t read(asio::mutable_buffer &dst) noexcept { return read(dst.data(), dst.size()); }

		/** Seeks data buffer to the specified read position. */
		constexpr std::uint64_t setpos(std::uint64_t pos) noexcept { return m_pos = pos; }

		constexpr void swap(asset_buffer &other) noexcept
		{
			std::swap(m_owned, other.m_owned);
			std::swap(m_data, other.m_data);
			std::swap(m_size, other.m_size);
			std::swap(m_pos, other.m_pos);
		}
		friend constexpr void swap(asset_buffer &a, asset_buffer &b) noexcept { a.swap(b); }

	private:
		std::byte *m_owned = nullptr;
		const std::byte *m_data = nullptr;
		std::uint64_t m_size = 0;
		std::uint64_t m_pos = 0;
	};

	namespace detail
	{
		/* Asset IO backend. IO backend is separate from package implementation in order to allow
		 * different package types to use the same asset IO backend. */
		class asset_io_data
		{
			struct vtable_t
			{
				expected<std::size_t, std::error_code> (*read)(asset_io_data *, void *, std::size_t) noexcept;
				expected<std::uint64_t, std::error_code> (*seek)(asset_io_data *, std::int64_t, seek_basis) noexcept;
				expected<std::uint64_t, std::error_code> (*setpos)(asset_io_data *, std::uint64_t) noexcept;
				expected<std::uint64_t, std::error_code> (*size)(const asset_io_data *) noexcept;
				expected<std::uint64_t, std::error_code> (*tell)(const asset_io_data *) noexcept;

				void (*destroy_data)(asset_io_data *);
			};

			static expected<std::size_t, std::error_code> file_read(asset_io_data *data, void *dst, std::size_t n) noexcept
			{
				return data->m_file.read(std::nothrow, dst, n);
			}
			static expected<std::size_t, std::error_code> buff_read(asset_io_data *data, void *dst, std::size_t n) noexcept
			{
				return data->m_buff.read(dst, n);
			}

			// clang-format off
			static expected<std::uint64_t, std::error_code> file_seek(asset_io_data *data, std::int64_t off, seek_basis dir) noexcept
			{
				return data->m_file.seek(std::nothrow, off, dir);
			}
			static expected<std::uint64_t, std::error_code> buff_seek(asset_io_data *data, std::int64_t off, seek_basis dir) noexcept
			{
				switch (dir)
				{
				case seek_set: return data->m_buff.setpos(static_cast<std::uint64_t>(off));
				case seek_cur: return data->m_buff.setpos(data->m_buff.tell() + static_cast<std::uint64_t>(off));
				case seek_end: return data->m_buff.setpos(data->m_buff.size() + static_cast<std::uint64_t>(off));
				}
				return unexpected{std::make_error_code(std::errc::invalid_argument)};
			}
			// clang-format on

			static expected<std::uint64_t, std::error_code> file_setpos(asset_io_data *data, std::uint64_t pos) noexcept
			{
				return data->m_file.setpos(std::nothrow, pos);
			}
			static expected<std::uint64_t, std::error_code> buff_setpos(asset_io_data *data, std::uint64_t pos) noexcept
			{
				return data->m_buff.setpos(pos);
			}

			static expected<std::uint64_t, std::error_code> file_size(const asset_io_data *data) noexcept
			{
				return data->m_file.size(std::nothrow);
			}
			static expected<std::uint64_t, std::error_code> buff_size(const asset_io_data *data) noexcept
			{
				return data->m_buff.size();
			}
			static expected<std::uint64_t, std::error_code> file_tell(const asset_io_data *data) noexcept
			{
				return data->m_file.tell(std::nothrow);
			}
			static expected<std::uint64_t, std::error_code> buff_tell(const asset_io_data *data) noexcept
			{
				return data->m_buff.tell();
			}

			static void destroy_file(asset_io_data *data) { std::destroy_at(&data->m_file); }
			static void destroy_buff(asset_io_data *data) { std::destroy_at(&data->m_buff); }

			static SEK_API_IMPORT const vtable_t file_vtable;
			static SEK_API_IMPORT const vtable_t buff_vtable;

		public:
			asset_io_data(const asset_io_data &) = delete;
			asset_io_data &operator=(const asset_io_data &) = delete;

			constexpr asset_io_data() noexcept : m_padding() {}
			constexpr asset_io_data(asset_io_data &&other) noexcept : m_padding() { swap(other); }
			constexpr asset_io_data &operator=(asset_io_data &&other) noexcept
			{
				swap(other);
				return *this;
			}

			explicit asset_io_data(native_file &&file) noexcept
				: m_vtable(&file_vtable), m_file(std::move(file))
			{
			}
			explicit asset_io_data(asset_buffer &&buff) noexcept : m_vtable(&buff_vtable), m_buff(std::move(buff)) {}

			~asset_io_data()
			{
				if (m_vtable) m_vtable->destroy_data(this);
			}

			constexpr native_file &init_file()
			{
				m_vtable = &file_vtable;
				return *std::construct_at(&m_file);
			}
			constexpr native_file &init_file(native_file &&file)
			{
				m_vtable = &file_vtable;
				return *std::construct_at(&m_file, std::forward<native_file>(file));
			}
			constexpr asset_buffer &init_buff(std::size_t n)
			{
				m_vtable = &buff_vtable;
				return *std::construct_at(&m_buff, n);
			}
			constexpr asset_buffer &init_buff(asset_buffer &&buff)
			{
				m_vtable = &buff_vtable;
				return *std::construct_at(&m_buff, std::forward<asset_buffer>(buff));
			}
			constexpr asset_buffer &init_buff(const void *data, std::size_t n)
			{
				m_vtable = &buff_vtable;
				return *std::construct_at(&m_buff, data, n);
			}

			[[nodiscard]] constexpr bool empty() const noexcept { return m_vtable == nullptr; }

			expected<std::size_t, std::error_code> read(void *dst, std::size_t n) noexcept
			{
				return m_vtable->read(this, dst, n);
			}
			expected<std::uint64_t, std::error_code> seek(std::int64_t off, seek_basis dir) noexcept
			{
				return m_vtable->seek(this, off, dir);
			}
			expected<std::uint64_t, std::error_code> setpos(std::uint64_t pos) noexcept
			{
				return m_vtable->setpos(this, pos);
			}
			expected<std::uint64_t, std::error_code> size() const noexcept { return m_vtable->size(this); }
			expected<std::uint64_t, std::error_code> tell() const noexcept { return m_vtable->tell(this); }

			[[nodiscard]] constexpr native_file *file() noexcept
			{
				return m_vtable == &file_vtable ? &m_file : nullptr;
			}
			[[nodiscard]] constexpr const native_file *file() const noexcept
			{
				return m_vtable == &file_vtable ? &m_file : nullptr;
			}

			constexpr void swap(asset_io_data &other) noexcept
			{
				std::swap(m_vtable, other.m_vtable);
				std::swap(m_padding, other.m_padding);
			}

		private:
			const vtable_t *m_vtable = nullptr;
			union
			{
				std::byte m_padding[std::max(sizeof(native_file), sizeof(asset_buffer))] = {};

				native_file m_file;
				asset_buffer m_buff;
			};
		};
	}	 // namespace detail
}	 // namespace sek