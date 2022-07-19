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
#include "sekhmet/detail/intern.hpp"
#include "sekhmet/detail/service.hpp"
#include "sekhmet/detail/uuid.hpp"
#include "sekhmet/system/native_file.hpp"

#include <shared_mutex>

namespace sek::engine
{
	class asset_source;
	class asset_ref;
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

	namespace detail
	{
		struct asset_buffer_t
		{
			constexpr asset_buffer_t() noexcept = default;
			constexpr asset_buffer_t(asset_buffer_t &&other) noexcept { swap(other); }

			explicit asset_buffer_t(std::int64_t size) : data(new std::byte[static_cast<std::uint64_t>(size)]{}) {}
			~asset_buffer_t() { delete[] data; }

			constexpr void swap(asset_buffer_t &other) noexcept { std::swap(data, other.data); }

			std::byte *data = nullptr;
		};

		inline asset_source make_asset_source(system::native_file &&file, std::int64_t size, std::int64_t offset);
		inline asset_source make_asset_source(asset_buffer_t &&buff, std::int64_t size);

		struct package_info;
		struct asset_info
		{
			struct loose_info_t
			{
				std::string asset_path; /* Path to asset's main data file. */
				std::string meta_path;	/* Path to asset's metadata file. */
			};
			struct archive_info_t
			{
				std::int64_t asset_offset;	 /* Offset into the archive at which asset's data is located. */
				std::int64_t asset_size;	 /* Size of the data within the archive. */
				std::int64_t asset_src_size; /* Decompressed size of the asset if any compression is used. */
				std::uint64_t asset_frames;	 /* Amount of compressed frames used (0 if not compressed). */

				std::int64_t meta_offset; /* Offset into the archive at which asset's metadata is located. */
				std::int64_t meta_size;	  /* Size of the asset metadata within the archive. */
				/* Metadata is never compressed, since it generally is not large, thus using compression
				 * will have more overhead than reading directly from a file. */
			};

			explicit asset_info(package_info *parent) : parent(parent) {}
			asset_info(type_selector_t<archive_info_t>, package_info *parent) : asset_info(parent) {}
			asset_info(type_selector_t<loose_info_t>, package_info *parent) : asset_info(parent)
			{
				std::construct_at(&loose_info);
			}
			inline ~asset_info();

			[[nodiscard]] constexpr bool has_metadata() const noexcept;

			package_info *parent; /* Parent package of the asset. */

			interned_string name;			 /* Optional human-readable name of the asset. */
			dense_set<interned_string> tags; /* Optional tags of the asset. */

			union
			{
				archive_info_t archive_info = {};
				loose_info_t loose_info;
			};
		};
		struct asset_table
		{
			using uuid_table_t = dense_map<uuid, asset_info *>;
			using name_table_t = dense_map<std::string_view, uuid>;

			class entry_iterator;
			class entry_ptr;

			typedef asset_ref value_type;
			typedef entry_iterator iterator;
			typedef entry_iterator const_iterator;
			typedef std::reverse_iterator<const_iterator> reverse_iterator;
			typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
			typedef asset_ref reference;
			typedef asset_ref const_reference;
			typedef entry_ptr pointer;
			typedef entry_ptr const_pointer;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;

			[[nodiscard]] constexpr bool empty() const noexcept { return uuid_table.empty(); }
			[[nodiscard]] constexpr size_type size() const noexcept { return uuid_table.size(); }

			[[nodiscard]] constexpr const_iterator begin() const noexcept;
			[[nodiscard]] constexpr const_iterator end() const noexcept;
			[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept;
			[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept;

			[[nodiscard]] constexpr const_iterator find(uuid) const;
			[[nodiscard]] constexpr const_iterator find(std::string_view) const;
			[[nodiscard]] constexpr const_iterator match(auto &&) const;
			[[nodiscard]] inline std::vector<reference> find_all(std::string_view) const;
			[[nodiscard]] inline std::vector<reference> match_all(auto &&) const;

			uuid_table_t uuid_table;
			name_table_t name_table;
		};
		struct package_info : asset_table
		{
			enum flags_t : std::int32_t
			{
				NO_FLAGS = 0,
				IS_ARCHIVE = 1,
				ARCHIVE_FORMAT_ZSTD = 0b0010'0, /* Archive is compressed with ZSTD. */
				ARCHIVE_FORMAT_MASK = 0b1111'0,

#ifdef SEK_EDITOR
				/* Used to designate in-editor project packages.
				 * Project packages can not be archived. */
				IS_PROJECT = 1 << 31,
#endif
			};

			package_info(std::filesystem::path &&path) : path(std::move(path)) {}
			package_info(const std::filesystem::path &path) : path(path) {}
			~package_info()
			{
				for (auto p : uuid_table) std::destroy_at(p.second);
			}

			inline asset_info *alloc_info() { return info_pool.allocate(); }
			inline void dealloc_info(asset_info *info) { info_pool.deallocate(info); }

			void insert_asset(uuid id, asset_info *info);

#ifdef SEK_EDITOR
			[[nodiscard]] constexpr bool is_project() const noexcept { return flags & IS_PROJECT; }
#endif
			[[nodiscard]] constexpr bool is_archive() const noexcept { return flags & IS_ARCHIVE; }
			[[nodiscard]] constexpr bool is_archive_flat() const noexcept
			{
				return (flags & (ARCHIVE_FORMAT_MASK | IS_ARCHIVE)) == IS_ARCHIVE;
			}
			[[nodiscard]] constexpr bool is_archive_zstd() const noexcept { return flags & ARCHIVE_FORMAT_ZSTD; }

			SEK_API void acquire();
			SEK_API void release();

			[[nodiscard]] system::native_file open_archive(std::int64_t offset) const;
			[[nodiscard]] asset_source open_asset_loose(const asset_info *info) const;
			[[nodiscard]] asset_source open_asset_flat(const asset_info *info) const;
			[[nodiscard]] asset_source open_asset_zstd(const asset_info *info) const;

			[[nodiscard]] SEK_API std::vector<std::byte> read_metadata(const asset_info *info) const;
			[[nodiscard]] SEK_API asset_source open_asset(const asset_info *info) const;

			std::atomic<std::size_t> ref_count = 0;
			flags_t flags = NO_FLAGS;

			std::filesystem::path path;
			sek::detail::basic_pool<asset_info> info_pool;
		};

		inline asset_info::~asset_info()
		{
			if (parent->is_archive())
				std::destroy_at(&archive_info);
			else
				std::destroy_at(&loose_info);
		}
	}	 // namespace detail

	/** @brief Structure providing a read-only access to data of an asset.
	 *
	 * Since assets may be either loose or compressed and archived, a special structure is needed to read asset data.
	 * In addition, to allow for implementation of storage optimization techniques (such as DirectStorage),
	 * streams cannot be used directly either, as access to the underlying file or data buffer is needed. */
	class asset_source
	{
	public:
		typedef typename system::native_file::seek_dir seek_dir;

		constexpr static seek_dir beg = system::native_file::beg;
		constexpr static seek_dir cur = system::native_file::cur;
		constexpr static seek_dir end = system::native_file::end;

	private:
		constexpr asset_source(std::int64_t size, std::int64_t offset) noexcept : m_size(size), m_offset(offset) {}
		inline asset_source(detail::asset_buffer_t &&buffer, std::int64_t size) : asset_source(size, -1)
		{
			std::construct_at(&m_buffer, std::move(buffer));
		}
		inline asset_source(system::native_file &&file, std::int64_t size, std::int64_t offset)
			: asset_source(size, offset)
		{
			std::construct_at(&m_file, std::move(file));
		}

		friend inline asset_source detail::make_asset_source(system::native_file &&, std::int64_t, std::int64_t);
		friend inline asset_source detail::make_asset_source(detail::asset_buffer_t &&, std::int64_t);

	public:
		asset_source(const asset_source &) = delete;
		asset_source &operator=(const asset_source &) = delete;

		/** Initializes an empty asset source. */
		constexpr asset_source() noexcept : padding{} {}
		constexpr asset_source(asset_source &&other) noexcept : asset_source() { swap(other); }
		constexpr asset_source &operator=(asset_source &&other) noexcept
		{
			swap(other);
			return *this;
		}

		~asset_source()
		{
			if (has_file())
				std::destroy_at(&m_file);
			else
				std::destroy_at(&m_buffer);
		}

		/** Reads asset data from the underlying file or buffer.
		 * @param dst Destination buffer.
		 * @param n Amount of bytes to read.
		 * @return Total amount of bytes read. */
		std::size_t read(void *dst, std::size_t n)
		{
			auto new_pos = m_read_pos + static_cast<std::int64_t>(n);
			if (new_pos > m_size || new_pos < 0) [[unlikely]]
			{
				n = static_cast<std::size_t>(m_size - m_read_pos);
				new_pos = m_size;
			}

			if (has_file())
				n = file().read(dst, n);
			else
				std::copy_n(m_buffer.data + m_read_pos, n, static_cast<std::byte *>(dst));
			m_read_pos = new_pos;
			return n;
		}
		/** Seeks the asset source to the specific offset within the asset.
		 * @param off Offset to seek to.
		 * @param dir Direction in which to seek.
		 * @return Current position within the asset or a negative integer on error. */
		std::int64_t seek(std::int64_t off, seek_dir dir)
		{
			if (empty()) [[unlikely]]
				return off == 0 ? 0 : -1;
			else
			{
				if (dir == beg)
					return seek_pos(base_offset() + off);
				else if (dir == cur)
					return seek_pos(m_read_pos + off);
				else if (dir == end)
					return seek_pos(size() + off);
			}
			return -1;
		}

		/** Returns the current read position. */
		[[nodiscard]] constexpr std::int64_t tell() const noexcept { return m_read_pos; }
		/** Returns the size of the asset. */
		[[nodiscard]] constexpr std::int64_t size() const noexcept { return m_size; }
		/** Checks if the asset source is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }

		/** Returns the base file offset of the asset.
		 * @note If the asset is not backed by a file, returns a negative integer. */
		[[nodiscard]] constexpr std::int64_t base_offset() const noexcept { return m_offset; }
		/** Checks if the asset is backed by a file. */
		[[nodiscard]] constexpr bool has_file() const noexcept { return base_offset() >= 0; }
		/** Returns reference to the underlying native file.
		 * @warning Undefined behavior if the asset is not backed by a file. */
		[[nodiscard]] constexpr system::native_file &file() noexcept { return m_file; }
		/** @copydoc file */
		[[nodiscard]] constexpr const system::native_file &file() const noexcept { return m_file; }
		/** Maps the underlying file to memory.
		 * @warning Undefined behavior if the asset is not backed by a file. */
		[[nodiscard]] system::native_filemap map() const noexcept { return {m_file, m_offset, m_size}; }
		/** Returns pointer to the underlying memory buffer.
		 * @warning Undefined behavior if the asset is not backed by a buffer. */
		[[nodiscard]] constexpr const std::byte *buffer() const noexcept { return m_buffer.data; }

		constexpr void swap(asset_source &other) noexcept
		{
			using std::swap;
			swap(m_size, other.m_size);
			swap(m_offset, other.m_offset);
			swap(m_read_pos, other.m_read_pos);
			swap(padding, other.padding);
		}
		friend constexpr void swap(asset_source &a, asset_source &b) noexcept { a.swap(b); }

	private:
		std::int64_t seek_pos(std::int64_t new_pos)
		{
			if (new_pos > m_size || new_pos < 0) [[unlikely]]
				return -1;
			else if (has_file())
			{
				const auto file_pos = file().seek(new_pos, beg);
				if (file_pos < 0) [[unlikely]]
					return -1;
				return (m_read_pos = file_pos - base_offset());
			}
			else
				return m_read_pos = new_pos;
		}

		std::int64_t m_size = 0;	 /* Total size of the asset. */
		std::int64_t m_offset = 0;	 /* Base offset within the file, -1 if not backed by a file. */
		std::int64_t m_read_pos = 0; /* Current read position with the base offset applied. */
		union
		{
			std::byte padding[sizeof(system::native_file)];
			detail::asset_buffer_t m_buffer;
			system::native_file m_file;
		};
	};

	namespace detail
	{
		inline asset_source make_asset_source(system::native_file &&file, std::int64_t size, std::int64_t offset)
		{
			return asset_source{std::move(file), size, offset};
		}
		inline asset_source make_asset_source(asset_buffer_t &&buff, std::int64_t size)
		{
			return asset_source{std::move(buff), size};
		}

		constexpr bool asset_info::has_metadata() const noexcept
		{
			return parent->is_archive() ? archive_info.meta_offset : !loose_info.meta_path.empty();
		}

		struct asset_info_ptr
		{
			constexpr explicit asset_info_ptr(asset_info *info) noexcept : info(info) {}

			constexpr asset_info_ptr() noexcept = default;
			constexpr asset_info_ptr(asset_info_ptr &&other) noexcept { swap(other); }
			constexpr asset_info_ptr &operator=(asset_info_ptr &&other) noexcept
			{
				swap(other);
				return *this;
			}
			asset_info_ptr(const asset_info_ptr &other) noexcept : info(other.info) { acquire(); }
			asset_info_ptr &operator=(const asset_info_ptr &other) noexcept
			{
				if (this != &other) reset(other.info);
				return *this;
			}
			~asset_info_ptr() { release(); }

			[[nodiscard]] constexpr bool empty() const noexcept { return info == nullptr; }
			[[nodiscard]] constexpr asset_info *operator->() const noexcept { return info; }

			constexpr void swap(asset_info_ptr &other) noexcept { std::swap(info, other.info); }

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

			asset_info *info = nullptr;
		};
		struct package_info_ptr
		{
			constexpr explicit package_info_ptr(package_info *pkg) noexcept : pkg(pkg) {}

			constexpr package_info_ptr() noexcept = default;
			constexpr package_info_ptr(package_info_ptr &&other) noexcept { swap(other); }
			constexpr package_info_ptr &operator=(package_info_ptr &&other) noexcept
			{
				swap(other);
				return *this;
			}
			package_info_ptr(const package_info_ptr &other) noexcept : pkg(other.pkg) { acquire(); }
			package_info_ptr &operator=(const package_info_ptr &other) noexcept
			{
				if (this != &other) reset(other.pkg);
				return *this;
			}
			~package_info_ptr() { release(); }

			[[nodiscard]] constexpr bool empty() const noexcept { return pkg == nullptr; }
			[[nodiscard]] constexpr package_info *operator->() const noexcept { return pkg; }

			constexpr void swap(package_info_ptr &other) noexcept { std::swap(pkg, other.pkg); }

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

			package_info *pkg = nullptr;
		};
	}	 // namespace detail

	/** @brief Structure used to reference an asset.
	 * @note Asset packages are kept alive as long as any of their assets are referenced. */
	class asset_ref
	{
		friend class detail::asset_table::entry_iterator;
		friend class asset_package;
		friend class asset_database;

		constexpr asset_ref(uuid id, detail::asset_info_ptr &&ptr) noexcept : m_id(std::move(id)), m_ptr(std::move(ptr))
		{
		}
		asset_ref(uuid id, detail::asset_info *info) : m_id(std::move(id)), m_ptr(info) { m_ptr.acquire(); }

	public:
		asset_ref() = delete;

		constexpr asset_ref(asset_ref &&) noexcept = default;
		constexpr asset_ref &operator=(asset_ref &&other) noexcept
		{
			m_id = std::move(other.m_id);
			m_ptr = std::move(other.m_ptr);
			return *this;
		}
		asset_ref(const asset_ref &) = default;
		asset_ref &operator=(const asset_ref &) = default;

		/** Returns the id of the asset. If the asset reference does not point to an asset, returns a nil uuid. */
		[[nodiscard]] constexpr uuid id() const noexcept { return m_id; }
		/** Returns reference to the name of the asset. */
		[[nodiscard]] constexpr const interned_string &name() const noexcept { return m_ptr->name; }
		/** Returns a set of string tags of the asset. */
		[[nodiscard]] constexpr const dense_set<interned_string> &tags() const noexcept { return m_ptr->tags; }

		/** Returns a handle to the parent package of the asset. */
		[[nodiscard]] inline asset_package package() const;
		/** Opens an asset source used to read asset's data.
		 * @throw asset_error On failure to open the file or archive containing the asset. */
		[[nodiscard]] asset_source open() const { return m_ptr->parent->open_asset(m_ptr.info); }
		/** Checks if the asset has metadata. */
		[[nodiscard]] bool has_metadata() const { return m_ptr->has_metadata(); }
		/** Returns a vector of bytes containing asset's metadata.
		 * @note If the asset does not have metadata, returns an empty vector.
		 * @throw asset_error On failure to open the file or archive containing the metadata. */
		[[nodiscard]] std::vector<std::byte> metadata() const { return m_ptr->parent->read_metadata(m_ptr.info); }

		constexpr void swap(asset_ref &other) noexcept
		{
			m_id.swap(other.m_id);
			m_ptr.swap(other.m_ptr);
		}
		friend constexpr void swap(asset_ref &a, asset_ref &b) noexcept { a.swap(b); }

		/** Returns true if both asset references reference the *exact* same asset.
		 * @note Multiple asset references with the same id may reference different assets.
		 * This may happen if the assets were obtained directly from packages (bypassing the database),
		 * thus no overrides could be resolved. */
		[[nodiscard]] constexpr bool operator==(const asset_ref &) const noexcept = default;

	private:
		uuid m_id;
		detail::asset_info_ptr m_ptr;
	};

	namespace detail
	{
		class asset_table::entry_ptr
		{
			friend class entry_iterator;

			constexpr explicit entry_ptr(asset_ref &&ref) noexcept : m_ref(std::move(ref)) {}

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

			[[nodiscard]] constexpr const asset_ref *get() const noexcept { return std::addressof(m_ref); }
			[[nodiscard]] constexpr const asset_ref *operator->() const noexcept { return get(); }
			[[nodiscard]] constexpr const asset_ref &operator*() const noexcept { return *get(); }

			constexpr void swap(entry_ptr &other) noexcept { m_ref.swap(other.m_ref); }
			friend constexpr void swap(entry_ptr &a, entry_ptr &b) noexcept { a.swap(b); }

			[[nodiscard]] constexpr bool operator==(const entry_ptr &) const noexcept = default;

		private:
			asset_ref m_ref;
		};
		class asset_table::entry_iterator
		{
			friend struct asset_table;

			using uuid_iter = typename uuid_table_t::const_iterator;

		public:
			typedef asset_ref value_type;
			typedef asset_ref reference;
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

		constexpr typename asset_table::const_iterator asset_table::begin() const noexcept
		{
			return const_iterator{uuid_table.begin()};
		}
		constexpr typename asset_table::const_iterator asset_table::end() const noexcept
		{
			return const_iterator{uuid_table.end()};
		}
		constexpr typename asset_table::const_reverse_iterator asset_table::rbegin() const noexcept
		{
			return const_reverse_iterator{end()};
		}
		constexpr typename asset_table::const_reverse_iterator asset_table::rend() const noexcept
		{
			return const_reverse_iterator{begin()};
		}

		constexpr typename asset_table::const_iterator asset_table::find(uuid id) const
		{
			return const_iterator{uuid_table.find(id)};
		}
		constexpr typename asset_table::const_iterator asset_table::find(std::string_view name) const
		{
			if (auto name_iter = name_table.find(name); name_iter != name_table.end()) [[likely]]
				return find(name_iter->second);
			else
				return end();
		}
		constexpr typename asset_table::const_iterator asset_table::match(auto &&pred) const
		{
			return std::find_if(begin(), end(), pred);
		}
		inline std::vector<typename asset_table::reference> asset_table::find_all(std::string_view name) const
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
		inline std::vector<typename asset_table::reference> asset_table::match_all(auto &&pred) const
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
		friend class asset_ref;
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

		constexpr void swap(asset_package &other) noexcept { m_ptr.swap(other.m_ptr); }
		friend constexpr void swap(asset_package &a, asset_package &b) noexcept { a.swap(b); }

		[[nodiscard]] constexpr bool operator==(const asset_package &) const noexcept = default;

	private:
		detail::package_info_ptr m_ptr;
	};

	inline asset_package asset_ref::package() const { return asset_package{m_ptr->parent}; }

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
		void restore_overrides(typename packages_t::const_iterator, typename packages_t::const_iterator);
		SEK_API typename packages_t::const_iterator erase_pkg(typename packages_t::const_iterator,
															  typename packages_t::const_iterator);
		typename packages_t::const_iterator erase_pkg(typename packages_t::const_iterator where)
		{
			return erase_pkg(where, std::next(where));
		}

		void insert_overrides(typename packages_t::const_iterator);
		SEK_API typename packages_t::const_iterator insert_pkg(typename packages_t::const_iterator, const asset_package &);
		SEK_API typename packages_t::const_iterator insert_pkg(typename packages_t::const_iterator, asset_package &&);

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
		const_iterator erase(const_iterator where) { return parent()->erase_pkg(where); }
		/** Removes all packages between [first, last) from the load order.
		 * @return Iterator to the package after the erased range. */
		const_iterator erase(const_iterator first, const_iterator last) { return parent()->erase_pkg(first, last); }

		/** Inserts a package at the specified position into the load order.
		 * @return Iterator to the inserted package. */
		const_iterator insert(const_iterator where, const asset_package &pkg)
		{
			return parent()->insert_pkg(where, pkg);
		}
		/** @copydoc insert */
		const_iterator insert(const_iterator where, asset_package &&pkg)
		{
			return parent()->insert_pkg(where, std::forward<asset_package>(pkg));
		}
		/** Inserts a package at the end of the load order. */
		void push_back(const asset_package &pkg) { insert(end(), pkg); }
		/** @copydoc push_back */
		void push_back(asset_package &&pkg) { insert(end(), std::forward<asset_package>(pkg)); }

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
}	 // namespace sek::engine

extern template class SEK_API_IMPORT sek::service<sek::shared_guard<sek::engine::asset_database>>;
