/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../type.hpp"

#ifndef SEK_NO_SIMD
#if defined(SEK_ARCH_x86)
#include "x86/category.hpp"
#endif
#endif

namespace sek
{
	namespace detail
	{
		inline namespace generic
		{
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_is_nan(mask_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<bool>(std::isnan(v[i]));
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_is_inf(mask_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<bool>(std::isinf(v[i]));
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_is_fin(mask_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<bool>(std::isfinite(v[i]));
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_is_neg(mask_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<bool>(std::signbit(v[i]));
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_is_norm(mask_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<bool>(std::isnormal(v[i]));
			}
		}	 // namespace generic
	}		 // namespace detail

	/** Checks if elements of the vector are `NaN`. */
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> is_nan(const basic_vec<U, M, Q> &v) noexcept
	{
		vec_mask<basic_vec<U, M, Q>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_is_nan(result.m_data, v.m_data);
		else
			detail::vector_is_nan(result.m_data, v.m_data);
		return result;
	}
	/** Checks if elements of the vector are a positive or negative infinity. */
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> is_inf(const basic_vec<U, M, Q> &v) noexcept
	{
		vec_mask<basic_vec<U, M, Q>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_is_inf(result.m_data, v.m_data);
		else
			detail::vector_is_inf(result.m_data, v.m_data);
		return result;
	}
	/** Checks if elements of the vector are finite. */
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> is_fin(const basic_vec<U, M, Q> &v) noexcept
	{
		vec_mask<basic_vec<U, M, Q>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_is_fin(result.m_data, v.m_data);
		else
			detail::vector_is_fin(result.m_data, v.m_data);
		return result;
	}
	/** Checks if elements of the vector are negative. */
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> is_neg(const basic_vec<U, M, Q> &v) noexcept
	{
		vec_mask<basic_vec<U, M, Q>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_is_neg(result.m_data, v.m_data);
		else
			detail::vector_is_neg(result.m_data, v.m_data);
		return result;
	}
	/** Checks if elements of the vector are normal. */
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> is_norm(const basic_vec<U, M, Q> &v) noexcept
	{
		vec_mask<basic_vec<U, M, Q>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_is_norm(result.m_data, v.m_data);
		else
			detail::vector_is_norm(result.m_data, v.m_data);
		return result;
	}
}	 // namespace sek
