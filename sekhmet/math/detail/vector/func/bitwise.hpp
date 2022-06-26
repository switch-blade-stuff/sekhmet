/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../type.hpp"

#ifndef SEK_NO_SIMD
#if defined(SEK_ARCH_x86)
#include "x86/bitwise.hpp"
#endif
#endif

namespace sek::math
{
	namespace detail
	{
		inline namespace generic
		{
			template<typename T, std::size_t N, storage_policy P>
			constexpr void vector_and(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = l[i] & r[i];
			}
			template<typename T, std::size_t N, storage_policy P>
			constexpr void vector_or(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = l[i] | r[i];
			}
			template<typename T, std::size_t N, storage_policy P>
			constexpr void vector_xor(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = l[i] ^ r[i];
			}
			template<typename T, std::size_t N, storage_policy P>
			constexpr void vector_inv(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = ~v[i];
			}
		}	 // namespace generic
	}		 // namespace detail

	/** Preforms a bitwise AND on two vectors. */
	template<std::integral U, std::size_t M, storage_policy Sp>
	constexpr basic_vec<U, M, Sp> &operator&=(basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_and(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_and(l.m_data, l.m_data, r.m_data);
		return l;
	}
	/** Returns a vector which is the result of bitwise AND of two vectors. */
	template<std::integral U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator&(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_and(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_and(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Preforms a bitwise OR on two vectors. */
	template<std::integral U, std::size_t M, storage_policy Sp>
	constexpr basic_vec<U, M, Sp> &operator|=(basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_or(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_or(l.m_data, l.m_data, r.m_data);
		return l;
	}
	/** Returns a vector which is the result of bitwise OR of two vectors. */
	template<std::integral U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator|(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_or(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_or(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Returns a vector which is the result of bitwise XOR of two vectors. */
	template<std::integral U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator^(const basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_xor(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_xor(result.m_data, l.m_data, r.m_data);
		return result;
	}
	/** Preforms a bitwise XOR on two vectors. */
	template<std::integral U, std::size_t M, storage_policy Sp>
	constexpr basic_vec<U, M, Sp> &operator^=(basic_vec<U, M, Sp> &l, const basic_vec<U, M, Sp> &r) noexcept
	{
		if (std::is_constant_evaluated())
			detail::generic::vector_xor(l.m_data, l.m_data, r.m_data);
		else
			detail::vector_xor(l.m_data, l.m_data, r.m_data);
		return l;
	}
	/** Returns a bitwise inverted copy of a vector. */
	template<std::integral U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> operator~(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_inv(result.m_data, v.m_data);
		else
			detail::vector_inv(result.m_data, v.m_data);
		return result;
	}
}	 // namespace sek::math
