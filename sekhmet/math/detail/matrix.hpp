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
	constexpr static basic_matrix identity() noexcept                                                                  \
	{                                                                                                                  \
		return basic_matrix{1};                                                                                        \
	}                                                                                                                  \
                                                                                                                       \
private:                                                                                                               \
	col_type data[N] = {}; /* Matrices stored as columns to optimize SIMD computation. */                              \
                                                                                                                       \
public:                                                                                                                \
	constexpr basic_matrix() noexcept = default;                                                                       \
	/** Initializes the main diagonal of the matrix to the provided value. */                                          \
	constexpr explicit basic_matrix(T v) noexcept                                                                      \
	{                                                                                                                  \
		for (std::size_t i = 0, j = 0; i < N && j < M; ++i, ++j) data[i][j] = v;                                       \
	}                                                                                                                  \
                                                                                                                       \
	constexpr explicit basic_matrix(const col_type(&cols)[N]) noexcept                                                 \
	{                                                                                                                  \
		std::copy_n(cols, N, data);                                                                                    \
	}                                                                                                                  \
	template<typename U, std::size_t O, std::size_t P>                                                                 \
	constexpr basic_matrix(const basic_matrix<U, O, P> &other) noexcept                                                \
		requires(std::convertible_to<U, T> && O != N && P != M)                                                        \
	{                                                                                                                  \
		for (std::size_t i = 0; i < N && i < O; ++i) data[i] = other.data[i];                                          \
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

	template<arithmetic T>
	struct basic_matrix<T, 4, 2>
	{
		SEK_MATH_MATRIX_COMMON(T, 4, 2)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1) noexcept : data({c0, c1}) {}
		constexpr basic_matrix(T x0, T y0, T x1, T y1, T x2, T y2, T x3, T y3) noexcept
			: basic_matrix({x0, x1, x2, x3}, {y0, y1, y2, y3})
		{
		}
	};
	template<arithmetic T>
	struct basic_matrix<T, 4, 3>
	{
		SEK_MATH_MATRIX_COMMON(T, 4, 3)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1) noexcept : data({c0, c1}) {}
		constexpr basic_matrix(T x0, T y0, T z0, T x1, T y1, T z1, T x2, T y2, T z2, T x3, T y3, T z3) noexcept
			: basic_matrix({x0, x1, x2, x3}, {y0, y1, y2, y3}, {z0, z1, z2, z3})
		{
		}
	};
	template<arithmetic T>
	struct basic_matrix<T, 4, 4>
	{
		SEK_MATH_MATRIX_COMMON(T, 4, 4)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1) noexcept : data({c0, c1}) {}
		constexpr basic_matrix(T x0, T y0, T z0, T w0, T x1, T y1, T z1, T w1, T x2, T y2, T z2, T w2, T x3, T y3, T z3, T w3) noexcept
			: basic_matrix({x0, x1, x2, x3}, {y0, y1, y2, y3}, {z0, z1, z2, z3}, {w0, w1, w2, w3})
		{
		}
	};

	template<arithmetic T>
	struct basic_matrix<T, 3, 2>
	{
		SEK_MATH_MATRIX_COMMON(T, 3, 2)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1) noexcept : data({c0, c1}) {}
		constexpr basic_matrix(T x0, T y0, T x1, T y1, T x2, T y2) noexcept : basic_matrix({x0, x1, x2}, {y0, y1, y2})
		{
		}
	};
	template<arithmetic T>
	struct basic_matrix<T, 3, 3>
	{
		SEK_MATH_MATRIX_COMMON(T, 3, 3)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1) noexcept : data({c0, c1}) {}
		constexpr basic_matrix(T x0, T y0, T z0, T x1, T y1, T z1, T x2, T y2, T z2) noexcept
			: basic_matrix({x0, x1, x2}, {y0, y1, y2}, {z0, z1, z2})
		{
		}
	};
	template<arithmetic T>
	struct basic_matrix<T, 3, 4>
	{
		SEK_MATH_MATRIX_COMMON(T, 3, 4)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1) noexcept : data({c0, c1}) {}
		constexpr basic_matrix(T x0, T y0, T z0, T w0, T x1, T y1, T z1, T w1, T x2, T y2, T z2, T w2) noexcept
			: basic_matrix({x0, x1, x2}, {y0, y1, y2}, {z0, z1, z2}, {w0, w1, w2})
		{
		}
	};

	template<arithmetic T>
	struct basic_matrix<T, 2, 2>
	{
		SEK_MATH_MATRIX_COMMON(T, 2, 2)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1) noexcept : data({c0, c1}) {}
		constexpr basic_matrix(T x0, T y0, T x1, T y1) noexcept : basic_matrix({x0, x1}, {y0, y1}) {}
	};
	template<arithmetic T>
	struct basic_matrix<T, 2, 3>
	{
		SEK_MATH_MATRIX_COMMON(T, 2, 3)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1) noexcept : data({c0, c1}) {}
		constexpr basic_matrix(T x0, T y0, T z0, T x1, T y1, T z1) noexcept : basic_matrix({x0, x1}, {y0, y1}, {z0, z1})
		{
		}
	};
	template<arithmetic T>
	struct basic_matrix<T, 2, 4>
	{
		SEK_MATH_MATRIX_COMMON(T, 2, 4)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1) noexcept : data({c0, c1}) {}
		constexpr basic_matrix(T x0, T y0, T z0, T w0, T x1, T y1, T z1, T w1) noexcept
			: basic_matrix({x0, x1}, {y0, y1}, {z0, z1}, {w0, w1})
		{
		}
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
