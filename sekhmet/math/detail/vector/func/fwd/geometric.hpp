/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../../fwd.hpp"

#define SEK_DETAIL_FRIEND_GEOMETRIC                                                                                    \
	template<typename U, std::size_t M, policy_t Q>                                                                    \
	friend constexpr basic_vec<U, M, Q> lerp(                                                                          \
		const basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept;                  \
	template<typename U, policy_t Q>                                                                                   \
	friend constexpr basic_vec<U, 3, Q> cross(const basic_vec<U, 3, Q> &, const basic_vec<U, 3, Q> &) noexcept         \
		requires std::is_signed_v<U>;                                                                                  \
	template<typename U, std::size_t M, policy_t Q>                                                                    \
	friend constexpr U dot(const basic_vec<U, M, Q> &, const basic_vec<U, M, Q> &) noexcept;                           \
	template<typename U, std::size_t M, policy_t Q>                                                                    \
	friend constexpr basic_vec<U, M, Q> norm(const basic_vec<U, M, Q> &) noexcept;                                     \
	template<typename U, std::size_t M, policy_t Q>                                                                    \
	friend constexpr U magn(const basic_vec<U, M, Q> &) noexcept;

namespace sek
{
	// clang-format off
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> lerp(const basic_vec<U, M, Q> &l, const basic_vec<U, M, Q> &r, const basic_vec<U, M, Q> &t) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> lerp(const basic_vec<U, M, Q> &l, const basic_vec<U, M, Q> &r, U t) noexcept;
	template<typename T, policy_t Q>
	[[nodiscard]] constexpr basic_vec<T, 3, Q> cross(const basic_vec<T, 3, Q> &l, const basic_vec<T, 3, Q> &r) noexcept requires std::is_signed_v<T>;
	// clang-format on

	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr U dot(const basic_vec<U, M, Q> &l, const basic_vec<U, M, Q> &r) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr U magn(const basic_vec<U, M, Q> &v) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> norm(const basic_vec<U, M, Q> &v) noexcept;
	template<typename U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr U dist(const basic_vec<U, M, Q> &l, const basic_vec<U, M, Q> &r) noexcept;

	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> reflect(const basic_vec<U, M, Q> &i, const basic_vec<U, M, Q> &n) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<U, M, Q> refract(const basic_vec<U, M, Q> &i, const basic_vec<U, M, Q> &n, U r) noexcept;
}	 // namespace sek
