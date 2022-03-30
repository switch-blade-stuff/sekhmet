//
// Created by switchblade on 2022-02-06.
//

#pragma once

#include <utility>

#include "../math/detail/util.hpp"
#include "aligned_storage.hpp"
#include "type_info.hpp"

namespace sek
{
	class any
	{
	};

	template<forward_iterator_for<any> Iter>
	constexpr void type_info::constructor_info::invoke(void *ptr, Iter args_begin, Iter args_end) const
	{
		/* TODO: Implement this. */
	}

	constexpr any type_info::get_attribute(type_id id) const noexcept
	{
		return attribute_info{data->get_attribute(id)}.get();
	}
	constexpr any type_info::attribute_info::get() const noexcept
	{
		/* TODO: Implement this. */
		return {};
	}
}	 // namespace sek