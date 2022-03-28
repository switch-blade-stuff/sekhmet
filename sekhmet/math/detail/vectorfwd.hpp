//
// Created by switchblade on 2021-12-19.
//

#pragma once

#include "util.hpp"

namespace sek::math::detail
{
	template<typename T, std::size_t N>
	requires(N != 0 && arithmetic<T>) union vector;
}	 // namespace sek::math::detail