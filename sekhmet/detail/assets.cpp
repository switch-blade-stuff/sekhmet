//
// Created by switchblade on 2022-04-04.
//

#define _CRT_SECURE_NO_WARNINGS

#include "assets.hpp"

#include <cstdio>
#include <cstring>
#include <memory>

#ifdef SEK_OS_WIN
#define MANIFEST_FILE_NAME L".manifest"
#define OS_FOPEN(path, mode) _wfopen(path, L##mode)
#else
#define MANIFEST_FILE_NAME ".manifest"
#define OS_FOPEN(path, mode) fopen(path, mode)
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
			node = adt::table{
				{"id", id},
				{"tags", tags},
				{"path", file_path.string()},
			};
			if (!metadata_path.empty()) node["metadata"] = metadata_path.string();
		}
		void loose_asset_record::deserialize(adt::node &&node)
		{
			std::move(node.at("id")).get(id);
			std::move(node.at("tags")).get(tags);

			file_path.assign(node.at("path").as_string());
			if (node.as_table().contains("metadata")) metadata_path = node.at("metadata").as_string();
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
			node["master"].set(true);
			if (!fragments.empty()) [[likely]]
			{
				auto &out_sequence = node.as_table().emplace("fragments", adt::sequence{}).first->second.as_sequence();
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
			FILE *manifest_file;
			if (is_directory(path))
			{
				result.flags = package_fragment::LOOSE_PACKAGE;
				if ((manifest_file = OS_FOPEN((path / MANIFEST_FILE_NAME).c_str(), "r")) != nullptr) [[likely]]
				{
					/* TODO: read TOML manifest. */
				}
			}
			else if ((manifest_file = OS_FOPEN(path.c_str(), "rb")) != nullptr) [[likely]]
			{
				/* Check that the package has a valid signature. */
				constexpr auto sign_size = sizeof(SEK_PACKAGE_SIGNATURE);
				char sign[sign_size];
				if (fread(sign, 1, sign_size, manifest_file) == sign_size && !memcmp(sign, SEK_PACKAGE_SIGNATURE, sign_size))
				{
					/* TODO: read UBJson manifest. */
				}
			}
			return result;
		}

		void package_fragment::deserialize(adt::node &&node)
		{
			auto &data = node.at("assets").as_sequence();
			assets.reserve(data.size());
			for(auto &n : data) assets.emplace_back(this).ptr->deserialize(std::move(n));
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
			catch (adt::node_error &)
			{
				/* Only deserialization exceptions are recoverable, since they indicate an invalid package and thus will return nullptr.
				 * Any other exceptions are either caused by fatal errors or by filesystem errors and are thus non-recoverable. */
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
		assets.reserve(assets.size() + other.assets.size());
		for (auto item = other.assets.begin(), end = other.assets.end(); item != end; ++item)
			assets.insert(other.assets.extract(item));
		other.assets.clear();

		packages.reserve(packages.size() + other.packages.size());
		for (auto item = other.packages.begin(), end = other.packages.end(); item != end; ++item)
			packages.insert(other.packages.extract(item));
		other.packages.clear();

		return *this;
	}
}	 // namespace sek