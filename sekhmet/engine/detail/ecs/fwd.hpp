/*
 * Created by switchblade on 19/07/22
 */

#pragma once

#include <memory>

namespace sek::engine
{
	class entity_t;

	template<typename T>
	struct component_traits;

	template<typename = void, typename = void, typename = std::allocator<entity_t>>
	class basic_entity_set;

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