/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../type.hpp"

namespace sek::math
{
	/** Returns a vector of `e` raised to the given power. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> exp(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_exp(result.m_data, v.m_data);
		else
			detail::vector_exp(result.m_data, v.m_data);
		return result;
	}
	/** Returns a vector of `2` raised to the given power. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> exp2(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_exp2(result.m_data, v.m_data);
		else
			detail::vector_exp2(result.m_data, v.m_data);
		return result;
	}
	/** Returns a vector of `e` raised to the given power, minus one. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> expm1(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_expm1(result.m_data, v.m_data);
		else
			detail::vector_expm1(result.m_data, v.m_data);
		return result;
	}
	/** Calculates natural logarithms of a vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> log(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_log(result.m_data, v.m_data);
		else
			detail::vector_log(result.m_data, v.m_data);
		return result;
	}
	/** Calculates common logarithms of a vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> log10(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_log10(result.m_data, v.m_data);
		else
			detail::vector_log10(result.m_data, v.m_data);
		return result;
	}
	/** Calculates base-2 logarithms of a vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> log2(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_log2(result.m_data, v.m_data);
		else
			detail::vector_log2(result.m_data, v.m_data);
		return result;
	}
	/** Calculates natural logarithms of 1 plus a vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> log1p(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_log1p(result.m_data, v.m_data);
		else
			detail::vector_log1p(result.m_data, v.m_data);
		return result;
	}

	/** Raises elements of a vector to the given power. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> pow(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_pow(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_pow(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** @copydoc pow */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> pow(const basic_vec<U, M, Sp> &l, U r) noexcept
	{
		return pow(l, basic_vec<U, M, Sp>{r});
	}
	/** Calculates square root of a vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> sqrt(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_sqrt(result.m_data, v.m_data);
		else
			detail::vector_sqrt(result.m_data, v.m_data);
		return result;
	}
	/** Calculates cubic root of a vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> cbrt(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_cbrt(result.m_data, v.m_data);
		else
			detail::vector_cbrt(result.m_data, v.m_data);
		return result;
	}
	/** Calculates reciprocal square root of a vector. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> rsqrt(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_rsqrt(result.m_data, v.m_data);
		else
			detail::vector_rsqrt(result.m_data, v.m_data);
		return result;
	}
}	 // namespace sek::math
