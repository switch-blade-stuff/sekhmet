/*
 * Created by switchblade on 10/08/22
 */

#include "archive_package.hpp"

#include "../logger.hpp"
#include "../zstd_ctx.hpp"
#include "asset_source.hpp"

namespace sek::engine::detail
{
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

	expected<asset_source, std::error_code> flat_package::open_at(archive_slice slice) const noexcept
	{
		const auto src_size = slice.src_size;
		const auto offset = slice.offset;
		const auto size = slice.size;

		if (!(offset && size && src_size)) [[unlikely]]
			return unexpected{std::make_error_code(std::errc::invalid_argument)};

		auto result = open_archive(offset);
		if (result.has_value()) [[likely]]
			return make_source(asset_io_data{*std::move(result)}, slice.offset, slice.size);
		return unexpected{result.error()};
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
			const auto in_frames = ctx.decompress(asset_zstd_pool(),
												  delegate{func_t<&reader_t::read>{}, reader},
												  delegate{func_t<&writer_t::write>{}, writer},
												  frames);
			if (in_frames != frames) [[unlikely]]
			{
				/* Mismatched frame count does not necessarily mean an error (data might be corrupted but that is up to the consumer to decide). */
				logger::warn() << fmt::format(
					"Mismatched asset frame count - expected {} but got {}. This might be a sign of corruption", frames, in_frames);
			}

			return make_source(std::move(data), 0, src_size);
		}
		return unexpected{result.error()};
	}
}	 // namespace sek::engine::detail