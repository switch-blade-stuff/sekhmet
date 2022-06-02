/*
 * ============================================================================
 * Sekhmet - C++20 game engine & editor
 * Copyright (C) 2022 switchblade
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 * ============================================================================
 *
 * Created by switchblade on 02/05/22
 */

#pragma once

#include "matrix.hpp"

namespace sek::math
{
	template<arithmetic T, storage_policy P>
	struct basic_matrix<T, 3, 2, P>
	{
		SEK_MATH_MATRIX_COMMON(T, 3, 2, P)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1, const col_type &c2) noexcept : data{c0, c1, c2}
		{
		}
		template<storage_policy Sp>
		constexpr basic_matrix(const basic_vector<T, 2, Sp> &c0,
							   const basic_vector<T, 2, Sp> &c1,
							   const basic_vector<T, 2, Sp> &c2) noexcept
			: data{col_type{c0}, col_type{c1}, col_type{c2}}
		{
		}
		constexpr basic_matrix(T xx, T yx, T zx, T xy, T yy, T zy) noexcept : basic_matrix({xx, xy}, {yx, yy}, {zx, zy})
		{
		}

		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 2, 2, Sp> &other) noexcept
			: basic_matrix{other[0], other[1], {}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 2, 3, Sp> &other) noexcept
			: basic_matrix{other[0], other[1], {}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 2, 4, Sp> &other) noexcept
			: basic_matrix{other[0], other[1], {}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 3, 3, Sp> &other) noexcept
			: basic_matrix{other[0], other[1], other[2]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 3, 4, Sp> &other) noexcept
			: basic_matrix{other[0], other[1], other[2]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 4, 2, Sp> &other) noexcept
			: basic_matrix{other[0], other[1], other[2]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 4, 3, Sp> &other) noexcept
			: basic_matrix{other[0], other[1], other[2]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 4, 4, Sp> &other) noexcept
			: basic_matrix{other[0], other[1], other[2]}
		{
		}
	};

	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_matrix<T, 2, 2, P> operator*(const basic_matrix<T, 3, 2, P> &l,
															   const basic_matrix<T, 2, 3, P> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);

		// clang-format off
		return basic_matrix<T, 2, 2, P>{{dot(r0, c0), dot(r1, c0)},
										{dot(r0, c1), dot(r1, c1)}};
		// clang-format on
	}
	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_matrix<T, 3, 2, P> operator*(const basic_matrix<T, 3, 2, P> &l,
															   const basic_matrix<T, 3, 3, P> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);
		const auto &c2 = r.col(2);

		// clang-format off
		return basic_matrix<T, 3, 2, P>{{dot(r0, c0), dot(r1, c0)},
										{dot(r0, c1), dot(r1, c1)},
										{dot(r0, c2), dot(r1, c2)}};
		// clang-format on
	}
	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_matrix<T, 4, 2, P> operator*(const basic_matrix<T, 3, 2, P> &l,
															   const basic_matrix<T, 4, 3, P> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);
		const auto &c2 = r.col(2);
		const auto &c3 = r.col(3);

		// clang-format off
		return basic_matrix<T, 4, 2, P>{{dot(r0, c0), dot(r1, c0)},
										{dot(r0, c1), dot(r1, c1)},
										{dot(r0, c2), dot(r1, c2)},
										{dot(r0, c3), dot(r1, c3)}};
		// clang-format on
	}

	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_vector<T, 2, P> operator*(const basic_matrix<T, 3, 2, P> &m,
															const basic_vector<T, 3, P> &v) noexcept
	{
		return m[0] * v[0] + m[1] * v[1] + m[2] * v[2];
	}
	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_vector<T, 3, P> operator*(const basic_vector<T, 2, P> &v,
															const basic_matrix<T, 3, 2, P> &m) noexcept
	{
		return basic_vector<T, 3, P>{dot(v, m[0]), dot(v, m[1]), dot(v, m[2])};
	}

	template<arithmetic T, storage_policy P>
	struct basic_matrix<T, 3, 3, P>
	{
		SEK_MATH_MATRIX_COMMON(T, 3, 3, P)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1, const col_type &c2) noexcept : data{c0, c1, c2}
		{
		}
		template<storage_policy Sp>
		constexpr basic_matrix(const basic_vector<T, 3, Sp> &c0,
							   const basic_vector<T, 3, Sp> &c1,
							   const basic_vector<T, 3, Sp> &c2) noexcept
			: data{col_type{c0}, col_type{c1}, col_type{c2}}
		{
		}
		constexpr basic_matrix(T xx, T yx, T zx, T xy, T yy, T zy, T xz, T yz, T zz) noexcept
			: basic_matrix({xx, xy, xz}, {yx, yy, yz}, {zx, zy, zz})
		{
		}

		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 2, 2, Sp> &other) noexcept
			: basic_matrix{other[0], other[1], {0, 0, 1}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 2, 3, Sp> &other) noexcept
			: basic_matrix{other[0], other[1], {0, 0, 1}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 2, 4, Sp> &other) noexcept
			: basic_matrix{other[0], other[1], {0, 0, 1}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 3, 2, Sp> &other) noexcept
			: basic_matrix{other[0], other[1], {other[2][0], other[2][1], 1}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 3, 4, Sp> &other) noexcept
			: basic_matrix{other[0], other[1], other[2]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 4, 2, Sp> &other) noexcept
			: basic_matrix{other[0], other[1], {other[2][0], other[2][1], 1}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 4, 3, Sp> &other) noexcept
			: basic_matrix{other[0], other[1], other[2]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 4, 4, Sp> &other) noexcept
			: basic_matrix{other[0], other[1], other[2]}
		{
		}
	};

	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_matrix<T, 2, 3, P> operator*(const basic_matrix<T, 3, 3, P> &l,
															   const basic_matrix<T, 2, 3, P> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto r2 = l.row(2);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);

		// clang-format off
		return basic_matrix<T, 2, 3, P>{{dot(r0, c0), dot(r1, c0), dot(r2, c0)},
										{dot(r0, c1), dot(r1, c1), dot(r2, c1)}};
		// clang-format on
	}
	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_matrix<T, 3, 3, P> operator*(const basic_matrix<T, 3, 3, P> &l,
															   const basic_matrix<T, 3, 3, P> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto r2 = l.row(2);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);
		const auto &c2 = r.col(2);

		// clang-format off
		return basic_matrix<T, 3, 3, P>{{dot(r0, c0), dot(r1, c0), dot(r2, c0)},
										{dot(r0, c1), dot(r1, c1), dot(r2, c1)},
										{dot(r0, c2), dot(r1, c2), dot(r2, c2)}};
		// clang-format on
	}
	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_matrix<T, 4, 3, P> operator*(const basic_matrix<T, 3, 3, P> &l,
															   const basic_matrix<T, 4, 3, P> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto r2 = l.row(2);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);
		const auto &c2 = r.col(2);
		const auto &c3 = r.col(3);

		// clang-format off
		return basic_matrix<T, 4, 3, P>{{dot(r0, c0), dot(r1, c0), dot(r2, c0)},
										{dot(r0, c1), dot(r1, c1), dot(r2, c1)},
										{dot(r0, c2), dot(r1, c2), dot(r2, c2)},
										{dot(r0, c3), dot(r1, c3), dot(r2, c3)}};
		// clang-format on
	}

	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_vector<T, 3> operator*(const basic_matrix<T, 3, 3> &m, const basic_vector<T, 3> &v) noexcept
	{
		return m[0] * v[0] + m[1] * v[1] + m[2] * v[2];
	}
	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_vector<T, 3> operator*(const basic_vector<T, 3> &v, const basic_matrix<T, 3, 3> &m) noexcept
	{
		return basic_vector<T, 3, P>{dot(v, m[0]), dot(v, m[1]), dot(v, m[2])};
	}

	template<arithmetic T, storage_policy P>
	struct basic_matrix<T, 3, 4, P>
	{
		SEK_MATH_MATRIX_COMMON(T, 3, 4, P)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1, const col_type &c2) noexcept : data{c0, c1, c2}
		{
		}
		template<storage_policy Sp>
		constexpr basic_matrix(const basic_vector<T, 4, Sp> &c0,
							   const basic_vector<T, 4, Sp> &c1,
							   const basic_vector<T, 4, Sp> &c2) noexcept
			: data{col_type{c0}, col_type{c1}, col_type{c2}}
		{
		}
		constexpr basic_matrix(T xx, T yx, T zx, T xy, T yy, T zy, T xz, T yz, T zz, T xw, T yw, T zw) noexcept
			: basic_matrix({xx, xy, xz, xw}, {yx, yy, yz, yw}, {zx, zy, zz, zw})
		{
		}

		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 2, 2, Sp> &other) noexcept
			: basic_matrix{other[0], other[1], {0, 0, 1}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 2, 3, Sp> &other) noexcept
			: basic_matrix{other[0], other[1], {0, 0, 1}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 2, 4, Sp> &other) noexcept
			: basic_matrix{other[0], other[1], {0, 0, 1}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 3, 2, Sp> &other) noexcept
			: basic_matrix{other[0], other[1], {other[2][0], other[2][1], 1}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 3, 3, Sp> &other) noexcept
			: basic_matrix{other[0], other[1], other[2]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 4, 2, Sp> &other) noexcept
			: basic_matrix{other[0], other[1], {other[2][0], other[2][1], 1}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 4, 3, Sp> &other) noexcept
			: basic_matrix{other[0], other[1], other[2]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 4, 4, Sp> &other) noexcept
			: basic_matrix{other[0], other[1], other[2]}
		{
		}
	};

	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_matrix<T, 2, 4, P> operator*(const basic_matrix<T, 3, 4, P> &l,
															   const basic_matrix<T, 2, 3, P> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto r2 = l.row(2);
		const auto r3 = l.row(3);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);

		// clang-format off
		return basic_matrix<T, 2, 4, P>{{dot(r0, c0), dot(r1, c0), dot(r2, c0), dot(r3, c0)},
										{dot(r0, c1), dot(r1, c1), dot(r2, c1), dot(r3, c1)}};
		// clang-format on
	}
	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_matrix<T, 3, 4, P> operator*(const basic_matrix<T, 3, 4, P> &l,
															   const basic_matrix<T, 3, 3, P> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto r2 = l.row(2);
		const auto r3 = l.row(3);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);
		const auto &c2 = r.col(2);

		// clang-format off
		return basic_matrix<T, 3, 4, P>{{dot(r0, c0), dot(r1, c0), dot(r2, c0), dot(r3, c0)},
										{dot(r0, c1), dot(r1, c1), dot(r2, c1), dot(r3, c1)},
										{dot(r0, c2), dot(r1, c2), dot(r2, c2), dot(r3, c2)}};
		// clang-format on
	}
	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_matrix<T, 4, 4, P> operator*(const basic_matrix<T, 3, 4, P> &l,
															   const basic_matrix<T, 4, 3, P> &r) noexcept
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
		return basic_matrix<T, 4, 4, P>{{dot(r0, c0), dot(r1, c0), dot(r2, c0), dot(r3, c0)},
										{dot(r0, c1), dot(r1, c1), dot(r2, c1), dot(r3, c1)},
										{dot(r0, c2), dot(r1, c2), dot(r2, c2), dot(r3, c2)},
										{dot(r0, c3), dot(r1, c3), dot(r2, c3), dot(r3, c3)}};
		// clang-format on
	}

	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_vector<T, 4, P> operator*(const basic_matrix<T, 3, 4, P> &m,
															const basic_vector<T, 3, P> &v) noexcept
	{
		return m[0] * v[0] + m[1] * v[1] + m[2] * v[2];
	}
	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_vector<T, 3, P> operator*(const basic_vector<T, 4, P> &v,
															const basic_matrix<T, 3, 4, P> &m) noexcept
	{
		return basic_vector<T, 3, P>{dot(v, m[0]), dot(v, m[1]), dot(v, m[2])};
	}
}	 // namespace sek::math