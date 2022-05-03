//
// Created by switchblade on 02/05/22.
//

#pragma once

#include "matrix.hpp"

namespace sek::math
{
	template<arithmetic T>
	struct basic_matrix<T, 3, 2>
	{
		SEK_MATH_MATRIX_COMMON(T, 3, 2)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1, const col_type &c2) noexcept : data{c0, c1, c2}
		{
		}
		constexpr basic_matrix(T xx, T yx, T zx,
							   T xy, T yy, T zy) noexcept : basic_matrix({xx, xy}, {yx, yy}, {zx, zy})
		{
		}
	};

	template<typename T>
	[[nodiscard]] constexpr basic_matrix<T, 2, 2> operator*(const basic_matrix<T, 3, 2> &l, const basic_matrix<T, 2, 3> &r) noexcept
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
	[[nodiscard]] constexpr basic_matrix<T, 3, 2> operator*(const basic_matrix<T, 3, 2> &l, const basic_matrix<T, 3, 3> &r) noexcept
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
	[[nodiscard]] constexpr basic_matrix<T, 4, 2> operator*(const basic_matrix<T, 3, 2> &l, const basic_matrix<T, 4, 3> &r) noexcept
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
	[[nodiscard]] constexpr basic_vector<T, 2> operator*(const basic_matrix<T, 3, 2> &m, const basic_vector<T, 3> &v) noexcept
	{
		return m[0] * v[0] + m[1] * v[1] + m[2] * v[2];
	}
	template<typename T>
	[[nodiscard]] constexpr basic_vector<T, 3> operator*(const basic_vector<T, 2> &v, const basic_matrix<T, 3, 2> &m) noexcept
	{
		return basic_vector<T, 2>{dot(v, m[0]), dot(v, m[1]), dot(v, m[2])};
	}

	template<arithmetic T>
	struct basic_matrix<T, 3, 3>
	{
		SEK_MATH_MATRIX_COMMON(T, 3, 3)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1, const col_type &c2) noexcept : data{c0, c1, c2}
		{
		}
		constexpr basic_matrix(T xx, T yx, T zx,
							   T xy, T yy, T zy,
							   T xz, T yz, T zz) noexcept
			: basic_matrix({xx, xy, xz}, {yx, yy, yz}, {zx, zy, zz})
		{
		}
	};

	template<typename T>
	[[nodiscard]] constexpr basic_matrix<T, 2, 3> operator*(const basic_matrix<T, 3, 3> &l, const basic_matrix<T, 2, 3> &r) noexcept
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
	[[nodiscard]] constexpr basic_matrix<T, 3, 3> operator*(const basic_matrix<T, 3, 3> &l, const basic_matrix<T, 3, 3> &r) noexcept
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
	[[nodiscard]] constexpr basic_matrix<T, 4, 3> operator*(const basic_matrix<T, 3, 3> &l, const basic_matrix<T, 4, 3> &r) noexcept
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
	[[nodiscard]] constexpr basic_vector<T, 3> operator*(const basic_matrix<T, 3, 3> &m, const basic_vector<T, 3> &v) noexcept
	{
		return m[0] * v[0] + m[1] * v[1] + m[2] * v[2];
	}
	template<typename T>
	[[nodiscard]] constexpr basic_vector<T, 3> operator*(const basic_vector<T, 3> &v, const basic_matrix<T, 3, 3> &m) noexcept
	{
		return basic_vector<T, 2>{dot(v, m[0]), dot(v, m[1]), dot(v, m[2])};
	}

	template<arithmetic T>
	struct basic_matrix<T, 3, 4>
	{
		SEK_MATH_MATRIX_COMMON(T, 3, 4)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1, const col_type &c2) noexcept : data{c0, c1, c2}
		{
		}
		constexpr basic_matrix(T xx, T yx, T zx,
							   T xy, T yy, T zy,
							   T xz, T yz, T zz,
							   T xw, T yw, T zw) noexcept
			: basic_matrix({xx, xy, xz, xw}, {yx, yy, yz, yw}, {zx, zy, zz, zw})
		{
		}
	};

	template<typename T>
	[[nodiscard]] constexpr basic_matrix<T, 2, 4> operator*(const basic_matrix<T, 3, 4> &l, const basic_matrix<T, 2, 3> &r) noexcept
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
	[[nodiscard]] constexpr basic_matrix<T, 3, 4> operator*(const basic_matrix<T, 3, 4> &l, const basic_matrix<T, 3, 3> &r) noexcept
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
	[[nodiscard]] constexpr basic_matrix<T, 4, 4> operator*(const basic_matrix<T, 3, 4> &l, const basic_matrix<T, 4, 3> &r) noexcept
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
	[[nodiscard]] constexpr basic_vector<T, 4> operator*(const basic_matrix<T, 3, 4> &m, const basic_vector<T, 3> &v) noexcept
	{
		return m[0] * v[0] + m[1] * v[1] + m[2] * v[2];
	}
	template<typename T>
	[[nodiscard]] constexpr basic_vector<T, 3> operator*(const basic_vector<T, 4> &v, const basic_matrix<T, 3, 4> &m) noexcept
	{
		return basic_vector<T, 2>{dot(v, m[0]), dot(v, m[1]), dot(v, m[2])};
	}
}	 // namespace sek::math