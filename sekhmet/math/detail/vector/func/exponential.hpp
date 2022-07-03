/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../type.hpp"

#ifndef SEK_NO_SIMD
#if defined(SEK_ARCH_x86)
#include "x86/exponential.hpp"
#endif
#endif

namespace sek::math
{
	namespace detail
	{
		inline namespace generic
		{
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_exp(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = std::exp(v[i]);
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_exp2(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = std::exp2(v[i]);
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_expm1(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = std::expm1(v[i]);
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_log(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = std::log(v[i]);
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_log10(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = std::log10(v[i]);
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_log2(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = std::log2(v[i]);
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_log1p(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = std::log1p(v[i]);
			}

			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_pow(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<T>(std::pow(l[i], r[i]));
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_sqrt(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<T>(std::sqrt(v[i]));
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_cbrt(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<T>(std::cbrt(v[i]));
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_rsqrt(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<T>(1) / static_cast<T>(std::sqrt(v[i]));
			}
		}	 // namespace generic
	}		 // namespace detail

	/** Returns a vector of `e` raised to the given power. */
	template<typename U, std::size_t M, policy_t Sp>
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
	template<typename U, std::size_t M, policy_t Sp>
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
	template<typename U, std::size_t M, policy_t Sp>
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
	template<typename U, std::size_t M, policy_t Sp>
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
	template<typename U, std::size_t M, policy_t Sp>
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
	template<typename U, std::size_t M, policy_t Sp>
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
	template<typename U, std::size_t M, policy_t Sp>
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
	template<typename U, std::size_t M, policy_t Sp>
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
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> pow(const basic_vec<U, M, Sp> &l, U r) noexcept
	{
		return pow(l, basic_vec<U, M, Sp>{r});
	}
	/** Calculates square root of a vector. */
	template<typename U, std::size_t M, policy_t Sp>
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
	template<typename U, std::size_t M, policy_t Sp>
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
	template<typename U, std::size_t M, policy_t Sp>
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
