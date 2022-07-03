/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../type.hpp"

#ifndef SEK_NO_SIMD
#if defined(SEK_ARCH_x86)
#include "x86/geom.hpp"
#endif
#endif

namespace sek::math
{
	namespace detail
	{
		inline namespace generic
		{
			template<typename T, std::size_t N, policy_t P>
			constexpr T vector_dot(const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
			{
				T result = {};
				for (std::size_t i = 0; i < N; ++i) result += l[i] * r[i];
				return result;
			}
			template<typename T, policy_t P>
			constexpr void vector_cross(vector_data<T, 3, P> &out, const vector_data<T, 3, P> &l, const vector_data<T, 3, P> &r) noexcept
			{
				out[0] = l[1] * r[2] - l[2] * r[1];
				out[1] = l[2] * r[0] - l[0] * r[2];
				out[2] = l[0] * r[1] - l[1] * r[0];
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_norm(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				const auto r = static_cast<T>(std::sqrt(vector_dot(v, v)));
				for (std::size_t i = 0; i < N; ++i) out[i] = v[i] / r;
			}
		}	 // namespace generic
	}		 // namespace detail

	// clang-format off
	/** Calculates linear interpolation or extrapolation between two vectors. Equivalent to `l + t * (r - l)`. */
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> lerp(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r, const basic_vec<U, M, Sp> &t) noexcept
	{
		return l + t * (r - l);
	}
	/** @copydoc lerp */
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> lerp(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r, U t) noexcept
	{
		return lerp(l, r, basic_vec<U, M, Sp>{t});
	}
	// clang-format on

	/** Calculates cross product of two vectors. */
	template<typename T, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<T, 3, Sp> cross(const basic_vec<T, 3, Sp> &l, const basic_vec<T, 3, Sp> &r) noexcept
		requires std::is_signed_v<T>
	{
		basic_vec<T, 3> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_cross(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_cross(result.m_data, l.m_data, r.m_data);
		return result;
	}

	/** Calculates dot product of two vectors. */
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr U dot(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			return detail::generic::vector_dot(l.m_data, r.m_data);
		else
			return detail::vector_dot(l.m_data, r.m_data);
	}
	/** Returns a length of the vector. */
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr U magn(const basic_vec<U, M, Sp> &v) noexcept
	{
		/* Magnitude of a vector A=XYZ is sqrt(X*X + Y*Y + Z*Z) = sqrt(dot(A, A)). */
		return std::sqrt(dot(v, v));
	}
	/** Returns a normalized copy of the vector. */
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> norm(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_norm(result.m_data, v.m_data);
		else
			detail::vector_norm(result.m_data, v.m_data);
		return result;
	}
	/** Returns a dist between two vectors. */
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr U dist(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		return magn(l - r);
	}
}	 // namespace sek::math
