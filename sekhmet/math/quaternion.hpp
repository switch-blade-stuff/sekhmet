/*
 * Created by switchblade on 22/06/22
 */

#pragma once

#include "detail/quaternion.hpp"

namespace sek::math
{
	using fquat = basic_quat<float>;
	using fquat_packed = basic_quat<float, storage_policy::SIZE>;
	using dquat = basic_quat<double>;
	using dquat_packed = basic_quat<double, storage_policy::SIZE>;
}	 // namespace sek::math