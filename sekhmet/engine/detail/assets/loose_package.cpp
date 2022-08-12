/*
 * Created by switchblade on 10/08/22
 */

#include "loose_package.hpp"

#include "asset_source.hpp"

namespace sek::engine::detail
{
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
}	 // namespace sek::engine::detail