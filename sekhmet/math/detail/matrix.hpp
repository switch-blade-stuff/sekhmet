//
// Created by switchblade on 02/05/22.
//

#pragma once

#include "sekhmet/detail/hash.hpp"
#include "util.hpp"
#include "vector.hpp"

/* Use a macro to avoid duplicating common definitions for matrix specializations. */
#define SEK_MATH_MATRIX_COMMON(T, N, M)                                                                                \
public:                                                                                                                \
	typedef T value_type;                                                                                              \
	typedef basic_vector<T, M> col_type;                                                                               \
	typedef basic_vector<T, N> row_type;                                                                               \
                                                                                                                       \
	/** Number of columns in the matrix. */                                                                            \
	constexpr static auto columns = N;                                                                                 \
	/** Number of rows in the matrix. */                                                                               \
	constexpr static auto rows = M;                                                                                    \
                                                                                                                       \
private:                                                                                                               \
	col_type data[N] = {}; /* Matrices stored as columns to optimize SIMD computation. */                              \
                                                                                                                       \
public:                                                                                                                \
	/** Initializes an identity matrix. */                                                                             \
	constexpr basic_matrix() noexcept : basic_matrix(1)                                                                \
	{                                                                                                                  \
	}                                                                                                                  \
	/** Initializes the main diagonal of the matrix to the provided value. */                                          \
	constexpr explicit basic_matrix(T v) noexcept                                                                      \
	{                                                                                                                  \
		for (std::size_t i = 0; i < N && i < M; ++i) col(i)[i] = v;                                                    \
	}                                                                                                                  \
                                                                                                                       \
	constexpr explicit basic_matrix(const col_type(&cols)[N]) noexcept                                                 \
	{                                                                                                                  \
		std::copy_n(cols, N, data);                                                                                    \
	}                                                                                                                  \
                                                                                                                       \
	/** Returns the corresponding column of the matrix. */                                                             \
	[[nodiscard]] constexpr col_type &operator[](std::size_t i) noexcept                                               \
	{                                                                                                                  \
		return data[i];                                                                                                \
	}                                                                                                                  \
	/** @copydoc operator[] */                                                                                         \
	[[nodiscard]] constexpr const col_type &operator[](std::size_t i) const noexcept                                   \
	{                                                                                                                  \
		return data[i];                                                                                                \
	}                                                                                                                  \
                                                                                                                       \
	/** @copydoc operator[] */                                                                                         \
	[[nodiscard]] constexpr col_type &col(std::size_t i) noexcept                                                      \
	{                                                                                                                  \
		return data[i];                                                                                                \
	}                                                                                                                  \
	/** @copydoc operator[] */                                                                                         \
	[[nodiscard]] constexpr const col_type &col(std::size_t i) const noexcept                                          \
	{                                                                                                                  \
		return data[i];                                                                                                \
	}                                                                                                                  \
                                                                                                                       \
private:                                                                                                               \
	template<std::size_t... Is>                                                                                        \
	[[nodiscard]] constexpr row_type row(std::index_sequence<Is...>, std::size_t i) const noexcept                     \
	{                                                                                                                  \
		return row_type{data[Is][i]...};                                                                               \
	}                                                                                                                  \
                                                                                                                       \
public:                                                                                                                \
	/** Returns copy of the corresponding row of the matrix. */                                                        \
	[[nodiscard]] constexpr row_type row(std::size_t i) const noexcept                                                 \
	{                                                                                                                  \
		return row(std::make_index_sequence<columns>{}, i);                                                            \
	}                                                                                                                  \
                                                                                                                       \
	[[nodiscard]] constexpr bool operator==(const basic_matrix &) const noexcept = default;                            \
                                                                                                                       \
	constexpr void swap(const basic_matrix &other) noexcept                                                            \
	{                                                                                                                  \
		std::swap(data, other.data);                                                                                   \
	}

namespace sek::math
{
	/** @brief Generic matrix.
	 * Matrices are stored in column-major form.
	 * @note Generic matrix types are not guaranteed to be SIMD-optimized. */
	template<arithmetic T, std::size_t N, std::size_t M>
	struct basic_matrix
	{
		SEK_MATH_MATRIX_COMMON(T, N, M)
	};

	template<typename T, std::size_t N, std::size_t M>
	[[nodiscard]] constexpr hash_t hash(const basic_matrix<T, N, M> &m) noexcept
	{
		hash_t result = 0;
		// clang-format off
		for (std::size_t i = 0; i < N; ++i)
			hash_combine(result, m[i]);
		// clang-format on
		return result;
	}
	template<typename T, std::size_t N, std::size_t M>
	constexpr void swap(basic_matrix<T, N, M> &a, basic_matrix<T, N, M> &b) noexcept
	{
		a.swap(b);
	}

	/** Returns a transposed copy of a matrix. */
	template<typename T, std::size_t N, std::size_t M>
	[[nodiscard]] constexpr basic_matrix<T, M, N> transpose(const basic_matrix<T, N, M> &m) noexcept
	{
		basic_matrix<T, M, N> result;
		for (std::size_t c = 0; c < N; ++c)
			for (std::size_t r = 0; r < M; ++r) result[r][c] = m[c][r];
		return result;
	}

	/** Returns a matrix which is the result of addition of two matrices. */
	template<typename T, std::size_t N, std::size_t M>
	[[nodiscard]] constexpr basic_matrix<T, N, M> operator+(const basic_matrix<T, N, M> &l, const basic_matrix<T, N, M> &r) noexcept
	{
		basic_matrix<T, N, M> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = l[i] + r[i];
		return result;
	}
	/** Adds a matrix to a matrix. */
	template<typename T, std::size_t N, std::size_t M>
	constexpr basic_matrix<T, N, M> &operator+=(basic_matrix<T, N, M> &l, const basic_matrix<T, N, M> &r) noexcept
	{
		for (std::size_t i = 0; i < N; ++i) l[i] += r[i];
		return l;
	}
	/** Returns a matrix which is the result of subtraction of two matrices. */
	template<typename T, std::size_t N, std::size_t M>
	[[nodiscard]] constexpr basic_matrix<T, N, M> operator-(const basic_matrix<T, N, M> &l, const basic_matrix<T, N, M> &r) noexcept
	{
		basic_matrix<T, N, M> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = l[i] - r[i];
		return result;
	}
	/** Subtracts a matrix from a matrix. */
	template<typename T, std::size_t N, std::size_t M>
	constexpr basic_matrix<T, N, M> &operator-=(basic_matrix<T, N, M> &l, const basic_matrix<T, N, M> &r) noexcept
	{
		for (std::size_t i = 0; i < N; ++i) l[i] -= r[i];
		return l;
	}

	/** Returns a copy of a matrix multiplied by a scalar. */
	template<typename T, std::size_t N, std::size_t M>
	[[nodiscard]] constexpr basic_matrix<T, N, M> operator*(const basic_matrix<T, N, M> &l, T r) noexcept
	{
		basic_matrix<T, N, M> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = l[i] * r;
		return result;
	}
	/** @copydoc operator* */
	template<typename T, std::size_t N, std::size_t M>
	[[nodiscard]] constexpr basic_matrix<T, N, M> operator*(T l, const basic_matrix<T, N, M> &r) noexcept
	{
		return r * l;
	}
	/** Multiplies matrix by a scalar. */
	template<typename T, std::size_t N, std::size_t M>
	constexpr basic_matrix<T, N, M> &operator*=(basic_matrix<T, N, M> &l, T r) noexcept
	{
		for (std::size_t i = 0; i < N; ++i) l[i] *= r;
		return l;
	}
	/** Returns a copy of a matrix divided by a scalar. */
	template<typename T, std::size_t N, std::size_t M>
	[[nodiscard]] constexpr basic_matrix<T, N, M> operator/(const basic_matrix<T, N, M> &l, T r) noexcept
	{
		basic_matrix<T, N, M> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = l[i] / r;
		return result;
	}
	/** Returns a matrix produced by dividing a scalar by a matrix. */
	template<typename T, std::size_t N, std::size_t M>
	[[nodiscard]] constexpr basic_matrix<T, N, M> operator/(T l, const basic_matrix<T, N, M> &r) noexcept
	{
		basic_matrix<T, N, M> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = l / r[i];
		return result;
	}
	/** Divides matrix by a scalar. */
	template<typename T, std::size_t N, std::size_t M>
	constexpr basic_matrix<T, N, M> &operator/=(basic_matrix<T, N, M> &l, T r) noexcept
	{
		for (std::size_t i = 0; i < N; ++i) l[i] /= r;
		return l;
	}

	/** Preforms a bitwise AND on two matrices. */
	template<typename T, std::size_t N, std::size_t M>
	constexpr basic_matrix<T, N, M> &operator&=(basic_matrix<T, N, M> &l, const basic_matrix<T, N, M> &r) noexcept
	{
		for (std::size_t i = 0; i < N; ++i) l[i] &= r[i];
		return l;
	}
	/** Returns a matrix which is the result of bitwise AND of two matrices. */
	template<typename T, std::size_t N, std::size_t M>
	[[nodiscard]] constexpr basic_matrix<T, N, M> operator&(const basic_matrix<T, N, M> &l, const basic_matrix<T, N, M> &r) noexcept
	{
		basic_matrix<T, N, M> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = l[i] & r[i];
		return result;
	}
	/** Preforms a bitwise OR on two matrices. */
	template<typename T, std::size_t N, std::size_t M>
	constexpr basic_matrix<T, N, M> &operator|=(basic_matrix<T, N, M> &l, const basic_matrix<T, N, M> &r) noexcept
	{
		for (std::size_t i = 0; i < N; ++i) l[i] |= r[i];
		return l;
	}
	/** Returns a matrix which is the result of bitwise OR of two matrices. */
	template<typename T, std::size_t N, std::size_t M>
	[[nodiscard]] constexpr basic_matrix<T, N, M> operator|(const basic_matrix<T, N, M> &l, const basic_matrix<T, N, M> &r) noexcept
	{
		basic_matrix<T, N, M> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = l[i] | r[i];
		return result;
	}
	/** Returns a matrix which is the result of bitwise XOR of two matrices. */
	template<typename T, std::size_t N, std::size_t M>
	[[nodiscard]] constexpr basic_matrix<T, N, M> operator^(const basic_matrix<T, N, M> &l, const basic_matrix<T, N, M> &r) noexcept
	{
		basic_matrix<T, N, M> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = l[i] ^ r[i];
		return result;
	}
	/** Preforms a bitwise XOR on two matrices. */
	template<typename T, std::size_t N, std::size_t M>
	constexpr basic_matrix<T, N, M> &operator^=(basic_matrix<T, N, M> &l, const basic_matrix<T, N, M> &r) noexcept
	{
		for (std::size_t i = 0; i < N; ++i) l[i] ^= r[i];
		return l;
	}
	/** Returns a bitwise inverted copy of a matrix. */
	template<typename T, std::size_t N, std::size_t M>
	[[nodiscard]] constexpr basic_matrix<T, N, M> operator~(const basic_matrix<T, N, M> &m) noexcept
	{
		basic_matrix<T, N, M> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = ~m[i];
		return result;
	}

	/** Returns a copy of the matrix. */
	template<typename T, std::size_t N, std::size_t M>
	[[nodiscard]] constexpr basic_matrix<T, N, M> operator+(const basic_matrix<T, N, M> &m) noexcept
		requires std::is_signed_v<T>
	{
		return m;
	}
	/** Returns a negated copy of the matrix. */
	template<typename T, std::size_t N, std::size_t M>
	[[nodiscard]] constexpr basic_matrix<T, N, M> operator-(const basic_matrix<T, N, M> &m) noexcept
		requires std::is_signed_v<T>
	{
		basic_matrix<T, N, M> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = -m[i];
		return result;
	}

	/** Returns a matrix which is the result of multiplying two matrices. */
	template<typename T, std::size_t C0, std::size_t R0, std::size_t C1>
	[[nodiscard]] constexpr basic_matrix<T, C1, R0> operator*(const basic_matrix<T, C0, R0> &l,
															  const basic_matrix<T, C1, C0> &r) noexcept
	{
		basic_matrix<T, C1, R0> result = {};
		for (std::size_t c1 = 0; c1 != C1; ++c1)
			for (std::size_t r0 = 0; r0 != R0; ++r0)
				for (std::size_t c0 = 0; c0 < C0; ++c0) result[c1][r0] += l[c0][r0] * r[c1][c0];
		return result;
	}
	/** Returns a vector which is the result of multiplying matrix by a vector. */
	template<typename T, std::size_t N, std::size_t M>
	[[nodiscard]] constexpr basic_vector<T, M> operator*(const basic_matrix<T, N, M> &m, const basic_vector<T, N> &v) noexcept
	{
		basic_vector<T, N> result = {};
		for (std::size_t i = 0; i < N; ++i) result += m[i] * v[i];
		return result;
	}
	/** Returns a vector which is the result of multiplying vector by a matrix. */
	template<typename T, std::size_t C0, std::size_t C1>
	[[nodiscard]] constexpr basic_vector<T, C1> operator*(const basic_vector<T, C0> &v, const basic_matrix<T, C1, C0> &m) noexcept
	{
		basic_vector<T, C1> result = {};
		for (std::size_t i = 0; i < C1; ++i) result[i] = dot(v, m[i]);
		return result;
	}

	/** Gets the Ith column of the matrix. */
	template<std::size_t I, typename T, std::size_t N, std::size_t M>
	[[nodiscard]] constexpr typename basic_matrix<T, N, M>::col_type &get(basic_matrix<T, N, M> &m) noexcept
	{
		return m[I];
	}
	/** @copydoc get */
	template<std::size_t I, typename T, std::size_t N, std::size_t M>
	[[nodiscard]] constexpr const typename basic_matrix<T, N, M>::col_type &get(const basic_matrix<T, N, M> &m) noexcept
	{
		return m[I];
	}
	/** Gets the Jth element of the Ith column of the matrix. */
	template<std::size_t I, std::size_t J, typename T, std::size_t N, std::size_t M>
	[[nodiscard]] constexpr T &get(basic_matrix<T, N, M> &m) noexcept
	{
		return m[I][J];
	}
	template<std::size_t I, std::size_t J, typename T, std::size_t N, std::size_t M>
	[[nodiscard]] constexpr const T &get(const basic_matrix<T, N, M> &m) noexcept
	{
		return m[I][J];
	}
}	 // namespace sek::math

template<typename T, std::size_t N, std::size_t M>
struct std::tuple_size<sek::math::basic_matrix<T, N, M>> : std::integral_constant<std::size_t, N>
{
};
template<std::size_t I, typename T, std::size_t N, std::size_t M>
struct std::tuple_element<I, sek::math::basic_matrix<T, N, M>>
{
	using type = typename sek::math::basic_matrix<T, N, M>::col_type;
};
