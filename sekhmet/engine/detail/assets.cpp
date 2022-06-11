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
		 * Compression format     1-4        1 - No compression
		 *                                   2 - ZSTD
		 * Reserved               4-31
		 * ============================================================================
		 * */

		constexpr std::array<char, 7> signature_str = {'\3', 'S', 'E', 'K', 'P', 'A', 'K'};
		constexpr std::size_t signature_size_min = signature_str.size() + 1;

		master_package *package_fragment::get_master() noexcept
		{
			return is_master() ? static_cast<master_package *>(this) : master;
		}
		void package_fragment::acquire()
		{
			if (is_master())
				static_cast<master_package *>(this)->acquire_impl();
			else
				master->acquire_impl();
		}
		void package_fragment::release()
		{
			if (is_master())
				static_cast<master_package *>(this)->release_impl();
			else
				master->release_impl();
		}

		inline static thread_pool &asset_zstd_pool()
		{
			static thread_pool instance;
			return instance;
		}

		system::native_file package_fragment::open_archive(std::int64_t offset) const
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
		std::vector<std::byte> package_fragment::read_metadata(const asset_info *info) const
		{
			std::vector<std::byte> result;
			if (system::native_file file; is_archive())
			{
				file = open_archive(info->archive_info.meta_offset);
				result.resize(static_cast<std::size_t>(info->archive_info.meta_size));
				file.read(result.data(), result.size());
			}
			else
			{
				const auto full_path = path / info->loose_info.meta_path;
				file = system::native_file{full_path, system::native_file::in | system::native_file::atend};
				if (!file.is_open()) [[unlikely]]
					throw asset_package_error(fmt::format("Failed to open asset metadata at path \"{}\"", full_path.string()));

				result.resize(static_cast<std::size_t>(file.tell()));
				file.seek(0, system::native_file::beg);
				file.read(result.data(), result.size());
			}

			return result;
		}
		asset_source package_fragment::open_asset_loose(const asset_info *info) const
		{
			const auto full_path = path / info->loose_info.asset_path;
			auto file = system::native_file{full_path, system::native_file::in | system::native_file::atend};
			if (!file.is_open()) [[unlikely]]
				throw asset_package_error(fmt::format("Failed to open asset at path \"{}\"", full_path.string()));

			const auto size = file.tell();
			file.seek(0, system::native_file::beg);
			return make_asset_source(std::move(file), size, 0);
		}
		asset_source package_fragment::open_asset_flat(const asset_info *info) const
		{
			const auto offset = info->archive_info.asset_offset;
			const auto size = info->archive_info.asset_size;
			return make_asset_source(open_archive(offset), size, offset);
		}
		asset_source package_fragment::open_asset_zstd(const asset_info *info) const
		{
			auto &ctx = zstd_thread_ctx::instance();

			const auto src_size = info->archive_info.asset_src_size;
			const auto frames = info->archive_info.asset_frames;
			const auto offset = info->archive_info.asset_offset;
			const auto size = info->archive_info.asset_size;

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
				std::size_t size, pos;
			} reader = {open_archive(offset), static_cast<std::size_t>(size), 0};
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
				std::size_t size, pos;
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
		asset_source package_fragment::open_asset(const asset_info *info) const
		{
			switch (flags & ARCHIVE_MASK)
			{
				case 0: return open_asset_loose(info);
				case ARCHIVE_FLAT: return open_asset_flat(info);
				case ARCHIVE_ZSTD: return open_asset_zstd(info);
				default: throw asset_package_error("Failed to open asset source - invalid package flags");
			}
		}
	}	 // namespace detail

	asset_package::asset_package(detail::master_package *pkg) : ptr(pkg) { ptr.acquire(); }
	void asset_package::reset() { ptr.reset(); }

	std::ranges::subrange<typename asset_database::package_iter> asset_database::packages() const
	{
		std::shared_lock<std::shared_mutex> l(mtx);
		return {current_packages.begin(), current_packages.end()};
	}
	void asset_database::add_package(const asset_package &pkg)
	{
		std::unique_lock<std::shared_mutex> l(mtx);
		if (find_package(pkg) == current_packages.end()) [[likely]]
		{
			current_packages.push_back(pkg);
			for (auto entry : pkg.ptr->uuid_table)
			{
				const auto uuid = entry.first;
				const auto info = entry.second;
				auto &new_name = info->name;

				/* Replace existing or add new entry to both uuid & name tables. */
				if (auto iter = uuid_table.find(uuid); iter != uuid_table.end()) [[unlikely]]
					iter->second = info;
				else
					uuid_table.emplace(uuid, info);

				if (!new_name.empty()) [[likely]]
				{
					// clang-format off
					name_table.emplace(std::piecewise_construct,
									   std::forward_as_tuple(new_name),
									   std::forward_as_tuple(uuid, info));
					// clang-format on
				}
			}
		}
	}
	void asset_database::remove_package(const asset_package &pkg)
	{
		std::unique_lock<std::shared_mutex> l(mtx);
		if (auto iter = find_package(pkg); iter != current_packages.end()) [[likely]]
		{
			current_packages.erase(iter);
			for (auto entry : pkg.ptr->uuid_table)
			{
				const auto uuid = entry.first;
				const auto info = entry.second;
				auto &old_name = info->name;

				/* Ignore all conflicting assets that do not have `pkg` as their parent, those belong to other packages.
				 * No need to check if the asset uuid is present within the database, since if the
				 * package is present within the database, it's assets are a subset of the database's assets. */
				auto asset_iter = uuid_table.find(uuid);
				if (asset_iter->second->parent->get_master() != pkg.ptr->get_master()) [[likely]]
					continue;

				auto name_iter = old_name.empty() ? name_table.end() : name_table.find(old_name);
				const auto update_name = name_iter->second.second == info;
				auto need_uuid_entry = true, uuid_replaced = false;
				auto need_name_entry = update_name, name_replaced = false;

				/* Find a replacement for the old UUID and/or name among lower priority packages.
				 * Replacement assets might not be the same for the old UUID and the name. */
				for (auto other_iter = iter; other_iter-- != current_packages.begin();)
				{
					/* Stop once both entries have been replaced. */
					if (!(need_uuid_entry || need_name_entry)) [[unlikely]]
						break;

					/* Find a replacement asset for the old UUID. */
					if (need_uuid_entry)
					{
						auto &other_table = other_iter->ptr->uuid_table;
						if (auto replacement_iter = other_table.find(uuid); replacement_iter != other_table.end())
						{
							asset_iter->second = replacement_iter->second;
							std::swap(need_uuid_entry, uuid_replaced);
						}
					}
					/* Find a replacement asset for the old name. */
					if (need_name_entry)
					{
						auto &other_table = other_iter->ptr->name_table;
						if (auto replacement_iter = other_table.find(old_name); replacement_iter != other_table.end())
						{
							name_iter->second = replacement_iter->second;
							std::swap(need_name_entry, name_replaced);
						}
					}
				}

				/* Erase the old UUID and name entries if they were not replaced. */
				if (!uuid_replaced) uuid_table.erase(asset_iter);
				if (!name_replaced && update_name) name_table.erase(name_iter);
			}
		}
	}
}	 // namespace sek::engine