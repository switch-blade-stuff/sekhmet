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
		constexpr std::uint8_t manifest_ver_max = 1;
		constexpr std::uint8_t archive_ver_max = 1;

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
					fmt::format("Failed to seek asset package \"{}\" to position [{}]", path.string(), offset));
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

		inline json::input_archive open_manifest(const std::filesystem::path &path)
		{
			const auto manifest_path = path / MANIFEST_FILE_NAME;
			if (!exists(manifest_path) && !is_regular_file(manifest_path)) [[unlikely]]
				goto invalid_manifest;

			{
				std::ifstream stream(manifest_path);
				if (!stream.is_open()) [[unlikely]]
					goto invalid_manifest;
				return json::input_archive{stream};
			}

		invalid_manifest:
			throw asset_package_error(fmt::format("Failed to open package manifest at \"{}\"", manifest_path.string()));
		}
		inline std::uint8_t get_manifest_version(auto &frame)
		{
			std::uint8_t ver = 0;
			if (!(frame.try_read(keyed_entry("version", ver)) && ver - 1 < manifest_ver_max)) [[unlikely]]
				throw asset_package_error("Unknown manifest version");
			return ver;
		}

		inline void deserialize(package_fragment &, typename json::input_archive::archive_frame &, master_package &);
		inline void deserialize(master_package &, typename json::input_archive::archive_frame &);

		struct tags_deserialize_proxy
		{
			void deserialize(auto &archive, std::size_t n)
			{
				tags.reserve(n);
				while (n-- != 0) tags.emplace(archive.read(std::in_place_type<std::string_view>));
			}
			void deserialize(auto &archive)
			{
				std::size_t size = 0;
				archive >> container_size(size);
				deserialize(archive, size);
			}

			dense_set<interned_string> &tags;
		};

		inline void deserialize(asset_info *info, typename json::input_archive::archive_frame &frame, package_fragment &parent)
		{
			std::construct_at(info, type_selector<asset_info::loose_info_t>, &parent);

			for (auto iter = frame.begin(), end = frame.end(); iter != end; ++iter)
			{
				const auto key = iter.key();
				if (key == "name")
					info->name = iter->read(std::in_place_type<std::string_view>);
				else if (key == "tags")
					iter->read(tags_deserialize_proxy{info->tags});
				else if (key == "data")
					info->loose_info.asset_path = iter->read(std::in_place_type<std::string_view>);
				else if (key == "metadata")
					info->loose_info.meta_path = iter->read(std::in_place_type<std::string_view>);
			}

			if (!info->loose_info.asset_path.empty()) [[unlikely]]
				throw archive_error("Missing asset data path");
		}
		//		inline void deserialize(asset_info *info, binary::input_archive &frame, package_fragment &parent)
		//		{
		//			std::construct_at(info, type_selector<asset_info::archive_info_t>, &parent);
		//
		//			frame >> info->archive_info.asset_offset;
		//			frame >> info->archive_info.asset_size;
		//			frame >> info->archive_info.asset_src_size;
		//			frame >> info->archive_info.asset_frames;
		//			frame >> info->archive_info.meta_offset;
		//			frame >> info->archive_info.meta_size;
		//
		//			info->name = frame.read(std::in_place_type<std::string_view>);
		//			auto tags_count = frame.read(std::in_place_type<std::size_t>);
		//			if (tags_count != 0) frame.read(tags_deserialize_proxy{info->tags}, tags_count);
		//
		//			if (info->archive_info.asset_offset == 0) [[unlikely]]
		//				throw archive_error("Invalid asset data offset");
		//		}

		struct fragments_deserialize_proxy
		{
			inline void deserialize(auto &archive, std::size_t n)
			{
				logger::info() << "Loading fragments...";

				master.fragments.reserve(n);
				for (std::unique_ptr<package_fragment> frag; n-- != 0;)
				{
					if (std::filesystem::path path = archive.read(std::in_place_type<std::string_view>); !exists(path))
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
							{ /* TODO: Load archive. */
							}
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

						/* Loaded successfully, add to master's list. */
						master.fragments.emplace_back(std::move(frag));
					}
				}
			}
			inline void deserialize(auto &archive)
			{
				std::size_t size = 0;
				archive >> container_size(size);
				deserialize(archive, size);
			}

			master_package &master;
		};
		struct assets_deserialize_proxy
		{
			struct info_deleter
			{
				inline void operator()(asset_info *ptr) const { master.dealloc_info(ptr); }
				master_package &master;
			};

			uuid read_entry(std::size_t i, typename json::input_archive::archive_frame &archive, asset_info *info)
			{
				const auto iter = archive.begin() + static_cast<std::ptrdiff_t>(i);
				const auto id = uuid{iter.key()};

				iter->read(info, pkg);
				return id;
			}
			//			uuid read_entry(std::size_t, binary::input_archive &archive, asset_info *info)
			//			{
			//				const auto id = uuid{archive.read(std::in_place_type<std::string_view>)};
			//				archive.read(info, pkg);
			//				return id;
			//			}

			void deserialize(auto &archive, master_package &master, std::size_t n)
			{
				auto next_info = std::unique_ptr<asset_info, info_deleter>{nullptr, info_deleter{master}};
				for (std::size_t i = 0; i < n; ++i)
				{
					/* Allocate new asset before deserialization. */
					if (!next_info) [[likely]]
						next_info.reset(master.alloc_info());

					try
					{
						auto id = read_entry(i, archive, next_info.get());
						master.uuid_table.emplace(id, next_info.release());
					}
					catch (archive_error &e)
					{
						logger::error() << fmt::format("Ignoring malformed asset entry. Parse error: \"{}\"", e.what());

						/* Destroy the asset, as it will be constructed by the deserialization function on next iteration. */
						std::destroy_at(next_info.get());
						continue;
					}
				}
			}
			void deserialize(auto &archive, master_package &master)
			{
				std::size_t size = 0;
				archive >> container_size(size);
				deserialize(archive, master, size);
			}

			package_fragment &pkg;
		};

		namespace v1
		{
			inline void deserialize(package_fragment &pkg, typename json::input_archive::archive_frame &frame, master_package &master)
			{
				logger::info() << fmt::format("Loading v1 package \"{}\"...", pkg.path.string());

				/* Try to deserialize assets. */
				frame.try_read(keyed_entry("assets", assets_deserialize_proxy{pkg}), master);
			}
			inline void deserialize(master_package &pkg, typename json::input_archive::archive_frame &frame)
			{
				/* Try to deserialize assets & fragments. */
				v1::deserialize(pkg, frame, pkg);
				frame.try_read(keyed_entry("fragments", fragments_deserialize_proxy{pkg}));
			}
		}	 // namespace v1

		inline void deserialize(package_fragment &pkg, typename json::input_archive::archive_frame &frame, master_package &master)
		{
			switch (get_manifest_version(frame))
			{
				case 1: v1::deserialize(pkg, frame, master); break;
			}
		}
		inline void deserialize(master_package &pkg, typename json::input_archive::archive_frame &frame)
		{
			if (bool flag = false; !(frame.try_read(keyed_entry("is_master", flag)) && flag)) [[unlikely]]
				throw asset_package_error("Not a master package");

			switch (get_manifest_version(frame))
			{
				case 1: v1::deserialize(pkg, frame); break;
			}
		}
	}	 // namespace detail

	asset_package::asset_package(detail::master_package *pkg) : m_ptr(pkg) { m_ptr.acquire(); }
	void asset_package::reset() { m_ptr.reset(); }

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
			logger::info() << fmt::format("Loaded [{}] packages", result.size());
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
			{ /* TODO: Load archive. */
			}
		}
		catch (...)
		{
			delete result;
			throw;
		}
		return asset_package{result};
	}

}	 // namespace sek::engine