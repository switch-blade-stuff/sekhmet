/*
 * Created by switchblade on 07/07/22
 */

#pragma once

#include "type.hpp"

namespace sek
{
	/** Checks if elements of the quaternion are `NaN`. */
	template<std::floating_point T, policy_t P>
	[[nodiscard]] constexpr auto is_nan(const basic_quat<T, P> &q) noexcept
	{
		return is_nan(q.vector());
	}
	/** Checks if elements of the quaternion are a positive or negative infinity. */
	template<std::floating_point T, policy_t P>
	[[nodiscard]] constexpr auto is_inf(const basic_quat<T, P> &q) noexcept
	{
		return is_inf(q.vector());
	}
	/** Checks if elements of the quaternion are finite. */
	template<std::floating_point T, policy_t P>
	[[nodiscard]] constexpr auto is_fin(const basic_quat<T, P> &q) noexcept
	{
		return is_fin(q.vector());
	}
	/** Checks if elements of the quaternion are negative. */
	template<std::floating_point T, policy_t P>
	[[nodiscard]] constexpr auto is_neg(const basic_quat<T, P> &q) noexcept
	{
		return is_neg(q.vector());
	}
	/** Checks if elements of the quaternion are normal. */
	template<std::floating_point T, policy_t P>
	[[nodiscard]] constexpr auto is_norm(const basic_quat<T, P> &q) noexcept
	{
		return is_norm(q.vector());
	}
}	 // namespace sek