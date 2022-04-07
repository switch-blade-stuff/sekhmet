//
// Created by switchblade on 2022-04-04.
//

#include "assets.hpp"

#include <atomic>
#include <vector>

#include "../math/detail/util.hpp"
#include "hset.hpp"

#ifdef SEK_OS_WIN
#define MANIFEST_FILE_NAME L".manifest"
#else
#define MANIFEST_FILE_NAME ".manifest"
#endif

namespace sek
{
	namespace detail
	{
		struct asset_record_base
		{
			package_fragment *parent;

			std::string id;
			hset<std::string> tags;
		};

		struct loose_asset_record : asset_record_base
		{
			std::filesystem::path file_path;
			std::filesystem::path metadata_path;
		};

		void serialize(adt::node &node, const loose_asset_record &record)
		{
			node = adt::table{
				{"id", record.id},
				{"tags", record.tags},
				{"path", record.file_path.string()},
			};

			if (!record.metadata_path.empty()) node["metadata"] = record.metadata_path.string();
		}
		void deserialize(const adt::node &node, loose_asset_record &record)
		{
			node.at("id").get(record.id);
			node.at("tags").get(record.tags);
			record.file_path.assign(node.at("path").as_string());

			if (node.as_table().contains("metadata")) record.metadata_path.assign(node.at("metadata").as_string());
		}

		struct archive_asset_record : asset_record_base
		{
			std::ptrdiff_t file_offset;
			std::ptrdiff_t file_size;
			std::ptrdiff_t metadata_offset;
			std::ptrdiff_t metadata_size;
		};

		void serialize(adt::node &node, const archive_asset_record &record)
		{
			node = adt::sequence{
				record.id,
				record.tags,
				record.file_offset,
				record.file_size,
				record.metadata_offset,
				record.metadata_size,
			};
		}
		void deserialize(const adt::node &node, archive_asset_record &record)
		{
			if (node.as_sequence().size() >= 6) [[likely]]
			{
				node[0].get(record.id);
				node[1].get(record.tags);
				node[2].get(record.file_offset);
				node[3].get(record.file_size);
				node[4].get(record.metadata_offset);
				node[5].get(record.metadata_size);
			}
			else
				throw adt::node_error("Invalid archive record size");
		}

		struct package_fragment
		{
			enum flags_t : int
			{
				LOOSE_PACKAGE = 1,
			};

			explicit package_fragment(flags_t flags) : flags(flags) { init_assets(); }
			virtual ~package_fragment() { destroy_assets(); }

			virtual master_package *get_master() { return master; }

			template<typename... Args>
			void init_assets(Args &&...args)
			{
				if (flags & LOOSE_PACKAGE)
					std::construct_at(&loose_assets, std::forward<Args>(args)...);
				else
					std::construct_at(&archive_assets, std::forward<Args>(args)...);
			}
			void destroy_assets()
			{
				if (flags & LOOSE_PACKAGE)
					std::destroy_at(&loose_assets);
				else
					std::destroy_at(&archive_assets);
			}

			union
			{
				std::atomic<std::size_t> ref_count;
				master_package *master;

				std::byte padding[math::max(sizeof(std::atomic<std::size_t>), sizeof(master_package *))] = {};
			};

			flags_t flags;

			union
			{
				std::vector<archive_asset_record> archive_assets;
				std::vector<loose_asset_record> loose_assets;
			};
		};

		struct master_package final : package_fragment
		{
			explicit master_package(flags_t flags) : package_fragment(flags) {}
			~master_package() final = default;

			master_package *get_master() final { return this; }

			void acquire() { ref_count += 1; }
			void release()
			{
				if (ref_count.fetch_sub(1) == 1) [[unlikely]]
					delete this;
			}
		};

		void acquire(package_fragment *fragment)
		{
			if (fragment) [[likely]]
				fragment->get_master()->acquire();
		}
		void release(package_fragment *fragment)
		{
			if (fragment) [[likely]]
				fragment->get_master()->acquire();
		}
		master_package *get_master(package_fragment *fragment) { return fragment ? fragment->get_master() : nullptr; }
	}	 // namespace detail
}	 // namespace sek