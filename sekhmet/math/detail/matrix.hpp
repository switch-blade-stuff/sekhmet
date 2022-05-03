//
// Created by switchblade on 02/05/22.
//

#pragma once

#include "sekhmet/detail/hash.hpp"
#include "util.hpp"
#include "vector.hpp"

/* Use a macro to avoid duplicating common definitions for matrix specializations. */
#define SEK_MATH_MATRIX_COMMON(T, N, M)                                                                                     \
public:                                                                                                                     \
	typedef T value_type;                                                                                                   \
	typedef basic_vector<T, M> col_type;                                                                                    \
	typedef basic_vector<T, N> row_type;                                                                                    \
                                                                                                                            \
	/** Number of columns in the matrix. */                                                                                 \
	constexpr static auto columns = N;                                                                                      \
	/** Number of rows in the matrix. */                                                                                    \
	constexpr static auto rows = M;                                                                                         \
                                                                                                                            \
	/** Returns the identity matrix. */                                                                                     \
	constexpr static std::enable_if_t<N == M, basic_matrix> identity() noexcept                                             \
	{                                                                                                                       \
		return basic_matrix{1};                                                                                             \
	}                                                                                                                       \
                                                                                                                            \
private:                                                                                                                    \
	col_type data[N] = {}; /* Matrices stored as columns to optimize SIMD computation. */                                   \
                                                                                                                            \
public:                                                                                                                     \
	constexpr basic_matrix() noexcept = default;                                                                            \
	/** Initializes the main diagonal of the matrix to the provided value. */                                               \
	constexpr explicit basic_matrix(T v) noexcept                                                                           \
	{                                                                                                                       \
		detail::unroll_matrix_op<min(N, M)>([](auto i, auto &d, auto v) { d[i][i] = v; }, data, v);                         \
	}                                                                                                                       \
                                                                                                                            \
	constexpr explicit basic_matrix(const col_type(&cols)[N]) noexcept                                                      \
	{                                                                                                                       \
		std::copy_n(cols, N, data);                                                                                         \
	}                                                                                                                       \
	template<typename U, std::size_t O, std::size_t P>                                                                      \
	constexpr basic_matrix(const basic_matrix<U, O, P> &other) noexcept                                                     \
		requires(std::convertible_to<U, T> && O != N && P != M)                                                             \
	{                                                                                                                       \
		detail::unroll_matrix_op<min(N, O)>([](auto i, auto &dst, const auto &src) { dst[i] = src[i]; }, data, other.data); \
	}                                                                                                                       \
                                                                                                                            \
	/** Returns the corresponding column of the matrix. */                                                                  \
	[[nodiscard]] constexpr col_type &operator[](std::size_t i) noexcept                                                    \
	{                                                                                                                       \
		return data[i];                                                                                                     \
	}                                                                                                                       \
	/** @copydoc operator[] */                                                                                              \
	[[nodiscard]] constexpr const col_type &operator[](std::size_t i) const noexcept                                        \
	{                                                                                                                       \
		return data[i];                                                                                                     \
	}                                                                                                                       \
                                                                                                                            \
	[[nodiscard]] constexpr bool operator==(const basic_matrix &) const noexcept = default;                                 \
                                                                                                                            \
	constexpr void swap(const basic_matrix &other) noexcept                                                                 \
	{                                                                                                                       \
		std::swap(data, other.data);                                                                                        \
	}

namespace sek::math
{
	namespace detail
	{
		template<std::size_t I, std::size_t N, typename F, typename... Args>
		constexpr void unroll_matrix_op(F &&f, Args &&...data) noexcept
		{
			if constexpr (I != N)
			{
				f(I, std::forward<Args>(data)...);
				unroll_matrix_op<I + 1, N>(std::forward<F>(f), std::forward<Args>(data)...);
			}
		}
		template<std::size_t N, typename F, typename... Args>
		constexpr void unroll_matrix_op(F &&f, Args &&...data) noexcept
		{
			if constexpr (N <= 4)
				unroll_matrix_op<0, N>(std::forward<F>(f), std::forward<Args>(data)...);
			else
				for (std::size_t i = 0; i < N; ++i) f(i, std::forward<Args>(data)...);
		}
	}	 // namespace detail

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
		constexpr basic_matrix(const col_type &c0, const col_type &c1) noexcept : data{c0, c1} {}
		constexpr basic_matrix(T xx, T yx, T xy, T yy, T xz, T yz, T xw, T yw) noexcept
			: basic_matrix({xx, xy, xz, xw}, {yx, yy, yz, yw})
		{
		}
	};
	template<arithmetic T>
	struct basic_matrix<T, 4, 3>
	{
		SEK_MATH_MATRIX_COMMON(T, 4, 3)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1) noexcept : data{c0, c1} {}
		constexpr basic_matrix(T xx, T yx, T zx, T xy, T yy, T zy, T xz, T yz, T zz, T xw, T yw, T zw) noexcept
			: basic_matrix({xx, xy, xz, xw}, {yx, yy, yz, yw}, {zx, zy, zz, zw})
		{
		}
	};
	template<arithmetic T>
	struct basic_matrix<T, 4, 4>
	{
		SEK_MATH_MATRIX_COMMON(T, 4, 4)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1) noexcept : data{c0, c1} {}
		constexpr basic_matrix(T xx, T yx, T zx, T wx, T xy, T yy, T zy, T wy, T xz, T yz, T zz, T wz, T xw, T yw, T zw, T ww) noexcept
			: basic_matrix({xx, xy, xz, xw}, {yx, yy, yz, yw}, {zx, zy, zz, zw}, {wx, wy, wz, ww})
		{
		}
	};

	template<arithmetic T>
	struct basic_matrix<T, 3, 2>
	{
		SEK_MATH_MATRIX_COMMON(T, 3, 2)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1) noexcept : data{c0, c1} {}
		constexpr basic_matrix(T xx, T yx, T xy, T yy, T xz, T yz) noexcept : basic_matrix({xx, xy, xz}, {yx, yy, yz})
		{
		}
	};
	template<arithmetic T>
	struct basic_matrix<T, 3, 3>
	{
		SEK_MATH_MATRIX_COMMON(T, 3, 3)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1) noexcept : data{c0, c1} {}
		constexpr basic_matrix(T xx, T yx, T zx, T xy, T yy, T zy, T xz, T yz, T zz) noexcept
			: basic_matrix({xx, xy, xz}, {yx, yy, yz}, {zx, zy, zz})
		{
		}
	};
	template<arithmetic T>
	struct basic_matrix<T, 3, 4>
	{
		SEK_MATH_MATRIX_COMMON(T, 3, 4)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1) noexcept : data{c0, c1} {}
		constexpr basic_matrix(T xx, T yx, T zx, T wx, T xy, T yy, T zy, T wy, T xz, T yz, T zz, T wz) noexcept
			: basic_matrix({xx, xy, xz}, {yx, yy, yz}, {zx, zy, zz}, {wx, wy, wz})
		{
		}
	};

	template<arithmetic T>
	struct basic_matrix<T, 2, 2>
	{
		SEK_MATH_MATRIX_COMMON(T, 2, 2)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1) noexcept : data{c0, c1} {}
		constexpr basic_matrix(T xx, T yx, T xy, T yy) noexcept : basic_matrix({xx, xy}, {yx, yy}) {}
	};
	template<arithmetic T>
	struct basic_matrix<T, 2, 3>
	{
		SEK_MATH_MATRIX_COMMON(T, 2, 3)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1) noexcept : data{c0, c1} {}
		constexpr basic_matrix(T xx, T yx, T zx, T xy, T yy, T zy) noexcept : basic_matrix({xx, xy}, {yx, yy}, {zx, zy})
		{
		}
	};
	template<arithmetic T>
	struct basic_matrix<T, 2, 4>
	{
		SEK_MATH_MATRIX_COMMON(T, 2, 4)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1) noexcept : data{c0, c1} {}
		constexpr basic_matrix(T xx, T yx, T zx, T wx, T xy, T yy, T zy, T wy) noexcept
			: basic_matrix({xx, xy}, {yx, yy}, {zx, zy}, {wx, wy})
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

	/** Returns a matrix which is the result of addition of two matrices. */
	template<typename T, std::size_t N, std::size_t M>
	[[nodiscard]] constexpr basic_matrix<T, N, M> operator+(const basic_matrix<T, N, M> &l, const basic_matrix<T, N, M> &r) noexcept
	{
		basic_matrix<T, N, M> result;
		detail::unroll_matrix_op<N>([](auto i, auto &out, const auto &l, const auto &r) { out[i] = l[i] + r[i]; }, result, l, r);
		return result;
	}
	/** Adds a matrix to a matrix. */
	template<typename T, std::size_t N, std::size_t M>
	constexpr basic_matrix<T, N, M> &operator+=(basic_matrix<T, N, M> &l, const basic_matrix<T, N, M> &r) noexcept
	{
		detail::unroll_matrix_op<N>([](auto i, auto &l, const auto &r) { l[i] += r[i]; }, l, r);
		return l;
	}
	/** Returns a matrix which is the result of subtraction of two matrices. */
	template<typename T, std::size_t N, std::size_t M>
	[[nodiscard]] constexpr basic_matrix<T, N, M> operator-(const basic_matrix<T, N, M> &l, const basic_matrix<T, N, M> &r) noexcept
	{
		basic_matrix<T, N, M> result;
		detail::unroll_matrix_op<N>([](auto i, auto &out, const auto &l, const auto &r) { out[i] = l[i] - r[i]; }, result, l, r);
		return result;
	}
	/** Subtracts a matrix from a matrix. */
	template<typename T, std::size_t N, std::size_t M>
	constexpr basic_matrix<T, N, M> &operator-=(basic_matrix<T, N, M> &l, const basic_matrix<T, N, M> &r) noexcept
	{
		detail::unroll_matrix_op<N>([](auto i, auto &l, const auto &r) { l[i] -= r[i]; }, l, r);
		return l;
	}

	/** Returns a copy of a matrix multiplied by a scalar. */
	template<typename T, std::size_t N, std::size_t M>
	[[nodiscard]] constexpr basic_matrix<T, N, M> operator*(const basic_matrix<T, N, M> &l, T r) noexcept
	{
		basic_matrix<T, N, M> result;
		detail::unroll_matrix_op<N>([](auto i, auto &out, const auto &l, auto r) { out[i] = l[i] * r; }, result, l, r);
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
		detail::unroll_matrix_op<N>([](auto i, auto &l, auto r) { l[i] *= r; }, l, r);
		return l;
	}
	/** Returns a copy of a matrix divided by a scalar. */
	template<typename T, std::size_t N, std::size_t M>
	[[nodiscard]] constexpr basic_matrix<T, N, M> operator/(const basic_matrix<T, N, M> &l, T r) noexcept
	{
		basic_matrix<T, N, M> result;
		detail::unroll_matrix_op<N>([](auto i, auto &out, const auto &l, auto r) { out[i] = l[i] / r; }, result, l, r);
		return result;
	}
	/** Returns a matrix produced by dividing a scalar by a matrix. */
	template<typename T, std::size_t N, std::size_t M>
	[[nodiscard]] constexpr basic_matrix<T, N, M> operator/(T l, const basic_matrix<T, N, M> &r) noexcept
	{
		basic_matrix<T, N, M> result;
		detail::unroll_matrix_op<N>([](auto i, auto &out, auto l, const auto &r) { out[i] = l / r[i]; }, result, l, r);
		return result;
	}
	/** Divides matrix by a scalar. */
	template<typename T, std::size_t N, std::size_t M>
	constexpr basic_matrix<T, N, M> &operator/=(basic_matrix<T, N, M> &l, T r) noexcept
	{
		detail::unroll_matrix_op<N>([](auto i, auto &l, auto r) { l[i] /= r; }, l, r);
		return l;
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
