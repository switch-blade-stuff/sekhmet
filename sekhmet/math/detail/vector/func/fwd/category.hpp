/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../../fwd.hpp"

#define SEK_DETAIL_FRIEND_CATEGORY                                                                            \
	template<std::floating_point U, std::size_t M, policy_t Q>                                                  \
	friend constexpr vec_mask<basic_vec<U, M, Q>> is_nan(const basic_vec<U, M, Q> &) noexcept;                       \
	template<std::floating_point U, std::size_t M, policy_t Q>                                                  \
	friend constexpr vec_mask<basic_vec<U, M, Q>> is_inf(const basic_vec<U, M, Q> &) noexcept;                       \
	template<std::floating_point U, std::size_t M, policy_t Q>                                                  \
	friend constexpr vec_mask<basic_vec<U, M, Q>> is_fin(const basic_vec<U, M, Q> &) noexcept;                       \
	template<std::floating_point U, std::size_t M, policy_t Q>                                                  \
	friend constexpr vec_mask<basic_vec<U, M, Q>> is_neg(const basic_vec<U, M, Q> &) noexcept;                       \
	template<std::floating_point U, std::size_t M, policy_t Q>                                                  \
	friend constexpr vec_mask<basic_vec<U, M, Q>> is_norm(const basic_vec<U, M, Q> &) noexcept;

namespace sek
{
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> is_nan(const basic_vec<U, M, Q> &v) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> is_inf(const basic_vec<U, M, Q> &v) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> is_fin(const basic_vec<U, M, Q> &v) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> is_neg(const basic_vec<U, M, Q> &v) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Q>> is_norm(const basic_vec<U, M, Q> &v) noexcept;
}	 // namespace sek
