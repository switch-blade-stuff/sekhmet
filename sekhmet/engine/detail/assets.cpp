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

#define _CRT_SECURE_NO_WARNINGS

#include "assets.hpp"

#include "logger.hpp"
#include "sekhmet/serialization/json.hpp"
#include "sekhmet/serialization/ubjson.hpp"
#include "zstd_ctx.hpp"

#ifdef SEK_OS_WIN
#define MANIFEST_FILE_NAME L".manifest"
#define FILE_OPEN _wfopen
#else
#define MANIFEST_FILE_NAME ".manifest"
#define FILE_OPEN fopen
#endif

namespace sek::engine
{
	namespace detail
	{
		constexpr static std::array<char, 8> signature = {'\3', 'S', 'E', 'K', 'P', 'A', 'K', '\0'};
		constexpr static std::size_t version_pos = 7;

		constexpr std::array<char, 8> make_signature(std::uint8_t ver) noexcept
		{
			auto result = signature;
			result[version_pos] = static_cast<char>(ver);
			return result;
		}
		constexpr std::uint8_t check_signature(std::array<char, 8> data) noexcept
		{
			if (std::equal(signature.begin(), signature.begin() + version_pos, data.begin())) [[likely]]
				return static_cast<std::uint8_t>(data[version_pos]);
			else
				return 0;
		}

		void package_fragment::acquire() { pack_vtable->acquire_func(this); }
		void package_fragment::release() { pack_vtable->release_func(this); }

		asset_source package_fragment::open_asset(const asset_info *info) const
		{
			return asset_vtable->asset_open_func(this, info);
		}
		std::vector<std::byte> package_fragment::load_meta(const asset_info *info) const
		{
			return asset_vtable->meta_load_func(this, info);
		}
		void package_fragment::destroy_asset(asset_info *info) const { asset_vtable->asset_dtor_func(info); }

		inline static void acquire_master(master_package *ptr) noexcept { ++ptr->ref_count; }
		inline static void release_master(master_package *ptr)
		{
			if (ptr->ref_count.fetch_sub(1) == 1) [[unlikely]]
				delete ptr;
		}

		constinit const typename package_fragment::pack_vtable_t master_vtable = {
			.acquire_func = +[](package_fragment *ptr) -> void { acquire_master(static_cast<master_package *>(ptr)); },
			.release_func = +[](package_fragment *ptr) -> void { release_master(static_cast<master_package *>(ptr)); },
		};
		constinit const typename package_fragment::pack_vtable_t fragment_vtable = {
			.acquire_func = +[](package_fragment *ptr) -> void { acquire_master(ptr->master); },
			.release_func = +[](package_fragment *ptr) -> void { release_master(ptr->master); },
		};

		constinit const typename package_fragment::asset_vtable_t loose_vtable = {
			.meta_load_func = +[](const package_fragment *frag, const asset_info *info) -> std::vector<std::byte>
			{
				auto full_path = frag->path / info->loose.meta_path;
				if (FILE * file; exists(full_path) && (file = FILE_OPEN(full_path.c_str(), "rb")) != nullptr) [[likely]]
				{
					std::vector<std::byte> result;

					try
					{
						fseek(file, 0, SEEK_END);
						result.resize(static_cast<std::size_t>(ftell(file)));
						fseek(file, 0, SEEK_SET);

						fread(result.data(), 1, result.size(), file);
						fclose(file);
					}
					catch (...)
					{
						fclose(file);
						throw;
					}

					return result;
				}

				logger::error() << fmt::format("Failed to open asset metadata at path \"{}\"", full_path.c_str());
				/* TODO: Throw asset error. */
				return {};
			},
			.asset_open_func = +[](const package_fragment *frag, const asset_info *info) -> asset_source
			{
				auto full_path = frag->path / info->loose.asset_path;
				if (FILE * file; exists(full_path) && (file = fopen(full_path.c_str(), "rb")) != nullptr) [[likely]]
					return {/*file*/};

				logger::error() << fmt::format("Failed to open asset at path \"{}\"", full_path.c_str());
				/* TODO: Throw asset error. */
				return {};
			},
			.asset_dtor_func = +[](asset_info *ptr) -> void
			{
				std::destroy_at(&ptr->loose);
				std::destroy_at(ptr);
			},
		};
		constinit const typename package_fragment::asset_vtable_t archive_vtable = {
			.meta_load_func = +[](const package_fragment *, const asset_info *) -> std::vector<std::byte>
			{
				/* TODO: Implement this. */
				return {};
			},
			.asset_open_func = +[](const package_fragment *, const asset_info *) -> asset_source
			{
				/* TODO: Implement this. */
				return {};
			},
			.asset_dtor_func = +[](asset_info *ptr) -> void { std::destroy_at(ptr); },
		};
		constinit const typename package_fragment::asset_vtable_t zstd_vtable = {
			.meta_load_func = +[](const package_fragment *, const asset_info *) -> std::vector<std::byte>
			{
				/* TODO: Implement this. */
				return {};
			},
			.asset_open_func = +[](const package_fragment *, const asset_info *) -> asset_source
			{
				/* TODO: Implement this. */
				return {};
			},
			.asset_dtor_func = +[](asset_info *ptr) -> void { std::destroy_at(ptr); },
		};
	}	 // namespace detail
}	 // namespace sek::engine