/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../type.hpp"

#ifndef SEK_NO_SIMD
#if defined(SEK_ARCH_x86)
#include "x86/geometric.hpp"
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
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> lerp(const basic_vec<U, M, Q> &l, const basic_vec<U, M, Q> &r, const basic_vec<U, M, Q> &t) noexcept
	{
		return l + t * (r - l);
	}
	/** @copydoc lerp */
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> lerp(const basic_vec<U, M, Q> &l, const basic_vec<U, M, Q> &r, U t) noexcept
	{
		return lerp(l, r, basic_vec<U, M, Q>{t});
	}
	// clang-format on

	/** Calculates cross product of two vectors. */
	template<typename T, policy_t Q>
	[[nodiscard]] constexpr basic_vec<T, 3, Q> cross(const basic_vec<T, 3, Q> &l, const basic_vec<T, 3, Q> &r) noexcept
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
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr U dot(const basic_vec<U, M, Q> &l, const basic_vec<U, M, Q> &r) noexcept
	{
		if (std::is_constant_evaluated())
			return detail::generic::vector_dot(l.m_data, r.m_data);
		else
			return detail::vector_dot(l.m_data, r.m_data);
	}
	/** Returns a length of the vector. */
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr U magn(const basic_vec<U, M, Q> &v) noexcept
	{
		/* Magnitude of a vector A=XYZ is sqrt(X*X + Y*Y + Z*Z) = sqrt(dot(A, A)). */
		return std::sqrt(dot(v, v));
	}
	/** Returns a normalized copy of the vector. */
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> norm(const basic_vec<U, M, Q> &v) noexcept
	{
		basic_vec<U, M, Q> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_norm(result.m_data, v.m_data);
		else
			detail::vector_norm(result.m_data, v.m_data);
		return result;
	}
	/** Returns a dist between two vectors. */
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr U dist(const basic_vec<U, M, Q> &l, const basic_vec<U, M, Q> &r) noexcept
	{
		return magn(l - r);
	}

	/** @brief Calculates the reflection direction for an incident vector and a surface normal.
	 *
	 * Result is calculated as
	 * @code{cpp}
	 * i - dot(n, i) * n * U{2}
	 * @endcode
	 *
	 * @param i Incident vector.
	 * @param n Normal vector.
	 * @return Reflection direction. */
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> reflect(const basic_vec<U, M, Q> &i, const basic_vec<U, M, Q> &n) noexcept
	{
		return i - n * dot(n, i) * static_cast<U>(2.0);
	}
	/** @brief Calculates the refraction direction for an incident vector and a surface normal.
	 *
	 * Result is calculated as
	 * @code{cpp}
	 * k = 1.0 - eta * eta * (1.0 - dot(N, I) * dot(N, I));
	 * if (k < 0.0)
	 * 	R = 0.0;
	 * else
	 * 	R = eta * I - (eta * dot(N, I) + sqrt(k)) * N;
	 * @endcode
	 *
	 * @param i Incident vector.
	 * @param n Normal vector.
	 * @param r Ratio of refraction indices.
	 * @return Reflection direction. */
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> refract(const basic_vec<U, M, Q> &i, const basic_vec<U, M, Q> &n, U r) noexcept
	{
		const auto dp = dot(n, i);

		/* k = 1.0 - eta * eta * (1.0 - dot(N, I) * dot(N, I)); */
		const auto k = static_cast<U>(1.0) - r * r * (static_cast<U>(1.0) - dp * dp);
		const auto m = fcmp_lt(k, zero);

		/* if (k < 0.0)
		 * 	R = 0.0;
		 * else
		 * 	R = eta * I - (eta * dot(N, I) + sqrt(k)) * N; */
		if (!fcmp_lt(k, static_cast<U>(0.0)))
			return i * r - n * (sqrt(k) + basic_vec<U, M, Q>{dp * r});
		else
			return basic_vec<U, M, Q>{};
	}
}	 // namespace sek::math
