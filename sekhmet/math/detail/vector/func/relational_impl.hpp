/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "../type.hpp"

namespace sek::math
{
	/** Checks if all components of the vector mask are `true`. */
	template<std::convertible_to<bool> U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr bool all(const vec_mask<basic_vec<U, M, Sp>> &m) noexcept
	{
		bool result = true;
		vectorize(m, [&result](auto value) noexcept { result = result && static_cast<bool>(value); });
		return result;
	}
	/** Checks if any components of the vector mask are `true`. */
	template<std::convertible_to<bool> U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr bool any(const vec_mask<basic_vec<U, M, Sp>> &m) noexcept
	{
		bool result = false;
		vectorize(m, [&result](auto value) noexcept { result = result || static_cast<bool>(value); });
		return result;
	}
	/** Checks if no components of the vector mask are `true`. */
	template<std::convertible_to<bool> U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr bool none(const vec_mask<basic_vec<U, M, Sp>> &m) noexcept
	{
		return !any(m);
	}

	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator==(const vec_mask<basic_vec<U, M, Sp>> &l,
																	 const vec_mask<basic_vec<U, M, Sp>> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Sp>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::mask_eq(result.m_data, l.m_data, r.m_data);
		else
			detail::mask_eq(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator!=(const vec_mask<basic_vec<U, M, Sp>> &l,
																	 const vec_mask<basic_vec<U, M, Sp>> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Sp>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::mask_ne(result.m_data, l.m_data, r.m_data);
		else
			detail::mask_ne(result.m_data, l.m_data, r.m_data);
		return result;
	}

	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator&&(const vec_mask<basic_vec<U, M, Sp>> &l,
																	 const vec_mask<basic_vec<U, M, Sp>> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Sp>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::mask_and(result.m_data, l.m_data, r.m_data);
		else
			detail::mask_and(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator||(const vec_mask<basic_vec<U, M, Sp>> &l,
																	 const vec_mask<basic_vec<U, M, Sp>> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Sp>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::mask_or(result.m_data, l.m_data, r.m_data);
		else
			detail::mask_or(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator!(const vec_mask<basic_vec<U, M, Sp>> &v) noexcept
	{
		vec_mask<basic_vec<U, M, Sp>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::mask_neg(result.m_data, v.m_data);
		else
			detail::mask_neg(result.m_data, v.m_data);
		return result;
	}

	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator==(const basic_vec<U, M, Sp> &l,
																	 const basic_vec<U, M, Sp> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Sp>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_eq(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_eq(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator!=(const basic_vec<U, M, Sp> &l,
																	 const basic_vec<U, M, Sp> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Sp>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_ne(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_ne(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator<(const basic_vec<U, M, Sp> &l,
																	const basic_vec<U, M, Sp> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Sp>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_lt(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_lt(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator<=(const basic_vec<U, M, Sp> &l,
																	 const basic_vec<U, M, Sp> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Sp>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_le(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_le(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator>(const basic_vec<U, M, Sp> &l,
																	const basic_vec<U, M, Sp> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Sp>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_gt(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_gt(result.m_data, l.m_data, r.m_data);
		return result;
	}
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>> operator>=(const basic_vec<U, M, Sp> &l,
																	 const basic_vec<U, M, Sp> &r) noexcept
	{
		vec_mask<basic_vec<U, M, Sp>> result = {};
		if (std::is_constant_evaluated())
			detail::generic::vector_ge(result.m_data, l.m_data, r.m_data);
		else
			detail::vector_ge(result.m_data, l.m_data, r.m_data);
		return result;
	}

	/** Checks if elements of vector a equals vector b using an epsilon. */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>>
		fcmp_eq(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &epsilon) noexcept
	{
		return abs(a - b) <= epsilon;
	}
	/** @copydoc fcmp_eq */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>>
		fcmp_eq(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, U epsilon) noexcept
	{
		return fcmp_eq(a, b, basic_vec<U, M, Sp>{epsilon});
	}
	/** Checks if elements of vector a does not equal vector vector b using an epsilon. */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>>
		fcmp_ne(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &epsilon) noexcept
	{
		return abs(a - b) > epsilon;
	}
	/** @copydoc fcmp_ne */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>>
		fcmp_ne(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, U epsilon) noexcept
	{
		return fcmp_ne(a, b, basic_vec<U, M, Sp>{epsilon});
	}
	/** Checks if elements of vector a is less than or equal to vector b using an epsilon. */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>>
		fcmp_le(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &epsilon) noexcept
	{
		return a <= b || fcmp_eq(a, b, epsilon);
	}
	/** @copydoc fcmp_le */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>>
		fcmp_le(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, U epsilon) noexcept
	{
		return fcmp_le(a, b, basic_vec<U, M, Sp>{epsilon});
	}
	/** Checks if elements of vector a is greater than or equal to vector b using an epsilon. */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>>
		fcmp_ge(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &epsilon) noexcept
	{
		return a >= b || fcmp_eq(a, b, epsilon);
	}
	/** @copydoc fcmp_ge */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>>
		fcmp_ge(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, U epsilon) noexcept
	{
		return fcmp_ge(a, b, basic_vec<U, M, Sp>{epsilon});
	}
	/** Checks if elements of vector a is less than vector b using an epsilon. */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>>
		fcmp_lt(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &epsilon) noexcept
	{
		return a < b && fcmp_ne(a, b, epsilon);
	}
	/** @copydoc fcmp_lt */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>>
		fcmp_lt(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, U epsilon) noexcept
	{
		return fcmp_lt(a, b, basic_vec<U, M, Sp>{epsilon});
	}
	/** Checks if elements of vector a is less than vector b using an epsilon. */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>>
		fcmp_gt(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &epsilon) noexcept
	{
		return a > b && fcmp_ne(a, b, epsilon);
	}
	/** @copydoc fcmp_gt */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr vec_mask<basic_vec<U, M, Sp>>
		fcmp_gt(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, U epsilon) noexcept
	{
		return fcmp_gt(a, b, basic_vec<U, M, Sp>{epsilon});
	}

	/** Returns a vector consisting of maximum elements of a and b. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> max(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_max(result.m_data, a.m_data, b.m_data);
		else
			detail::vector_max(result.m_data, a.m_data, b.m_data);
		return result;
	}
	/** Returns a vector consisting of minimum elements of a and b. */
	template<typename U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> min(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b) noexcept
	{
		basic_vec<U, M, Sp> result;
		if (std::is_constant_evaluated())
			detail::generic::vector_min(result.m_data, a.m_data, b.m_data);
		else
			detail::vector_min(result.m_data, a.m_data, b.m_data);
		return result;
	}
	/** Clamps elements of a vector between a minimum and a maximum. */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp>
		clamp(const basic_vec<U, M, Sp> &value, const basic_vec<U, M, Sp> &min_val, const basic_vec<U, M, Sp> &max_val) noexcept
	{
		return max(min_val, min(max_val, value));
	}

	/** Returns a vector consisting of minimum elements of a and b using an epsilon. */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp>
		fmin(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &epsilon) noexcept
	{
		return interleave(a, b, fcmp_le(a, b, epsilon));
	}
	/** @copydoc fmin */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fmin(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, U epsilon) noexcept
	{
		return fmin(a, b, basic_vec<U, M, Sp>{epsilon});
	}
	/** Returns a vector consisting of maximum elements of a and b using an epsilon. */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp>
		fmax(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, const basic_vec<U, M, Sp> &epsilon) noexcept
	{
		return interleave(a, b, fcmp_ge(a, b, epsilon));
	}
	/** @copydoc fmax */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fmax(const basic_vec<U, M, Sp> &a, const basic_vec<U, M, Sp> &b, U epsilon) noexcept
	{
		return fmax(a, b, basic_vec<U, M, Sp>{epsilon});
	}

	/** Clamps elements of a vector between a minimum and a maximum using an epsilon. */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fclamp(const basic_vec<U, M, Sp> &value,
													   const basic_vec<U, M, Sp> &min_val,
													   const basic_vec<U, M, Sp> &max_val,
													   const basic_vec<U, M, Sp> &epsilon) noexcept
	{
		return fmax(min_val, fmin(max_val, value, epsilon), epsilon);
	}
	/** @copydoc fclamp */
	template<std::floating_point U, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<U, M, Sp> fclamp(const basic_vec<U, M, Sp> &value,
													   const basic_vec<U, M, Sp> &min_val,
													   const basic_vec<U, M, Sp> &max_val,
													   U epsilon) noexcept
	{
		return fclamp(value, min_val, max_val, basic_vec<U, M, Sp>{epsilon});
	}

}	 // namespace sek::math