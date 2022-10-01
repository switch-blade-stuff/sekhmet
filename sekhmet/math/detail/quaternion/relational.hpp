/*
 * Created by switchblade on 07/07/22
 */

#pragma once

#include "type.hpp"

namespace sek
{
	/** Checks if elements of quaternion a equals quaternion b using an epsilon. */
	template<std::floating_point T, policy_t P>
	[[nodiscard]] constexpr auto fcmp_eq(const basic_quat<T, P> &a, const basic_quat<T, P> &b, const basic_vec<T, 4, P> &epsilon) noexcept
	{
		return fcmp_eq(a.vector(), b.vector(), epsilon);
	}
	/** @copydoc fcmp_eq */
	template<std::floating_point T, policy_t P>
	[[nodiscard]] constexpr auto
		fcmp_eq(const basic_quat<T, P> &a, const basic_quat<T, P> &b, T epsilon = std::numeric_limits<T>::epsilon()) noexcept
	{
		return fcmp_eq(a, b, basic_vec<T, 4, P>{epsilon});
	}
	/** Checks if elements of quaternion a does not equal quaternion vector b using an epsilon. */
	template<std::floating_point T, policy_t P>
	[[nodiscard]] constexpr auto fcmp_ne(const basic_quat<T, P> &a, const basic_quat<T, P> &b, const basic_vec<T, 4, P> &epsilon) noexcept
	{
		return fcmp_ne(a.vector(), b.vector(), epsilon);
	}
	/** @copydoc fcmp_ne */
	template<std::floating_point T, policy_t P>
	[[nodiscard]] constexpr auto
		fcmp_ne(const basic_quat<T, P> &a, const basic_quat<T, P> &b, T epsilon = std::numeric_limits<T>::epsilon()) noexcept
	{
		return fcmp_ne(a, b, basic_vec<T, 4, P>{epsilon});
	}
	/** Checks if elements of quaternion a is less than or equal to quaternion b using an epsilon. */
	template<std::floating_point T, policy_t P>
	[[nodiscard]] constexpr auto fcmp_le(const basic_quat<T, P> &a, const basic_quat<T, P> &b, const basic_vec<T, 4, P> &epsilon) noexcept
	{
		return fcmp_le(a.vector(), b.vector(), epsilon);
	}
	/** @copydoc fcmp_le */
	template<std::floating_point T, policy_t P>
	[[nodiscard]] constexpr auto
		fcmp_le(const basic_quat<T, P> &a, const basic_quat<T, P> &b, T epsilon = std::numeric_limits<T>::epsilon()) noexcept
	{
		return fcmp_le(a, b, basic_vec<T, 4, P>{epsilon});
	}
	/** Checks if elements of quaternion a is greater than or equal to quaternion b using an epsilon. */
	template<std::floating_point T, policy_t P>
	[[nodiscard]] constexpr auto fcmp_ge(const basic_quat<T, P> &a, const basic_quat<T, P> &b, const basic_vec<T, 4, P> &epsilon) noexcept
	{
		return fcmp_ge(a.vector(), b.vector(), epsilon);
	}
	/** @copydoc fcmp_ge */
	template<std::floating_point T, policy_t P>
	[[nodiscard]] constexpr auto
		fcmp_ge(const basic_quat<T, P> &a, const basic_quat<T, P> &b, T epsilon = std::numeric_limits<T>::epsilon()) noexcept
	{
		return fcmp_ge(a, b, basic_vec<T, 4, P>{epsilon});
	}
	/** Checks if elements of quaternion a is less than quaternion b using an epsilon. */
	template<std::floating_point T, policy_t P>
	[[nodiscard]] constexpr auto fcmp_lt(const basic_quat<T, P> &a, const basic_quat<T, P> &b, const basic_vec<T, 4, P> &epsilon) noexcept
	{
		return fcmp_lt(a.vector(), b.vector(), epsilon);
	}
	/** @copydoc fcmp_lt */
	template<std::floating_point T, policy_t P>
	[[nodiscard]] constexpr auto
		fcmp_lt(const basic_quat<T, P> &a, const basic_quat<T, P> &b, T epsilon = std::numeric_limits<T>::epsilon()) noexcept
	{
		return fcmp_lt(a, b, basic_vec<T, 4, P>{epsilon});
	}
	/** Checks if elements of quaternion a is less than quaternion b using an epsilon. */
	template<std::floating_point T, policy_t P>
	[[nodiscard]] constexpr auto fcmp_gt(const basic_quat<T, P> &a, const basic_quat<T, P> &b, const basic_vec<T, 4, P> &epsilon) noexcept
	{
		return fcmp_gt(a.vector(), b.vector(), epsilon);
	}
	/** @copydoc fcmp_gt */
	template<std::floating_point T, policy_t P>
	[[nodiscard]] constexpr auto
		fcmp_gt(const basic_quat<T, P> &a, const basic_quat<T, P> &b, T epsilon = std::numeric_limits<T>::epsilon()) noexcept
	{
		return fcmp_gt(a, b, basic_vec<T, 4, P>{epsilon});
	}
}	 // namespace sek