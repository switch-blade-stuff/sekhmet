//
// Created by switchblade on 2022-04-04.
//

#define _CRT_SECURE_NO_WARNINGS

#include "assets.hpp"

#include <fstream>
#include <memory>

#include "adt/toml_archive.hpp"
#include "adt/ubj/ubj_archive.hpp"

#ifdef SEK_OS_WIN
#define MANIFEST_FILE_NAME L".manifest"
#else
#define MANIFEST_FILE_NAME ".manifest"
#endif

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
			std::move(node.at("id")).get(id);
			std::move(node.at("tags")).get(tags);

			file_path.assign(std::move(node.at("path").as_string()));
			if (node.as_table().contains("metadata")) metadata_path.assign(std::move(node.at("metadata").as_string()));
		}

		void archive_asset_record::serialize(adt::node &node) const
		{
			node = adt::sequence{id, tags, file_offset, file_size, metadata_offset, metadata_size};
		}
		void archive_asset_record::deserialize(adt::node &&node)
		{
			if (node.as_sequence().size() >= 6) [[likely]]
			{
				std::move(node[0]).get(id);
				std::move(node[1]).get(tags);
				std::move(node[2]).get(file_offset);
				std::move(node[3]).get(file_size);
				std::move(node[4]).get(metadata_offset);
				std::move(node[5]).get(metadata_size);
			}
			else
				throw adt::node_error("Invalid archive record size");
		}

		package_base::record_handle::record_handle(package_fragment *parent)
		{
			if (parent->flags & LOOSE_PACKAGE)
				ptr = new loose_asset_record{parent};
			else
				ptr = new archive_asset_record{parent};
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
					//					adt::toml_input_archive{manifest_stream}.read(result.manifest);
				}
			}
			else if (std::ifstream manifest_stream{path, std::ios::binary}; manifest_stream.is_open()) [[likely]]
			{
				/* Check that the package has a valid signature. */
				constexpr auto sign_size = sizeof(SEK_PACKAGE_SIGNATURE);
				char sign[sign_size];

				if (static_cast<std::size_t>(manifest_stream.readsome(sign, sign_size)) == sign_size &&
					std::equal(std::begin(sign), std::end(sign), std::begin(SEK_PACKAGE_SIGNATURE), std::end(SEK_PACKAGE_SIGNATURE)))
					adt::ubj_input_archive{manifest_stream}.read(result.manifest);
			}
			return result;
		}

		void package_fragment::deserialize(adt::node &&node)
		{
			auto &data = node.at("assets").as_sequence();
			assets.reserve(data.size());
			for (auto &n : data) assets.emplace_back(this).ptr->deserialize(std::move(n));
		}
		void master_package::deserialize(adt::node &&node)
		{
			if (node.as_table().contains("fragments"))
			{
				auto &fragments_seq = node.at("fragments").as_sequence();
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
			/* Only deserialization exceptions are recoverable, since they indicate an invalid package and thus will return nullptr.
			 * Any other exceptions are either caused by fatal errors or by filesystem errors and are thus non-recoverable. */
			catch (adt::archive_error &)
			{
				/* Log archive error. */
			}
			catch (adt::node_error &)
			{
				/* Log node error. */
			}
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