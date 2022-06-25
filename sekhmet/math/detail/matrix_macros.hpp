/*
 * Created by switchblade on 24/06/22
 */

#pragma once

#define SEK_DETAIL_MATRIX_COMMON(T, N, M, P)                                                                           \
public:                                                                                                                \
	typedef T value_type;                                                                                              \
	typedef basic_vec<T, M, P> col_type;                                                                               \
	typedef basic_vec<T, N, P> row_type;                                                                               \
                                                                                                                       \
	/** Number of columns in the matrix. */                                                                            \
	constexpr static auto columns = N;                                                                                 \
	/** Number of rows in the matrix. */                                                                               \
	constexpr static auto rows = M;                                                                                    \
                                                                                                                       \
private:                                                                                                               \
	col_type m_data[N] = {}; /* Matrices stored as columns to optimize SIMD computation. */                         \
                                                                                                                       \
public:                                                                                                                \
	/** Initializes an identity matrix. */                                                                             \
	constexpr basic_mat() noexcept : basic_mat(1)                                                                      \
	{                                                                                                                  \
	}                                                                                                                  \
                                                                                                                       \
	/** Initializes the main diagonal of the matrix to the provided value. */                                          \
	constexpr explicit basic_mat(T v) noexcept                                                                         \
	{                                                                                                                  \
		for (std::size_t i = 0; i < N && i < M; ++i) col(i)[i] = v;                                                    \
	}                                                                                                                  \
                                                                                                                       \
	constexpr explicit basic_mat(const col_type(&cols)[N]) noexcept                                                    \
	{                                                                                                                  \
		std::copy_n(cols, N, m_data);                                                                               \
	}                                                                                                                  \
                                                                                                                       \
	/** Returns the corresponding column of the matrix. */                                                             \
	[[nodiscard]] constexpr col_type &operator[](std::size_t i) noexcept                                               \
	{                                                                                                                  \
		return m_data[i];                                                                                           \
	}                                                                                                                  \
	/** @copydoc operator[] */                                                                                         \
	[[nodiscard]] constexpr const col_type &operator[](std::size_t i) const noexcept                                   \
	{                                                                                                                  \
		return m_data[i];                                                                                           \
	}                                                                                                                  \
	/** @copydoc operator[] */                                                                                         \
	[[nodiscard]] constexpr col_type &col(std::size_t i) noexcept                                                      \
	{                                                                                                                  \
		return m_data[i];                                                                                           \
	}                                                                                                                  \
	/** @copydoc operator[] */                                                                                         \
	[[nodiscard]] constexpr const col_type &col(std::size_t i) const noexcept                                          \
	{                                                                                                                  \
		return m_data[i];                                                                                           \
	}                                                                                                                  \
                                                                                                                       \
private:                                                                                                               \
	template<std::size_t... Is>                                                                                        \
	[[nodiscard]] constexpr row_type row(std::index_sequence<Is...>, std::size_t i) const noexcept                     \
	{                                                                                                                  \
		return row_type{m_data[Is][i]...};                                                                          \
	}                                                                                                                  \
                                                                                                                       \
public:                                                                                                                \
	/** Returns copy of the corresponding row of the matrix. */                                                        \
	[[nodiscard]] constexpr row_type row(std::size_t i) const noexcept                                                 \
	{                                                                                                                  \
		return row(std::make_index_sequence<columns>{}, i);                                                            \
	}                                                                                                                  \
                                                                                                                       \
	[[nodiscard]] constexpr bool operator==(const basic_mat &) const noexcept = default;                               \
                                                                                                                       \
	constexpr void swap(const basic_mat &other) noexcept                                                               \
	{                                                                                                                  \
		std::swap(m_data, other.m_data);                                                                         \
	}
