/*
 * Created by switchblade on 22/06/22
 */

#pragma once

#include "detail/quaternion.hpp"

namespace sek
{
	using fquat = basic_quat<float>;
	using fquat_packed = basic_quat<float, policy_t::PACKED>;
	using dquat = basic_quat<double>;
	using dquat_packed = basic_quat<double, policy_t::PACKED>;
}	 // namespace sek