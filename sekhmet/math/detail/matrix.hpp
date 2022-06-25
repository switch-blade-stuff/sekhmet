/*
 * Created by switchblade on 02/05/22
 */

#pragma once

#include "sekhmet/detail/hash.hpp"

#include "matrix_macros.hpp"
#include "util.hpp"
#include "vector.hpp"

namespace sek::math
{
	/** @brief Structure representing a mathematical matrix.
	 * Matrices are stored in column-major form.
	 * @tparam T Type of values stored in the matrix.
	 * @tparam N Amount of columns of the matrix.
	 * @tparam M Amount of rows of the matrix.
	 * @tparam Policy Policy used for storage & optimization.
	 * @note Generic matrix types are not guaranteed to be SIMD-optimized. */
	template<arithmetic T, std::size_t N, std::size_t M, storage_policy Policy = storage_policy::OPTIMAL>
	class basic_mat;

	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr hash_t hash(const basic_mat<T, N, M, Sp> &m) noexcept
	{
		hash_t result = 0;
		// clang-format off
		for (std::size_t i = 0; i < N; ++i)
			hash_combine(result, m[i]);
		// clang-format on
		return result;
	}
	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	constexpr void swap(basic_mat<T, N, M, Sp> &a, basic_mat<T, N, M, Sp> &b) noexcept
	{
		a.swap(b);
	}

	/** Returns a transposed copy of a matrix. */
	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_mat<T, M, N, Sp> transpose(const basic_mat<T, N, M, Sp> &m) noexcept
	{
		basic_mat<T, M, N, Sp> result;
		for (std::size_t c = 0; c < N; ++c)
			for (std::size_t r = 0; r < M; ++r) result[r][c] = m[c][r];
		return result;
	}

	/** Returns a matrix which is the result of addition of two matrices. */
	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_mat<T, N, M, Sp> operator+(const basic_mat<T, N, M, Sp> &l,
															 const basic_mat<T, N, M, Sp> &r) noexcept
	{
		basic_mat<T, N, M, Sp> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = l[i] + r[i];
		return result;
	}
	/** Adds a matrix to a matrix. */
	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	constexpr basic_mat<T, N, M, Sp> &operator+=(basic_mat<T, N, M, Sp> &l, const basic_mat<T, N, M, Sp> &r) noexcept
	{
		for (std::size_t i = 0; i < N; ++i) l[i] += r[i];
		return l;
	}
	/** Returns a matrix which is the result of subtraction of two matrices. */
	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_mat<T, N, M, Sp> operator-(const basic_mat<T, N, M, Sp> &l,
															 const basic_mat<T, N, M, Sp> &r) noexcept
	{
		basic_mat<T, N, M, Sp> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = l[i] - r[i];
		return result;
	}
	/** Subtracts a matrix from a matrix. */
	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	constexpr basic_mat<T, N, M, Sp> &operator-=(basic_mat<T, N, M, Sp> &l, const basic_mat<T, N, M, Sp> &r) noexcept
	{
		for (std::size_t i = 0; i < N; ++i) l[i] -= r[i];
		return l;
	}

	/** Returns a copy of a matrix multiplied by a scalar. */
	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_mat<T, N, M, Sp> operator*(const basic_mat<T, N, M, Sp> &l, T r) noexcept
	{
		basic_mat<T, N, M, Sp> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = l[i] * r;
		return result;
	}
	/** @copydoc operator* */
	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_mat<T, N, M, Sp> operator*(T l, const basic_mat<T, N, M, Sp> &r) noexcept
	{
		return r * l;
	}
	/** Multiplies matrix by a scalar. */
	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	constexpr basic_mat<T, N, M, Sp> &operator*=(basic_mat<T, N, M, Sp> &l, T r) noexcept
	{
		for (std::size_t i = 0; i < N; ++i) l[i] *= r;
		return l;
	}
	/** Returns a copy of a matrix divided by a scalar. */
	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_mat<T, N, M, Sp> operator/(const basic_mat<T, N, M, Sp> &l, T r) noexcept
	{
		basic_mat<T, N, M, Sp> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = l[i] / r;
		return result;
	}
	/** Returns a matrix produced by dividing a scalar by a matrix. */
	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_mat<T, N, M, Sp> operator/(T l, const basic_mat<T, N, M, Sp> &r) noexcept
	{
		basic_mat<T, N, M, Sp> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = l / r[i];
		return result;
	}
	/** Divides matrix by a scalar. */
	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	constexpr basic_mat<T, N, M, Sp> &operator/=(basic_mat<T, N, M, Sp> &l, T r) noexcept
	{
		for (std::size_t i = 0; i < N; ++i) l[i] /= r;
		return l;
	}

	/** Preforms a bitwise AND on two matrices. */
	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	constexpr basic_mat<T, N, M, Sp> &operator&=(basic_mat<T, N, M, Sp> &l, const basic_mat<T, N, M, Sp> &r) noexcept
	{
		for (std::size_t i = 0; i < N; ++i) l[i] &= r[i];
		return l;
	}
	/** Returns a matrix which is the result of bitwise AND of two matrices. */
	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_mat<T, N, M, Sp> operator&(const basic_mat<T, N, M, Sp> &l,
															 const basic_mat<T, N, M, Sp> &r) noexcept
	{
		basic_mat<T, N, M, Sp> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = l[i] & r[i];
		return result;
	}
	/** Preforms a bitwise OR on two matrices. */
	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	constexpr basic_mat<T, N, M, Sp> &operator|=(basic_mat<T, N, M, Sp> &l, const basic_mat<T, N, M, Sp> &r) noexcept
	{
		for (std::size_t i = 0; i < N; ++i) l[i] |= r[i];
		return l;
	}
	/** Returns a matrix which is the result of bitwise OR of two matrices. */
	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_mat<T, N, M, Sp> operator|(const basic_mat<T, N, M, Sp> &l,
															 const basic_mat<T, N, M, Sp> &r) noexcept
	{
		basic_mat<T, N, M, Sp> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = l[i] | r[i];
		return result;
	}
	/** Returns a matrix which is the result of bitwise XOR of two matrices. */
	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_mat<T, N, M, Sp> operator^(const basic_mat<T, N, M, Sp> &l,
															 const basic_mat<T, N, M, Sp> &r) noexcept
	{
		basic_mat<T, N, M, Sp> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = l[i] ^ r[i];
		return result;
	}
	/** Preforms a bitwise XOR on two matrices. */
	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	constexpr basic_mat<T, N, M, Sp> &operator^=(basic_mat<T, N, M, Sp> &l, const basic_mat<T, N, M, Sp> &r) noexcept
	{
		for (std::size_t i = 0; i < N; ++i) l[i] ^= r[i];
		return l;
	}
	/** Returns a bitwise inverted copy of a matrix. */
	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_mat<T, N, M, Sp> operator~(const basic_mat<T, N, M, Sp> &m) noexcept
	{
		basic_mat<T, N, M, Sp> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = ~m[i];
		return result;
	}

	/** Returns a copy of the matrix. */
	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_mat<T, N, M, Sp> operator+(const basic_mat<T, N, M, Sp> &m) noexcept
		requires std::is_signed_v<T>
	{
		return m;
	}
	/** Returns a negated copy of the matrix. */
	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_mat<T, N, M, Sp> operator-(const basic_mat<T, N, M, Sp> &m) noexcept
		requires std::is_signed_v<T>
	{
		basic_mat<T, N, M, Sp> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = -m[i];
		return result;
	}

	/** Returns a matrix which is the result of multiplying two matrices. */
	template<typename T, std::size_t C0, std::size_t R0, std::size_t C1, storage_policy Sp>
	[[nodiscard]] constexpr basic_mat<T, C1, R0, Sp> operator*(const basic_mat<T, C0, R0, Sp> &l,
															   const basic_mat<T, C1, C0, Sp> &r) noexcept
	{
		basic_mat<T, C1, R0, Sp> result = {};
		for (std::size_t c1 = 0; c1 != C1; ++c1)
			for (std::size_t r0 = 0; r0 != R0; ++r0)
				for (std::size_t c0 = 0; c0 < C0; ++c0) result[c1][r0] += l[c0][r0] * r[c1][c0];
		return result;
	}
	/** Returns a vector which is the result of multiplying matrix by a vector. */
	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, M> operator*(const basic_mat<T, N, M, Sp> &m, const basic_vec<T, N, Sp> &v) noexcept
	{
		basic_vec<T, N> result = {};
		for (std::size_t i = 0; i < N; ++i) result += m[i] * v[i];
		return result;
	}
	/** Returns a vector which is the result of multiplying vector by a matrix. */
	template<typename T, std::size_t C0, std::size_t C1, storage_policy Sp>
	[[nodiscard]] constexpr basic_vec<T, C1, Sp> operator*(const basic_vec<T, C0, Sp> &v, const basic_mat<T, C1, C0, Sp> &m) noexcept
	{
		basic_vec<T, C1> result = {};
		for (std::size_t i = 0; i < C1; ++i) result[i] = dot(v, m[i]);
		return result;
	}

	/** Gets the Ith column of the matrix. */
	template<std::size_t I, typename T, std::size_t N, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr typename basic_mat<T, N, M, Sp>::col_type &get(basic_mat<T, N, M, Sp> &m) noexcept
	{
		return m[I];
	}
	/** @copydoc get */
	template<std::size_t I, typename T, std::size_t N, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr const typename basic_mat<T, N, M, Sp>::col_type &get(const basic_mat<T, N, M, Sp> &m) noexcept
	{
		return m[I];
	}
	/** Gets the Jth element of the Ith column of the matrix. */
	template<std::size_t I, std::size_t J, typename T, std::size_t N, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr T &get(basic_mat<T, N, M, Sp> &m) noexcept
	{
		return m[I][J];
	}
	template<std::size_t I, std::size_t J, typename T, std::size_t N, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr const T &get(const basic_mat<T, N, M, Sp> &m) noexcept
	{
		return m[I][J];
	}
}	 // namespace sek::math

template<typename T, std::size_t N, std::size_t M, sek::math::storage_policy Sp>
struct std::tuple_size<sek::math::basic_mat<T, N, M, Sp>> : std::integral_constant<std::size_t, N>
{
};
template<std::size_t I, typename T, std::size_t N, std::size_t M, sek::math::storage_policy Sp>
struct std::tuple_element<I, sek::math::basic_mat<T, N, M, Sp>>
{
	using type = typename sek::math::basic_mat<T, N, M, Sp>::col_type;
};
