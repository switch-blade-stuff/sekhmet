/*
 * Created by switchblade on 10/07/22
 */

#include "resources.hpp"

#include "sekhmet/detail/assert.hpp"

#include <fmt/format.h>

template class SEK_API_EXPORT sek::service<sek::access_guard<sek::engine::resource_cache, std::recursive_mutex>>;

namespace sek::engine
{
	resource_error::~resource_error() = default;

	[[noreturn]] static void invalid_asset(const asset_ref &asset)
	{
		throw resource_error(fmt::format("Asset \"{}\" {{{}}} is not a valid resource", asset.name(), asset.id().to_string()));
	}
	[[noreturn]] static void invalid_type(std::string_view name)
	{
		throw resource_error(fmt::format("\"{}\" is not a valid resource type", name));
	}

	resource_cache::metadata_t::metadata_t(const asset_ref &asset)
	{
		if (auto metadata = asset.metadata(); !metadata.empty()) [[likely]]
		{
			std::uint8_t version = 0;
			metadata.read(&version, sizeof(version));

			if (version != 1) [[unlikely]]
				goto bad_asset;

			/* Initialize & read the resource type. */
			std::string name(static_cast<std::size_t>(metadata.size() - metadata.tell()), '\0');
			if (metadata.read(name.data(), name.size()) == 0 || name.back() != '\0') [[unlikely]]
				goto bad_asset;

			/* Trim characters after null terminator & get the type. */
			name.erase(name.find('\0'));
			type = type_info::get(name);
			if (!type) [[unlikely]]
				goto bad_type;

			attr = type.get_attribute<attribute_t>().as_ptr<const attribute_t>();
			if (!attr) [[unlikely]]
			{
			bad_type:
				invalid_type(name);
			}
		}
		else
		{
		bad_asset:
			invalid_asset(asset);
		}
	}

	any resource_cache::load_anonymous(metadata_t metadata, asset_source &src)
	{
		auto result = metadata.type.construct();
		float dummy;
		metadata.attr->m_deserialize(result.data(), src, dummy);
		return result;
	}
	any resource_cache::load_anonymous(type_info type, asset_source &src)
	{
		metadata_t meta;
		if ((meta.type = type) && (meta.attr = type.get_attribute<attribute_t>().as_ptr<const attribute_t>())) [[likely]]
			return load_anonymous(meta, src);
		invalid_type(type.name());
	}
	any resource_cache::load_anonymous(const asset_ref &asset)
	{
		auto src = asset.open();
		return load_anonymous(metadata_t{asset}, src);
	}

	std::pair<std::shared_ptr<void>, resource_cache::metadata_t *> resource_cache::load_impl(const asset_ref &asset, bool copy)
	{
		const auto id = asset.id();

		std::shared_ptr<void> ptr;
		metadata_t *metadata;

		if (auto existing = m_cache.find(id); existing == m_cache.end()) [[unlikely]]
		{
			/* Create new entry. */
			existing = m_cache.emplace(id, asset).first;
			metadata = &existing->second.metadata;
			m_types[metadata->type.name()].emplace(id);
			goto init_value;
		}
		else
		{
			metadata = &existing->second.metadata;
			if (existing->second.data.expired())
			{
				/* If the entry is empty, initialize new value. */
			init_value:
				ptr = metadata->attr->m_instantiate();
				existing->second.data = ptr;

				/* Read the entry from the asset. */
				auto src = asset.open();
				float dummy;
				metadata->attr->m_deserialize(ptr.get(), src, dummy);
			}
			else
				ptr = existing->second.data.lock();
		}

		/* At this point we have an empty entry with a valid resource type. */
		if (copy) [[unlikely]]
			ptr = metadata->attr->m_copy(ptr.get());
		return {ptr, metadata};
	}

	std::size_t resource_cache::clear(type_info type)
	{
		std::size_t result = 0;
		if (auto iter = m_types.find(type.name()); iter != m_types.end()) [[likely]]
		{
			/* Remove all cache entries from the resource cache. */
			for (auto id : iter->second) result += m_cache.erase(id);
			/* Remove type cache entry. */
			m_types.erase(iter);
		}
		return result;
	}
	void resource_cache::clear(uuid id)
	{
		if (auto iter = m_cache.find(id); iter != m_cache.end()) [[likely]]
		{
			/* Remove the resource from the type cache's set. */
			auto type_iter = m_types.find(iter->second.metadata.type.name());
			SEK_ASSERT(type_iter != m_types.end(), "Cached resource's type must be reflected");
			type_iter->second.erase(id);

			/* Remove resource cache entry. */
			m_cache.erase(id);
		}
	}
	void resource_cache::clear()
	{
		m_types.clear();
		m_cache.clear();
	}
}	 // namespace sek::engine
