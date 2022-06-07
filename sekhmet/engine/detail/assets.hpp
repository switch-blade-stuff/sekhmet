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

#include "sekhmet/detail/dense_map.hpp"
#include "sekhmet/detail/dense_set.hpp"
#include "sekhmet/detail/intern.hpp"
#include "sekhmet/detail/basic_pool.hpp"
#include "sekhmet/detail/service.hpp"
#include "sekhmet/detail/uuid.hpp"

namespace sek::engine
{
	class asset_source;

	namespace detail
	{
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
				std::uint64_t asset_offset;	  /* Offset into the archive at which asset's data is located. */
				std::uint64_t asset_size;	  /* Size of the data within the archive. */
				std::uint64_t asset_src_size; /* Decompressed size of the asset if any compression is used. */
				std::uint64_t asset_frames;	  /* Amount of compressed frames used (0 if not compressed). */

				std::uint64_t meta_offset; /* Offset into the archive at which asset's metadata is located. */
				std::uint64_t meta_size;   /* Size of the asset metadata within the archive. */
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
			void construct_asset(asset_info *) const;
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
	}	 // namespace detail

	/** @brief Structure used to represent a data source of an asset.
	 *
	 * Since assets may be either loose or compressed and archived, a special structure is needed to read asset data.
	 * In addition, to allow for implementation of optimization techniques (such as DirectStorage),
	 * streams cannot be used directly either, as access to the underlying file or data buffer is needed. */
	class asset_source
	{
	};
}	 // namespace sek::engine
