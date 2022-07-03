/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../../fwd.hpp"

#define SEK_DETAIL_FRIEND_RELATIONAL                                                                                            \
	template<typename U, std::size_t M, policy_t Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator==(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept; \
	template<typename U, std::size_t M, policy_t Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator!=(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept; \
	template<typename U, std::size_t M, policy_t Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator<(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;  \
	template<typename U, std::size_t M, policy_t Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator<=(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept; \
	template<typename U, std::size_t M, policy_t Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator>(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;  \
	template<typename U, std::size_t M, policy_t Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator>=(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept; \
                                                                                                                                  \
	template<typename U, std::size_t M, policy_t Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator==(const vec_mask<basic_vec<U, M, Sp>> &,                              \
															  const vec_mask<basic_vec<U, M, Sp>> &) noexcept;                    \
	template<typename U, std::size_t M, policy_t Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator!=(const vec_mask<basic_vec<U, M, Sp>> &,                              \
															  const vec_mask<basic_vec<U, M, Sp>> &) noexcept;                    \
                                                                                                                                  \
	template<typename U, std::size_t M, policy_t Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator&&(const vec_mask<basic_vec<U, M, Sp>> &,                              \
															  const vec_mask<basic_vec<U, M, Sp>> &) noexcept;                    \
	template<typename U, std::size_t M, policy_t Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator||(const vec_mask<basic_vec<U, M, Sp>> &,                              \
															  const vec_mask<basic_vec<U, M, Sp>> &) noexcept;                    \
	template<typename U, std::size_t M, policy_t Sp>                                                                        \
	friend constexpr vec_mask<basic_vec<U, M, Sp>> operator!(const vec_mask<basic_vec<U, M, Sp>> &) noexcept;                     \
                                                                                                                                  \
	template<typename U, std::size_t M, policy_t Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> max(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;                  \
	template<typename U, std::size_t M, policy_t Sp>                                                                        \
	friend constexpr basic_vec<U, M, Sp> min(const basic_vec<U, M, Sp> &, const basic_vec<U, M, Sp> &) noexcept;

namespace sek::math
{
	template<std::convertible_to<bool> U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr bool all(const vec_mask<basic_vec<U, M, Sp>> &m) noexcept;
	template<std::convertible_to<bool> U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr bool any(const vec_mask<basic_vec<U, M, Sp>> &m) noexcept;
	template<std::convertible_to<bool> U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr bool none(const vec_mask<basic_vec<U, M, Sp>> &m) noexcept;

	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator==(const vec_mask<basic_vec<U, M, Sp>> &l,
																	 const vec_mask<basic_vec<U, M, Sp>> &r) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator!=(const vec_mask<basic_vec<U, M, Sp>> &l,
																	 const vec_mask<basic_vec<U, M, Sp>> &r) noexcept;

	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator&&(const vec_mask<basic_vec<U, M, Sp>> &l,
																	 const vec_mask<basic_vec<U, M, Sp>> &r) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator||(const vec_mask<basic_vec<U, M, Sp>> &l,
																	 const vec_mask<basic_vec<U, M, Sp>> &r) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator!(const vec_mask<basic_vec<U, M, Sp>> &v) noexcept;

	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator==(const basic_vec<U, M, Sp> &l,
																	 const basic_vec<U, M, Sp> &r) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator!=(const basic_vec<U, M, Sp> &l,
																	 const basic_vec<U, M, Sp> &r) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator<(const basic_vec<U, M, Sp> &l,
																	const basic_vec<U, M, Sp> &r) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator<=(const basic_vec<U, M, Sp> &l,
																	 const basic_vec<U, M, Sp> &r) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator>(const basic_vec<U, M, Sp> &l,
																	const basic_vec<U, M, Sp> &r) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator>=(const basic_vec<U, M, Sp> &l,
																	 const basic_vec<U, M, Sp> &r) noexcept;

	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>>
		fcmp_eq(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &epsilon) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> fcmp_eq(const basic_vec<U, M, Sp> &a,
																  const basic_vec<U, M, Sp> &b,
																  U epsilon = std::numeric_limits<U>::epsilon()) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>>
		fcmp_ne(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &epsilon) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> fcmp_ne(const basic_vec<U, M, Sp> &a,
																  const basic_vec<U, M, Sp> &b,
																  U epsilon = std::numeric_limits<U>::epsilon()) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>>
		fcmp_le(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &epsilon) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> fcmp_le(const basic_vec<U, M, Sp> &a,
																  const basic_vec<U, M, Sp> &b,
																  U epsilon = std::numeric_limits<U>::epsilon()) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>>
		fcmp_ge(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &epsilon) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> fcmp_ge(const basic_vec<U, M, Sp> &a,
																  const basic_vec<U, M, Sp> &b,
																  U epsilon = std::numeric_limits<U>::epsilon()) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>>
		fcmp_lt(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &epsilon) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> fcmp_lt(const basic_vec<U, M, Sp> &a,
																  const basic_vec<U, M, Sp> &b,
																  U epsilon = std::numeric_limits<U>::epsilon()) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>>
		fcmp_gt(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &epsilon) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> fcmp_gt(const basic_vec<U, M, Sp> &a,
																  const basic_vec<U, M, Sp> &b,
																  U epsilon = std::numeric_limits<U>::epsilon()) noexcept;

	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> max(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b) noexcept;
	template<typename U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> min(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> clamp(const basic_vec<U, M, Sp> &value,
													  const basic_vec<U, M, Sp> &min_val,
													  const basic_vec<U, M, Sp> &max_val) noexcept;

	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp>
		fmin(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &epsilon) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fmin(const basic_vec<U, M, Sp> &a,
													 const basic_vec<U, M, Sp> &b,
													 U epsilon = std::numeric_limits<U>::epsilon()) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp>
		fmax(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &epsilon) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fmax(const basic_vec<U, M, Sp> &a,
													 const basic_vec<U, M, Sp> &b,
													 U epsilon = std::numeric_limits<U>::epsilon()) noexcept;

	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fclamp(const basic_vec<U, M, Sp> &value,
													   const basic_vec<U, M, Sp> &min_val,
													   const basic_vec<U, M, Sp> &max_val,
													   const basic_vec<U, M, Sp> &epsilon) noexcept;
	template<std::floating_point U, std::size_t M, policy_t Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fclamp(const basic_vec<U, M, Sp> &value,
													   const basic_vec<U, M, Sp> &min_val,
													   const basic_vec<U, M, Sp> &max_val,
													   U = std::numeric_limits<U>::epsilon()) noexcept;

}	 // namespace sek::math