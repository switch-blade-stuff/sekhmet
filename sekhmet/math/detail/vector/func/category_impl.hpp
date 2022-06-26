/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../type.hpp"

namespace sek::math
{
	/** Checks if elements of the vector are `NaN`. */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> is_nan(const basic_vec<U, M, Sp> &v) noexcept
	{
		vec_mask<basic_vec<U, M, Sp>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_is_nan(result.m_data, v.m_data);
		else
			detail::vector_is_nan(result.m_data, v.m_data);
		return result;
	}
	/** Checks if elements of the vector are a positive or negative infinity. */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> is_inf(const basic_vec<U, M, Sp> &v) noexcept
	{
		vec_mask<basic_vec<U, M, Sp>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_is_inf(result.m_data, v.m_data);
		else
			detail::vector_is_inf(result.m_data, v.m_data);
		return result;
	}
	/** Checks if elements of the vector are finite. */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> is_fin(const basic_vec<U, M, Sp> &v) noexcept
	{
		vec_mask<basic_vec<U, M, Sp>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_is_fin(result.m_data, v.m_data);
		else
			detail::vector_is_fin(result.m_data, v.m_data);
		return result;
	}
	/** Checks if elements of the vector are negative. */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> is_neg(const basic_vec<U, M, Sp> &v) noexcept
	{
		vec_mask<basic_vec<U, M, Sp>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_is_neg(result.m_data, v.m_data);
		else
			detail::vector_is_neg(result.m_data, v.m_data);
		return result;
	}
	/** Checks if elements of the vector are normal. */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> is_norm(const basic_vec<U, M, Sp> &v) noexcept
	{
		vec_mask<basic_vec<U, M, Sp>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_is_norm(result.m_data, v.m_data);
		else
			detail::vector_is_norm(result.m_data, v.m_data);
		return result;
	}
}	 // namespace sek::math
