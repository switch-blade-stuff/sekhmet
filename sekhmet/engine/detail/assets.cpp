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

#ifdef SEK_OS_WIN
#define _CRT_SECURE_NO_WARNINGS	   // NOLINT
#define MANIFEST_FILE_NAME L".manifest"
#else
#define MANIFEST_FILE_NAME ".manifest"
#endif

#include "assets.hpp"

#include "logger.hpp"
#include "sekhmet/serialization/json.hpp"
#include "sekhmet/serialization/ubjson.hpp"
#include "zstd_ctx.hpp"

namespace sek::engine
{
	inline static std::string format_asset_name(const detail::asset_info *info, uuid id)
	{
		char id_str[37] = {0};
		id.to_string(id_str);
		return fmt::format(R"("{}" {{{}}})", info->name.sv(), id_str);
	}

	namespace detail
	{
		/*
		 * Archive header format:
		 * ============================================================================
		 * =                                    V1                                    =
		 * ============================================================================
		 *    File offsets                  Description
		 * 0x0000  -  0x0007                Signature ("\3SEKPAK" + version byte)
		 * 0x0008  -  0x000b                Header flags (master, compression type, etc.)
		 * 0x000c  -  0x000f                Total header size
		 * 0x0010  -  0x0013                CRC32 of the header
		 * 0x0014  -  0x0017                Number of assets of the package
		 * 0x0018  -  end_assets            Asset info for every asset
		 * ======================== Master package header data ========================
		 * end_assets - end_assets + 4      Number of fragments (if any) of the package
		 * end_assets + 5 - header_end      File names of fragments
		 * =============================== Header flags ===============================
		 * Description           Bit(s)      Values
		 * Master flag             0         0 - Fragment
		 *                                   1 - Master
		 * Compression format     1-3        0 - No compression
		 *                                   1 - ZSTD
		 * Reserved               4-31
		 * ============================================================================
		 * */

		enum header_flags : std::int32_t
		{
			MASTER = 1,
			FORMAT_ZSTD = 0b0010,
			FORMAT_MASK = 0b1110,
		};

		constexpr std::array<char, 7> signature_str = {'\3', 'S', 'E', 'K', 'P', 'A', 'K'};
		constexpr std::size_t signature_size_min = signature_str.size() + 1;

		master_package::~master_package()
		{
			for (auto entry : asset_table::uuid_table) entry.second->destroy();
		}

		inline static void acquire_master(master_package *ptr) noexcept { ++ptr->ref_count; }
		inline static void release_master(master_package *ptr)
		{
			if (ptr->ref_count.fetch_sub(1) == 1) [[unlikely]]
				delete ptr;
		}

		constinit const typename package_fragment::pack_vtable_t package_fragment::fragment_vtable = {
			.acquire_func = +[](package_fragment *ptr) -> void { acquire_master(ptr->master); },
			.release_func = +[](package_fragment *ptr) -> void { release_master(ptr->master); },
		};
		constinit const typename package_fragment::pack_vtable_t package_fragment::master_vtable = {
			.acquire_func = +[](package_fragment *ptr) -> void { acquire_master(static_cast<master_package *>(ptr)); },
			.release_func = +[](package_fragment *ptr) -> void { release_master(static_cast<master_package *>(ptr)); },
		};

		constinit const typename package_fragment::asset_vtable_t package_fragment::loose_vtable = {
			.meta_load_func = +[](const package_fragment *frag, const asset_info *info) -> std::vector<std::byte>
			{
				auto full_path = frag->path / info->loose.meta_path;
				auto file = system::native_file{full_path, system::native_file::in | system::native_file::atend};
				if (!file.is_open()) [[unlikely]]
					throw asset_package_error(fmt::format("Failed to open asset metadata at path \"{}\"", full_path.string()));

				std::vector<std::byte> result;
				result.resize(static_cast<std::size_t>(file.tell()));
				file.seek(0, system::native_file::beg);
				file.read(result.data(), result.size());
				return result;
			},
			.asset_open_func = +[](const package_fragment *frag, const asset_info *info) -> asset_source
			{
				auto full_path = frag->path / info->loose.asset_path;
				auto file = system::native_file{full_path, system::native_file::in | system::native_file::atend};
				if (!file.is_open()) [[unlikely]]
					throw asset_package_error(fmt::format("Failed to open asset at path \"{}\"", full_path.string()));

				const auto size = file.tell();
				file.seek(0, system::native_file::beg);
				return make_asset_source(std::move(file), size, 0);
			},
		};

		inline static thread_pool &asset_zstd_pool()
		{
			static thread_pool instance;
			return instance;
		}

		inline static system::native_file open_fragment(const std::filesystem::path &path, std::int64_t offset)
		{
			auto file = system::native_file{path, system::native_file::in};
			if (!file.is_open()) [[unlikely]]
				throw asset_package_error(fmt::format("Failed to open asset package \"{}\"", path.string()));
			else if (file.seek(offset, system::native_file::beg) < 0) [[unlikely]]
			{
				throw asset_package_error(
					fmt::format("Failed to seek asset package \"{}\" to position [{}]", path.string(), offset));
			}
			return file;
		}
		inline static std::vector<std::byte> load_archive_metadata(const package_fragment *frag, const asset_info *info)
		{
			std::vector<std::byte> result;

			auto file = open_fragment(frag->path, info->archive.meta_offset);
			result.resize(static_cast<std::size_t>(info->archive.meta_size));
			file.read(result.data(), result.size());
			return result;
		}
		inline static asset_source load_archive_asset(const package_fragment *frag, const asset_info *info)
		{
			const auto offset = info->archive.asset_offset;
			const auto size = info->archive.asset_size;
			return make_asset_source(open_fragment(frag->path, offset), size, offset);
		}
		inline static asset_source load_zstd_asset(const package_fragment *frag, const asset_info *info)
		{
			auto &ctx = zstd_thread_ctx::instance();

			const auto src_size = info->archive.asset_src_size;
			const auto frames = info->archive.asset_frames;
			const auto offset = info->archive.asset_offset;
			const auto size = info->archive.asset_size;

			struct reader_t
			{
				std::size_t read(void *dst, std::size_t n)
				{
					auto new_pos = pos + n;
					if (new_pos > size) [[unlikely]]
					{
						new_pos = size;
						n = size - pos;
					}

					pos = new_pos;
					return file.read(dst, n);
				}

				system::native_file file;
				std::size_t size;
				std::size_t pos;
			} reader = {open_fragment(frag->path, offset), static_cast<std::size_t>(size), 0};
			struct writer_t
			{
				std::size_t write(const void *src, std::size_t n)
				{
					auto new_pos = pos + n;
					if (new_pos > size) [[unlikely]]
					{
						new_pos = size;
						n = size - pos;
					}

					memcpy(buffer.data + std::exchange(pos, new_pos), src, n);
					return n;
				}

				asset_buffer_t buffer;
				std::size_t size;
				std::size_t pos;
			} writer = {asset_buffer_t{src_size}, static_cast<std::size_t>(src_size), 0};

			try
			{
				auto result = ctx.decompress(asset_zstd_pool(),
											 delegate{func_t<&reader_t::read>{}, reader},
											 delegate{func_t<&writer_t::write>{}, writer},
											 frames);
				if (result != frames) [[unlikely]]
				{
					/* Mismatched frame count does not necessarily mean an error (data might be corrupted but that is up to the consumer to decide). */
					logger::warn() << fmt::format(
						"Mismatched asset frame count - expected [{}] but got [{}]. This might be a sign of corruption",
						frames,
						result);
				}
			}
			catch (zstd_error &e)
			{
				throw asset_package_error(fmt::format(R"(Exception in "zstd_thread_ctx::decompress": "{}")", e.what()));
			}

			return make_asset_source(std::move(writer.buffer), src_size, 0);
		}

		constinit const typename package_fragment::asset_vtable_t package_fragment::archive_vtable = {
			.meta_load_func = load_archive_metadata,
			.asset_open_func = load_archive_asset,
		};
		constinit const typename package_fragment::asset_vtable_t package_fragment::zstd_vtable = {
			.meta_load_func = load_archive_metadata,
			.asset_open_func = load_zstd_asset,
		};
	}	 // namespace detail

	asset_handle::asset_handle(uuid id, detail::asset_info *ptr) : asset_id(std::move(id)), asset_ptr(ptr)
	{
		asset_ptr.acquire();
	}
	package_handle::package_handle(detail::package_fragment *ptr) : fragment_ptr(ptr) { fragment_ptr.acquire(); }
}	 // namespace sek::engine