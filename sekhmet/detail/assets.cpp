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

		void package_base::acquire() { master()->ref_count++; }
		void package_base::release()
		{
			auto m = master();
			if (m->ref_count.fetch_sub(1) == 1) [[unlikely]]
				delete m;
		}
	}	 // namespace detail
}	 // namespace sek