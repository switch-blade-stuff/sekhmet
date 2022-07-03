/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../fwd.hpp"

#define SEK_DETAIL_FRIEND_UTILITY                                                                                      \
	template<std::floating_point U, std::size_t M, policy_t Sp>                                                  \
	friend constexpr basic_vec<U, M, Sp> round(const basic_vec<U, M, Sp> &) noexcept;                                  \
	template<std::floating_point U, std::size_t M, policy_t Sp>                                                  \
	friend constexpr basic_vec<U, M, Sp> floor(const basic_vec<U, M, Sp> &) noexcept;                                  \
	template<std::floating_point U, std::size_t M, policy_t Sp>                                                  \
	friend constexpr basic_vec<U, M, Sp> ceil(const basic_vec<U, M, Sp> &) noexcept;                                   \
	template<std::floating_point U, std::size_t M, policy_t Sp>                                                  \
	friend constexpr basic_vec<U, M, Sp> trunc(const basic_vec<U, M, Sp> &) noexcept;                                  \
                                                                                                                       \
	template<std::size_t... I, typename U, std::size_t M, policy_t Sp>                                           \
	friend constexpr vec_mask<basic_vec<U, sizeof...(I), Sp>> shuffle(const vec_mask<basic_vec<U, M, Sp>> &) noexcept; \
	template<std::size_t... I, typename U, std::size_t M, policy_t Sp>                                           \
	friend constexpr basic_vec<U, sizeof...(I), Sp> shuffle(const basic_vec<U, M, Sp> &) noexcept;                     \
                                                                                                                       \
	template<typename U, std::size_t M, policy_t Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> interleave(                                                                   \
		const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &, const vec_mask<basic_vec<U, M, Sp>> &) noexcept;

namespace sek::math
{
	template<std::size_t I, typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr decltype(auto) get(vec_mask<basic_vec<U, M, Sp>> &m) noexcept;
	template<std::size_t I, typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr decltype(auto) get(const vec_mask<basic_vec<U, M, Sp>> &m) noexcept;
	template<std::size_t I, typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr decltype(auto) get(basic_vec<U, M, Sp> &v) noexcept;
	template<std::size_t I, typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr decltype(auto) get(const basic_vec<U, M, Sp> &v) noexcept;

	template<typename U, std::size_t M, policy_t Sp, typename F>
	constexpr void vectorize(const vec_mask<basic_vec<U, M, Sp>> &m, F &&f);
	template<typename U, std::size_t M, policy_t Sp, typename F>
	constexpr void vectorize(vec_mask<basic_vec<U, M, Sp>> &m, F &&f);
	template<typename U, std::size_t M, policy_t Sp, typename F>
	constexpr void vectorize(const basic_vec<U, M, Sp> &v, F &&f);
	template<typename U, std::size_t M, policy_t Sp, typename F>
	constexpr void vectorize(basic_vec<U, M, Sp> &v, F &&f);

	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> round(const basic_vec<U, M, Sp> &v) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> floor(const basic_vec<U, M, Sp> &v) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> ceil(const basic_vec<U, M, Sp> &v) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> trunc(const basic_vec<U, M, Sp> &v) noexcept;

	template<std::size_t... I, typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, sizeof...(I), Sp>> shuffle(const vec_mask<basic_vec<U, M, Sp>> &m) noexcept;
	template<std::size_t... I, typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, sizeof...(I), Sp> shuffle(const basic_vec<U, M, Sp> &v) noexcept;

	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> interleave(const basic_vec<U, M, Sp> &l,
														   const basic_vec<U, M, Sp> &r,
														   const vec_mask<basic_vec<U, M, Sp>> &mask) noexcept;
}	 // namespace sek::math
