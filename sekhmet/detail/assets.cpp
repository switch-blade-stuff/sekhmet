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

#include "sekhmet/serialization/json.hpp"
#include "sekhmet/serialization/ubjson.hpp"
#include "zstd_util.hpp"

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

		using loose_input_archive = json::basic_input_archive<json::allow_comments>;
		using loose_output_archive = json::basic_output_archive<json::inline_arrays>;
		using packed_input_archive = ubj::basic_input_archive<ubj::highp_error>;
		using packed_output_archive = ubj::basic_output_archive<ubj::fixed_size | ubj::fixed_type>;

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

		void deserialize(loose_asset_info &info, loose_input_archive::archive_frame &archive)
		{
			std::string_view tmp_str;

			archive >> serialization::keyed_entry("file", tmp_str);
			info.file = tmp_str;

			if (archive.try_read(serialization::keyed_entry("name", tmp_str)) && !tmp_str.empty()) [[likely]]
				info.name = tmp_str;
		}
		void deserialize(archive_asset_info &info, packed_input_archive::archive_frame &archive)
		{
			archive >> info.slice.first >> info.slice.second;
			if (std::string_view tmp; archive.try_read(tmp) && !tmp.empty()) [[likely]]
				info.name = tmp;
		}
		void serialize(const loose_asset_info &info, loose_output_archive::archive_frame &archive)
		{
#ifndef SEK_OS_WIN
			archive << serialization::keyed_entry("file", info.file.native());
#else
			archive << serialization::keyed_entry("file", info.file.string());
#endif
			if (!info.name.empty()) [[likely]]
				archive << serialization::keyed_entry("name", info.name);
		}
		void serialize(const archive_asset_info &info, packed_output_archive::archive_frame &archive)
		{
			archive << serialization::array_mode();
			archive << info.slice.first << info.slice.second;
			if (!info.name.empty()) [[likely]]
				archive << serialization::keyed_entry("name", info.name);
		}

		std::filesystem::path loose_asset_info::full_path() const { return parent->path / file; }

		void package_base::acquire() { master()->acquire_impl(); }
		void package_base::release() { master()->release_impl(); }
		void master_package::acquire_impl() { ref_count++; }
		void master_package::release_impl()
		{
			if (ref_count.fetch_sub(1) == 1) [[unlikely]]
				delete this;
		}

		archive_master_package::~archive_master_package()
		{
			for (auto entry : database.assets) destroy_asset_info(entry.second);
		}
		void archive_master_package::destroy_asset_info(asset_info_base *info)
		{
			std::destroy_at(static_cast<archive_asset_info *>(info));
		}
		loose_master_package::~loose_master_package()
		{
			for (auto entry : database.assets) destroy_asset_info(entry.second);
		}
		void loose_master_package::destroy_asset_info(asset_info_base *info)
		{
			std::destroy_at(static_cast<loose_asset_info *>(info));
		}

		static auto &asset_thread_pool()
		{
			static thread_pool instance;
			return instance;
		}

		std::string asset_handle::read_archive() const
		{
			constexpr auto writer = +[](std::string *s, const void *src, std::size_t n)
			{
				auto chars = static_cast<const char *>(src);
				s->insert(s->size(), chars, n);
				return n;
			};

			auto &ctx = zstd_thread_ctx::instance();
			auto &pool = asset_thread_pool();

			auto archive_info = info->as_archive();
			auto path = archive_info->parent->path;
			auto slice = archive_info->slice;
			std::string result;

			FILE *archive_file = fopen(path.c_str(), "rb");
			{
				if (archive_file == nullptr) [[unlikely]]
				{
					std::string msg = "Failed to open asset archive at path \"";
					throw std::runtime_error(msg.append(path.native()).append(1, '\"'));
				}

#if defined(_POSIX_C_SOURCE)
#if _FILE_OFFSET_BITS < 64
				auto err = fseeko64(archive_file, static_cast<off64_t>(slice.first), SEEK_SET);
#else
				auto err = fseeko(archive_file, static_cast<off_t>(slice.first), SEEK_SET);
#endif
#elif defined(SEK_OS_WIN)
				auto err = _fseeki64(archive_file, static_cast<__int64>(slice.first), SEEK_SET);
#else
				auto err = fseek(archive_file, static_cast<long>(slice.first), SEEK_SET);
#endif
				if (err != 0) [[unlikely]]
				{
					std::string msg = "Failed to seek asset archive at path \"";
					msg.append(path.native()).append("\" to position [").append(std::to_string(slice.first)).append(1, ']');
					throw std::runtime_error(msg);
				}
			}

			try
			{
				ctx.decompress(pool, zstd_thread_ctx::file_reader{archive_file, slice.second}, sek::delegate{writer, result});
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