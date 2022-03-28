//
// Created by switchblade on 2022-03-13.
//

#pragma once

#include "detail/meta_containers.hpp"
#include "detail/meta_util.hpp"

namespace sek
{
	using detail::auto_constant;
	using detail::filter_index_sequence;
	using detail::forward_iterator_for;
	using detail::forward_range_for;
	using detail::is_in_values;
	using detail::is_in_values_v;
	using detail::is_template;
	using detail::is_template_instance;
	using detail::is_template_instance_v;
	using detail::is_template_v;
	using detail::mutable_global;
	using detail::not_void;
	using detail::pack_member;
	using detail::pack_member_t;
	using detail::rebind;
	using detail::rebind_t;
	using detail::template_extent;
	using detail::template_type;
	using detail::template_type_instance;
	using detail::trivial_type;
}	 // namespace sek
