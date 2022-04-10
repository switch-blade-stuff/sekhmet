//
// Created by switchblade on 2022-04-04.
//

#define _CRT_SECURE_NO_WARNINGS

#include "assets.hpp"

#include <cstring>
#include <fstream>
#include <memory>

#include "adt/toml_archive.hpp"
#include "adt/ubj/ubj_archive.hpp"

#ifdef SEK_OS_WIN
#define MANIFEST_FILE_NAME L".manifest"
#else
#define MANIFEST_FILE_NAME ".manifest"
#endif

#define RECORD_ERROR_MSG "Invalid asset record"

namespace sek
{
	template<>
	SEK_API_EXPORT std::atomic<asset_repository *> &basic_service<asset_repository>::global_ptr() noexcept
	{
		static std::atomic<asset_repository *> value;
		return value;
	}
	std::shared_mutex &asset_repository::global_mtx() noexcept
	{
		static std::shared_mutex value;
		return value;
	}

	namespace detail
	{
		void loose_asset_record::serialize(adt::node &node) const
		{
			auto &table = (node = adt::table{{"id", id}, {"tags", tags}, {"path", file_path.string()}}).as_table();
			if (!metadata_path.empty()) table.emplace("metadata", metadata_path.string());
		}
		void loose_asset_record::deserialize(adt::node &&node)
		{
			auto &table = node.as_table();
			{
				auto id_iter = table.find("id");
				auto path_iter = table.find("path");
				if (id_iter != table.end() && path_iter != table.end()) [[likely]]
				{
					std::move(id_iter->second).get(id);
					file_path.assign(std::move(path_iter->second.as_string()));
				}
				else
					throw std::runtime_error(RECORD_ERROR_MSG);
			}

			if (auto tags_iter = table.find("tags"); tags_iter != table.end()) std::move(tags_iter->second).get(tags);
			if (auto metadata_iter = table.find("path"); metadata_iter != table.end())
				metadata_path.assign(std::move(metadata_iter->second.as_string()));
		}

		void archive_asset_record::serialize(adt::node &node) const
		{
			node = adt::sequence{id, tags, file_offset, file_size, metadata_offset, metadata_size};
		}
		void archive_asset_record::deserialize(adt::node &&node)
		{
			if (auto &seq = node.as_sequence(); seq.size() >= 6) [[likely]]
			{
				std::move(seq[0]).get(id);
				std::move(seq[1]).get(tags);
				std::move(seq[2]).get(file_offset);
				std::move(seq[3]).get(file_size);
				std::move(seq[4]).get(metadata_offset);
				std::move(seq[5]).get(metadata_size);
			}
			else
				throw std::runtime_error(RECORD_ERROR_MSG);
		}

		void package_fragment::serialize(adt::node &node) const { node = adt::table{{"assets", assets}}; }
		void master_package::serialize(adt::node &node) const
		{
			auto &table = node.as_table();
			table.emplace("master", true);
			if (!fragments.empty()) [[likely]]
			{
				auto &out_sequence = table.emplace("fragments", adt::sequence{}).first->second.as_sequence();
				for (auto &fragment : fragments) out_sequence.emplace_back(relative(fragment.path, path).string());
			}
			package_fragment::serialize(node);
		}

		struct package_info
		{
			adt::node manifest = {};
			package_fragment::flags_t flags = {};
		};
		static package_info get_package_info(const std::filesystem::path &path)
		{
			package_info result;
			if (is_directory(path))
			{
				result.flags = package_fragment::LOOSE_PACKAGE;
				if (std::ifstream manifest_stream{path / MANIFEST_FILE_NAME}; manifest_stream.is_open()) [[likely]]
				{
					// adt::toml_input_archive{manifest_stream}.read(result.manifest);
				}
			}
			else if (std::ifstream manifest_stream{path, std::ios::binary}; manifest_stream.is_open()) [[likely]]
			{
				/* Check that the package has a valid signature.
				 * TODO: Use versioned signature to allow for future expansion. */
				constexpr auto sign_size = sizeof(SEK_PACKAGE_SIGNATURE);
				char sign[sign_size];

				if (static_cast<std::size_t>(manifest_stream.readsome(sign, sign_size)) == sign_size &&
					!strncmp(sign, SEK_PACKAGE_SIGNATURE, sign_size))
					adt::ubj_input_archive{manifest_stream}.read(result.manifest);
			}
			return result;
		}

		void package_fragment::deserialize(adt::node &&node)
		{
			auto &table = node.as_table();
			if (auto assets_iter = table.find("assets"); assets_iter != table.end()) [[likely]]
			{
				auto &data = assets_iter->second.as_sequence();
				assets.reserve(data.size());
				for (auto &n : data) assets.emplace_back(this).ptr->deserialize(std::move(n));
			}
		}
		void master_package::deserialize(adt::node &&node)
		{
			auto &table = node.as_table();
			if (auto fragments_iter = table.find("fragments"); fragments_iter != table.end())
			{
				auto &fragments_seq = fragments_iter->second.as_sequence();
				fragments.reserve(fragments_seq.size());
				for (auto &fragment : fragments_seq)
				{
					auto fragment_path = path / fragment.as_string();
					auto info = get_package_info(fragment_path);
					add_fragment(std::move(fragment_path), info.flags).deserialize(std::move(info.manifest));
				}
			}
			package_fragment::deserialize(std::move(node));
		}

		master_package *load_package(std::filesystem::path &&path)
		{
			try
			{
				auto info = get_package_info(path);
				auto &table = info.manifest.as_table();
				if (auto flag_iter = table.find("master"); flag_iter != table.end() && flag_iter->second.as_bool())
				{
					auto package = std::make_unique<master_package>(std::move(path), info.flags);
					package->deserialize(std::move(info.manifest));
					return package.release();
				}
			}
			/* Ugly catch ladder. Needed here since we want to return nullptr on invalid package.
			 * `adt::archive_error` & `adt::node_error` handle invalid parsing & deserialization of package manifest,
			 * while `std::logic_error` & `std::runtime_error` handle edge cases such as filesystem errors &
			 * out-of-range hashmap access. */
			catch (adt::archive_error &)
			{
				/* Log archive error. */
			}
			catch (adt::node_error &)
			{
				/* Log node error. */
			}
			catch (std::logic_error &)
			{
				/* Log logic error. */
			}
			catch (std::runtime_error &)
			{
				/* Log runtime error. */
			}
			/* Other exceptions are fatal. */
			return nullptr;
		}
	}	 // namespace detail

	asset asset_repository::find(std::string_view id) const noexcept
	{
		auto asset_iter = assets.find(id);
		if (asset_iter == assets.end()) [[unlikely]]
			return asset{};
		return asset{asset_iter->second};
	}

	asset_repository &asset_repository::merge(asset_repository &&other)
	{
		/* Transfer & clear other's assets. */
		assets.reserve(assets.size() + other.assets.size());
		for (auto item = other.assets.begin(), end = other.assets.end(); item != end; ++item)
			assets.insert(other.assets.extract(item));
		other.assets.clear();

		/* Transfer & clear other's packages. */
		packages.reserve(packages.size() + other.packages.size());
		for (auto item = other.packages.begin(), end = other.packages.end(); item != end; ++item)
			packages.insert(other.packages.extract(item));
		other.packages.clear();

		return *this;
	}
}	 // namespace sek