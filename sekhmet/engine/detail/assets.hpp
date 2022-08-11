/*
 * Created by switchblade on 2022-04-04
 */

#pragma once

#include <atomic>
#include <filesystem>
#include <memory>
#include <utility>

#include "sekhmet/detail/access_guard.hpp"
#include "sekhmet/detail/basic_pool.hpp"
#include "sekhmet/detail/dense_map.hpp"
#include "sekhmet/detail/dense_set.hpp"
#include "sekhmet/detail/event.hpp"
#include "sekhmet/detail/expected.hpp"
#include "sekhmet/detail/intern.hpp"
#include "sekhmet/detail/service.hpp"
#include "sekhmet/detail/uuid.hpp"
#include "sekhmet/system/native_file.hpp"

#include <shared_mutex>

namespace sek::engine
{
	class asset_source;
	class asset_handle;
	class asset_package;
	class asset_database;

	/** @brief Exception thrown by the asset system on runtime errors. */
	class SEK_API asset_error : public std::runtime_error
	{
	public:
		asset_error() : std::runtime_error("Unknown asset error") {}
		explicit asset_error(std::string &&msg) : std::runtime_error(std::move(msg)) {}
		explicit asset_error(const std::string &msg) : std::runtime_error(msg) {}
		explicit asset_error(const char *msg) : std::runtime_error(msg) {}
		~asset_error() override;
	};

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
		[[nodiscard]] constexpr std::size_t read(asio::mutable_buffer &dst) noexcept
		{
			return read(dst.data(), dst.size());
		}

		/** Seeks data buffer to the specified read position. */
		[[nodiscard]] constexpr std::uint64_t setpos(std::uint64_t pos) noexcept { return m_pos = pos; }

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
		class package_info;
		class asset_io_data;

		/* Asset IO backend. IO backend is separate from package implementation in order to allow
		 * different package types to use the same asset IO backend. */
		struct asset_io_vtable
		{
			expected<std::size_t, std::error_code> (*read)(asset_io_data *, void *, std::size_t) noexcept;
			expected<std::uint64_t, std::error_code> (*seek)(asset_io_data *, std::int64_t, system::seek_basis) noexcept;
			expected<std::uint64_t, std::error_code> (*setpos)(asset_io_data *, std::uint64_t) noexcept;

			expected<std::uint64_t, std::error_code> (*size)(const asset_io_data *) noexcept;
			expected<std::uint64_t, std::error_code> (*tell)(const asset_io_data *) noexcept;

			void (*destroy_data)(asset_io_data *);
		};
		class asset_io_data
		{
			static expected<std::size_t, std::error_code> file_read(asset_io_data *data, void *dst, std::size_t n) noexcept
			{
				return data->m_file.read(std::nothrow, dst, n);
			}
			static expected<std::size_t, std::error_code> buff_read(asset_io_data *data, void *dst, std::size_t n) noexcept
			{
				return data->m_buff.read(dst, n);
			}

			// clang-format off
			static expected<std::uint64_t, std::error_code> file_seek(asset_io_data *data, std::int64_t off, system::seek_basis dir) noexcept
			{
				return data->m_file.seek(std::nothrow, off, dir);
			}
			static expected<std::uint64_t, std::error_code> buff_seek(asset_io_data *data, std::int64_t off, system::seek_basis dir) noexcept
			{
				switch (dir)
				{
				case system::seek_set: return data->m_buff.setpos(static_cast<std::uint64_t>(off));
				case system::seek_cur: return data->m_buff.setpos(data->m_buff.tell() + static_cast<std::uint64_t>(off));
				case system::seek_end: return data->m_buff.setpos(data->m_buff.size() + static_cast<std::uint64_t>(off));
				default: return unexpected{std::make_error_code(std::errc::invalid_argument)};
				}
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

			static SEK_API_IMPORT const asset_io_vtable file_vtable;
			static SEK_API_IMPORT const asset_io_vtable buff_vtable;

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

			explicit asset_io_data(system::native_file &&file) noexcept
				: m_vtable(&file_vtable), m_file(std::move(file))
			{
			}
			explicit asset_io_data(asset_buffer &&buff) noexcept : m_vtable(&buff_vtable), m_buff(std::move(buff)) {}

			~asset_io_data()
			{
				if (m_vtable) m_vtable->destroy_data(this);
			}

			constexpr system::native_file &init_file()
			{
				m_vtable = &file_vtable;
				return *std::construct_at(&m_file);
			}
			constexpr asset_buffer &init_buff(std::size_t n)
			{
				m_vtable = &buff_vtable;
				return *std::construct_at(&m_buff, n);
			}
			constexpr asset_buffer &init_buff(const void *data, std::size_t n)
			{
				m_vtable = &buff_vtable;
				return *std::construct_at(&m_buff, data, n);
			}

			[[nodiscard]] constexpr bool empty() const noexcept { return m_vtable == nullptr; }
			[[nodiscard]] constexpr bool has_file() const noexcept { return m_vtable == &file_vtable; }
			[[nodiscard]] constexpr bool has_buffer() const noexcept { return m_vtable == &buff_vtable; }

			[[nodiscard]] constexpr system::native_file &file() noexcept { return m_file; }
			[[nodiscard]] constexpr const system::native_file &file() const noexcept { return m_file; }
			[[nodiscard]] constexpr asset_buffer &buffer() noexcept { return m_buff; }
			[[nodiscard]] constexpr const asset_buffer &buffer() const noexcept { return m_buff; }

			expected<std::size_t, std::error_code> read(void *dst, std::size_t n) noexcept
			{
				return m_vtable->read(this, dst, n);
			}

			expected<std::uint64_t, std::error_code> size() const noexcept { return m_vtable->size(this); }
			expected<std::uint64_t, std::error_code> tell() const noexcept { return m_vtable->tell(this); }

			expected<std::uint64_t, std::error_code> seek(std::int64_t off, system::seek_basis dir) noexcept
			{
				return m_vtable->seek(this, off, dir);
			}
			expected<std::uint64_t, std::error_code> setpos(std::uint64_t pos) noexcept
			{
				return m_vtable->setpos(this, pos);
			}

			constexpr void swap(asset_io_data &other) noexcept
			{
				std::swap(m_vtable, other.m_vtable);
				std::swap(m_padding, other.m_padding);
			}

		private:
			const asset_io_vtable *m_vtable = nullptr;
			union
			{
				std::byte m_padding[sizeof(system::native_file)] = {};
				system::native_file m_file;
				asset_buffer m_buff;
			};
		};
	}	 // namespace detail

	/** @brief Structure providing a read-only access to data of an asset.
	 *
	 * Since assets may be either loose or compressed and archived, a special structure is needed to read asset data.
	 * In addition, to allow for implementation of storage optimization techniques (such as DirectStorage),
	 * streams cannot be used directly either, as access to the underlying file or data buffer is needed. */
	class asset_source
	{
		friend class detail::package_info;

	public:
		typedef typename system::native_file::seek_basis seek_basis;

		constexpr static seek_basis seek_set = system::native_file::seek_set;
		constexpr static seek_basis seek_cur = system::native_file::seek_cur;
		constexpr static seek_basis seek_end = system::native_file::seek_end;

	private:
		constexpr asset_source(detail::asset_io_data &&data, std::uint64_t offset, std::uint64_t size) noexcept
			: m_data(std::move(data)), m_offset(offset), m_size(size)
		{
		}

	public:
		asset_source(const asset_source &) = delete;
		asset_source &operator=(const asset_source &) = delete;

		/** Initializes asset source from a native file.
		 * @param file File containing the asset.
		 * @throw std::system_error On implementation-defined file errors. */
		explicit asset_source(system::native_file &&file) : m_data(detail::asset_io_data(std::move(file)))
		{
			m_offset = m_data.file().tell();
			m_size = m_data.file().size();
		}
		/** @copydoc asset_source
		 * @param offset Offset from the start of the file at which the asset's data starts.
		 * @note File will be seeked to the specified offset. */
		asset_source(system::native_file &&file, std::uint64_t offset)
			: m_data(detail::asset_io_data(std::move(file))), m_offset(offset)
		{
			m_size = m_data.file().size() - offset;
			m_data.file().setpos(offset);
		}
		/** @copydoc asset_source
		 * @param size Size of the asset. */
		asset_source(system::native_file &&file, std::uint64_t offset, std::uint64_t size)
			: m_data(detail::asset_io_data(std::move(file))), m_offset(offset), m_size(size)
		{
			m_data.file().setpos(offset);
		}

		/** Initializes asset source from an asset buffer.
		 * @param buff Buffer containing the asset. */
		explicit asset_source(asset_buffer &&buff) noexcept : m_data(detail::asset_io_data(std::move(buff)))
		{
			m_size = m_data.buffer().size();
		}
		/** @copydoc asset_source
		 * @param offset Offset from the start of the buffer at which the asset's data starts.
		 * @note Buffer will be seeked to the specified offset. */
		asset_source(asset_buffer &&buff, std::uint64_t offset) noexcept
			: m_data(detail::asset_io_data(std::move(buff))), m_offset(offset)
		{
			m_size = m_data.buffer().size() - offset;
			m_data.buffer().setpos(offset);
		}
		/** @copydoc asset_source
		 * @param size Size of the asset. */
		asset_source(asset_buffer &&buff, std::uint64_t offset, std::uint64_t size) noexcept
			: m_data(detail::asset_io_data(std::move(buff))), m_offset(offset), m_size(size)
		{
			m_data.buffer().setpos(offset);
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

		/** Checks if the asset source is backed by a file. */
		[[nodiscard]] constexpr bool has_file() const noexcept { return m_data.has_file(); }
		/** Checks if the asset source is backed by a buffer. */
		[[nodiscard]] constexpr bool has_buffer() const noexcept { return m_data.has_buffer(); }
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

		/** Returns reference to the underlying native file. */
		[[nodiscard]] constexpr const system::native_file &file() const noexcept { return m_data.file(); }
		/** Returns reference to the underlying asset buffer. */
		[[nodiscard]] constexpr const asset_buffer &buffer() const noexcept { return m_data.buffer(); }

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

	namespace detail
	{
		struct asset_info;
		struct asset_table
		{
			using uuid_table_t = dense_map<uuid, asset_info *>;
			using name_table_t = dense_map<std::string_view, uuid>;

			class entry_iterator;
			class entry_ptr;

			typedef asset_handle value_type;
			typedef entry_iterator iterator;
			typedef entry_iterator const_iterator;
			typedef std::reverse_iterator<const_iterator> reverse_iterator;
			typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
			typedef asset_handle reference;
			typedef asset_handle const_reference;
			typedef entry_ptr pointer;
			typedef entry_ptr const_pointer;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;

			[[nodiscard]] constexpr bool empty() const noexcept { return uuid_table.empty(); }
			[[nodiscard]] constexpr size_type size() const noexcept { return uuid_table.size(); }

			[[nodiscard]] constexpr auto begin() const noexcept;
			[[nodiscard]] constexpr auto end() const noexcept;
			[[nodiscard]] constexpr auto rbegin() const noexcept;
			[[nodiscard]] constexpr auto rend() const noexcept;

			[[nodiscard]] constexpr auto find(uuid) const;
			[[nodiscard]] constexpr auto find(std::string_view) const;
			[[nodiscard]] constexpr auto match(auto &&) const;
			[[nodiscard]] inline auto find_all(std::string_view) const;
			[[nodiscard]] inline auto match_all(auto &&) const;

			dense_map<uuid, asset_info *> uuid_table;
			dense_map<std::string_view, uuid> name_table;
		};

		class package_info : public asset_table
		{
		public:
			static asset_source make_source(asset_io_data &&, std::uint64_t, std::uint64_t) noexcept;

		public:
			constexpr package_info() = default;
			SEK_API virtual ~package_info();

			SEK_API void acquire();
			SEK_API void release();

			void insert(uuid id, asset_info *info);
			void erase(uuid id);

			[[nodiscard]] virtual asset_info *alloc_info() = 0;
			virtual void dealloc_info(asset_info *info) = 0;
			virtual void destroy_info(asset_info *info) = 0;
			void delete_info(asset_info *info)
			{
				destroy_info(info);
				dealloc_info(info);
			}

			virtual expected<asset_source, std::error_code> open_asset(const asset_info *) const noexcept = 0;
			virtual expected<asset_source, std::error_code> open_metadata(const asset_info *) const noexcept = 0;

			[[nodiscard]] virtual bool has_metadata(const asset_info *) const noexcept = 0;

		protected:
			void destroy_all();

		private:
			std::atomic<std::size_t> m_refs;

		public:
#ifdef SEK_EDITOR
			event<void(const asset_handle &)> asset_added;
			event<void(const asset_handle &)> asset_removed;
#endif
		};
		struct asset_info
		{
			asset_info() = delete;
			asset_info(const asset_info &) = delete;
			asset_info(asset_info &&) = delete;

			constexpr explicit asset_info(package_info *parent) : parent(parent) {}

			[[nodiscard]] bool has_metadata() const noexcept { return parent->has_metadata(this); }

			package_info *parent; /* Parent package of the asset. */

			interned_string name;			 /* Optional human-readable name of the asset. */
			dense_set<interned_string> tags; /* Optional tags of the asset. */
		};

		/* Package stored locally on the device. */
		class local_package
		{
		public:
			local_package() = delete;

			explicit local_package(const std::filesystem::path &path) : m_path(path) {}
			explicit local_package(std::filesystem::path &&path) : m_path(std::move(path)) {}

			[[nodiscard]] expected<system::native_file, std::error_code> open_archive(std::uint64_t offset) const;

			[[nodiscard]] constexpr const std::filesystem::path &path() const noexcept { return m_path; }

		private:
			std::filesystem::path m_path;
		};

		/* Package format implementations. */
		class loose_package final : public package_info, public local_package
		{
		protected:
			class loose_info : public asset_info
			{
			public:
				loose_info(package_info *p, std::string_view asset_path, std::string_view meta_path) : asset_info(p)
				{
					if (!asset_path.empty())
					{
						m_asset_path = static_cast<char *>(malloc((m_asset_path_size = asset_path.size()) + 1));
						if (m_asset_path == nullptr) [[unlikely]]
							throw std::bad_alloc();

						std::copy_n(asset_path.data(), asset_path.size(), m_asset_path);
					}
					if (!meta_path.empty())
					{
						m_meta_path = static_cast<char *>(malloc((m_meta_path_size = meta_path.size()) + 1));
						if (m_meta_path == nullptr) [[unlikely]]
							throw std::bad_alloc();

						std::copy_n(meta_path.data(), meta_path.size(), m_meta_path);
					}
				}
				~loose_info()
				{
					free(m_asset_path);
					free(m_meta_path);
				}

				[[nodiscard]] constexpr std::string_view asset_path() const noexcept
				{
					return {m_asset_path, m_asset_path_size};
				}
				[[nodiscard]] constexpr std::string_view meta_path() const noexcept
				{
					return {m_meta_path, m_meta_path_size};
				}

			private:
				/* Path to asset's main file. */
				char *m_asset_path = nullptr;
				std::size_t m_asset_path_size = 0;

				/* Path to asset's metadata file. */
				char *m_meta_path = nullptr;
				std::size_t m_meta_path_size = 0;
			};

		public:
			explicit loose_package(const std::filesystem::path &path) : local_package(path) {}
			explicit loose_package(std::filesystem::path &&path) : local_package(std::move(path)) {}

			~loose_package() final { destroy_all(); }

			[[nodiscard]] asset_info *alloc_info() final { return m_pool.allocate(); }
			void dealloc_info(asset_info *info) final { m_pool.deallocate(static_cast<loose_info *>(info)); }
			void destroy_info(asset_info *info) final { std::destroy_at(static_cast<loose_info *>(info)); }

			expected<asset_source, std::error_code> open_asset(const asset_info *) const noexcept final;
			expected<asset_source, std::error_code> open_metadata(const asset_info *) const noexcept final;

			[[nodiscard]] constexpr bool has_metadata(const asset_info *info) const noexcept final
			{
				return !static_cast<const loose_info *>(info)->meta_path().empty();
			}

		private:
			expected<asset_source, std::error_code> open_at(std::string_view) const noexcept;

			sek::detail::basic_pool<loose_info> m_pool;
		};
		class archive_package final : public package_info, public local_package
		{
		protected:
			struct archive_slice
			{
				std::uint64_t offset;
				std::uint64_t size;
			};
			struct archive_info : asset_info
			{
				archive_slice asset_slice;
				archive_slice meta_slice;
			};

		public:
			explicit archive_package(const std::filesystem::path &path) : local_package(path) {}
			explicit archive_package(std::filesystem::path &&path) : local_package(std::move(path)) {}

			~archive_package() final { destroy_all(); }

			[[nodiscard]] asset_info *alloc_info() final { return m_pool.allocate(); }
			void dealloc_info(asset_info *info) final { m_pool.deallocate(static_cast<archive_info *>(info)); }
			void destroy_info(asset_info *info) final { std::destroy_at(static_cast<archive_info *>(info)); }

			expected<asset_source, std::error_code> open_asset(const asset_info *) const noexcept final;
			expected<asset_source, std::error_code> open_metadata(const asset_info *) const noexcept final;

			[[nodiscard]] constexpr bool has_metadata(const asset_info *info) const noexcept final
			{
				return static_cast<const archive_info *>(info)->meta_slice.offset != 0;
			}

		private:
			expected<asset_source, std::error_code> open_at(archive_slice) const noexcept;

			sek::detail::basic_pool<archive_info> m_pool;
		};
		class zstd_package final : public package_info, public local_package
		{
		protected:
			struct archive_slice
			{
				std::uint64_t offset;
				std::uint64_t size;		/* Compressed size. */
				std::uint64_t src_size; /* Decompressed size. */
				std::uint32_t frames;	/* Amount of compressed frames used (0 if not compressed). */
			};
			struct zstd_info : asset_info
			{
				archive_slice asset_slice;
				archive_slice meta_slice;
			};

		public:
			explicit zstd_package(const std::filesystem::path &path) : local_package(path) {}
			explicit zstd_package(std::filesystem::path &&path) : local_package(std::move(path)) {}

			~zstd_package() final { destroy_all(); }

			[[nodiscard]] asset_info *alloc_info() final { return m_pool.allocate(); }
			void dealloc_info(asset_info *info) final { m_pool.deallocate(static_cast<zstd_info *>(info)); }
			void destroy_info(asset_info *info) final { std::destroy_at(static_cast<zstd_info *>(info)); }

			expected<asset_source, std::error_code> open_asset(const asset_info *) const noexcept final;
			expected<asset_source, std::error_code> open_metadata(const asset_info *) const noexcept final;

			[[nodiscard]] constexpr bool has_metadata(const asset_info *info) const noexcept final
			{
				return static_cast<const zstd_info *>(info)->meta_slice.offset != 0;
			}

		private:
			expected<asset_source, std::error_code> open_at(archive_slice) const noexcept;

			sek::detail::basic_pool<zstd_info> m_pool;
		};

		struct asset_info_ptr
		{
			constexpr asset_info_ptr() noexcept = default;
			~asset_info_ptr() { release(); }

			asset_info_ptr(const asset_info_ptr &other) noexcept : info(other.info) { acquire(); }
			asset_info_ptr &operator=(const asset_info_ptr &other) noexcept
			{
				if (this != &other) reset(other.info);
				return *this;
			}

			constexpr asset_info_ptr(asset_info_ptr &&other) noexcept { swap(other); }
			constexpr asset_info_ptr &operator=(asset_info_ptr &&other) noexcept
			{
				swap(other);
				return *this;
			}

			constexpr explicit asset_info_ptr(asset_info *info) noexcept : info(info) {}

			[[nodiscard]] constexpr bool empty() const noexcept { return info == nullptr; }
			[[nodiscard]] constexpr asset_info *operator->() const noexcept { return info; }

			void acquire() const
			{
				if (info != nullptr) [[likely]]
					info->parent->acquire();
			}
			void release() const
			{
				if (info != nullptr) [[likely]]
					info->parent->release();
			}
			void reset(asset_info *new_info)
			{
				release();
				info = new_info;
				acquire();
			}
			void reset()
			{
				release();
				info = nullptr;
			}

			[[nodiscard]] constexpr bool operator==(const asset_info_ptr &other) const noexcept
			{
				return info == other.info;
			}

			constexpr void swap(asset_info_ptr &other) noexcept { std::swap(info, other.info); }

			asset_info *info = nullptr;
		};
		struct package_info_ptr
		{
			constexpr package_info_ptr() noexcept = default;
			~package_info_ptr() { release(); }

			package_info_ptr(const package_info_ptr &other) noexcept : pkg(other.pkg) { acquire(); }
			package_info_ptr &operator=(const package_info_ptr &other) noexcept
			{
				if (this != &other) reset(other.pkg);
				return *this;
			}

			constexpr package_info_ptr(package_info_ptr &&other) noexcept { swap(other); }
			constexpr package_info_ptr &operator=(package_info_ptr &&other) noexcept
			{
				swap(other);
				return *this;
			}

			constexpr explicit package_info_ptr(package_info *pkg) noexcept : pkg(pkg) {}

			[[nodiscard]] constexpr bool empty() const noexcept { return pkg == nullptr; }
			[[nodiscard]] constexpr package_info *operator->() const noexcept { return pkg; }

			void acquire() const
			{
				if (pkg != nullptr) [[likely]]
					pkg->acquire();
			}
			void release() const
			{
				if (pkg != nullptr) [[likely]]
					pkg->release();
			}
			void reset(package_info *new_pkg)
			{
				release();
				pkg = new_pkg;
				acquire();
			}
			void reset()
			{
				release();
				pkg = nullptr;
			}

			[[nodiscard]] constexpr bool operator==(const package_info_ptr &other) const noexcept
			{
				return pkg == other.pkg;
			}

			constexpr void swap(package_info_ptr &other) noexcept { std::swap(pkg, other.pkg); }

			package_info *pkg = nullptr;
		};
	}	 // namespace detail

	/** @brief Handle to a unique asset of a package.
	 * @note Asset packages are kept alive as long as any of their assets are referenced. */
	class asset_handle
	{
		friend class detail::asset_table::entry_iterator;
		friend class asset_package;
		friend class asset_database;

		constexpr asset_handle(uuid id, detail::asset_info_ptr &&ptr) noexcept
			: m_id(std::move(id)), m_ptr(std::move(ptr))
		{
		}
		asset_handle(uuid id, detail::asset_info *info) : m_id(std::move(id)), m_ptr(info) { m_ptr.acquire(); }

	public:
		/** Initializes an empty asset handle. */
		constexpr asset_handle() noexcept = default;

		constexpr asset_handle(asset_handle &&) noexcept = default;
		constexpr asset_handle &operator=(asset_handle &&other) noexcept
		{
			m_id = std::move(other.m_id);
			m_ptr = std::move(other.m_ptr);
			return *this;
		}
		asset_handle(const asset_handle &) = default;
		asset_handle &operator=(const asset_handle &) = default;

		/** Checks if the asset handle references an asset. */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_ptr.empty(); }
		/** @copydoc empty */
		[[nodiscard]] constexpr operator bool() const noexcept { return !empty(); }

		/** Returns the id of the asset. If the asset handle does not point to an asset, returns a nil uuid. */
		[[nodiscard]] constexpr uuid id() const noexcept { return m_id; }
		/** Returns reference to the name of the asset. */
		[[nodiscard]] constexpr const interned_string &name() const noexcept { return m_ptr->name; }
		/** Returns a set of string tags of the asset. */
		[[nodiscard]] constexpr const dense_set<interned_string> &tags() const noexcept { return m_ptr->tags; }

		/** Returns a handle to the parent package of the asset. */
		[[nodiscard]] inline asset_package package() const;

		/** @brief Opens an asset source used to read asset's data.
		 * @return `asset_source` containing asset's data.
		 * @throw std::system_error On failure to open the file or archive containing the asset. */
		[[nodiscard]] SEK_API asset_source open() const;
		/** @copybrief open
		 * @return `asset_source` containing asset's data or an error code. */
		[[nodiscard]] SEK_API expected<asset_source, std::error_code> open(std::nothrow_t) const noexcept;

		/** Checks if the asset has metadata. */
		[[nodiscard]] bool has_metadata() const { return m_ptr->has_metadata(); }

		/** @brief Opens an asset source used to read asset's metadata.
		 * @return `asset_source` containing asset's metadata.
		 * @throw std::system_error On failure to open the file or archive containing the asset. */
		[[nodiscard]] SEK_API asset_source metadata() const;
		/** @copybrief open
		 * @return `asset_source` containing asset's metadata or an error code. */
		[[nodiscard]] SEK_API expected<asset_source, std::error_code> metadata(std::nothrow_t) const noexcept;

		constexpr void swap(asset_handle &other) noexcept
		{
			m_id.swap(other.m_id);
			m_ptr.swap(other.m_ptr);
		}
		friend constexpr void swap(asset_handle &a, asset_handle &b) noexcept { a.swap(b); }

		/** Returns true if both asset handles reference the *exact* same asset.
		 * @note Multiple asset handles with the same id may reference different assets.
		 * This may happen if the assets were obtained directly from packages (bypassing the database),
		 * thus no overrides could be resolved. */
		[[nodiscard]] constexpr bool operator==(const asset_handle &) const noexcept = default;

	private:
		uuid m_id = uuid::nil();
		detail::asset_info_ptr m_ptr;
	};

	namespace detail
	{
		class asset_table::entry_ptr
		{
			friend class entry_iterator;

			constexpr explicit entry_ptr(asset_handle &&ref) noexcept : m_ref(std::move(ref)) {}

		public:
			entry_ptr() = delete;
			entry_ptr(const entry_ptr &) = delete;
			entry_ptr &operator=(const entry_ptr &) = delete;

			constexpr entry_ptr(entry_ptr &&other) noexcept : m_ref(std::move(other.m_ref)) {}
			constexpr entry_ptr &operator=(entry_ptr &&other) noexcept
			{
				m_ref = std::move(other.m_ref);
				return *this;
			}

			[[nodiscard]] constexpr const asset_handle *get() const noexcept { return std::addressof(m_ref); }
			[[nodiscard]] constexpr const asset_handle *operator->() const noexcept { return get(); }
			[[nodiscard]] constexpr const asset_handle &operator*() const noexcept { return *get(); }

			constexpr void swap(entry_ptr &other) noexcept { m_ref.swap(other.m_ref); }
			friend constexpr void swap(entry_ptr &a, entry_ptr &b) noexcept { a.swap(b); }

			[[nodiscard]] constexpr bool operator==(const entry_ptr &) const noexcept = default;

		private:
			asset_handle m_ref;
		};
		class asset_table::entry_iterator
		{
			friend struct asset_table;

			using uuid_iter = typename uuid_table_t::const_iterator;

		public:
			typedef asset_handle value_type;
			typedef asset_handle reference;
			typedef entry_ptr pointer;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;
			typedef std::bidirectional_iterator_tag iterator_category;

		private:
			constexpr entry_iterator(uuid_iter iter) noexcept : m_iter(iter) {}

		public:
			constexpr entry_iterator() noexcept = default;
			constexpr entry_iterator(const entry_iterator &) noexcept = default;
			constexpr entry_iterator(entry_iterator &&) noexcept = default;
			constexpr entry_iterator &operator=(const entry_iterator &) noexcept = default;
			constexpr entry_iterator &operator=(entry_iterator &&) noexcept = default;

			constexpr entry_iterator &operator++() noexcept
			{
				m_iter++;
				return *this;
			}
			entry_iterator operator++(int) noexcept
			{
				auto temp = *this;
				m_iter++;
				return temp;
			}
			constexpr entry_iterator &operator--() noexcept
			{
				m_iter--;
				return *this;
			}
			entry_iterator operator--(int) noexcept
			{
				auto temp = *this;
				m_iter--;
				return temp;
			}

			[[nodiscard]] reference operator*() const { return reference{m_iter->first, m_iter->second}; }
			[[nodiscard]] pointer operator->() const { return pointer{operator*()}; }

			[[nodiscard]] constexpr bool operator==(const entry_iterator &) const noexcept = default;

			constexpr void swap(entry_iterator &other) noexcept { m_iter.swap(other.m_iter); }
			friend constexpr void swap(entry_iterator &a, entry_iterator &b) noexcept { a.swap(b); }

		private:
			uuid_iter m_iter;
		};

		constexpr auto asset_table::begin() const noexcept { return const_iterator{uuid_table.begin()}; }
		constexpr auto asset_table::end() const noexcept { return const_iterator{uuid_table.end()}; }
		constexpr auto asset_table::rbegin() const noexcept { return const_reverse_iterator{end()}; }
		constexpr auto asset_table::rend() const noexcept { return const_reverse_iterator{begin()}; }

		constexpr auto asset_table::find(uuid id) const { return const_iterator{uuid_table.find(id)}; }
		constexpr auto asset_table::find(std::string_view name) const
		{
			if (auto name_iter = name_table.find(name); name_iter != name_table.end()) [[likely]]
				return find(name_iter->second);
			else
				return end();
		}
		constexpr auto asset_table::match(auto &&pred) const { return std::find_if(begin(), end(), pred); }
		inline auto asset_table::find_all(std::string_view name) const
		{
			std::vector<reference> result;
			std::for_each(begin(),
						  end(),
						  [&result, &name](auto entry)
						  {
							  if (entry.name() == name) result.push_back(std::move(entry));
						  });
			return result;
		}
		inline auto asset_table::match_all(auto &&pred) const
		{
			std::vector<reference> result;
			std::for_each(begin(),
						  end(),
						  [&result, &pred](auto entry)
						  {
							  if (pred(entry)) result.push_back(std::move(entry));
						  });
			return result;
		}
	}	 // namespace detail

	/** @brief Reference-counted handle used to reference an asset package. */
	class asset_package
	{
		friend class asset_handle;
		friend class asset_database;

		using table_t = detail::asset_table;

	public:
		typedef typename table_t::value_type value_type;
		typedef typename table_t::iterator iterator;
		typedef typename table_t::const_iterator const_iterator;
		typedef typename table_t::reverse_iterator reverse_iterator;
		typedef typename table_t::const_reverse_iterator const_reverse_iterator;
		typedef typename table_t::pointer pointer;
		typedef typename table_t::const_pointer const_pointer;
		typedef typename table_t::reference reference;
		typedef typename table_t::const_reference const_reference;
		typedef typename table_t::size_type size_type;
		typedef typename table_t::difference_type difference_type;

		/** Loads a package at the specified path.
		 * @throw asset_error If the path does not contain a valid package or
		 * an implementation-defined error occurred during loading of package metadata. */
		[[nodiscard]] static SEK_API asset_package load(const std::filesystem::path &path);
		/** Load all packages in the specified directory.
		 * @throw asset_error If the path is not a valid directory. */
		[[nodiscard]] static SEK_API std::vector<asset_package> load_all(const std::filesystem::path &path);

	private:
		constexpr explicit asset_package(detail::package_info_ptr &&ptr) noexcept : m_ptr(std::move(ptr)) {}
		SEK_API explicit asset_package(detail::package_info *pkg);

	public:
		asset_package() = delete;

		constexpr asset_package(asset_package &&) noexcept = default;
		constexpr asset_package &operator=(asset_package &&other) noexcept
		{
			m_ptr = std::move(other.m_ptr);
			return *this;
		}
		asset_package(const asset_package &) = default;
		asset_package &operator=(const asset_package &) = default;

		/** Returns path of the asset package. */
		[[nodiscard]] constexpr const std::filesystem::path &path() const noexcept { return m_ptr->path; }

		/** Checks if the asset package is empty (does not contain any assets). */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_ptr->empty(); }
		/** Returns the number of assets contained within the package. */
		[[nodiscard]] constexpr auto size() const noexcept { return m_ptr->size(); }

		/** Returns iterator to the first asset of the package. */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return m_ptr->begin(); }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
		/** Returns iterator one past the last asset of the package. */
		[[nodiscard]] constexpr const_iterator end() const noexcept { return m_ptr->end(); }
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }
		/** Returns reverse iterator to the last asset of the package. */
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return m_ptr->rbegin(); }
		/** Returns reverse iterator to the first asset of the package. */
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return m_ptr->rend(); }

		/** Returns iterator to the asset with a given id. */
		[[nodiscard]] constexpr const_iterator find(uuid id) const { return m_ptr->find(id); }
		/** Returns iterator to the asset with a given name. */
		[[nodiscard]] constexpr const_iterator find(std::string_view name) const { return m_ptr->find(name); }
		/** Returns a vector of all assets with the specified name. */
		[[nodiscard]] std::vector<reference> find_all(std::string_view name) const { return m_ptr->find_all(name); }
		/** Checks if the package contains an asset with a given id. */
		[[nodiscard]] constexpr bool constins(uuid id) const { return find(id) != end(); }
		/** Checks if the package contains an asset with a given name. */
		[[nodiscard]] constexpr bool constins(std::string_view name) const { return find(name) != end(); }

		/** Returns iterator to the first asset that matches the predicate. */
		template<typename P>
		[[nodiscard]] constexpr const_iterator match(P &&pred) const
		{
			return m_ptr->match(std::forward<P>(pred));
		}
		/** Returns a vector of all assets that match the predicate. */
		template<typename P>
		[[nodiscard]] std::vector<reference> match_all(P &&pred) const
		{
			return m_ptr->match_all(std::forward<P>(pred));
		}

#ifdef SEK_EDITOR
		/** Returns event proxy for asset removal event. */
		[[nodiscard]] event_proxy<event<void(const asset_handle &)>> on_asset_removed() const
		{
			return m_ptr->asset_removed;
		}
		/** Returns event proxy for asset creation event. */
		[[nodiscard]] event_proxy<event<void(const asset_handle &)>> on_asset_added() const
		{
			return m_ptr->asset_added;
		}
#endif

		constexpr void swap(asset_package &other) noexcept { m_ptr.swap(other.m_ptr); }
		friend constexpr void swap(asset_package &a, asset_package &b) noexcept { a.swap(b); }

		[[nodiscard]] constexpr bool operator==(const asset_package &) const noexcept = default;

	private:
		detail::package_info_ptr m_ptr;
	};

	inline asset_package asset_handle::package() const { return asset_package{m_ptr->parent}; }

	/** @brief Proxy range used to manipulate load order of asset database's packages.
	 *
	 * @note Any modifications to the load order of the packages will trigger an update of the
	 * parent database's asset tables.
	 * @warning Proxy may not outlive parent database. */
	template<bool Mutable>
	class package_proxy;

	/** @brief Service used to manage global database of assets and asset packages. */
	class asset_database : public service<shared_guard<asset_database>>
	{
		template<bool>
		friend class package_proxy;

		friend shared_guard<asset_database>;

	protected:
		using packages_t = std::vector<asset_package>;
		using assets_t = detail::asset_table;

	public:
		typedef typename assets_t::value_type value_type;
		typedef typename assets_t::iterator iterator;
		typedef typename assets_t::const_iterator const_iterator;
		typedef typename assets_t::reverse_iterator reverse_iterator;
		typedef typename assets_t::const_reverse_iterator const_reverse_iterator;
		typedef typename assets_t::pointer pointer;
		typedef typename assets_t::const_pointer const_pointer;
		typedef typename assets_t::reference reference;
		typedef typename assets_t::const_reference const_reference;
		typedef typename assets_t::size_type size_type;
		typedef typename assets_t::difference_type difference_type;

	protected:
		constexpr asset_database() = default;

	public:
		/** Checks if the asset database is empty (does not contain any assets). */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_assets.empty(); }
		/** Returns the number of assets contained within the database. */
		[[nodiscard]] constexpr auto size() const noexcept { return m_assets.size(); }

		/** Returns iterator to the first asset of the database. */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return m_assets.begin(); }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
		/** Returns iterator one past the last asset of the database. */
		[[nodiscard]] constexpr const_iterator end() const noexcept { return m_assets.end(); }
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }
		/** Returns reverse iterator to the last asset of the database. */
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return m_assets.rbegin(); }
		/** Returns reverse iterator to the first asset of the database. */
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return m_assets.rend(); }

		/** Returns iterator to the asset with a given id. */
		[[nodiscard]] constexpr const_iterator find(uuid id) const { return m_assets.find(id); }
		/** Returns iterator to the asset with a given name. */
		[[nodiscard]] constexpr const_iterator find(std::string_view name) const { return m_assets.find(name); }
		/** Returns a vector of all assets with the specified name. */
		[[nodiscard]] std::vector<reference> find_all(std::string_view name) const { return m_assets.find_all(name); }
		/** Checks if the database contains an asset with a given id. */
		[[nodiscard]] constexpr bool constins(uuid id) const { return find(id) != end(); }
		/** Checks if the database contains an asset with a given name. */
		[[nodiscard]] constexpr bool constins(std::string_view name) const { return find(name) != end(); }

		/** Returns iterator to the first asset that matches the predicate. */
		template<typename P>
		[[nodiscard]] constexpr const_iterator match(P &&pred) const
		{
			return m_assets.match(std::forward<P>(pred));
		}
		/** Returns a vector of all assets that match the predicate. */
		template<typename P>
		[[nodiscard]] std::vector<reference> match_all(P &&pred) const
		{
			return m_assets.match_all(std::forward<P>(pred));
		}

		/** Clears the contents of the asset database by removing all assets & packages. */
		SEK_API void clear();

		/** Returns a proxy range used to manipulate the load order of packages. */
		[[nodiscard]] constexpr auto packages() noexcept;
		/** @copydoc packages */
		[[nodiscard]] constexpr auto packages() const noexcept;

	protected:
		SEK_API void restore_asset(typename packages_t::const_iterator, uuid, const detail::asset_info *);
		SEK_API void override_asset(typename packages_t::const_iterator, uuid, detail::asset_info *);

		typename packages_t::const_iterator insert_impl(typename packages_t::iterator);
		SEK_API typename packages_t::const_iterator insert(typename packages_t::const_iterator, const asset_package &);
		SEK_API typename packages_t::const_iterator insert(typename packages_t::const_iterator, asset_package &&);

		typename packages_t::const_iterator erase_impl(typename packages_t::iterator);
		SEK_API typename packages_t::const_iterator erase(typename packages_t::const_iterator,
														  typename packages_t::const_iterator);
		SEK_API typename packages_t::const_iterator erase(typename packages_t::const_iterator);

		SEK_API void swap(typename packages_t::const_iterator, typename packages_t::const_iterator);

#ifdef SEK_ENGINE
		void handle_asset_removed(const asset_handle &);
		void handle_asset_added(const asset_handle &);
#endif

		packages_t m_packages;
		assets_t m_assets;
	};

	template<>
	class package_proxy<false>
	{
		friend class asset_database;

	protected:
		using packages_t = typename asset_database::packages_t;

	public:
		typedef typename packages_t::value_type value_type;
		typedef typename packages_t::const_pointer pointer;
		typedef typename packages_t::const_pointer const_pointer;
		typedef typename packages_t::const_reference reference;
		typedef typename packages_t::const_reference const_reference;
		typedef typename packages_t::const_iterator iterator;
		typedef typename packages_t::const_iterator const_iterator;
		typedef typename packages_t::const_reverse_iterator reverse_iterator;
		typedef typename packages_t::const_reverse_iterator const_reverse_iterator;
		typedef typename packages_t::size_type size_type;
		typedef typename packages_t::difference_type difference_type;

	protected:
		constexpr explicit package_proxy(const asset_database &parent) noexcept : m_parent(&parent) {}

	public:
		package_proxy() = delete;

		constexpr package_proxy(const package_proxy &) noexcept = default;
		constexpr package_proxy(package_proxy &&) noexcept = default;
		constexpr package_proxy &operator=(const package_proxy &) noexcept = default;
		constexpr package_proxy &operator=(package_proxy &&) noexcept = default;

		constexpr package_proxy(const package_proxy<true> &other) noexcept;
		constexpr package_proxy(package_proxy<true> &&other) noexcept;
		constexpr package_proxy &operator=(const package_proxy<true> &other) noexcept;
		constexpr package_proxy &operator=(package_proxy<true> &&other) noexcept;

		/** Returns iterator to the first package. */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return packages().cbegin(); }
		/** @copydoc cbegin */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return cbegin(); }
		/** Returns iterator one past the last package. */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return packages().cend(); }
		/** @copydoc cend */
		[[nodiscard]] constexpr const_iterator end() const noexcept { return cend(); }
		/** Returns reverse iterator one past the last package. */
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return packages().crbegin(); }
		/** @copydoc crbegin */
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return crbegin(); }
		/** Returns reverse iterator to the fist package. */
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return packages().crend(); }
		/** @copydoc crbegin */
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return crend(); }

		/** Returns reference to the first package. */
		[[nodiscard]] constexpr const_reference front() const noexcept { return packages().front(); }
		/** Returns reference to the last package. */
		[[nodiscard]] constexpr const_reference back() const noexcept { return packages().back(); }
		/** Returns reference to the package at the specified index. */
		[[nodiscard]] constexpr const_reference at(size_type i) const noexcept { return packages().at(i); }
		/** @copydoc at */
		[[nodiscard]] constexpr const_reference operator[](size_type i) const noexcept { return at(i); }

		/** Returns the current amount of loaded packages. */
		[[nodiscard]] constexpr size_type size() const noexcept { return packages().size(); }
		/** Checks if no packages are loaded. */
		[[nodiscard]] constexpr bool empty() const noexcept { return packages().empty(); }

		constexpr void swap(package_proxy &other) noexcept { std::swap(m_parent, other.m_parent); }
		friend constexpr void swap(package_proxy &a, package_proxy &b) noexcept { a.swap(b); }

	protected:
		const asset_database *m_parent;

	private:
		[[nodiscard]] constexpr const packages_t &packages() const noexcept { return m_parent->m_packages; }
	};
	template<>
	class package_proxy<true> : public package_proxy<false>
	{
		friend class asset_database;

		using base_t = package_proxy<false>;

	public:
		typedef typename base_t::value_type value_type;
		typedef typename base_t::const_pointer pointer;
		typedef typename base_t::const_pointer const_pointer;
		typedef typename base_t::const_reference reference;
		typedef typename base_t::const_reference const_reference;
		typedef typename base_t::const_iterator iterator;
		typedef typename base_t::const_iterator const_iterator;
		typedef typename base_t::const_reverse_iterator reverse_iterator;
		typedef typename base_t::const_reverse_iterator const_reverse_iterator;
		typedef typename base_t::size_type size_type;
		typedef typename base_t::difference_type difference_type;

	protected:
		constexpr explicit package_proxy(asset_database &parent) noexcept : base_t(parent) {}

	public:
		package_proxy() = delete;

		constexpr package_proxy(const package_proxy &) noexcept = default;
		constexpr package_proxy(package_proxy &&) noexcept = default;
		constexpr package_proxy &operator=(const package_proxy &) noexcept = default;
		constexpr package_proxy &operator=(package_proxy &&) noexcept = default;

		/** Removes a package at the specified position from the load order.
		 * @return Iterator to the package after the erased one. */
		const_iterator erase(const_iterator where) { return parent()->erase(where); }
		/** Removes all packages between [first, last) from the load order.
		 * @return Iterator to the package after the erased range. */
		const_iterator erase(const_iterator first, const_iterator last) { return parent()->erase(first, last); }

		/** Inserts a package at the specified position into the load order.
		 * @return Iterator to the inserted package. */
		const_iterator insert(const_iterator where, const asset_package &pkg) { return parent()->insert(where, pkg); }
		/** @copydoc insert */
		const_iterator insert(const_iterator where, asset_package &&pkg)
		{
			return parent()->insert(where, std::forward<asset_package>(pkg));
		}
		/** Inserts a package at the end of the load order. */
		void push_back(const asset_package &pkg) { insert(end(), pkg); }
		/** @copydoc push_back */
		void push_back(asset_package &&pkg) { insert(end(), std::forward<asset_package>(pkg)); }

		/** Swaps load order of packages `a` and `b`.
		 * @return Iterator to the package after the erased range. */
		void swap(const_iterator a, const_iterator b) { return parent()->swap(a, b); }

		constexpr void swap(package_proxy &other) noexcept { base_t::swap(other); }
		friend constexpr void swap(package_proxy &a, package_proxy &b) noexcept { a.swap(b); }

	private:
		[[nodiscard]] constexpr asset_database *parent() const noexcept
		{
			return const_cast<asset_database *>(base_t::m_parent);
		}
		[[nodiscard]] constexpr typename base_t::packages_t &packages() const noexcept { return parent()->m_packages; }
	};

	constexpr package_proxy<false>::package_proxy(const package_proxy<true> &other) noexcept : m_parent(other.m_parent)
	{
	}
	constexpr package_proxy<false>::package_proxy(package_proxy<true> &&other) noexcept : m_parent(other.m_parent) {}
	constexpr package_proxy<false> &package_proxy<false>::operator=(const package_proxy<true> &other) noexcept
	{
		m_parent = other.m_parent;
		return *this;
	}
	constexpr package_proxy<false> &package_proxy<false>::operator=(package_proxy<true> &&other) noexcept
	{
		m_parent = other.m_parent;
		return *this;
	}

	constexpr auto asset_database::packages() noexcept { return package_proxy<true>{*this}; }
	constexpr auto asset_database::packages() const noexcept { return package_proxy<false>{*this}; }

	/* TODO: Refactor implementation to support error_code */
}	 // namespace sek::engine

extern template class SEK_API_IMPORT sek::service<sek::shared_guard<sek::engine::asset_database>>;
