/*
 * Created by switchblade on 19/07/22
 */

#pragma once

#include <memory>

namespace sek::engine
{
	class entity_t;

	template<typename = std::allocator<entity_t>, bool = false>
	class basic_entity_set_old;

	template<typename T>
	struct component_traits;
	template<typename T, typename = std::allocator<T>>
	class basic_component_pool;
	template<typename T>
	class component_storage;

	template<typename...>
	struct order_by_t
	{
	};
	template<typename...>
	struct include_t
	{
	};
	template<typename...>
	struct exclude_t
	{
	};
	template<typename...>
	class entity_query;

	class entity_world;
}	 // namespace sek::engine