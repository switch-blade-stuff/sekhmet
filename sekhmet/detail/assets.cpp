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

#define _CRT_SECURE_NO_WARNINGS

#include "assets.hpp"

#include "logger.hpp"
#include "sekhmet/serialization/json.hpp"
#include "sekhmet/serialization/ubjson.hpp"
#include "zstd_ctx.hpp"

#ifdef SEK_OS_WIN
#define MANIFEST_FILE_NAME L".manifest"
#else
#define MANIFEST_FILE_NAME ".manifest"
#endif

#define RECORD_ERROR_MSG "Invalid asset record"

namespace sek
{
	template class SEK_API_EXPORT service<asset_repository>;

	namespace detail
	{
		namespace json = serialization::json;
		namespace ubj = serialization::ubj;

		constexpr static std::array<char, 8> signature = {'\3', 'S', 'E', 'K', 'P', 'A', 'K', '\0'};
		constexpr static std::size_t version_pos = 7;

		constexpr std::array<char, 8> make_signature(std::uint8_t ver) noexcept
		{
			auto result = signature;
			result[version_pos] = static_cast<char>(ver);
			return result;
		}
		constexpr std::uint8_t check_signature(std::array<char, 8> data) noexcept
		{
			if (std::equal(signature.begin(), signature.begin() + version_pos, data.begin())) [[likely]]
				return static_cast<std::uint8_t>(data[version_pos]);
			else
				return 0;
		}

		std::filesystem::path asset_info::loose_path() const { return parent->path / loose.file_path; }

		void package_base::acquire() noexcept { master()->acquire_impl(); }
		void package_base::release() { master()->release_impl(); }
		void master_package::acquire_impl() noexcept { ref_count++; }
		void master_package::release_impl()
		{
			if (ref_count.fetch_sub(1) == 1) [[unlikely]]
				delete this;
		}

		static auto &asset_thread_pool()
		{
			static thread_pool instance;
			return instance;
		}

		std::string asset_handle::read_archive() const
		{
			constexpr auto writer_func = +[](std::string *s, const void *src, std::size_t n)
			{
				auto chars = static_cast<const char *>(src);
				s->insert(s->size(), chars, n);
				return n;
			};

			auto &ctx = zstd_thread_ctx::instance();
			auto &pool = asset_thread_pool();

			const auto &archive_info = info->archive;
			const auto &path = info->parent->path;

			std::string result;
			result.reserve(archive_info.src_size);

			/* Open the asset file & seek asset position. */
			FILE *archive_file = fopen(path.c_str(), "rb");
			{
				if (archive_file == nullptr) [[unlikely]]
				{
					throw std::runtime_error(
						SEK_LOG_FORMAT_NS::format("Failed to open asset archive at path \"{}\"", path.native()));
				}

#if defined(_POSIX_C_SOURCE)
#if _FILE_OFFSET_BITS < 64
				auto err = fseeko64(archive_file, static_cast<off64_t>(archive_info.offset), SEEK_SET);
#else
				auto err = fseeko(archive_file, static_cast<off_t>(archive_info.offset), SEEK_SET);
#endif
#elif defined(SEK_OS_WIN)
				auto err = _fseeki64(archive_file, static_cast<__int64>(archive_info.offset), SEEK_SET);
#else
				auto err = fseek(archive_file, static_cast<long>(archive_info.offset), SEEK_SET);
#endif
				if (err != 0) [[unlikely]]
				{
					fclose(archive_file);
					throw std::runtime_error(SEK_LOG_FORMAT_NS::format(
						"Failed to seek asset archive at path \"{}\" to position [{}]", path.native(), archive_info.offset));
				}
			}

			/* Attempt to decompress the asset, re-throw any exceptions. */
			try
			{
				auto reader = zstd_thread_ctx::file_reader{archive_file, archive_info.size};
				auto writer = sek::delegate{writer_func, result};
				if (const auto got_frames = ctx.decompress(pool, reader, writer, archive_info.frames);
					got_frames != archive_info.frames) [[unlikely]]
				{
					/* Log on inconsistent frame count. This may not be an error, since compression is successful. */
					logger::warn() << SEK_LOG_FORMAT_NS::format(
						"Inconsistent asset frame count for asset \"{}\" ({}). Expected [{}], got [{}]",
						info->name.sv(),
						id.to_string(),
						archive_info.frames,
						got_frames);
				}
				fclose(archive_file);
			}
			catch (...)
			{
				fclose(archive_file);
				throw;
			}

			return result;
		}

		template SEK_API_EXPORT basic_asset_stream<char, std::char_traits<char>>
			asset_handle::stream(std::ios::openmode, const std::locale &) const;
		template SEK_API_EXPORT basic_asset_stream<wchar_t, std::char_traits<wchar_t>>
			asset_handle::stream(std::ios::openmode, const std::locale &) const;
	}	 // namespace detail

	//	asset asset::load(uuid id) { return asset(); }
	//	asset asset::load(std::string_view name) { return asset(); }
}	 // namespace sek