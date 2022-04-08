//
// Created by switchblade on 2022-04-04.
//

#include "assets.hpp"

#ifdef SEK_OS_WIN
#define MANIFEST_FILE_NAME L".manifest"
#else
#define MANIFEST_FILE_NAME ".manifest"
#endif

namespace sek::detail
{
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
}	 // namespace sek::detail