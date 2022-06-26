/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../fwd.hpp"

#define SEK_DETAIL_FRIEND_TRIGONOMETRIC                                                                       \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> sin(const basic_vec<U, M, Sp> &) noexcept;                                    \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> cos(const basic_vec<U, M, Sp> &) noexcept;                                    \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> tan(const basic_vec<U, M, Sp> &) noexcept;                                    \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> asin(const basic_vec<U, M, Sp> &) noexcept;                                   \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> acos(const basic_vec<U, M, Sp> &) noexcept;                                   \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> atan(const basic_vec<U, M, Sp> &) noexcept;                                   \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> sinh(const basic_vec<U, M, Sp> &) noexcept;                                   \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> cosh(const basic_vec<U, M, Sp> &) noexcept;                                   \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> tanh(const basic_vec<U, M, Sp> &) noexcept;                                   \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> asinh(const basic_vec<U, M, Sp> &) noexcept;                                  \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> acosh(const basic_vec<U, M, Sp> &) noexcept;                                  \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> atanh(const basic_vec<U, M, Sp> &) noexcept;                                  \
                                                                                                                       \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> fast_sin(const basic_vec<U, M, Sp> &) noexcept;                               \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> fast_cos(const basic_vec<U, M, Sp> &) noexcept;                               \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> fast_tan(const basic_vec<U, M, Sp> &) noexcept;                               \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> fast_asin(const basic_vec<U, M, Sp> &) noexcept;                              \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> fast_acos(const basic_vec<U, M, Sp> &) noexcept;                              \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> fast_atan(const basic_vec<U, M, Sp> &) noexcept;                              \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> fast_sinh(const basic_vec<U, M, Sp> &) noexcept;                              \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> fast_cosh(const basic_vec<U, M, Sp> &) noexcept;                              \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> fast_tanh(const basic_vec<U, M, Sp> &) noexcept;                              \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> fast_asinh(const basic_vec<U, M, Sp> &) noexcept;                             \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> fast_acosh(const basic_vec<U, M, Sp> &) noexcept;                             \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> fast_atanh(const basic_vec<U, M, Sp> &) noexcept;                             \
                                                                                                                       \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> rad(const basic_vec<U, M, Sp> &) noexcept;                                    \
	template<typename U, std::size_t M, storage_policy Sp>                                                             \
	friend constexpr basic_vec<U, M, Sp> deg(const basic_vec<U, M, Sp> &) noexcept;

namespace sek::math
{
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> sin(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> cos(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> tan(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> asin(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> acos(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> atan(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> sinh(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> cosh(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> tanh(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> asinh(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> acosh(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> atanh(const basic_vec<U, M, Sp> &v) noexcept;

	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fast_sin(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fast_cos(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fast_tan(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fast_asin(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fast_acos(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fast_atan(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fast_sinh(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fast_cosh(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fast_tanh(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fast_asinh(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fast_acosh(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fast_atanh(const basic_vec<U, M, Sp> &v) noexcept;

	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> rad(const basic_vec<U, M, Sp> &v) noexcept;
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> deg(const basic_vec<U, M, Sp> &v) noexcept;
}	 // namespace sek::math
