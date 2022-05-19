//
// Created by switchblade on 2022-04-04.
//

#define _CRT_SECURE_NO_WARNINGS

#include "assets.hpp"

#include "sekhmet/serialization/json.hpp"
#include "sekhmet/serialization/ubjson.hpp"

#ifdef SEK_OS_WIN
#define MANIFEST_FILE_NAME L".manifest"
#else
#define MANIFEST_FILE_NAME ".manifest"
#endif

#define RECORD_ERROR_MSG "Invalid asset record"

namespace sek
{
	namespace detail
	{
		template class SEK_API_EXPORT service<asset_repository>;

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

		void master_package::acquire_impl() { ref_count++; }
		void master_package::release_impl()
		{
			if (ref_count.fetch_sub(1) == 1) [[unlikely]]
				delete this;
		}
		void package_base::acquire() { master()->acquire_impl(); }
		void package_base::release() { master()->release_impl(); }

		std::filesystem::path loose_asset_info::full_path() const { return parent->path / file; }
		filemap asset_handle::to_filemap(filemap::openmode mode) const
		{
			/* Reset "out" mode if the parent is an archive. */
			if (parent()->is_archive() && (mode & filemap::out)) [[unlikely]]
				mode ^= filemap::out;

			const auto slice = info->as_archive()->slice;
			const auto &path = parent()->path;
			if (!std::filesystem::exists(path)) [[unlikely]]
				throw std::runtime_error("Invalid asset package archive path");
			return filemap{parent()->path, slice.first, slice.second, mode};
		}

		void asset_database::merge(const asset_database &other)
		{
			for (auto entry : other.assets)
			{
				const auto id = entry.first;
				const auto ptr = entry.second;

				assets.emplace(id, ptr);
				if (auto &name = ptr->name; !name.empty()) [[likely]]
					name_table.emplace(name.sv(), id);
			}
		}
	}	 // namespace detail

	asset asset::load(uuid id) { return asset(); }
	asset asset::load(std::string_view name) { return asset(); }
}	 // namespace sek