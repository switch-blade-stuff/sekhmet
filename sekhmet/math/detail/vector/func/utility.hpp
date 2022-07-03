/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../type.hpp"

#ifndef SEK_NO_SIMD
#if defined(SEK_ARCH_x86)
#include "x86/utility.hpp"
#endif
#endif

namespace sek::math
{
	namespace detail
	{
		inline namespace generic
		{
			template<std::size_t J, typename T, std::size_t N, std::size_t M, policy_t P, std::size_t I, std::size_t... Is>
			constexpr void shuffle_unwrap(mask_data<T, N, P> &out, const mask_data<T, M, P> &m, std::index_sequence<I, Is...>) noexcept
			{
				out[J] = m[I];
				if constexpr (sizeof...(Is) != 0) shuffle_unwrap<J + 1>(out, m, std::index_sequence<Is...>{});
			}
			template<typename T, std::size_t N, std::size_t M, policy_t P, std::size_t... Is>
			constexpr void mask_shuffle(mask_data<T, N, P> &out, const mask_data<T, M, P> &m, std::index_sequence<Is...> s) noexcept
			{
				shuffle_unwrap<0>(out, m, s);
			}

			template<std::size_t J, typename T, std::size_t N, std::size_t M, policy_t P1, policy_t P2, std::size_t I, std::size_t... Is>
			constexpr void shuffle_unwrap(vector_data<T, N, P1> &out,
										  const vector_data<T, M, P2> &v,
										  std::index_sequence<I, Is...>) noexcept
			{
				out[J] = v[I];
				if constexpr (sizeof...(Is) != 0) shuffle_unwrap<J + 1>(out, v, std::index_sequence<Is...>{});
			}
			template<typename T, std::size_t N, std::size_t M, policy_t P1, policy_t P2, std::size_t... Is>
			constexpr void vector_shuffle(vector_data<T, N, P1> &out,
										  const vector_data<T, M, P2> &v,
										  std::index_sequence<Is...> s) noexcept
			{
				shuffle_unwrap<0>(out, v, s);
			}

			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_interleave(vector_data<T, N, P> &out,
											 const vector_data<T, N, P> &l,
											 const vector_data<T, N, P> &r,
											 const mask_data<T, N, P> &m) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = m[i] ? l[i] : r[i];
			}

			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_round(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<T>(std::round(v[i]));
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_floor(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<T>(std::floor(v[i]));
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_ceil(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<T>(std::ceil(v[i]));
			}
			template<typename T, std::size_t N, policy_t P>
			constexpr void vector_trunc(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
			{
				for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<T>(std::trunc(v[i]));
			}
		}	 // namespace generic
	}		 // namespace detail

	/** Gets the Ith element of the vector mask. */
	template<std::size_t I, typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr decltype(auto) get(vec_mask<basic_vec<U, M, Sp>> &m) noexcept
	{
		return m[I];
	}
	/** @copydoc get */
	template<std::size_t I, typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr decltype(auto) get(const vec_mask<basic_vec<U, M, Sp>> &m) noexcept
	{
		return m[I];
	}
	/** Gets the Ith element of the vector. */
	template<std::size_t I, typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr decltype(auto) get(basic_vec<U, M, Sp> &v) noexcept
	{
		return v[I];
	}
	/** @copydoc get */
	template<std::size_t I, typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr decltype(auto) get(const basic_vec<U, M, Sp> &v) noexcept
	{
		return v[I];
	}

	/** Applies a functor to every element of the vector mask. */
	template<typename U, std::size_t M, policy_t Sp, typename F>
	constexpr void vectorize(const vec_mask<basic_vec<U, M, Sp>> &m, F &&f)
	{
		for (std::size_t i = 0; i < M; ++i) f(m[i]);
	}
	/** @copydoc vectorize */
	template<typename U, std::size_t M, policy_t Sp, typename F>
	constexpr void vectorize(vec_mask<basic_vec<U, M, Sp>> &m, F &&f)
	{
		for (std::size_t i = 0; i < M; ++i) f(m[i]);
	}
	/** Applies a functor to every element of the vector. */
	template<typename U, std::size_t M, policy_t Sp, typename F>
	constexpr void vectorize(const basic_vec<U, M, Sp> &v, F &&f)
	{
		for (std::size_t i = 0; i < M; ++i) f(v[i]);
	}
	/** @copydoc vectorize */
	template<typename U, std::size_t M, policy_t Sp, typename F>
	constexpr void vectorize(basic_vec<U, M, Sp> &v, F &&f)
	{
		for (std::size_t i = 0; i < M; ++i) f(v[i]);
	}

	/** Returns a vector consisting of rounded values of `v`.
	 * @example round({.1, .2, 2.3}) -> {0, 0, 2} */
	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> round(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_round(result.m_data, v.m_data);
		else
			detail::vector_round(result.m_data, v.m_data);
		return result;
	}
	/** Returns a vector consisting of rounded-down values of `v`.
	 * @example round({.1, .2, 2.3}) -> {0, 0, 2} */
	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> floor(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_floor(result.m_data, v.m_data);
		else
			detail::vector_floor(result.m_data, v.m_data);
		return result;
	}
	/** Returns a vector consisting of rounded-up values of `v`.
	 * @example round({.1, .2, 2.3}) -> {1, 1, 3} */
	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> ceil(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_ceil(result.m_data, v.m_data);
		else
			detail::vector_ceil(result.m_data, v.m_data);
		return result;
	}
	/** Returns a vector consisting of truncated values of `v`.
	 * @example round({.1, .2, 2.3}) -> {0, 0, 2} */
	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> trunc(const basic_vec<U, M, Sp> &v) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_trunc(result.m_data, v.m_data);
		else
			detail::vector_trunc(result.m_data, v.m_data);
		return result;
	}

	/** Produces a new vector mask which is the result of shuffling elements of another mask.
	 * @tparam I Indices of elements of the source mask in the order they should be shuffled to the destination mask.
	 * @return Result mask who's elements are specified by `I`. */
	template<std::size_t... I, typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, sizeof...(I), Sp>> shuffle(const vec_mask<basic_vec<U, M, Sp>> &m) noexcept
	{
		using Idx = std::index_sequence<I...>;
		if constexpr (std::is_same_v<Idx, std::make_index_sequence<M>>)
			return m;
		else
		{
			vec_mask<basic_vec<U, sizeof...(I), Sp>> result;
			if (std::is_constant_evaluated())
				detail::generic::mask_shuffle(result.m_data, m.m_data, Idx{});
			else
				detail::mask_shuffle(result.m_data, m.m_data, Idx{});
			return result;
		}
	}
	/** Shuffles elements of a vector according to the provided indices.
	 * @tparam I Indices of elements of the source vector in the order they should be shuffled to the destination vector.
	 * @return Result vector who's elements are specified by `I`.
	 * @example shuffle<2, 1, 0>({3, 4, 5}) -> {5, 4, 3} */
	template<std::size_t... I, typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, sizeof...(I), Sp> shuffle(const basic_vec<U, M, Sp> &v) noexcept
	{
		using Idx = std::index_sequence<I...>;
		if constexpr (std::is_same_v<Idx, std::make_index_sequence<M>>)
			return v;
		else
		{
			basic_vec<U, sizeof...(I)> result;
			if (std::is_constant_evaluated())
				detail::generic::vector_shuffle(result.m_data, v.m_data, Idx{});
			else
				detail::vector_shuffle(result.m_data, v.m_data, Idx{});
			return result;
		}
	}

	/** Interleaves elements of two vectors according to the provided mask.
	 * @param l Left-hand vector.
	 * @param r Right-hand vector.
	 * @param mask Mask used to select vector elements.
	 * `true` will select the left-hand element, `false` will select the right-hand element.
	 * @return Result of the interleave operation. */
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> interleave(const basic_vec<U, M, Sp> &l,
														   const basic_vec<U, M, Sp> &r,
														   const vec_mask<basic_vec<U, M, Sp>> &mask) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_interleave(result.m_data, l.m_data, r.m_data, mask.m_data);
		else
			detail::vector_interleave(result.m_data, l.m_data, r.m_data, mask.m_data);
		return result;
	}
}	 // namespace sek::math
