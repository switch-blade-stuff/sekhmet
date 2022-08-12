/*
 * Created by switchblade on 10/08/22
 */

#pragma once

namespace sek::engine
{
	class asset_buffer;
	class asset_source;
	class asset_handle;
	class asset_package;
	class asset_database;

	class asset_error;

	namespace detail
	{
		struct asset_info;
		struct asset_table;
		class package_info;
		class asset_io_data;

		class loose_package;
		class archive_package;
		class zstd_package;
	}	 // namespace detail
}	 // namespace sek::engine