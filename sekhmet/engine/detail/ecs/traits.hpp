/*
 * Created by switchblade on 19/07/22
 */

#pragma once

#include "fwd.hpp"

namespace sek
{
	/** @brief Helper type used to obtain traits of a component type.
	 *
	 * Component traits must contain a compile-time constant of type `std::size_t` named `page_size`, specifying
	 * size of allocation pages used by component pools, a `allocator_type` typedef used to specify the allocator of
	 * used to allocate components. */
	template<typename T>
	struct component_traits
	{
		/** Size of individual component pages of component sets. */
		constexpr static std::size_t page_size = 1024;
		/** Allocator type used for component pages. */
		typedef std::allocator<T> allocator_type;
	};

}	 // namespace sek