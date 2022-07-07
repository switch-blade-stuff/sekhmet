/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../policy.hpp"
#include "../util.hpp"

namespace sek::math
{
	template<typename T, std::size_t N, policy_t Policy>
	class basic_vec;
	template<typename...>
	class vec_mask;
}	 // namespace sek::math