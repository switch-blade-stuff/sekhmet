/*
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

#include "sekhmet/serialization/binary.hpp"
#include "sekhmet/serialization/json.hpp"

#include "logger.hpp"
#include "zstd_ctx.hpp"

template class SEK_API_EXPORT sek::service<sek::shared_guard<sek::engine::asset_database>>;

namespace sek::engine
{
	using namespace sek::serialization;

	template<typename T>
	inline static T return_if(expected<T, std::error_code> &&exp)
	{
		if (!exp.has_value()) [[unlikely]]
			throw std::system_error(exp.error());
		return std::move(exp.value());
	}
	template<typename T>
	inline static void return_if(expected<void, std::error_code> &&exp)
	{
		if (!exp.has_value()) [[unlikely]]
			throw std::system_error(exp.error());
	}

	namespace detail
	{
		using namespace sek::detail;

		/* Asset IO. */
		SEK_API_EXPORT const asset_io_vtable asset_io_data::file_vtable = {
			.read = asset_io_data::file_read,
			.seek = asset_io_data::file_seek,
			.setpos = asset_io_data::file_setpos,
			.size = asset_io_data::file_size,
			.tell = asset_io_data::file_tell,
			.destroy_data = asset_io_data::destroy_file,
		};
		SEK_API_EXPORT const asset_io_vtable asset_io_data::buff_vtable = {
			.read = asset_io_data::buff_read,
			.seek = asset_io_data::buff_seek,
			.setpos = asset_io_data::buff_setpos,
			.size = asset_io_data::buff_size,
			.tell = asset_io_data::buff_tell,
			.destroy_data = asset_io_data::destroy_buff,
		};

		asset_source package_info::make_source(asset_io_data &&data, std::uint64_t offset, std::uint64_t size) noexcept
		{
			return asset_source{std::move(data), offset, size};
		}

		void package_info::acquire() { ++m_refs; }
		void package_info::release()
		{
			if (m_refs.fetch_sub(1) == 1) [[unlikely]]
				delete this;
		}
		void package_info::insert(uuid id, asset_info *info)
		{
			if (auto existing = uuid_table.find(id); existing != uuid_table.end())
				delete_info(std::exchange(existing->second, info));
			else
				uuid_table.emplace(id, info);

			/* If the new asset has a name, add or replace the name entry. */
			if (!info->name.empty()) [[likely]]
			{
				if (auto existing = name_table.find(info->name); existing != name_table.end())
					existing->second = id;
				else
					name_table.emplace(info->name, id);
			}
		}
		void package_info::erase(uuid id)
		{
			if (const auto id_entry = uuid_table.find(id); id_entry != uuid_table.end())
			{
				const auto info = id_entry->second;

				/* Erase the asset from both tables. */
				uuid_table.erase(id_entry);
				if (const auto &name = info->name; !name.empty()) /* If the asset has a name, erase the name table entry. */
				{
					const auto name_entry = name_table.find(name);
					if (name_entry != name_table.end() && name_entry->second == id) name_table.erase(name_entry);
				}

				/* Destroy & de-allocate the old asset. */
				delete_info(info);
			}
		}

		// clang-format off
		void package_info::destroy_all() { for (auto entry : uuid_table) destroy_info(entry.second); }
		// clang-format on

		expected<system::native_file, std::error_code> local_package::open_archive(std::uint64_t offset) const
		{
			system::native_file file;

			auto result = file.open(std::nothrow, m_path, system::native_file::read_only);
			if (result && (result = file.setpos(std::nothrow, offset))) [[likely]]
				return file;
			return unexpected{result.error()};
		}

		expected<asset_source, std::error_code> loose_package::open_at(std::string_view local_path) const noexcept
		{
			if (local_path.empty()) [[unlikely]]
				return unexpected{std::make_error_code(std::errc::invalid_argument)};

			asset_io_data data;
			auto &file = data.init_file();

			const auto result = file.open(std::nothrow, path() / local_path, system::native_file::read_only);
			if (result.has_value()) [[likely]]
			{
				const auto size = file.size();
				return make_source(std::move(data), 0, size);
			}
			return unexpected{result.error()};
		}
		expected<asset_source, std::error_code> loose_package::open_asset(const asset_info *info) const noexcept
		{
			return open_at(static_cast<const loose_info *>(info)->asset_path());
		}
		expected<asset_source, std::error_code> loose_package::open_metadata(const asset_info *info) const noexcept
		{
			return open_at(static_cast<const loose_info *>(info)->meta_path());
		}

		expected<asset_source, std::error_code> archive_package::open_at(archive_slice slice) const noexcept
		{
			if (!(slice.offset && slice.size)) [[unlikely]]
				return unexpected{std::make_error_code(std::errc::invalid_argument)};

			auto result = open_archive(slice.offset);
			if (result.has_value()) [[likely]]
				return make_source(asset_io_data{*std::move(result)}, slice.offset, slice.size);
			return unexpected{result.error()};
		}
		expected<asset_source, std::error_code> archive_package::open_asset(const asset_info *info) const noexcept
		{
			return open_at(static_cast<const archive_info *>(info)->asset_slice);
		}
		expected<asset_source, std::error_code> archive_package::open_metadata(const asset_info *info) const noexcept
		{
			return open_at(static_cast<const archive_info *>(info)->meta_slice);
		}

		inline static thread_pool &asset_zstd_pool()
		{
			/* TODO: Refactor to use ASIO-compatible thread pool. */
			static thread_pool instance;
			return instance;
		}

		expected<asset_source, std::error_code> zstd_package::open_at(archive_slice slice) const noexcept
		{
			const auto src_size = slice.src_size;
			const auto offset = slice.offset;
			const auto frames = slice.frames;
			const auto size = slice.size;

			if (!(offset && size && src_size)) [[unlikely]]
				return unexpected{std::make_error_code(std::errc::invalid_argument)};

			auto result = open_archive(offset);
			if (result.has_value()) [[likely]]
			{
				asset_io_data data;

				auto &ctx = zstd_thread_ctx::instance();
				auto &buff = data.init_buff(src_size);
				auto &file = *result;

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

					system::native_file &file;
					std::size_t size;
					std::size_t pos = 0;
				} reader = {file, static_cast<std::size_t>(size)};
				struct writer_t
				{
					std::size_t write(const void *src, std::size_t n)
					{
						auto new_pos = pos + n;
						if (new_pos > buffer.size()) [[unlikely]]
						{
							new_pos = buffer.size();
							n = buffer.size() - pos;
						}

						memcpy(buffer.owned_bytes() + std::exchange(pos, new_pos), src, n);
						return n;
					}

					asset_buffer &buffer;
					std::size_t pos = 0;
				} writer = {buff};

				/* TODO: Refactor ZSTD context API to work with error codes. */
				const auto decompressed = ctx.decompress(asset_zstd_pool(),
														 delegate{func_t<&reader_t::read>{}, reader},
														 delegate{func_t<&writer_t::write>{}, writer},
														 frames);
				if (decompressed != frames) [[unlikely]]
				{
					/* Mismatched frame count does not necessarily mean an error (data might be corrupted but that is up to the consumer to decide). */
					logger::warn() << fmt::format(
						"Mismatched asset frame count - expected {} but got {}. This might be a sign of corruption",
						frames,
						decompressed);
				}

				return make_source(std::move(data), 0, src_size);
			}
			return unexpected{result.error()};
		}
		expected<asset_source, std::error_code> zstd_package::open_asset(const asset_info *info) const noexcept
		{
			return open_at(static_cast<const zstd_info *>(info)->asset_slice);
		}
		expected<asset_source, std::error_code> zstd_package::open_metadata(const asset_info *info) const noexcept
		{
			return open_at(static_cast<const zstd_info *>(info)->meta_slice);
		}
	}	 // namespace detail

	std::size_t asset_source::read(void *dst, std::size_t n) { return return_if(read(std::nothrow, dst, n)); }
	std::size_t asset_source::read(asio::mutable_buffer &buff) { return return_if(read(std::nothrow, buff)); }
	expected<std::size_t, std::error_code> asset_source::read(std::nothrow_t, void *dst, std::size_t n) noexcept
	{
		auto new_pos = m_read_pos + static_cast<std::uint64_t>(n);
		if (new_pos > m_size) [[unlikely]]
		{
			n = static_cast<std::size_t>(m_size - m_read_pos);
			new_pos = m_size;
		}

		auto result = m_data.read(dst, n);
		if (result.has_value()) [[likely]]
			m_read_pos += *result;
		return result;
	}
	expected<std::size_t, std::error_code> asset_source::read(std::nothrow_t, asio::mutable_buffer &buff) noexcept
	{
		return read(buff.data(), buff.size());
	}

	std::uint64_t asset_source::seek(std::int64_t off, seek_basis dir)
	{
		return return_if(seek(std::nothrow, off, dir));
	}
	std::uint64_t asset_source::setpos(std::uint64_t pos) { return return_if(setpos(std::nothrow, pos)); }
	expected<std::uint64_t, std::error_code> asset_source::seek(std::nothrow_t, std::int64_t off, asset_source::seek_basis dir) noexcept
	{
		if (!empty()) [[likely]]
			return m_data.seek(off + static_cast<std::int64_t>(m_offset), dir);
		return unexpected{std::make_error_code(std::errc::invalid_argument)};
	}
	expected<std::uint64_t, std::error_code> asset_source::setpos(std::nothrow_t, std::uint64_t pos) noexcept
	{
		if (!empty()) [[likely]]
			return m_data.setpos(m_offset + pos);
		return unexpected{std::make_error_code(std::errc::invalid_argument)};
	}

	asset_source asset_handle::open() const { return return_if(open(std::nothrow)); }
	asset_source asset_handle::metadata() const { return return_if(metadata(std::nothrow)); }
	expected<asset_source, std::error_code> asset_handle::open(std::nothrow_t) const noexcept
	{
		return m_ptr->parent->open_asset(m_ptr.info);
	}
	expected<asset_source, std::error_code> asset_handle::metadata(std::nothrow_t) const noexcept
	{
		return m_ptr->parent->open_metadata(m_ptr.info);
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
		 * 0x0008  -  0x000b                Header flags (compression type, etc.)
		 * 0x000c  -  0x000f                Number of assets of the package (may be 0)
		 * 0x0010  -  end_assets            Asset info for every asset
		 * =============================== Header flags ===============================
		 * Description           Bit(s)      Values
		 * Archive flag            0           0   - Loose package
		 *                                     1   - Archive package
		 * Project flag            1           0   - Not a project (all runtime packages)
		 *                                     1   - Editor project (editor-managed loose packages)
		 * Compression format     2-5          0   - Not used (used for non-archive packages)
		 *                                     1   - No compression
		 *                                     2   - ZSTD compression
		 *                                    3-15 - Reserved
		 * Reserved               6-31         0
		 * ======================== Uncompressed asset entry ==========================
		 * Entry offsets                  Description
		 * 0x00 - 0x0f                    Asset UUID
		 * 0x10 - 0x17                    Asset data offset
		 * 0x18 - 0x1f                    Asset size
		 * 0x20 - 0x27                    Metadata offset
		 * 0x28 - 0x2f                    Metadata size
		 * 0x30 - end_name                Null-terminated name string (optional)
		 * n_tags - n_tags + 4            Number of asset tags (may be 0)
		 * n_tags + 5 - end_tags          Null-terminated asset tag strings
		 * ====================== ZSTD-compressed asset entry ========================
		 * Entry offsets                  Description
		 * 0x00 - 0x0f                    Asset UUID
		 * 0x10 - 0x17                    Asset offset
		 * 0x18 - 0x1f                    Asset size (compressed)
		 * 0x20 - 0x27                    Asset size (decompressed)
		 * 0x28 - 0x2f                    Metadata offset
		 * 0x30 - 0x37                    Metadata size (compressed)
		 * 0x38 - 0x3f                    Metadata size (decompressed)
		 * 0x40 - 0x43                    Asset frame count (Always 0 if not compressed)
		 * 0x44 - 0x47                    Metadata frame count (Always 0 if not compressed)
		 * 0x48 - end_name                Null-terminated name string (optional)
		 * n_tags - n_tags + 4            Number of asset tags (may be 0)
		 * n_tags + 5 - end_tags          Null-terminated asset tag strings
		 * ============================================================================
		 * */

		constexpr std::array<char, 7> signature_str = {'\3', 'S', 'E', 'K', 'P', 'A', 'K'};
		constexpr std::uint8_t manifest_ver_max = 1;
		constexpr std::uint8_t archive_ver_max = 1;

		using json_input = typename json::input_archive::archive_frame;
		using binary_input = binary::input_archive;
		using flags_t = package_info::flags_t;

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
			throw asset_error(fmt::format("Failed to open package manifest at \"{}\"", manifest_path.string()));
		}
		inline auto open_header(const std::filesystem::path &path)
		{
			if (!exists(path) && !is_regular_file(path)) [[unlikely]]
				goto invalid_header;

			{
				auto file = new system::native_file(path, system::native_file::in);
				if (!file->is_open()) [[unlikely]]
					goto invalid_header;

				std::pair<std::unique_ptr<system::native_file>, binary_input> result = {
					std::unique_ptr<system::native_file>{file},
					binary_input{*file},
				};
				return result;
			}

		invalid_header:
			throw asset_error(fmt::format("Failed to open archive package at \"{}\"", path.string()));
		}
		inline std::uint8_t get_manifest_version(typename json::input_archive::archive_frame &frame)
		{
			std::uint8_t ver = 0;
			if (!(frame.try_read(keyed_entry("version", ver)) && ver - 1 < manifest_ver_max)) [[unlikely]]
				throw asset_error("Unknown manifest version");
			return ver;
		}
		inline std::uint8_t get_header_version(binary::input_archive &archive)
		{
			for (auto c_req : signature_str)
			{
				if (char c; !(archive.try_read(c) && c == c_req)) [[unlikely]]
					throw asset_error("Invalid header signature");
			}
			std::uint8_t ver = 0;
			if (!(archive.try_read(ver) && ver - 1 < archive_ver_max)) [[unlikely]]
				throw asset_error("Unknown header version");
			return ver;
		}
		inline auto get_header_flags(binary::input_archive &archive)
		{
			std::int32_t value;
			if (!archive.try_read(value)) [[unlikely]]
				throw asset_error("Invalid header flags");
			return static_cast<flags_t>(value);
		}

		inline void deserialize(package_info &, binary_input &);
		inline void deserialize(package_info &, json_input &);

		namespace v1
		{
			struct assets_proxy
			{
				struct info_deleter
				{
					inline void operator()(asset_info *ptr) const { pkg.dealloc_info(ptr); }
					package_info &pkg;
				};
				struct info_proxy
				{
					struct tags_proxy
					{
						void deserialize(binary_input &archive) const
						{
							auto size = archive.read(std::in_place_type<std::uint32_t>);
							tags.reserve(size);
							while (size-- != 0) tags.emplace(archive.read(std::in_place_type<std::string>));
						}
						void deserialize(json_input &archive) const
						{
							std::size_t size = 0;
							archive >> container_size(size);
							tags.reserve(size);
							while (size-- != 0) tags.emplace(archive.read(std::in_place_type<std::string_view>));
						}

						dense_set<interned_string> &tags;
					};

					void deserialize(binary_input &archive, package_info &parent) const
					{
						std::construct_at(info, &parent);

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
					void deserialize(json_input &archive, package_info &parent) const
					{
						std::construct_at(info, &parent);

						for (auto iter = archive.begin(), end = archive.end(); iter != end; ++iter)
						{
							const auto key = iter.key();
							if (key == "name")
								info->name = iter->read(std::in_place_type<std::string_view>);
							else if (key == "tags")
								iter->read(tags_proxy{info->tags});
							else if (key == "data")
							{
								const auto path = iter->read(std::in_place_type<std::string_view>);
								info->loose_info.asset_path = asset_info::loose_info_t::copy_path(path);
							}
							else if (key == "metadata")
							{
								const auto path = iter->read(std::in_place_type<std::string_view>);
								info->loose_info.meta_path = asset_info::loose_info_t::copy_path(path);
							}
						}

						if (info->loose_info.asset_path.empty()) [[unlikely]]
							throw archive_error("Missing asset data path");
					}

					asset_info *info;
				};

				uuid read_entry(std::size_t, binary_input &archive, asset_info *info) const
				{
					std::array<std::byte, 16> bytes = {};
					for (auto &byte : bytes) byte = std::byte{archive.read(std::in_place_type<std::uint8_t>)};

					archive.read(info_proxy{info}, pkg);
					return uuid{bytes};
				}
				uuid read_entry(std::size_t i, json_input &archive, asset_info *info) const
				{
					const auto iter = archive.begin() + static_cast<std::ptrdiff_t>(i);
					const auto id = uuid{iter.key()};

					iter->read(info_proxy{info}, pkg);
					return id;
				}
				void read_assets(auto &archive, std::size_t n) const
				{
					pkg.uuid_table.reserve(n);
					pkg.name_table.reserve(n);

					std::unique_ptr<asset_info, info_deleter> info{nullptr, info_deleter{pkg}};
					std::size_t total = 0;
					for (std::size_t i = 0; i < n; ++i)
					{
						uuid id;
						if (info == nullptr) [[likely]]
							info.reset(pkg.alloc_info());

						try
						{
							id = read_entry(i, archive, info.get());
						}
						catch (archive_error &e)
						{
							logger::error() << fmt::format("Ignoring malformed asset entry. Parse error: \"{}\"", e.what());
							std::destroy_at(info.get());
							continue;
						}

						pkg.insert(id, info.release());
						++total; /* Increment total only after the entry was successfully inserted. */
					}

					logger::info() << fmt::format("Loaded {} asset(s)", total);
				}

				void deserialize(binary_input &archive) const
				{
					read_assets(archive, archive.read(std::in_place_type<std::uint32_t>));
				}
				void deserialize(json_input &archive) const
				{
					std::size_t size = 0;
					archive >> container_size(size);
					read_assets(archive, size);
				}

				package_info &pkg;
			};

			inline void deserialize(package_info &pkg, binary_input &archive)
			{
				pkg.flags = static_cast<flags_t>(get_header_flags(archive) | package_info::IS_ARCHIVE);
				logger::info() << fmt::format("Loading v1 archive package (compression: {}) \"{}\"",
											  pkg.is_archive_zstd() ? "ZSTD" : "none",
											  relative(pkg.path).string());

				/* Try to deserialize assets. */
				archive.try_read(assets_proxy{pkg});
			}
			inline void deserialize(package_info &pkg, json_input &frame)
			{
				logger::info() << fmt::format("Loading v1 loose package \"{}\"", relative(pkg.path).string());

				/* Try to deserialize assets. */
				frame.try_read(keyed_entry("assets", assets_proxy{pkg}));
			}
		}	 // namespace v1

		inline void deserialize(package_info &pkg, binary_input &archive)
		{
			switch (get_header_version(archive))
			{
				case 1: v1::deserialize(pkg, archive); break;
			}
		}
		inline void deserialize(package_info &pkg, json_input &frame)
		{
			switch (get_manifest_version(frame))
			{
				case 1: v1::deserialize(pkg, frame); break;
			}
		}
	}	 // namespace detail

	asset_error::~asset_error() = default;

	asset_package::asset_package(detail::package_info *pkg) : m_ptr(pkg) { m_ptr.acquire(); }

	std::vector<asset_package> asset_package::load_all(const std::filesystem::path &path)
	{
		if (!exists(path) && !is_directory(path)) [[unlikely]]
			throw asset_error(fmt::format("\"{}\" is not a valid directory", path.string()));

		std::vector<asset_package> result;
		{
			logger::info() << fmt::format("Loading packages in directory \"{}\"", path.string());
			for (auto &entry : std::filesystem::directory_iterator(path)) try
				{
					result.emplace_back(load(entry.path()));
				}
				catch (asset_error &e)
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
			throw asset_error(fmt::format("\"{}\" is not a valid package path", path.string()));

		auto result = std::make_unique<detail::package_info>(path);
		if (is_directory(path))
			detail::open_manifest(path).read(*result);
		else
			detail::open_header(path).second.read(*result);
		return asset_package{result.release()};
	}

	void asset_database::clear()
	{
		m_assets.name_table.clear();
		m_assets.uuid_table.clear();
		m_packages.clear();
	}

	void asset_database::override_asset(typename packages_t::const_iterator parent, uuid id, detail::asset_info *info)
	{
		auto can_override = [&](detail::package_info *pkg)
		{
			auto pred = [pkg](auto &ptr) { return ptr.m_ptr.pkg == pkg; };
			return std::none_of(parent, m_packages.cend(), pred);
		};

		auto &uuid_table = m_assets.uuid_table;
		auto &name_table = m_assets.name_table;
		/* If the UUID entry does exist, check if it's package is higher in the load order than us.
		 * If it is, skip the entry. Otherwise, override the entry. */
		if (auto entry = uuid_table.find(id); entry != uuid_table.end() && !can_override(entry->second->parent)) [[unlikely]]
			return;
		uuid_table.insert({id, info});

		/* If the new entry has a name, and such name is already present within the name table,
		 * check if we can override the name. */
		if (const auto &name = info->name; !name.empty()) [[likely]]
		{
			auto existing = name_table.find(name);
			if (existing == name_table.end() || can_override(uuid_table.at(existing->second)->parent)) [[likely]]
				name_table.insert({name, id});
		}
	}

	void asset_database::restore_asset(typename packages_t::const_iterator parent, uuid id, const detail::asset_info *info)
	{
		auto &uuid_table = m_assets.uuid_table;
		auto &name_table = m_assets.name_table;
		const auto &name = info->name;

		/* Skip the entry if it is not present within the database. */
		auto existing_uuid = uuid_table.find(id);
		if (existing_uuid != uuid_table.end() && existing_uuid->second->parent == parent->m_ptr.pkg) [[likely]]
		{
			/* Find the existing name entry. */
			auto existing_name = name.empty() ? name_table.end() : name_table.find(name);
			/* Find conflicting assets within the packages below `first`.
			 * Name entry is replaced only if it points to the replaced asset. */
			bool found_uuid = false, found_name = existing_name == name_table.end() || existing_name->second != id;
			for (auto pkg_iter = parent;;)
			{
				if (found_uuid && found_name) [[unlikely]]
					break;
				else if (pkg_iter-- == m_packages.begin()) [[unlikely]]
				{
					/* No replacements found, erase the entries. */
					if (!found_name) name_table.erase(existing_name);
					if (!found_uuid) uuid_table.erase(existing_uuid);
					break;
				}

				auto &pkg = *pkg_iter->m_ptr.pkg;
				if (!found_uuid) [[likely]]
				{
					/* If a replacement UUID table entry is found, replace it and clear the flag. */
					if (auto replacement = pkg.uuid_table.find(id); replacement != pkg.uuid_table.end())
					{
						existing_uuid->second = replacement->second;
						found_uuid = true;
					}
				}
				if (!found_name) [[likely]]
				{
					/* If a replacement name table entry is found, replace it and clear the flag. */
					if (auto replacement = pkg.name_table.find(name); replacement != pkg.name_table.end())
					{
						existing_name->second = replacement->second;
						found_name = true;
					}
				}
			}
		}
	}

	typename asset_database::packages_t::const_iterator asset_database::insert_impl(typename packages_t::iterator pkg)
	{
		/* Override assets of the package. */
		for (auto [id, info] : pkg->m_ptr->uuid_table) override_asset(pkg, id, info);

#ifdef SEK_ENGINE
		/* In editor mode, packages may be modified and database needs to be updated accordingly to handle overrides. */
		pkg->on_asset_removed() += delegate<void(const asset_ref &)>{this, &asset_database::handle_asset_added};
		pkg->on_asset_added() += delegate<void(const asset_handle &)>{this, &asset_database::handle_asset_added};
#endif

		return pkg;
	}
	typename asset_database::packages_t::const_iterator asset_database::insert(typename packages_t::const_iterator where,
																			   const asset_package &pkg)
	{
		return insert_impl(m_packages.insert(where, pkg));
	}
	typename asset_database::packages_t::const_iterator asset_database::insert(typename packages_t::const_iterator where,
																			   asset_package &&pkg)
	{
		return insert_impl(m_packages.insert(where, std::forward<asset_package>(pkg)));
	}

	typename asset_database::packages_t::const_iterator asset_database::erase_impl(typename packages_t::iterator pkg)
	{
		/* Restore assets overridden by the package. */
		for (const auto &[id, info] : pkg->m_ptr->uuid_table) restore_asset(pkg, id, info);

#ifdef SEK_ENGINE
		/* Unsubscribe from any package editor events. */
		pkg->on_asset_removed() -= delegate<void(const asset_handle &)>{this, &asset_database::handle_asset_added};
		pkg->on_asset_added() -= delegate<void(const asset_handle &)>{this, &asset_database::handle_asset_added};
#endif

		return pkg;
	}
	typename asset_database::packages_t::const_iterator asset_database::erase(typename packages_t::const_iterator first,
																			  typename packages_t::const_iterator last)
	{
		for (auto pkg = last; pkg-- != first;) erase_impl(m_packages.begin() + std::distance(m_packages.cbegin(), pkg));
		return m_packages.erase(first, last);
	}
	typename asset_database::packages_t::const_iterator asset_database::erase(typename packages_t::const_iterator pkg)
	{
		return m_packages.erase(erase_impl(m_packages.begin() + std::distance(m_packages.cbegin(), pkg)));
	}

	void asset_database::swap(typename packages_t::const_iterator a, typename packages_t::const_iterator b)
	{
		/* Figure out which handle is higher in the lower order. */
		const auto high = m_packages.begin() + std::distance(m_packages.cbegin(), std::max(a, b));
		const auto low = m_packages.begin() + std::distance(m_packages.cbegin(), std::min(a, b));

		/* Restore overrides for the higher-order package. Lower-order package does not need to be restored,
		 * since it's assets would have already been overridden by packages above it and as it is moving up,
		 * we do not care about the non-overridden ones. */
		for (const auto &[id, info] : high->m_ptr->uuid_table) restore_asset(high, id, info);

		/* Swap handles of the two packages. */
		std::iter_swap(high, low);

		/* Override assets at new load order positions. */
		for (const auto &[id, info] : high->m_ptr->uuid_table) override_asset(high, id, info);
		for (const auto &[id, info] : low->m_ptr->uuid_table) override_asset(low, id, info);
	}

#ifdef SEK_ENGINE
	void asset_database::handle_asset_removed(const asset_handle &asset)
	{
		/* Find load order of the package the asset belongs to. */
		const auto pred = [ptr = asset.m_ptr->parent](const asset_package &pkg) { return pkg.m_ptr.pkg == ptr; };
		const auto parent = std::find_if(m_packages.cbegin(), m_packages.cend(), pred);

		/* Since we are receiving events, package must be present within the database. */
		SEK_ASSERT(parent != m_packages.cend());

		/* Restore overrides of the asset. */
		restore_asset(parent, asset.m_id, asset.m_ptr.info);
	}
	void asset_database::handle_asset_added(const asset_handle &asset)
	{
		/* Find load order of the package the asset belongs to. */
		const auto pred = [ptr = asset.m_ptr->parent](const asset_package &pkg) { return pkg.m_ptr.pkg == ptr; };
		const auto parent = std::find_if(m_packages.cbegin(), m_packages.cend(), pred);

		/* Since we are receiving events, package must be present within the database. */
		SEK_ASSERT(parent != m_packages.cend());

		/* Override asset for the parent package. */
		override_asset(parent, asset.m_id, asset.m_ptr.info);
	}
#endif
}	 // namespace sek::engine