//
// Created by switchblade on 02/05/22.
//

#pragma once

#include "matrix.hpp"

namespace sek::math
{
	template<arithmetic T>
	struct basic_matrix<T, 2, 2>
	{
		SEK_MATH_MATRIX_COMMON(T, 2, 2)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1) noexcept : data{c0, c1} {}
		constexpr basic_matrix(T xx, T yx, T xy, T yy) noexcept : basic_matrix({xx, xy}, {yx, yy}) {}

		constexpr explicit basic_matrix(const basic_matrix<T, 2, 3> &other) noexcept : basic_matrix{other[0], other[1]}
		{
		}
		constexpr explicit basic_matrix(const basic_matrix<T, 2, 4> &other) noexcept : basic_matrix{other[0], other[1]}
		{
		}
		constexpr explicit basic_matrix(const basic_matrix<T, 3, 2> &other) noexcept : basic_matrix{other[0], other[1]}
		{
		}
		constexpr explicit basic_matrix(const basic_matrix<T, 3, 3> &other) noexcept : basic_matrix{other[0], other[1]}
		{
		}
		constexpr explicit basic_matrix(const basic_matrix<T, 3, 4> &other) noexcept : basic_matrix{other[0], other[1]}
		{
		}
		constexpr explicit basic_matrix(const basic_matrix<T, 4, 2> &other) noexcept : basic_matrix{other[0], other[1]}
		{
		}
		constexpr explicit basic_matrix(const basic_matrix<T, 4, 3> &other) noexcept : basic_matrix{other[0], other[1]}
		{
		}
		constexpr explicit basic_matrix(const basic_matrix<T, 4, 4> &other) noexcept : basic_matrix{other[0], other[1]}
		{
		}
	};

	template<typename T>
	[[nodiscard]] constexpr basic_matrix<T, 2, 2> operator*(const basic_matrix<T, 2, 2> &l, const basic_matrix<T, 2, 2> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);

		// clang-format off
		return basic_matrix<T, 2, 2>{{dot(r0, c0), dot(r1, c0)},
									 {dot(r0, c1), dot(r1, c1)}};
		// clang-format on
	}
	template<typename T>
	[[nodiscard]] constexpr basic_matrix<T, 3, 2> operator*(const basic_matrix<T, 2, 2> &l, const basic_matrix<T, 3, 2> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);
		const auto &c2 = r.col(2);

		// clang-format off
		return basic_matrix<T, 3, 2>{{dot(r0, c0), dot(r1, c0)},
									 {dot(r0, c1), dot(r1, c1)},
									 {dot(r0, c2), dot(r1, c2)}};
		// clang-format on
	}
	template<typename T>
	[[nodiscard]] constexpr basic_matrix<T, 4, 2> operator*(const basic_matrix<T, 2, 2> &l, const basic_matrix<T, 4, 2> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);
		const auto &c2 = r.col(2);
		const auto &c3 = r.col(3);

		// clang-format off
		return basic_matrix<T, 4, 2>{{dot(r0, c0), dot(r1, c0)},
									 {dot(r0, c1), dot(r1, c1)},
									 {dot(r0, c2), dot(r1, c2)},
									 {dot(r0, c3), dot(r1, c3)}};
		// clang-format on
	}

	template<typename T>
	[[nodiscard]] constexpr basic_vector<T, 2> operator*(const basic_matrix<T, 2, 2> &m, const basic_vector<T, 2> &v) noexcept
	{
		return m[0] * v[0] + m[1] * v[1];
	}
	template<typename T>
	[[nodiscard]] constexpr basic_vector<T, 2> operator*(const basic_vector<T, 2> &v, const basic_matrix<T, 2, 2> &m) noexcept
	{
		return basic_vector<T, 2>{dot(v, m[0]), dot(v, m[1])};
	}

	template<arithmetic T>
	struct basic_matrix<T, 2, 3>
	{
		SEK_MATH_MATRIX_COMMON(T, 2, 3)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1) noexcept : data{c0, c1} {}
		constexpr basic_matrix(T xx, T yx, T xy, T yy, T xz, T yz) noexcept : basic_matrix({xx, xy, xz}, {yx, yy, yz})
		{
		}

		constexpr explicit basic_matrix(const basic_matrix<T, 2, 2> &other) noexcept : basic_matrix{other[0], other[1]}
		{
		}
		constexpr explicit basic_matrix(const basic_matrix<T, 2, 4> &other) noexcept : basic_matrix{other[0], other[1]}
		{
		}
		constexpr explicit basic_matrix(const basic_matrix<T, 3, 2> &other) noexcept : basic_matrix{other[0], other[1]}
		{
		}
		constexpr explicit basic_matrix(const basic_matrix<T, 3, 3> &other) noexcept : basic_matrix{other[0], other[1]}
		{
		}
		constexpr explicit basic_matrix(const basic_matrix<T, 3, 4> &other) noexcept : basic_matrix{other[0], other[1]}
		{
		}
		constexpr explicit basic_matrix(const basic_matrix<T, 4, 2> &other) noexcept : basic_matrix{other[0], other[1]}
		{
		}
		constexpr explicit basic_matrix(const basic_matrix<T, 4, 3> &other) noexcept : basic_matrix{other[0], other[1]}
		{
		}
		constexpr explicit basic_matrix(const basic_matrix<T, 4, 4> &other) noexcept : basic_matrix{other[0], other[1]}
		{
		}
	};

	template<typename T>
	[[nodiscard]] constexpr basic_matrix<T, 2, 3> operator*(const basic_matrix<T, 2, 3> &l, const basic_matrix<T, 2, 2> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto r2 = l.row(2);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);

		// clang-format off
		return basic_matrix<T, 2, 3>{{dot(r0, c0), dot(r1, c0), dot(r2, c0)},
									 {dot(r0, c1), dot(r1, c1), dot(r2, c1)}};
		// clang-format on
	}
	template<typename T>
	[[nodiscard]] constexpr basic_matrix<T, 3, 3> operator*(const basic_matrix<T, 2, 3> &l, const basic_matrix<T, 3, 2> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto r2 = l.row(2);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);
		const auto &c2 = r.col(2);

		// clang-format off
		return basic_matrix<T, 3, 3>{{dot(r0, c0), dot(r1, c0), dot(r2, c0)},
									 {dot(r0, c1), dot(r1, c1), dot(r2, c1)},
									 {dot(r0, c2), dot(r1, c2), dot(r2, c2)}};
		// clang-format on
	}
	template<typename T>
	[[nodiscard]] constexpr basic_matrix<T, 4, 3> operator*(const basic_matrix<T, 2, 3> &l, const basic_matrix<T, 4, 2> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto r2 = l.row(2);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);
		const auto &c2 = r.col(2);
		const auto &c3 = r.col(3);

		// clang-format off
		return basic_matrix<T, 4, 3>{{dot(r0, c0), dot(r1, c0), dot(r2, c0)},
									 {dot(r0, c1), dot(r1, c1), dot(r2, c1)},
									 {dot(r0, c2), dot(r1, c2), dot(r2, c2)},
									 {dot(r0, c3), dot(r1, c3), dot(r2, c3)}};
		// clang-format on
	}

	template<typename T>
	[[nodiscard]] constexpr basic_vector<T, 3> operator*(const basic_matrix<T, 2, 3> &m, const basic_vector<T, 2> &v) noexcept
	{
		return m[0] * v[0] + m[1] * v[1];
	}
	template<typename T>
	[[nodiscard]] constexpr basic_vector<T, 2> operator*(const basic_vector<T, 3> &v, const basic_matrix<T, 2, 3> &m) noexcept
	{
		return basic_vector<T, 2>{dot(v, m[0]), dot(v, m[1])};
	}

	template<arithmetic T>
	struct basic_matrix<T, 2, 4>
	{
		SEK_MATH_MATRIX_COMMON(T, 2, 4)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1) noexcept : data{c0, c1} {}
		constexpr basic_matrix(T xx, T yx, T xy, T yy, T xz, T yz, T xw, T yw) noexcept
			: basic_matrix({xx, xy, xz, xw}, {yx, yy, yz, yw})
		{
		}

		constexpr explicit basic_matrix(const basic_matrix<T, 2, 2> &other) noexcept : basic_matrix{other[0], other[1]}
		{
		}
		constexpr explicit basic_matrix(const basic_matrix<T, 2, 3> &other) noexcept : basic_matrix{other[0], other[1]}
		{
		}
		constexpr explicit basic_matrix(const basic_matrix<T, 3, 2> &other) noexcept : basic_matrix{other[0], other[1]}
		{
		}
		constexpr explicit basic_matrix(const basic_matrix<T, 3, 3> &other) noexcept : basic_matrix{other[0], other[1]}
		{
		}
		constexpr explicit basic_matrix(const basic_matrix<T, 3, 4> &other) noexcept : basic_matrix{other[0], other[1]}
		{
		}
		constexpr explicit basic_matrix(const basic_matrix<T, 4, 2> &other) noexcept : basic_matrix{other[0], other[1]}
		{
		}
		constexpr explicit basic_matrix(const basic_matrix<T, 4, 3> &other) noexcept : basic_matrix{other[0], other[1]}
		{
		}
		constexpr explicit basic_matrix(const basic_matrix<T, 4, 4> &other) noexcept : basic_matrix{other[0], other[1]}
		{
		}
	};

	template<typename T>
	[[nodiscard]] constexpr basic_matrix<T, 2, 4> operator*(const basic_matrix<T, 2, 4> &l, const basic_matrix<T, 2, 2> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto r2 = l.row(2);
		const auto r3 = l.row(3);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);

		// clang-format off
		return basic_matrix<T, 2, 4>{{dot(r0, c0), dot(r1, c0), dot(r2, c0), dot(r3, c0)},
									 {dot(r0, c1), dot(r1, c1), dot(r2, c1), dot(r3, c1)}};
		// clang-format on
	}
	template<typename T>
	[[nodiscard]] constexpr basic_matrix<T, 3, 4> operator*(const basic_matrix<T, 2, 4> &l, const basic_matrix<T, 3, 2> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto r2 = l.row(2);
		const auto r3 = l.row(3);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);
		const auto &c2 = r.col(2);

		// clang-format off
		return basic_matrix<T, 3, 4>{{dot(r0, c0), dot(r1, c0), dot(r2, c0), dot(r3, c0)},
									 {dot(r0, c1), dot(r1, c1), dot(r2, c1), dot(r3, c1)},
									 {dot(r0, c2), dot(r1, c2), dot(r2, c2), dot(r3, c2)}};
		// clang-format on
	}
	template<typename T>
	[[nodiscard]] constexpr basic_matrix<T, 4, 4> operator*(const basic_matrix<T, 2, 4> &l, const basic_matrix<T, 4, 2> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto r2 = l.row(2);
		const auto r3 = l.row(3);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);
		const auto &c2 = r.col(2);
		const auto &c3 = r.col(3);

		// clang-format off
		return basic_matrix<T, 4, 4>{{dot(r0, c0), dot(r1, c0), dot(r2, c0), dot(r3, c0)},
									 {dot(r0, c1), dot(r1, c1), dot(r2, c1), dot(r3, c1)},
									 {dot(r0, c2), dot(r1, c2), dot(r2, c2), dot(r3, c2)},
									 {dot(r0, c3), dot(r1, c3), dot(r2, c3), dot(r3, c3)}};
		// clang-format on
	}

	template<typename T>
	[[nodiscard]] constexpr basic_vector<T, 4> operator*(const basic_matrix<T, 2, 4> &m, const basic_vector<T, 2> &v) noexcept
	{
		return m[0] * v[0] + m[1] * v[1];
	}
	template<typename T>
	[[nodiscard]] constexpr basic_vector<T, 2> operator*(const basic_vector<T, 4> &v, const basic_matrix<T, 2, 4> &m) noexcept
	{
		return basic_vector<T, 2>{dot(v, m[0]), dot(v, m[1])};
	}
}	 // namespace sek::math