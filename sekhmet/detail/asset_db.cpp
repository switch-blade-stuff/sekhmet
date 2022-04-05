//
// Created by switchblade on 2022-04-04.
//

#include "asset_db.hpp"

#ifdef SEK_OS_WIN
#define MANIFEST_FILE_NAME L".manifest"
#else
#define MANIFEST_FILE_NAME ".manifest"
#endif

namespace sek
{
	template struct SEK_API_EXPORT basic_service<asset_db>;

	std::fstream asset_db::open_package_manifest(const std::filesystem::path &path, std::ios::openmode mode)
	{
		auto manifest_path = path;
		if (is_directory(manifest_path))
			manifest_path /= MANIFEST_FILE_NAME;
		else
		{
			/* Remove the `out` flag if it is not a directory. */
			if (mode & std::ios::out) mode ^= std::ios::out;
			/* Add binary flag since it is an archive. */
			mode |= std::ios::binary;
		}

		return exists(manifest_path) ? std::fstream{manifest_path, mode} : std::fstream{};
	}
	adt::node asset_db::load_package_manifest(const std::filesystem::path &path)
	{
		adt::node result;
		auto manifest_stream = open_package_manifest(path, std::ios::in);
		if (manifest_stream.is_open())
		{ /* TODO: Read either TOML or UBJson data from the manifest stream. */
		}

		return result;
	}

	package_handle asset_db::load_package(const std::filesystem::path &path, bool /*overwrite*/)
	{
		package_handle result;
		auto relative_path = get_relative_path(path);
		auto manifest_path = data_dir_path / get_manifest_path(relative_path);
		if (exists(manifest_path))
			if (auto manifest = load_package_manifest(manifest_path); !manifest.empty()) [[likely]]
			{ /* TODO: allocate the appropriate package & initialize it. */
			}

		return result;
	}
	int asset_db::check_package(const std::filesystem::path &path) const
	{
		if (auto manifest_path = data_dir_path / get_manifest_path(get_relative_path(path)); exists(manifest_path))
			if (auto manifest = load_package_manifest(manifest_path); manifest.is_table()) [[likely]]
			{
				if (manifest.as_table().contains("master") && manifest.at("master").as_bool())
					return 1;
				else
					return -1;
			}
		return 0;
	}
}	 // namespace sek