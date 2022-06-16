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

#include <fstream>

#include "logger.hpp"
#include "sekhmet/serialization/binary.hpp"
#include "sekhmet/serialization/json.hpp"
#include "zstd_ctx.hpp"

namespace sek::engine
{
	using namespace sek::serialization;

	inline std::string format_asset_name(const detail::asset_info *info, uuid id)
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
		 * File offsets                     Description
		 * 0x0000  -  0x0007                Signature ("\3SEKPAK" + version byte)
		 * 0x0008  -  0x000b                Header flags (master, compression type, etc.)
		 * 0x000c  -  0x000f                Number of assets of the package (may be 0)
		 * 0x0010  -  end_assets            Asset info for every asset
		 * ======================== Master package header data ========================
		 * n_frags + 0 - n_frags + 4        Number of fragments of the package (may be 0)
		 * n_frags + 5 - end_frags          Null-terminated file names of fragments
		 * =============================== Header flags ===============================
		 * Description           Bit(s)      Values
		 * Master flag             0           0   - Fragment
		 *                                     1   - Master
		 * Archive flag            1           0   - Loose package
		 *                                     1   - Archive package
		 * Project flag            2           0   - Not a project (all runtime packages)
		 *                                     1   - Editor project (editor-managed loose packages)
		 * Compression format     3-6          0   - Not used (used for non-archive packages)
		 *                                     1   - No compression
		 *                                     2   - ZSTD compression
		 *                                    3-15 - Reserved
		 * Reserved               7-31         0
		 * =============================== Asset entry ================================
		 * Entry offsets                  Description
		 * 0x00 - 0x0f                    Asset UUID
		 * 0x10 - 0x17                    Asset data offset
		 * 0x18 - 0x1f                    Asset data size (compressed)
		 * 0x20 - 0x27                    Asset source size (decompressed)
		 * 0x28 - 0x2f                    Asset frame count (Always 0 if not compressed)
		 * 0x30 - 0x37                    Asset metadata offset
		 * 0x38 - 0x3f                    Asset metadata size (never compressed)
		 * 0x40 - end_name                Null-terminated name string (optional)
		 * n_tags - n_tags + 4            Number of asset tags (may be 0)
		 * n_tags + 5 - end_tags          Null-terminated asset tag strings
		 * ============================================================================
		 * */

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

		inline thread_pool &asset_zstd_pool()
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
					fmt::format("Failed to seek asset package \"{}\" to position {}", path.string(), offset));
			}
			return file;
		}
		std::vector<std::byte> package_fragment::read_metadata(const asset_info *info) const
		{
			if (!info->has_metadata()) [[unlikely]]
				return {};

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
						"Mismatched asset frame count - expected {} but got {}. This might be a sign of corruption", frames, result);
				}
			}
			catch (zstd_error &e)
			{
				throw asset_package_error(fmt::format(R"(Exception in "zstd_thread_ctx::decompress": "{}")", e.what()));
			}

			return make_asset_source(std::move(writer.buffer), src_size);
		}
		asset_source package_fragment::open_asset(const asset_info *info) const
		{
			switch (flags & (ARCHIVE_FORMAT_MASK | IS_ARCHIVE))
			{
				case 0: return open_asset_loose(info);
				case IS_ARCHIVE: return open_asset_flat(info);
				case IS_ARCHIVE | ARCHIVE_FORMAT_ZSTD: return open_asset_zstd(info);
				default: throw asset_package_error("Failed to open asset source - invalid package flags");
			}
		}

		void master_package::insert_asset(uuid id, asset_info *info)
		{
			if (auto existing = uuid_table.find(id); existing != uuid_table.end())
			{
				auto old_info = existing->second;
				existing->second = info;

				if (auto &old_name = old_info->name; !old_name.empty()) [[likely]]
				{
					/* If the old UUID asset has a name, and that name points to the old asset's UUID,
					 * remove the old name entry. Name entries are not removed if they point to a different asset. */
					auto existing_name = name_table.find(old_name);
					if (existing_name != name_table.end() && existing_name->second.first == existing->first) [[likely]]
						name_table.erase(existing_name);
				}

				/* Destroy & de-allocate the old UUID asset. */
				std::destroy_at(old_info);
				dealloc_info(old_info);
			}
			else
				uuid_table.emplace(id, info);

			/* If the new asset has a name, add or replace the name entry. */
			if (!info->name.empty()) [[likely]]
			{
				if (auto existing = name_table.find(info->name); existing != name_table.end())
					existing->second = {id, info};
				else
				{
					// clang-format off
					name_table.emplace(std::piecewise_construct,
									   std::forward_as_tuple(info->name),
									   std::forward_as_tuple(id, info));
					// clang-format on
				}
			}
		}

		constexpr std::array<char, 7> signature_str = {'\3', 'S', 'E', 'K', 'P', 'A', 'K'};
		constexpr std::uint8_t manifest_ver_max = 1;
		constexpr std::uint8_t archive_ver_max = 1;

		using json_frame = typename json::input_archive::archive_frame;
		using binary_archive = binary::input_archive;
		using flags_t = package_fragment::flags_t;

		inline auto open_manifest(const std::filesystem::path &path)
		{
			const auto manifest_path = path / MANIFEST_FILE_NAME;
			if (!exists(manifest_path) && !is_regular_file(manifest_path)) [[unlikely]]
				goto invalid_manifest;

			{
				system::native_file file(manifest_path, system::native_file::in);
				if (!file.is_open()) [[unlikely]]
					goto invalid_manifest;
				return json::input_archive{file};
			}

		invalid_manifest:
			throw asset_package_error(fmt::format("Failed to open package manifest at \"{}\"", manifest_path.string()));
		}
		inline auto open_header(const std::filesystem::path &path)
		{
			if (!exists(path) && !is_regular_file(path)) [[unlikely]]
				goto invalid_header;

			{
				auto file = new system::native_file(path, system::native_file::in);
				if (!file->is_open()) [[unlikely]]
					goto invalid_header;

				std::pair<std::unique_ptr<system::native_file>, binary_archive> result = {
					std::unique_ptr<system::native_file>{file},
					binary_archive{*file},
				};
				return result;
			}

		invalid_header:
			throw asset_package_error(fmt::format("Failed to open archive package at \"{}\"", path.string()));
		}
		inline std::uint8_t get_manifest_version(typename json::input_archive::archive_frame &frame)
		{
			std::uint8_t ver = 0;
			if (!(frame.try_read(keyed_entry("version", ver)) && ver - 1 < manifest_ver_max)) [[unlikely]]
				throw asset_package_error("Unknown manifest version");
			return ver;
		}
		inline std::uint8_t get_header_version(binary::input_archive &archive)
		{
			for (auto c_req : signature_str)
			{
				if (char c; !(archive.try_read(c) && c == c_req)) [[unlikely]]
					throw asset_package_error("Invalid header signature");
			}
			std::uint8_t ver = 0;
			if (!(archive.try_read(ver) && ver - 1 < archive_ver_max)) [[unlikely]]
				throw asset_package_error("Unknown header version");
			return ver;
		}
		inline auto get_header_flags(binary::input_archive &archive)
		{
			std::int32_t value;
			if (!archive.try_read(value)) [[unlikely]]
				throw asset_package_error("Invalid header flags");
			return static_cast<flags_t>(value);
		}

		inline void deserialize(package_fragment &, json_frame &, master_package &);
		inline void deserialize(master_package &, json_frame &);
		inline void deserialize(package_fragment &, binary_archive &, master_package &);
		inline void deserialize(master_package &, binary_archive &);

		namespace v1
		{
			struct fragments_proxy
			{
				std::filesystem::path full_path(json_frame &archive)
				{
					return master.path.parent_path() / archive.read(std::in_place_type<std::string_view>);
				}
				std::filesystem::path full_path(binary_archive &archive)
				{
					return master.path.parent_path() / archive.read(std::in_place_type<std::string>);
				}
				void read_fragments(auto &archive, std::size_t n)
				{
					logger::info() << "Loading fragments...";

					master.fragments.reserve(n);

					std::size_t i = 0, total = 0;
					for (std::unique_ptr<package_fragment> frag; i < n; ++i)
					{
						auto path = full_path(archive);
						if (!exists(path))
							logger::warn() << fmt::format("Ignoring invalid fragment path \"{}\"", path.string());
						else
						{
							/* Allocate & construct next fragment. */
							frag = std::make_unique<package_fragment>(&master, package_fragment::NO_FLAGS, std::move(path));

							try
							{
								if (is_directory(frag->path))
									detail::open_manifest(frag->path).read(*frag, master);
								else
									detail::open_header(frag->path).second.read(*frag, master);

								master.fragments.emplace_back(std::move(frag));
								++total;
							}
							catch (asset_package_error &e)
							{
								logger::error() << fmt::format("Failed to load fragment at path \"{}\"."
															   " Got exception: \"{}\". Skipping...",
															   frag->path.string(),
															   e.what());

								/* Reset invalid fragment & continue on failure.
								 * We still want to load any valid fragments. */
								frag.reset();
								continue;
							}
						}
					}

					logger::info() << fmt::format("Loaded {} fragment(s)", total);
				}

				void deserialize(json_frame &archive)
				{
					std::size_t size = 0;
					archive >> container_size(size);
					read_fragments(archive, size);
				}
				void deserialize(binary_archive &archive)
				{
					read_fragments(archive, archive.read(std::in_place_type<std::uint32_t>));
				}

				master_package &master;
			};
			struct assets_proxy
			{
				struct info_deleter
				{
					inline void operator()(asset_info *ptr) const { master.dealloc_info(ptr); }
					master_package &master;
				};
				struct info_proxy
				{
					struct tags_proxy
					{
						void deserialize(json_frame &archive)
						{
							std::size_t size = 0;
							archive >> container_size(size);
							tags.reserve(size);
							while (size-- != 0) tags.emplace(archive.read(std::in_place_type<std::string_view>));
						}
						void deserialize(binary_archive &archive)
						{
							auto size = archive.read(std::in_place_type<std::uint32_t>);
							tags.reserve(size);
							while (size-- != 0) tags.emplace(archive.read(std::in_place_type<std::string>));
						}

						dense_set<interned_string> &tags;
					};

					void deserialize(json_frame &archive, package_fragment &parent)
					{
						std::construct_at(info, type_selector<asset_info::loose_info_t>, &parent);

						for (auto iter = archive.begin(), end = archive.end(); iter != end; ++iter)
						{
							const auto key = iter.key();
							if (key == "name")
								info->name = iter->read(std::in_place_type<std::string_view>);
							else if (key == "tags")
								iter->read(tags_proxy{info->tags});
							else if (key == "data")
								info->loose_info.asset_path = iter->read(std::in_place_type<std::string_view>);
							else if (key == "metadata")
								info->loose_info.meta_path = iter->read(std::in_place_type<std::string_view>);
						}

						if (info->loose_info.asset_path.empty()) [[unlikely]]
							throw archive_error("Missing asset data path");
					}
					void deserialize(binary_archive &archive, package_fragment &parent)
					{
						std::construct_at(info, type_selector<asset_info::archive_info_t>, &parent);

						archive >> info->archive_info.asset_offset;
						archive >> info->archive_info.asset_size;
						archive >> info->archive_info.asset_src_size;
						archive >> info->archive_info.asset_frames;
						archive >> info->archive_info.meta_offset;
						archive >> info->archive_info.meta_size;

						info->name = archive.read(std::in_place_type<std::string>);
						archive.read(tags_proxy{info->tags});

						if (info->archive_info.asset_offset == 0) [[unlikely]]
							throw archive_error("Invalid asset data offset");
					}

					asset_info *info;
				};

				uuid read_entry(std::size_t i, json_frame &archive, asset_info *info)
				{
					const auto iter = archive.begin() + static_cast<std::ptrdiff_t>(i);
					const auto id = uuid{iter.key()};

					iter->read(info_proxy{info}, pkg);
					return id;
				}
				uuid read_entry(std::size_t, binary_archive &archive, asset_info *info)
				{
					std::array<std::byte, 16> bytes = {};
					for (auto &byte : bytes) byte = std::byte{archive.read(std::in_place_type<std::uint8_t>)};

					archive.read(info_proxy{info}, pkg);
					return uuid{bytes};
				}
				void read_assets(auto &archive, master_package &master, std::size_t n)
				{
					logger::info() << "Loading assets...";

					master.uuid_table.reserve(n);
					master.name_table.reserve(n);

					auto next_info = std::unique_ptr<asset_info, info_deleter>{nullptr, info_deleter{master}};
					std::size_t total = 0;
					for (std::size_t i = 0; i < n; ++i)
					{
						/* Allocate new asset before deserialization. */
						if (!next_info) [[likely]]
							next_info.reset(master.alloc_info());

						try
						{
							const auto id = read_entry(i, archive, next_info.get());
							master.insert_asset(id, next_info.release());
							++total;
						}
						catch (archive_error &e)
						{
							logger::error() << fmt::format("Ignoring malformed asset entry. Parse error: \"{}\"", e.what());

							/* Destroy the asset, as it will be constructed by the deserialization function on next iteration. */
							std::destroy_at(next_info.get());
							continue;
						}
					}

					logger::info() << fmt::format("Loaded {} asset(s)", total);
				}

				void deserialize(json_frame &archive, master_package &master)
				{
					std::size_t size = 0;
					archive >> container_size(size);
					read_assets(archive, master, size);
				}
				void deserialize(binary_archive &archive, master_package &master)
				{
					read_assets(archive, master, archive.read(std::in_place_type<std::uint32_t>));
				}

				package_fragment &pkg;
			};

			inline void deserialize(package_fragment &pkg, json_frame &frame, master_package &master)
			{
				logger::info() << fmt::format("Loading v1 loose package \"{}\"", relative(pkg.path).string());

				/* Try to deserialize assets. */
				frame.try_read(keyed_entry("assets", assets_proxy{pkg}), master);
			}
			inline void deserialize(master_package &pkg, json_frame &frame)
			{
				/* Try to deserialize assets & fragments. */
				v1::deserialize(pkg, frame, pkg);
				frame.try_read(keyed_entry("fragments", fragments_proxy{pkg}));
			}

			inline void deserialize(package_fragment &pkg, binary_archive &archive, flags_t flags, master_package &master)
			{
				pkg.flags = static_cast<flags_t>(flags | package_fragment::IS_ARCHIVE);
				logger::info() << fmt::format("Loading v1 archive package (compression: {}) \"{}\"",
											  pkg.is_archive_zstd() ? "ZSTD" : "none",
											  relative(pkg.path).string());

				/* Try to deserialize assets. */
				archive.try_read(assets_proxy{pkg}, master);
			}
			inline void deserialize(master_package &pkg, binary_archive &archive)
			{
				const auto flags = get_header_flags(archive);
				if (!(flags & package_fragment::IS_MASTER)) [[unlikely]]
					throw asset_package_error("Not a master package");

				/* Try to deserialize assets & fragments. */
				deserialize(pkg, archive, flags, pkg);
				archive.try_read(fragments_proxy{pkg});
			}
		}	 // namespace v1

		inline void deserialize(package_fragment &pkg, json_frame &frame, master_package &master)
		{
			switch (get_manifest_version(frame))
			{
				case 1: v1::deserialize(pkg, frame, master); break;
			}
		}
		inline void deserialize(master_package &pkg, json_frame &frame)
		{
			if (bool flag = false; !(frame.try_read(keyed_entry("is_master", flag)) && flag)) [[unlikely]]
				throw asset_package_error("Not a master package");

			switch (get_manifest_version(frame))
			{
				case 1: v1::deserialize(pkg, frame); break;
			}
		}

		inline void deserialize(package_fragment &pkg, binary_archive &archive, master_package &master)
		{
			switch (get_header_version(archive))
			{
				case 1: v1::deserialize(pkg, archive, get_header_flags(archive), master); break;
			}
		}
		inline void deserialize(master_package &pkg, binary_archive &archive)
		{
			switch (get_header_version(archive))
			{
				case 1: v1::deserialize(pkg, archive); break;
			}
		}
	}	 // namespace detail

	asset_package::asset_package(detail::master_package *pkg) : m_ptr(pkg) { m_ptr.acquire(); }

	std::vector<asset_package> asset_package::load_all(const std::filesystem::path &path)
	{
		if (!exists(path) && !is_directory(path)) [[unlikely]]
			throw asset_package_error(fmt::format("\"{}\" is not a valid directory", path.string()));

		std::vector<asset_package> result;
		{
			logger::info() << fmt::format("Loading packages in directory \"{}\"", path.string());
			for (auto &entry : std::filesystem::directory_iterator(path)) try
				{
					result.emplace_back(load(entry.path()));
				}
				catch (asset_package_error &e)
				{
					logger::info() << fmt::format("Skipping invalid package path \"{}\"."
												  " Reason: \"{}\"",
												  entry.path().string(),
												  e.what());
				}
			logger::info() << fmt::format("Loaded {} packages", result.size());
		}
		return result;
	}
	asset_package asset_package::load(const std::filesystem::path &path)
	{
		if (!exists(path)) [[unlikely]]
			throw asset_package_error(fmt::format("\"{}\" is not a valid package path", path.string()));

		auto *result = new detail::master_package(std::filesystem::path{path});
		try
		{
			if (is_directory(path))
				detail::open_manifest(path).read(*result);
			else
				detail::open_header(path).second.read(*result);
		}
		catch (...)
		{
			delete result;
			throw;
		}
		return asset_package{result};
	}

}	 // namespace sek::engine