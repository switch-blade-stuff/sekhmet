/*
 * Created by switchblade on 19/07/22
 */

#pragma once

#include <memory>

#include "../../../detail/ebo_base_helper.hpp"

namespace sek
{
	class entity_t;

	template<typename>
	struct component_traits;

	template<typename>
	class basic_entity_set;
	template<typename>
	class component_set;

	template<typename...>
	struct owned_t;
	template<typename...>
	struct included_t;
	template<typename...>
	struct optional_t;
	template<typename...>
	struct excluded_t;

	class entity_world;

	template<typename, typename, typename, typename, typename>
	class entity_query;
	template<typename, typename, typename>
	class component_view;
	template<typename, typename, typename, typename>
	class component_collection;

}	 // namespace sek