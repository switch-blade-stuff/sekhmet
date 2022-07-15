/*
 * Created by switchblade on 14/07/22
 */

#pragma once

#include "sekhmet/dense_map.hpp"

#include "entity.hpp"

namespace sek::engine
{
	template<typename...>
	class entity_query;
	class entity_world;

	/** @brief Query used to obtain a set of components to iterate over.
	 *
	 * Queries are used to obtain a (potentially ordered) set of components from a world to iterate over and can
	 * capture, exclude and order-by component types.
	 *
	 * A query will iterate over all entities that contain captured components, excluding any entity that
	 * contains excluded components. For example, query capturing `component_a` and excluding `component_b` will
	 * iterate over entities containing `component_a` but not `component_b`.
	 *
	 * Ordering queries sort the respective component types. This requires ordered components to be
	 * allocated from non-fixed pools. Ordered components' sort order will be locked for as long as the query exists
	 * and will be automatically updated when components are created or removed.
	 *
	 * Ordering queries can "specialize" other queries (ex. ordering query for `component_a` can be specialized
	 * to order by `component_a` and `component_b`). However, if an ordering query's components are conflicting
	 * with another query (ex. `component_b`, `component_c` and `component_a`, `component_c`), an exception will
	 * be thrown, as components will not be sorted. An exception will also be thrown if components' pools do
	 * not support sorting (are fixed).
	 *
	 * @tparam Capture Component types to capture within the query.
	 * @tparam Exclude Components to exclude from the query.
	 * @tparam Order Component types to order the query by. */
	template<typename... Capture, typename... Exclude, typename... Order>
	class entity_query<type_seq_t<Capture...>, type_seq_t<Exclude...>, type_seq_t<Order...>>
	{
	};

	/** @brief A world is a special container used to associate entities with their components.
	 *
	 * Internally, a world contains a table of component pools (and dense index arrays) indexed by their type,
	 * and a sparse array of entities used to associate component indexes to their entities.
	 *
	 * Worlds also support component events, allowing the user to execute code when a components are created,
	 * removed or modified.
	 *
	 * @warning Operations on entity worlds are not thread-safe and must be synchronized externally
	 * (ex. through an access guard). */
	class entity_world
	{
	};
}	 // namespace sek::engine