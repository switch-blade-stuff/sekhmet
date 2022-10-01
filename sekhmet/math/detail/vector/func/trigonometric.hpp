/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../type.hpp"

#ifndef SEK_NO_SIMD
#if defined(SEK_ARCH_x86)
#include "x86/trigonometric.hpp"
#endif
#endif

namespace sek
{
	namespace detail
	{
		inline namespace generic
		{
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_sin(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<T>(std::sin(v[i]));
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_cos(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<T>(std::cos(v[i]));
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_tan(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<T>(std::tan(v[i]));
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_cot(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				auto one = vector_data<T, N, P>{};
				for (std::size_t i = 0; i < N; ++i) one[i] = static_cast<T>(1);

				/* cot(x) = 1 / tan(x) */
				vector_tan(out, v);
				vector_div(out, one, out);
			}

			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_asin(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<T>(std::asin(v[i]));
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_acos(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<T>(std::acos(v[i]));
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_atan(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<T>(std::atan(v[i]));
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_acot(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				auto pi2 = vector_data<T, N, P>{};
				for (std::size_t i = 0; i < N; ++i) pi2[i] = std::numbers::pi_v<T> / 2;

				/* acot(x) = pi / 2 - atan(x) */
				vector_atan(out, v);
				vector_sub(out, pi2, out);
			}

			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_sinh(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<T>(std::sinh(v[i]));
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_cosh(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<T>(std::cosh(v[i]));
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_tanh(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<T>(std::tanh(v[i]));
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_coth(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				auto one = vector_data<T, N, P>{};
				for (std::size_t i = 0; i < N; ++i) one[i] = static_cast<T>(1);

				/* coth(x) = 1 / tanh(x) */
				vector_tanh(out, v);
				vector_div(out, one, out);
			}

			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_asinh(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<T>(std::asinh(v[i]));
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_acosh(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<T>(std::acosh(v[i]));
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_atanh(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<T>(std::atanh(v[i]));
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_acoth(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				auto one = vector_data<T, N, P>{};
				for (std::size_t i = 0; i < N; ++i) one[i] = static_cast<T>(1);
				auto two = vector_data<T, N, P>{};
				for (std::size_t i = 0; i < N; ++i) two[i] = static_cast<T>(2);

				/* acoth(x) = 0.5 * ln((x + 1) / (x - 1)) */

				auto xp1 = v;
				vector_add(xp1, xp1, one);
				auto xm1 = v;
				vector_sub(xm1, xm1, one);
				vector_div(out, xp1, xm1);
				vector_log(out, out);
				vector_div(out, out, two);
			}
		}	 // namespace generic
	}		 // namespace detail

	/** Calculates a sine of the elements of the vector. */
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> sin(const basic_vec<U, M, Q> &v) noexcept
	{
		basic_vec<U, M, Q> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_sin(result.m_data, v.m_data);
		else
			detail::vector_sin(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a cosine of the elements of the vector. */
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> cos(const basic_vec<U, M, Q> &v) noexcept
	{
		basic_vec<U, M, Q> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_cos(result.m_data, v.m_data);
		else
			detail::vector_cos(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a tangent of the elements of the vector. */
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> tan(const basic_vec<U, M, Q> &v) noexcept
	{
		basic_vec<U, M, Q> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_tan(result.m_data, v.m_data);
		else
			detail::vector_tan(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a cotangent of the elements of the vector. */
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> cot(const basic_vec<U, M, Q> &v) noexcept
	{
		basic_vec<U, M, Q> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_cot(result.m_data, v.m_data);
		else
			detail::vector_cot(result.m_data, v.m_data);
		return result;
	}

	/** Calculates a arc sine of the elements of the vector. */
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> asin(const basic_vec<U, M, Q> &v) noexcept
	{
		basic_vec<U, M, Q> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_asin(result.m_data, v.m_data);
		else
			detail::vector_asin(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a arc cosine of the elements of the vector. */
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> acos(const basic_vec<U, M, Q> &v) noexcept
	{
		basic_vec<U, M, Q> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_acos(result.m_data, v.m_data);
		else
			detail::vector_acos(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a arc tangent of the elements of the vector. */
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> atan(const basic_vec<U, M, Q> &v) noexcept
	{
		basic_vec<U, M, Q> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_atan(result.m_data, v.m_data);
		else
			detail::vector_atan(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a arc cotangent of the elements of the vector. */
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> acot(const basic_vec<U, M, Q> &v) noexcept
	{
		basic_vec<U, M, Q> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_acot(result.m_data, v.m_data);
		else
			detail::vector_acot(result.m_data, v.m_data);
		return result;
	}

	/** Calculates a hyperbolic sine of the elements of the vector. */
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> sinh(const basic_vec<U, M, Q> &v) noexcept
	{
		basic_vec<U, M, Q> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_sinh(result.m_data, v.m_data);
		else
			detail::vector_sinh(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a hyperbolic cosine of the elements of the vector. */
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> cosh(const basic_vec<U, M, Q> &v) noexcept
	{
		basic_vec<U, M, Q> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_cosh(result.m_data, v.m_data);
		else
			detail::vector_cosh(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a hyperbolic tangent of the elements of the vector. */
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> tanh(const basic_vec<U, M, Q> &v) noexcept
	{
		basic_vec<U, M, Q> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_tanh(result.m_data, v.m_data);
		else
			detail::vector_tanh(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a hyperbolic cotangent of the elements of the vector. */
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> coth(const basic_vec<U, M, Q> &v) noexcept
	{
		basic_vec<U, M, Q> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_coth(result.m_data, v.m_data);
		else
			detail::vector_coth(result.m_data, v.m_data);
		return result;
	}

	/** Calculates a hyperbolic arc sine of the elements of the vector. */
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> asinh(const basic_vec<U, M, Q> &v) noexcept
	{
		basic_vec<U, M, Q> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_asinh(result.m_data, v.m_data);
		else
			detail::vector_asinh(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a hyperbolic arc cosine of the elements of the vector. */
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> acosh(const basic_vec<U, M, Q> &v) noexcept
	{
		basic_vec<U, M, Q> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_acosh(result.m_data, v.m_data);
		else
			detail::vector_acosh(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a hyperbolic arc tangent of the elements of the vector. */
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> atanh(const basic_vec<U, M, Q> &v) noexcept
	{
		basic_vec<U, M, Q> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_atanh(result.m_data, v.m_data);
		else
			detail::vector_atanh(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a hyperbolic arc cotangent of the elements of the vector. */
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> acoth(const basic_vec<U, M, Q> &v) noexcept
	{
		basic_vec<U, M, Q> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_acoth(result.m_data, v.m_data);
		else
			detail::vector_acoth(result.m_data, v.m_data);
		return result;
	}

	/** Converts a degree angle vector to radian angle vector. */
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> rad(const basic_vec<U, M, Q> &v) noexcept
	{
		return v * basic_vec<U, M, Q>{std::numbers::pi_v<U> / static_cast<U>(180.0)};
	}
	/** Converts a radian angle vector to degree angle vector. */
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> deg(const basic_vec<U, M, Q> &v) noexcept
	{
		return v * basic_vec<U, M, Q>{static_cast<U>(180.0) / std::numbers::pi_v<U>};
	}
}	 // namespace sek
