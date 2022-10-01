/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../../fwd.hpp"

#define SEK_DETAIL_FRIEND_UTILITY                                                                                      \
	template<std::floating_point U, std::size_t M, policy_t Q>                                                         \
	friend constexpr basic_vec<U, M, Q> round(const basic_vec<U, M, Q> &) noexcept;                                    \
	template<std::floating_point U, std::size_t M, policy_t Q>                                                         \
	friend constexpr basic_vec<U, M, Q> floor(const basic_vec<U, M, Q> &) noexcept;                                    \
	template<std::floating_point U, std::size_t M, policy_t Q>                                                         \
	friend constexpr basic_vec<U, M, Q> ceil(const basic_vec<U, M, Q> &) noexcept;                                     \
	template<std::floating_point U, std::size_t M, policy_t Q>                                                         \
	friend constexpr basic_vec<U, M, Q> trunc(const basic_vec<U, M, Q> &) noexcept;                                    \
                                                                                                                       \
	template<std::size_t... I, typename U, std::size_t M, policy_t Q>                                                  \
	friend constexpr vec_mask<basic_vec<U, sizeof...(I), Q>> shuffle(const vec_mask<basic_vec<U, M, Q>> &) noexcept;   \
	template<std::size_t... I, typename U, std::size_t M, policy_t Q>                                                  \
	friend constexpr basic_vec<U, sizeof...(I), Q> shuffle(const basic_vec<U, M, Q> &) noexcept;                       \
                                                                                                                       \
	template<typename U, std::size_t M, policy_t Q>                                                                    \
	friend constexpr basic_vec<U, M, Q> interleave(                                                                    \
		const basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &, const vec_mask<basic_vec<U, M, Q>> &) noexcept;

namespace sek::math
{
	template<std::size_t I, typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr decltype(auto) get(vec_mask<basic_vec<U, M, Q>> &m) noexcept;
	template<std::size_t I, typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr decltype(auto) get(const vec_mask<basic_vec<U, M, Q>> &m) noexcept;
	template<std::size_t I, typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr decltype(auto) get(basic_vec<U, M, Q> &v) noexcept;
	template<std::size_t I, typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr decltype(auto) get(const basic_vec<U, M, Q> &v) noexcept;

	template<typename U, std::size_t M, policy_t Q, typename F>
	constexpr void vectorize(const vec_mask<basic_vec<U, M, Q>> &m, F &&f);
	template<typename U, std::size_t M, policy_t Q, typename F>
	constexpr void vectorize(vec_mask<basic_vec<U, M, Q>> &m, F &&f);
	template<typename U, std::size_t M, policy_t Q, typename F>
	constexpr void vectorize(const basic_vec<U, M, Q> &v, F &&f);
	template<typename U, std::size_t M, policy_t Q, typename F>
	constexpr void vectorize(basic_vec<U, M, Q> &v, F &&f);

	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> round(const basic_vec<U, M, Q> &v) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> floor(const basic_vec<U, M, Q> &v) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> ceil(const basic_vec<U, M, Q> &v) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> trunc(const basic_vec<U, M, Q> &v) noexcept;

	template<std::size_t... I, typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, sizeof...(I), Q>> shuffle(const vec_mask<basic_vec<U, M, Q>> &m) noexcept;
	template<std::size_t... I, typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, sizeof...(I), Q> shuffle(const basic_vec<U, M, Q> &v) noexcept;

	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> interleave(const basic_vec<U, M, Q> &l,
														  const basic_vec<U, M, Q> &r,
														  const vec_mask<basic_vec<U, M, Q>> &mask) noexcept;
}	 // namespace sek::math
