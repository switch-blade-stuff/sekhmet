/*
 * Created by switchblade on 10/08/22
 */

#include "asset_io.hpp"

SEK_API_EXPORT const sek::engine::detail::asset_io_data::vtable_t sek::engine::detail::asset_io_data::file_vtable = {
	.read = asset_io_data::file_read,
	.seek = asset_io_data::file_seek,
	.setpos = asset_io_data::file_setpos,
	.size = asset_io_data::file_size,
	.tell = asset_io_data::file_tell,
	.destroy_data = asset_io_data::destroy_file,
};
SEK_API_EXPORT const sek::engine::detail::asset_io_data::vtable_t sek::engine::detail::asset_io_data::buff_vtable = {
	.read = asset_io_data::buff_read,
	.seek = asset_io_data::buff_seek,
	.setpos = asset_io_data::buff_setpos,
	.size = asset_io_data::buff_size,
	.tell = asset_io_data::buff_tell,
	.destroy_data = asset_io_data::destroy_buff,
};