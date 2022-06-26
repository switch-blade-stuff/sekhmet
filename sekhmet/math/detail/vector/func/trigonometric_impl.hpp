/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../type.hpp"

namespace sek::math
{
	/** Calculates a sine of the elements of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> sin(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_sin(result.m_data, v.m_data);
		else
			detail::vector_sin(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a cosine of the elements of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> cos(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_cos(result.m_data, v.m_data);
		else
			detail::vector_cos(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a tangent of the elements of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> tan(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_tan(result.m_data, v.m_data);
		else
			detail::vector_tan(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a arc sine of the elements of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> asin(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_asin(result.m_data, v.m_data);
		else
			detail::vector_asin(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a arc cosine of the elements of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> acos(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_acos(result.m_data, v.m_data);
		else
			detail::vector_acos(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a arc tangent of the elements of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> atan(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_atan(result.m_data, v.m_data);
		else
			detail::vector_atan(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a hyperbolic sine of the elements of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> sinh(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_sinh(result.m_data, v.m_data);
		else
			detail::vector_sinh(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a hyperbolic cosine of the elements of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> cosh(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_cosh(result.m_data, v.m_data);
		else
			detail::vector_cosh(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a hyperbolic tangent of the elements of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> tanh(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_tanh(result.m_data, v.m_data);
		else
			detail::vector_tanh(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a hyperbolic arc sine of the elements of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> asinh(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_asinh(result.m_data, v.m_data);
		else
			detail::vector_asinh(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a hyperbolic arc cosine of the elements of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> acosh(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_acosh(result.m_data, v.m_data);
		else
			detail::vector_acosh(result.m_data, v.m_data);
		return result;
	}
	/** Calculates a hyperbolic arc tangent of the elements of the vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> atanh(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_atanh(result.m_data, v.m_data);
		else
			detail::vector_atanh(result.m_data, v.m_data);
		return result;
	}

	/** Converts a degree angle vector to radian angle vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> rad(const basic_vec<U, M, Sp> &v) noexcept
	{
		return v * basic_vec<U, M, Sp>{std::numbers::pi_v<U> / static_cast<U>(180.0)};
	}
	/** Converts a radian angle vector to degree angle vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> deg(const basic_vec<U, M, Sp> &v) noexcept
	{
		return v * basic_vec<U, M, Sp>{static_cast<U>(180.0) / std::numbers::pi_v<U>};
	}
}	 // namespace sek::math
