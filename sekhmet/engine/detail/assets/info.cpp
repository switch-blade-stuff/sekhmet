/*
 * Created by switchblade on 10/08/22
 */

#include "info.hpp"

#include "asset_source.hpp"

namespace sek::engine::detail
{
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
	void package_info::destroy_all()
	{
		for (auto entry : uuid_table) destroy_info(entry.second);
	}

	expected<system::native_file, std::error_code> local_package::open_archive(std::uint64_t offset) const
	{
		system::native_file file;

		auto result = file.open(std::nothrow, m_path, system::native_file::read_only);
		if (result && (result = file.setpos(std::nothrow, offset))) [[likely]]
			return file;
		return unexpected{result.error()};
	}
}	 // namespace sek::engine::detail