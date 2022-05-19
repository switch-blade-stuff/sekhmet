//
// Created by switchblade on 2022-04-04.
//

#pragma once

#include <atomic>
#include <filesystem>

#include "assert.hpp"
#include "dense_map.hpp"
#include "filemap.hpp"
#include "intern.hpp"
#include "uuid.hpp"
#include <memory_resource>

namespace sek
{
	namespace detail
	{
		struct package_base;

		struct asset_info_base
		{
			asset_info_base() noexcept = default;
			asset_info_base(const asset_info_base &) noexcept = default;
			asset_info_base(asset_info_base &&) noexcept = default;

			/* Parent fragment of the asset. */
			package_base *parent = nullptr;
			/* Optional name of the asset. */
			interned_string name;
		};
		struct loose_asset_info final : asset_info_base
		{
			loose_asset_info() noexcept = default;
			loose_asset_info(const loose_asset_info &) = default;
			loose_asset_info(loose_asset_info &&) noexcept = default;

			/* Path of the asset file within a loose package. */
			std::filesystem::path file;
		};
		struct archive_asset_info final : asset_info_base
		{
			archive_asset_info() noexcept = default;
			archive_asset_info(const archive_asset_info &) = default;
			archive_asset_info(archive_asset_info &&) noexcept = default;

			/* Position & size of the asset within an archive. */
			std::pair<std::ptrdiff_t, std::size_t> slice = {};
		};

		struct master_package;
		struct fragment_package;
		struct package_base
		{
			enum flags_t : int
			{
				NO_FLAGS = 0,
				IS_MASTER = 1,
				IS_ARCHIVE = 2,
			};

			[[nodiscard]] constexpr auto *as_master() noexcept;
			[[nodiscard]] constexpr auto *as_fragment() noexcept;

			[[nodiscard]] constexpr bool is_master() const noexcept { return flags & IS_MASTER; }
			[[nodiscard]] constexpr master_package *master() noexcept;

			SEK_API void acquire();
			SEK_API void release();

			std::filesystem::path path;
			flags_t flags;
		};
		struct fragment_package : package_base
		{
			fragment_package() = delete;
			explicit fragment_package(master_package *master) : master_ptr(master) {}

			master_package *master_ptr;
		};
		struct master_package : package_base
		{
			template<std::derived_from<asset_info_base> T>
			T *alloc_asset_info()
			{
				return static_cast<T *>(info_pool.allocate(sizeof(T), alignof(T)));
			}

			std::pmr::unsynchronized_pool_resource info_pool = {};

			/* Asset infos are stored by-reference to allow for pool allocation & keep pointers stable.
			 * Multi-key maps are not used since asset names are optional, UUIDs are used as primary keys
			 * and should be preferred instead. */
			dense_map<uuid, asset_info_base *> assets;
			dense_map<std::string_view, uuid> name_table;

			std::vector<fragment_package> fragments;
			std::atomic<std::size_t> ref_count;
		};

		constexpr auto *package_base::as_master() noexcept { return static_cast<master_package *>(this); }
		constexpr auto *package_base::as_fragment() noexcept { return static_cast<fragment_package *>(this); }

		constexpr master_package *package_base::master() noexcept
		{
			return is_master() ? as_master() : as_fragment()->master_ptr;
		}
	}	 // namespace detail
}	 // namespace sek
