/*
 * Created by switchblade on 07/07/22
 */

#pragma once

#include "../../utility.hpp"
#include "../policy.hpp"

namespace sek::math
{
	template<std::floating_point T, policy_t Policy = policy_t::DEFAULT>
	class basic_quat;
}