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

#include "sekhmet/detail/basic_pool.hpp"
#include "sekhmet/detail/dense_map.hpp"
#include "sekhmet/detail/dense_set.hpp"
#include "sekhmet/detail/intern.hpp"
#include "sekhmet/detail/service.hpp"
#include "sekhmet/detail/uuid.hpp"
#include "sekhmet/system/native_file.hpp"

namespace sek::engine
{
	class asset_source;

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

		struct package_fragment;
		struct master_package;

		struct asset_info
		{
			struct loose_info
			{
				std::string asset_path; /* Path to asset's main data file. */
				std::string meta_path;	/* Path to asset's metadata file. */
			};
			struct archive_info
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

		private:
			asset_info(package_fragment *parent, interned_string name) : parent(parent), name(std::move(name)) {}

		public:
			asset_info(type_selector_t<archive_info>, package_fragment *parent, interned_string name)
				: asset_info(parent, std::move(name))
			{
			}
			asset_info(type_selector_t<loose_info>, package_fragment *parent, interned_string name)
				: asset_info(parent, std::move(name))
			{
				std::construct_at(&loose);
			}
			~asset_info() {}

			package_fragment *parent = nullptr; /* Parent fragment of the asset. */

			interned_string name;			 /* Optional human-readable name of the asset. */
			dense_set<interned_string> tags; /* Optional tags of the asset. */

			union
			{
				archive_info archive = {};
				loose_info loose;
			};
		};
		struct asset_database
		{
			dense_map<uuid, asset_info *> uuid_table;
			dense_map<std::string_view, uuid> name_table;
		};
		struct package_fragment
		{
			struct pack_vtable_t
			{
				void (*acquire_func)(package_fragment *);
				void (*release_func)(package_fragment *);
			};
			struct asset_vtable_t
			{
				std::vector<std::byte> (*meta_load_func)(const package_fragment *, const asset_info *);
				asset_source (*asset_open_func)(const package_fragment *, const asset_info *);
				void (*asset_dtor_func)(asset_info *); /* Function used to destroy an asset in-place. */
			};

			~package_fragment()
			{
				// clang-format off
				for (auto entry : assets)
					destroy_asset(entry);
				// clang-format on
			}

			void acquire();
			void release();

			asset_source open_asset(const asset_info *) const;
			std::vector<std::byte> load_meta(const asset_info *) const;
			void destroy_asset(asset_info *) const;

			union
			{
				master_package *master = nullptr;
				std::atomic<std::size_t> ref_count;
			};

			const pack_vtable_t *pack_vtable;	/* Vtable used for master/fragment operations. */
			const asset_vtable_t *asset_vtable; /* Vtable used for loose/archive/compressed operations. */

			std::filesystem::path path;		/* Path to fragment's directory or archive file. */
			dense_set<asset_info *> assets; /* Assets of this fragment. */
		};
		struct master_package : package_fragment
		{
			std::vector<package_fragment> fragments; /* Fragments of this package. */
			asset_database db;						 /* Database containing assets of all fragments. */
		};

		inline asset_source make_asset_source(system::native_file &&file, std::int64_t size, std::int64_t offset);
		inline asset_source make_asset_source(asset_buffer_t &&buff, std::int64_t size, std::int64_t offset);
	}	 // namespace detail

	/** @brief Structure used to represent a data source of an asset.
	 *
	 * Since assets may be either loose or compressed and archived, a special structure is needed to read asset data.
	 * In addition, to allow for implementation of storage optimization techniques (such as DirectStorage),
	 * streams cannot be used directly either, as access to the underlying file or data buffer is needed. */
	class asset_source
	{
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
		std::int64_t seek(std::int64_t off, system::native_file::seek_dir dir)
		{
			if (empty()) [[unlikely]]
				return off == 0 ? 0 : -1;
			else
			{
				if (dir == system::native_file::beg)
					return seek_pos(base_offset() + off);
				else if (dir == system::native_file::cur)
					return seek_pos(read_pos + off);
				else if (dir == system::native_file::end)
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
				const auto file_pos = file().seek(new_pos, system::native_file::beg);
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
	}	 // namespace detail
}	 // namespace sek::engine
