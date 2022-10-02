//
// Created by switch_blade on 2022-10-02.
//

#pragma once

#include "type.hpp"

namespace sek
{
	/** Calculates dot product of two quaternions. */
	template<typename T, policy_t P>
	[[nodiscard]] constexpr T dot(const basic_quat<T, P> &a, const basic_quat<T, P> &b) noexcept
	{
		return dot(a.vector(), b.vector());
	}
	/** Returns the magnitude (length) of a quaternion. */
	template<typename T, policy_t P>
	[[nodiscard]] constexpr T magn(const basic_quat<T, P> &q) noexcept
	{
		return magn(q.vector());
	}
	/** Returns a normalized copy of a quaternion. */
	template<typename T, policy_t P>
	[[nodiscard]] constexpr basic_quat<T, P> norm(const basic_quat<T, P> &q) noexcept
	{
		const auto m = magn(q);
		if (fcmp_le(m, static_cast<T>(0)))
			return basic_quat<T, P>{static_cast<T>(1), static_cast<T>(0), static_cast<T>(0), static_cast<T>(0)};
		return basic_quat<T, P>(q.vector() * (static_cast<T>(1) / m));
	}
	/** Calculates cross product of two quaternions. */
	template<typename T, policy_t P>
	[[nodiscard]] constexpr basic_quat<T, P> cross(const basic_quat<T, P> &a, const basic_quat<T, P> &b) noexcept
	{
		const auto va = a.vector().www() * b.vector().xyz();
		const auto vb = a.vector().xyz() * b.vector().www();
		const auto vc = a.vector().yzx() * b.vector().zxy();
		const auto vd = a.vector().zxy() * b.vector().yzx();

		auto data = basic_vec<T, 4, P>{va + vb + vc - vd};
		data.w() = a.w() * b.w() - a.x() * b.x() - a.y() * b.y() - a.z() * b.z();
		return basic_quat<T, P>{data};
	}
}	 // namespace sek