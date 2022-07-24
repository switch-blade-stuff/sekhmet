/*
 * Created by switchblade on 19/07/22
 */

#pragma once

#include "fwd.hpp"

namespace sek::engine
{
	/** @brief Helper type used to obtain traits of a component type.
	 *
	 * Component traits must contain a compile-time constant of type `std::size_t` named `page_size`, specifying
	 * size of allocation pages used by component pools, a `allocator_type` typedef used to specify the allocator of
	 * used to allocate components.
	 *
	 * Optionally, traits may define an `is_fixed` typedef which, if present, will prevent components of this type
	 * from being sorted either by a component pool or ordering queries. */
	template<typename T>
	struct component_traits
	{
		constexpr static std::size_t page_size = 1024;
		typedef std::allocator<T> allocator_type;
	};

	template<typename T>
	concept fixed_component = requires { typename component_traits<T>::is_fixed; };

}	 // namespace sek::engine