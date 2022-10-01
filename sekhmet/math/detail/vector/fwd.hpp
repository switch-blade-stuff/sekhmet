/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../../utility.hpp"
#include "../policy.hpp"

namespace sek
{
	template<typename T, std::size_t N, policy_t Policy>
	class basic_vec;
	template<typename...>
	class vec_mask;
}	 // namespace sek