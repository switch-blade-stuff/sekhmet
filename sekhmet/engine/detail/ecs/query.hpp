/*
 * Created by switchblade on 19/07/22
 */

#pragma once

#include "component_set.hpp"

namespace sek::engine
{
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
	 * @tparam OrderBy Component types to order the query by.
	 * @tparam Include Component types to include by the query.
	 * @tparam Exclude Component types to exclude from the query.
	 *
	 * @note Included and excluded component sets must not intersect.
	 * @note Ordered-by components cannot be fixed-storage (`component_traits::is_fixed` must not be defined).
	 * @note Ordered-by components are implicitly included.
	 * @note Query must include at least 1 component. */
	template<typename... OrderBy, typename... Include, typename... Exclude>
	class entity_query<order_by_t<OrderBy...>, include_t<Include...>, exclude_t<Exclude...>>
	{
		static_assert(!(is_in_v<Exclude, OrderBy..., Include...> || ...),
					  "Excluded and included component types must not intersect");
		static_assert((!fixed_component<OrderBy> && ...), "Cannot order fixed-storage components");

		using order_seq = unique_type_seq_t<type_seq_t<OrderBy...>>;
		using include_seq = unique_type_seq_t<type_seq_t<OrderBy..., Include...>>;
		using exclude_seq = unique_type_seq_t<type_seq_t<Exclude...>>;

		static_assert(include_seq::size != 0, "Query must include at least 1 component");

	public:
	private:
	};

	/** @brief Helper alias used to define a non-ordering query. */
	template<typename Include, typename Exclude = exclude_t<>>
	using weak_query = entity_query<order_by_t<>, Include, Exclude>;
}	 // namespace sek::engine