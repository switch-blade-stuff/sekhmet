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
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 * ============================================================================
 *
 * Created by switchblade on 2022-04-04
 */

#pragma once

#include <atomic>
#include <filesystem>
#include <memory>
#include <utility>

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

	/** @brief Exception thrown when operation on an asset package fails. */
	class asset_package_error : public std::runtime_error
	{
	public:
		asset_package_error() : std::runtime_error("Unknown asset package error") {}
		explicit asset_package_error(std::string &&msg) : std::runtime_error(std::move(msg)) {}
		explicit asset_package_error(const std::string &msg) : std::runtime_error(msg) {}
		explicit asset_package_error(const char *msg) : std::runtime_error(msg) {}
		~asset_package_error() override = default;
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
		inline asset_source make_asset_source(asset_buffer_t &&buff, std::int64_t size, std::int64_t offset);

		struct package_fragment;
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

			explicit asset_info(package_fragment *parent) : parent(parent) {}
			asset_info(type_selector_t<archive_info_t>, package_fragment *parent) : asset_info(parent) {}
			asset_info(type_selector_t<loose_info_t>, package_fragment *parent) : asset_info(parent)
			{
				std::construct_at(&loose_info);
			}
			~asset_info();

			[[nodiscard]] constexpr bool has_metadata() const noexcept;

			package_fragment *parent; /* Parent fragment of the asset. */

			interned_string name;			 /* Optional human-readable name of the asset. */
			dense_set<interned_string> tags; /* Optional tags of the asset. */

			union
			{
				archive_info_t archive_info = {};
				loose_info_t loose_info;
			};
		};

		struct master_package;
		struct package_fragment
		{
			enum flags_t : std::int32_t
			{
				NO_FLAGS = 0,
				IS_MASTER = 1,
				IS_ARCHIVE = 2,
#ifdef SEK_EDITOR
				IS_PROJECT = 4 | IS_MASTER, /* Used to designate in-editor project packages. */
#endif
				ARCHIVE_FORMAT_ZSTD = 0b0010'000, /* Archive is compressed with ZSTD. */
				ARCHIVE_FORMAT_MASK = 0b111'000,
			};

			package_fragment(flags_t flags, std::filesystem::path &&path) : flags(flags), path(std::move(path)) {}
			package_fragment(master_package *master, flags_t flags, std::filesystem::path &&path)
				: package_fragment(flags, std::move(path))
			{
				this->master = master;
			}

			[[nodiscard]] constexpr bool is_master() const noexcept { return flags & IS_MASTER; }
#ifdef SEK_EDITOR
			[[nodiscard]] constexpr bool is_project() const noexcept { return (flags & IS_PROJECT) == IS_PROJECT; }
#endif
			[[nodiscard]] constexpr bool is_archive() const noexcept { return flags & IS_ARCHIVE; }
			[[nodiscard]] constexpr bool is_archive_flat() const noexcept
			{
				return (flags & (ARCHIVE_FORMAT_MASK | IS_ARCHIVE)) == IS_ARCHIVE;
			}
			[[nodiscard]] constexpr bool is_archive_zstd() const noexcept { return flags & ARCHIVE_FORMAT_ZSTD; }

			[[nodiscard]] SEK_API master_package *get_master() noexcept;
			SEK_API void acquire();
			SEK_API void release();

			[[nodiscard]] system::native_file open_archive(std::int64_t offset) const;
			[[nodiscard]] asset_source open_asset_loose(const asset_info *info) const;
			[[nodiscard]] asset_source open_asset_flat(const asset_info *info) const;
			[[nodiscard]] asset_source open_asset_zstd(const asset_info *info) const;

			[[nodiscard]] SEK_API std::vector<std::byte> read_metadata(const asset_info *info) const;
			[[nodiscard]] SEK_API asset_source open_asset(const asset_info *info) const;

			union
			{
				std::atomic<std::size_t> ref_count = 0;
				master_package *master;
			};
			flags_t flags;

			std::filesystem::path path; /* Path to fragment's directory or archive file. */
		};

		asset_info::~asset_info()
		{
			if (parent->is_archive())
				std::destroy_at(&archive_info);
			else
				std::destroy_at(&loose_info);
		}

		struct asset_table
		{
			using uuid_table_t = dense_map<uuid, asset_info *>;
			using name_table_t = dense_map<std::string_view, std::pair<uuid, asset_info *>>;

			uuid_table_t uuid_table;
			name_table_t name_table;
		};
		struct master_package : package_fragment, asset_table
		{
			master_package(std::filesystem::path &&path) : package_fragment(IS_MASTER, std::move(path)) {}
			~master_package()
			{
				for (auto p : uuid_table) std::destroy_at(p.second);
			}

			inline void acquire_impl() { ++ref_count; }
			inline void release_impl()
			{
				if (ref_count.fetch_sub(1) == 1) [[unlikely]]
					delete this;
			}

			inline asset_info *alloc_info() { return info_pool.allocate(); }
			inline void dealloc_info(asset_info *info) { info_pool.deallocate(info); }

			void insert_asset(uuid id, asset_info *info);

			std::vector<std::unique_ptr<package_fragment>> fragments;
			sek::detail::basic_pool<asset_info> info_pool;
		};
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
		constexpr asset_source(std::int64_t size, std::int64_t offset) noexcept : data_size(size), file_offset(offset)
		{
		}
		inline asset_source(detail::asset_buffer_t &&buffer, std::int64_t size, std::int64_t offset)
			: asset_source(size, offset)
		{
			std::construct_at(&asset_buffer, std::move(buffer));
		}
		inline asset_source(system::native_file &&file, std::int64_t size, std::int64_t offset)
			: asset_source(size, offset)
		{
			std::construct_at(&asset_file, std::move(file));
		}

		friend inline asset_source detail::make_asset_source(detail::asset_buffer_t &&, std::int64_t, std::int64_t);
		friend inline asset_source detail::make_asset_source(system::native_file &&, std::int64_t, std::int64_t);

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
				std::destroy_at(&asset_file);
			else
				std::destroy_at(&asset_buffer);
		}

		/** Reads asset data from the underlying file or buffer.
		 * @param dst Destination buffer.
		 * @param n Amount of bytes to read.
		 * @return Total amount of bytes read. */
		std::size_t read(void *dst, std::size_t n)
		{
			auto new_pos = read_pos + static_cast<std::int64_t>(n);
			if (new_pos > data_size || new_pos < 0) [[unlikely]]
			{
				n = static_cast<std::size_t>(data_size - read_pos);
				new_pos = data_size;
			}

			if (has_file())
			{
				read_pos = new_pos;
				return file().read(dst, n);
			}
			else
			{
				std::copy_n(asset_buffer.data + std::exchange(read_pos, new_pos), n, static_cast<std::byte *>(dst));
				return n;
			}
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
					return seek_pos(read_pos + off);
				else if (dir == end)
					return seek_pos(size() + off);
			}
			return -1;
		}

		/** Returns the current read position. */
		[[nodiscard]] constexpr std::int64_t tell() const noexcept { return read_pos; }
		/** Returns the size of the asset. */
		[[nodiscard]] constexpr std::int64_t size() const noexcept { return data_size; }
		/** Checks if the asset source is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }

		/** Returns the base file offset of the asset.
		 * @note If the asset is not backed by a file, returns a negative integer. */
		[[nodiscard]] constexpr std::int64_t base_offset() const noexcept { return file_offset; }
		/** Checks if the asset is backed by a file. */
		[[nodiscard]] constexpr bool has_file() const noexcept { return base_offset() >= 0; }
		/** Returns reference to the underlying native file.
		 * @warning Undefined behavior if the asset is not backed by a file. */
		[[nodiscard]] constexpr system::native_file &file() noexcept { return asset_file; }
		/** @copydoc file */
		[[nodiscard]] constexpr const system::native_file &file() const noexcept { return asset_file; }
		/** Returns pointer to the underlying memory buffer.
		 * @warning Undefined behavior if the asset is not backed by a buffer. */
		[[nodiscard]] constexpr const std::byte *buffer() const noexcept { return asset_buffer.data; }

		constexpr void swap(asset_source &other) noexcept
		{
			using std::swap;
			swap(data_size, other.data_size);
			swap(file_offset, other.file_offset);
			swap(read_pos, other.read_pos);
			swap(padding, other.padding);
		}
		friend constexpr void swap(asset_source &a, asset_source &b) noexcept { a.swap(b); }

	private:
		std::int64_t seek_pos(std::int64_t new_pos)
		{
			if (new_pos > data_size || new_pos < 0) [[unlikely]]
				return -1;
			else if (has_file())
			{
				const auto file_pos = file().seek(new_pos, beg);
				if (file_pos < 0) [[unlikely]]
					return -1;
				return (read_pos = file_pos - base_offset());
			}
			else
				return read_pos = new_pos;
		}

		std::int64_t data_size = 0;	  /* Total size of the asset. */
		std::int64_t file_offset = 0; /* Base offset within the file, -1 if not backed by a file. */
		std::int64_t read_pos = 0;	  /* Current read position with the base offset applied. */
		union
		{
			std::byte padding[sizeof(system::native_file)];
			detail::asset_buffer_t asset_buffer;
			system::native_file asset_file;
		};
	};

	namespace detail
	{
		inline asset_source make_asset_source(system::native_file &&file, std::int64_t size, std::int64_t offset)
		{
			return asset_source{std::move(file), size, offset};
		}
		inline asset_source make_asset_source(asset_buffer_t &&buff, std::int64_t size, std::int64_t offset)
		{
			return asset_source{std::move(buff), size, offset};
		}

		constexpr bool asset_info::has_metadata() const noexcept
		{
			return parent->is_archive() ? archive_info.meta_offset : loose_info.meta_path.empty();
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

			asset_info *info = nullptr;
		};
		struct package_ptr
		{
			constexpr explicit package_ptr(master_package *pkg) noexcept : pkg(pkg) {}

			constexpr package_ptr() noexcept = default;
			constexpr package_ptr(package_ptr &&other) noexcept { swap(other); }
			constexpr package_ptr &operator=(package_ptr &&other) noexcept
			{
				swap(other);
				return *this;
			}
			package_ptr(const package_ptr &other) noexcept : pkg(other.pkg) { acquire(); }
			package_ptr &operator=(const package_ptr &other) noexcept
			{
				if (this != &other) reset(other.pkg);
				return *this;
			}
			~package_ptr() { release(); }

			[[nodiscard]] constexpr bool empty() const noexcept { return pkg == nullptr; }
			[[nodiscard]] constexpr master_package *operator->() const noexcept { return pkg; }

			constexpr void swap(package_ptr &other) noexcept { std::swap(pkg, other.pkg); }

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
			void reset(master_package *new_pkg)
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

			master_package *pkg = nullptr;
		};
	}	 // namespace detail

	/** @brief Structure used to reference an asset.
	 * @note Asset packages are kept alive as long as any of their assets are referenced. */
	class asset_ref
	{
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
		/** Returns reference to the name of the asset.
		 * @warning Undefined behavior if the asset reference is empty. */
		[[nodiscard]] constexpr const interned_string &name() const noexcept { return m_ptr->name; }
		/** Returns a handle to the parent package of the asset. */
		[[nodiscard]] asset_package package() const;
		/** Opens an asset source used to read asset's data.
		 * @throw asset_package_error On failure to open asset. */
		[[nodiscard]] asset_source open() const { return m_ptr->parent->open_asset(m_ptr.info); }
		/** Checks if the asset has metadata. */
		[[nodiscard]] bool has_metadata() const { return m_ptr->has_metadata(); }
		/** Returns a vector of bytes containing asset's metadata.
		 * @note If the asset does not have metadata, returns empty vector.
		 * @throw asset_package_error On failure to open metadata or archive file. */
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
		[[nodiscard]] constexpr bool operator==(const asset_ref &other) const noexcept
		{
			return m_ptr.info == other.m_ptr.info;
		}

	private:
		uuid m_id;
		detail::asset_info_ptr m_ptr;
	};
	/** @brief Reference-counted handle used to reference an asset package. */
	class asset_package
	{
		friend class asset_ref;
		friend class asset_database;

	public:
		/** Loads a package at the specified path.
		 * @throw asset_package_error If the path does not contain a valid package or
		 * an implementation-defined error occurred during loading. */
		[[nodiscard]] static SEK_API asset_package load(const std::filesystem::path &path);
		/** Load all packages in the specified directory.
		 * @throw asset_package_error If the path is not a valid directory. */
		[[nodiscard]] static SEK_API std::vector<asset_package> load_all(const std::filesystem::path &path);

	private:
		constexpr explicit asset_package(detail::package_ptr &&ptr) noexcept : m_ptr(std::move(ptr)) {}
		SEK_API explicit asset_package(detail::master_package *pkg);

	public:
		/** Initializes an empty package handle. */
		constexpr asset_package() noexcept = default;
		constexpr asset_package(asset_package &&) noexcept = default;
		constexpr asset_package &operator=(asset_package &&other) noexcept
		{
			m_ptr = std::move(other.m_ptr);
			return *this;
		}
		asset_package(const asset_package &) = default;
		asset_package &operator=(const asset_package &) = default;

		/** Checks if the package handle is empty (does not reference any asset package). */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_ptr.empty(); }
		/** Returns path of the asset package.
		 * @warning Undefined behavior if the package handle is empty. */
		[[nodiscard]] constexpr const std::filesystem::path &path() const noexcept { return m_ptr->path; }

		/** Resets the package handle to an empty state. */
		SEK_API void reset();

	private:
		detail::package_ptr m_ptr;
	};

	asset_package asset_ref::package() const { return asset_package{m_ptr->parent->get_master()}; }

	class asset_database : detail::asset_table
	{
	public:
	private:
		mutable std::shared_mutex m_mtx;
	};
}	 // namespace sek::engine
