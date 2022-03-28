//
// Created by switchblade on 2021-12-05.
//

#pragma once

#include "detail/aligned_storage.hpp"
#include "detail/ebo_base_helper.hpp"
#include "detail/flagged_integer.hpp"
#include "detail/flagged_ptr.hpp"
#include "detail/uuid.hpp"
#include "detail/version.hpp"
#include "math/detail/util.hpp"

namespace sek
{
	using detail::aligned_storage;
	using detail::basic_version;
	using detail::ebo_base_helper;
	using detail::flagged_integer_t;
	using detail::flagged_ptr_t;
	using detail::hash;
	using detail::swap;
	using detail::type_storage;
	using detail::uuid;
	using detail::version;
}	 // namespace sek
