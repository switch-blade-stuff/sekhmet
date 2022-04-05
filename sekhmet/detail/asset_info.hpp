//
// Created by switchblade on 2022-04-04.
//

#pragma once

#include <atomic>
#include <filesystem>
#include <memory>

#include "adt/node.hpp"
#include "adt/serialize_impl.hpp"
#include "hset.hpp"
#include "uuid.hpp"

namespace sek::detail
{
	struct asset_package_base;
	struct asset_record_base
	{
		asset_package_base *parent;

		std::string id;
		hset<std::string> tags;
	};

	struct loose_asset_record : asset_record_base
	{
		std::filesystem::path asset_path;
		std::filesystem::path metadata_path;
	};

	void serialize(adt::node &node, const loose_asset_record &record)
	{
		node = adt::table{
			{"id", record.id},
			{"file", record.asset_path.string()},
		};

		if (!record.metadata_path.empty()) node.as_table().emplace("metadata", record.metadata_path.string());
		if (!record.tags.empty()) node.as_table()["tags"].set(record.tags);
	}
	void deserialize(const adt::node &node, loose_asset_record &record)
	{
		if (node.is_table()) [[likely]]
		{
			record.id = node.at("id").as_string();
			record.asset_path = node.at("file").as_string();

			if (node.as_table().contains("tags"))
			{
				record.tags.clear();
				node.at("tags").get(record.tags);
			}
			if (node.as_table().contains("metadata")) record.asset_path = node.at("metadata").as_string();
		}
	}

	static_assert(adt::detail::serializable_type<loose_asset_record>);
	static_assert(adt::detail::deserializable_type<loose_asset_record>);

	struct archive_asset_record : asset_record_base
	{
		std::ptrdiff_t asset_offset = 0;
		std::ptrdiff_t asset_size = 0;
		std::ptrdiff_t metadata_offset = 0;
		std::ptrdiff_t metadata_size = 0;
	};

	void serialize(adt::node &node, const archive_asset_record &record)
	{
		node = adt::sequence{
			record.id,
			record.asset_offset,
			record.asset_size,
			record.metadata_offset,
			record.metadata_size,
		};

		if (!record.tags.empty())
		{
			adt::node tags_node;
			tags_node.set(record.tags);
			node.as_sequence().push_back(std::move(tags_node));
		}
	}
	void deserialize(const adt::node &node, archive_asset_record &record)
	{
		if (node.is_sequence()) [[likely]]
		{
			auto &seq = node.as_sequence();
			if (seq.size() >= 5) [[likely]]
			{
				seq[0].get(record.id);
				seq[1].get(record.asset_offset);
				seq[2].get(record.asset_size);
				seq[3].get(record.metadata_offset);
				seq[4].get(record.metadata_size);
				if (seq.size() > 5 && seq[5].is_sequence()) seq[5].get(record.tags);
			}
		}
	}

	static_assert(adt::detail::serializable_type<archive_asset_record>);
	static_assert(adt::detail::deserializable_type<archive_asset_record>);

	struct asset_collection
	{
		hmap<std::string_view, asset_record_base *> asset_map;
	};

	struct asset_package_base : asset_collection
	{
		enum flags_t : int
		{
			ARCHIVE_PACKAGE = 1,
			READ_ONLY_PACKAGE = ARCHIVE_PACKAGE,
			MASTER_PACKAGE = 2,
		};

		[[nodiscard]] constexpr bool is_read_only() const noexcept
		{
			return (flags & READ_ONLY_PACKAGE) == READ_ONLY_PACKAGE;
		}
		[[nodiscard]] constexpr bool is_archive() const noexcept { return flags & ARCHIVE_PACKAGE; }
		[[nodiscard]] constexpr bool is_master() const noexcept { return flags & MASTER_PACKAGE; }

		[[nodiscard]] constexpr asset_package_base *get_master() noexcept
		{
			return is_master() ? this : master->get_master();
		}

		constexpr void acquire() noexcept
		{
			if (!is_master()) [[unlikely]]
				master->acquire();
			else
				ref_count.fetch_add(1);
		}
		constexpr void release()
		{
			if (!is_master()) [[unlikely]]
				master->release();
			else if (ref_count.fetch_sub(1) == 1) [[unlikely]]
				delete this;
		}

		union
		{
			std::atomic<std::size_t> ref_count = 0;
			asset_package_base *master; /* Non-master packages do not have their own reference counter. */
		};

		flags_t flags;

		std::filesystem::path path;
		std::vector<std::unique_ptr<asset_package_base>> fragments;
	};

	struct loose_package : asset_package_base
	{
		std::vector<loose_asset_record> records;
	};

	struct archive_package : asset_package_base
	{
		std::vector<archive_asset_record> records;
	};
}	 // namespace sek::detail