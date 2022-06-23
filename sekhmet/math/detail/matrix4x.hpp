/*
 * Created by switchblade on 02/05/22
 */

#pragma once

#include "matrix.hpp"

namespace sek::math
{
	template<arithmetic T, storage_policy P>
	class basic_mat<T, 4, 2, P>
	{
		SEK_MATH_MATRIX_COMMON(T, 4, 2, P)

	public:
		constexpr basic_mat(const col_type &c0, const col_type &c1, const col_type &c2, const col_type &c3) noexcept
			: m_data{c0, c1, c2, c3}
		{
		}
		template<storage_policy Sp>
		constexpr basic_mat(const basic_vec<T, 2, Sp> &c0,
							   const basic_vec<T, 2, Sp> &c1,
							   const basic_vec<T, 2, Sp> &c2,
							   const basic_vec<T, 2, Sp> &c3) noexcept
			: m_data{col_type{c0}, col_type{c1}, col_type{c2}, col_type{c3}}
		{
		}
		constexpr basic_mat(T xx, T yx, T zx, T wx, T xy, T yy, T zy, T wy) noexcept
			: basic_mat({xx, xy}, {yx, yy}, {zx, zy}, {wx, wy})
		{
		}

		template<storage_policy Sp>
		constexpr explicit basic_mat(const basic_mat<T, 2, 2, Sp> &other) noexcept
			: basic_mat{other[0], other[1], {}, {}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_mat(const basic_mat<T, 2, 3, Sp> &other) noexcept
			: basic_mat{other[0], other[1], {}, {}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_mat(const basic_mat<T, 2, 4, Sp> &other) noexcept
			: basic_mat{other[0], other[1], {}, {}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_mat(const basic_mat<T, 3, 2, Sp> &other) noexcept
			: basic_mat{other[0], other[1], other[2], {}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_mat(const basic_mat<T, 3, 3, Sp> &other) noexcept
			: basic_mat{other[0], other[1], other[2], {}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_mat(const basic_mat<T, 3, 4, Sp> &other) noexcept
			: basic_mat{other[0], other[1], other[2], {}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_mat(const basic_mat<T, 4, 3, Sp> &other) noexcept
			: basic_mat{other[0], other[1], other[2], other[3]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_mat(const basic_mat<T, 4, 4, Sp> &other) noexcept
			: basic_mat{other[0], other[1], other[2], other[3]}
		{
		}
	};

	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_mat<T, 2, 2, P> operator*(const basic_mat<T, 4, 2, P> &l,
															   const basic_mat<T, 2, 4, P> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);

		// clang-format off
		return basic_mat<T, 2, 2, P>{dot(r0, c0), dot(r1, c0),
										dot(r0, c1), dot(r1, c1)};
		// clang-format on
	}
	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_mat<T, 3, 2, P> operator*(const basic_mat<T, 4, 2, P> &l,
															   const basic_mat<T, 3, 4, P> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);
		const auto &c2 = r.col(2);

		// clang-format off
		return basic_mat<T, 3, 2, P>{dot(r0, c0), dot(r1, c0),
										dot(r0, c1), dot(r1, c1),
										dot(r0, c2), dot(r1, c2)};
		// clang-format on
	}
	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_mat<T, 4, 2, P> operator*(const basic_mat<T, 4, 2, P> &l,
															   const basic_mat<T, 4, 4, P> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);
		const auto &c2 = r.col(2);
		const auto &c3 = r.col(3);

		// clang-format off
		return basic_mat<T, 4, 2, P>{dot(r0, c0), dot(r1, c0),
										dot(r0, c1), dot(r1, c1),
										dot(r0, c2), dot(r1, c2),
										dot(r0, c3), dot(r1, c3)};
		// clang-format on
	}

	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_vec<T, 2, P> operator*(const basic_mat<T, 4, 2, P> &m,
															const basic_vec<T, 4, P> &v) noexcept
	{
		return m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3] * v[3];
	}
	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_vec<T, 4, P> operator*(const basic_vec<T, 2, P> &v,
															const basic_mat<T, 4, 2, P> &m) noexcept
	{
		return basic_vec<T, 4, P>{dot(v, m[0]), dot(v, m[1]), dot(v, m[2]), dot(v, m[3])};
	}

	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_mat<T, 2, 4, Sp> transpose(const basic_mat<T, 4, 2, Sp> &m) noexcept
	{
		const auto r0 = m.row(0);
		const auto r1 = m.row(1);
		return basic_mat<T, 2, 4, Sp>{r0, r1};
	}

	template<arithmetic T, storage_policy P>
	class basic_mat<T, 4, 3, P>
	{
		SEK_MATH_MATRIX_COMMON(T, 4, 3, P)

	public:
		constexpr basic_mat(const col_type &c0, const col_type &c1, const col_type &c2, const col_type &c3) noexcept
			: m_data{c0, c1, c2, c3}
		{
		}
		template<storage_policy Sp>
		constexpr basic_mat(const basic_vec<T, 3, Sp> &c0,
							   const basic_vec<T, 3, Sp> &c1,
							   const basic_vec<T, 3, Sp> &c2,
							   const basic_vec<T, 3, Sp> &c3) noexcept
			: m_data{col_type{c0}, col_type{c1}, col_type{c2}, col_type{c3}}
		{
		}
		constexpr basic_mat(T xx, T yx, T zx, T wx, T xy, T yy, T zy, T wy, T xz, T yz, T zz, T wz) noexcept
			: basic_mat({xx, xy, xz}, {yx, yy, yz}, {zx, zy, zz}, {wx, wy, wz})
		{
		}

		template<storage_policy Sp>
		constexpr explicit basic_mat(const basic_mat<T, 2, 2, Sp> &other) noexcept
			: basic_mat{other[0], other[1], {0, 0, 1}, {}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_mat(const basic_mat<T, 2, 3, Sp> &other) noexcept
			: basic_mat{other[0], other[1], {0, 0, 1}, {}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_mat(const basic_mat<T, 2, 4, Sp> &other) noexcept
			: basic_mat{other[0], other[1], {0, 0, 1}, {}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_mat(const basic_mat<T, 3, 2, Sp> &other) noexcept
			: basic_mat{other[0], other[1], {other[2][0], other[2][1], 1}, {}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_mat(const basic_mat<T, 3, 3, Sp> &other) noexcept
			: basic_mat{other[0], other[1], other[2], {}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_mat(const basic_mat<T, 3, 4, Sp> &other) noexcept
			: basic_mat{other[0], other[1], other[2], {}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_mat(const basic_mat<T, 4, 2, Sp> &other) noexcept
			: basic_mat{other[0], other[1], {other[2][0], other[2][1], 1}, other[3]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_mat(const basic_mat<T, 4, 4, Sp> &other) noexcept
			: basic_mat{other[0], other[1], other[2], other[3]}
		{
		}
	};

	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_mat<T, 2, 3, P> operator*(const basic_mat<T, 4, 3, P> &l,
															   const basic_mat<T, 2, 4, P> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto r2 = l.row(2);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);

		// clang-format off
		return basic_mat<T, 2, 3, P>{dot(r0, c0), dot(r1, c0), dot(r2, c0),
										dot(r0, c1), dot(r1, c1), dot(r2, c1)};
		// clang-format on
	}
	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_mat<T, 3, 3, P> operator*(const basic_mat<T, 4, 3, P> &l,
															   const basic_mat<T, 3, 4, P> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto r2 = l.row(2);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);
		const auto &c2 = r.col(2);

		// clang-format off
		return basic_mat<T, 3, 3, P>{dot(r0, c0), dot(r1, c0), dot(r2, c0),
										dot(r0, c1), dot(r1, c1), dot(r2, c1),
										dot(r0, c2), dot(r1, c2), dot(r2, c2)};
		// clang-format on
	}
	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_mat<T, 4, 3, P> operator*(const basic_mat<T, 4, 3, P> &l,
															   const basic_mat<T, 4, 4, P> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto r2 = l.row(2);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);
		const auto &c2 = r.col(2);
		const auto &c3 = r.col(3);

		// clang-format off
		return basic_mat<T, 4, 3, P>{dot(r0, c0), dot(r1, c0), dot(r2, c0),
										dot(r0, c1), dot(r1, c1), dot(r2, c1),
										dot(r0, c2), dot(r1, c2), dot(r2, c2),
										dot(r0, c3), dot(r1, c3), dot(r2, c3)};
		// clang-format on
	}

	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_vec<T, 3, P> operator*(const basic_mat<T, 4, 3, P> &m,
															const basic_vec<T, 4, P> &v) noexcept
	{
		return m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3] * v[3];
	}
	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_vec<T, 4, P> operator*(const basic_vec<T, 3, P> &v,
															const basic_mat<T, 4, 3, P> &m) noexcept
	{
		return basic_vec<T, 4, P>{dot(v, m[0]), dot(v, m[1]), dot(v, m[2]), dot(v, m[3])};
	}

	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_mat<T, 3, 4, Sp> transpose(const basic_mat<T, 4, 3, Sp> &m) noexcept
	{
		const auto r0 = m.row(0);
		const auto r1 = m.row(1);
		const auto r2 = m.row(2);
		return basic_mat<T, 3, 4, Sp>{r0, r1, r2};
	}

	template<arithmetic T, storage_policy P>
	class basic_mat<T, 4, 4, P>
	{
		SEK_MATH_MATRIX_COMMON(T, 4, 4, P)

	public:
		constexpr basic_mat(const col_type &c0, const col_type &c1, const col_type &c2, const col_type &c3) noexcept
			: m_data{c0, c1, c2, c3}
		{
		}
		template<storage_policy Sp>
		constexpr basic_mat(const basic_vec<T, 4, Sp> &c0,
							   const basic_vec<T, 4, Sp> &c1,
							   const basic_vec<T, 4, Sp> &c2,
							   const basic_vec<T, 4, Sp> &c3) noexcept
			: m_data{col_type{c0}, col_type{c1}, col_type{c2}, col_type{c3}}
		{
		}
		constexpr basic_mat(T xx, T yx, T zx, T wx, T xy, T yy, T zy, T wy, T xz, T yz, T zz, T wz, T xw, T yw, T zw, T ww) noexcept
			: basic_mat({xx, xy, xz, xw}, {yx, yy, yz, yw}, {zx, zy, zz, zw}, {wx, wy, wz, ww})
		{
		}

		template<storage_policy Sp>
		constexpr explicit basic_mat(const basic_mat<T, 2, 2, Sp> &other) noexcept
			: basic_mat{other[0], other[1], {0, 0, 1}, {0, 0, 0, 1}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_mat(const basic_mat<T, 2, 3, Sp> &other) noexcept
			: basic_mat{other[0], other[1], {0, 0, 1}, {0, 0, 0, 1}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_mat(const basic_mat<T, 2, 4, Sp> &other) noexcept
			: basic_mat{other[0], other[1], {0, 0, 1}, {0, 0, 0, 1}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_mat(const basic_mat<T, 3, 2, Sp> &other) noexcept
			: basic_mat{other[0], other[1], {other[2][0], other[2][1], 1}, {0, 0, 0, 1}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_mat(const basic_mat<T, 3, 3, Sp> &other) noexcept
			: basic_mat{other[0], other[1], other[2], {0, 0, 0, 1}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_mat(const basic_mat<T, 3, 4, Sp> &other) noexcept
			: basic_mat{other[0], other[1], other[2], {0, 0, 0, 1}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_mat(const basic_mat<T, 4, 2, Sp> &other) noexcept
			: basic_mat{other[0], other[1], {other[2][0], other[2][1], 1}, {other[3][0], other[3][1], 0, 1}}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_mat(const basic_mat<T, 4, 3, Sp> &other) noexcept
			: basic_mat{other[0], other[1], other[2], other[3]}
		{
		}
	};

	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_mat<T, 2, 4, P> operator*(const basic_mat<T, 4, 4, P> &l,
															   const basic_mat<T, 2, 4, P> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto r2 = l.row(2);
		const auto r3 = l.row(3);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);

		// clang-format off
		return basic_mat<T, 2, 4, P>{dot(r0, c0), dot(r1, c0), dot(r2, c0), dot(r3, c0),
										dot(r0, c1), dot(r1, c1), dot(r2, c1), dot(r3, c1)};
		// clang-format on
	}
	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_mat<T, 3, 4, P> operator*(const basic_mat<T, 4, 4, P> &l,
															   const basic_mat<T, 3, 4, P> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto r2 = l.row(2);
		const auto r3 = l.row(3);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);
		const auto &c2 = r.col(2);

		// clang-format off
		return basic_mat<T, 3, 4, P>{dot(r0, c0), dot(r1, c0), dot(r2, c0), dot(r3, c0),
										dot(r0, c1), dot(r1, c1), dot(r2, c1), dot(r3, c1),
										dot(r0, c2), dot(r1, c2), dot(r2, c2), dot(r3, c2)};
		// clang-format on
	}
	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_mat<T, 4, 4, P> operator*(const basic_mat<T, 4, 4, P> &l,
															   const basic_mat<T, 4, 4, P> &r) noexcept
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
		return basic_mat<T, 4, 4, P>{dot(r0, c0), dot(r1, c0), dot(r2, c0), dot(r3, c0),
										dot(r0, c1), dot(r1, c1), dot(r2, c1), dot(r3, c1),
										dot(r0, c2), dot(r1, c2), dot(r2, c2), dot(r3, c2),
										dot(r0, c3), dot(r1, c3), dot(r2, c3), dot(r3, c3)};
		// clang-format on
	}

	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_vec<T, 4, P> operator*(const basic_mat<T, 4, 4, P> &m,
															const basic_vec<T, 4, P> &v) noexcept
	{
		return m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3] * v[3];
	}
	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_vec<T, 4, P> operator*(const basic_vec<T, 4, P> &v,
															const basic_mat<T, 4, 4, P> &m) noexcept
	{
		return basic_vec<T, 4, P>{dot(v, m[0]), dot(v, m[1]), dot(v, m[2]), dot(v, m[3])};
	}

	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_mat<T, 4, 4, Sp> transpose(const basic_mat<T, 4, 4, Sp> &m) noexcept
	{
		const auto r0 = m.row(0);
		const auto r1 = m.row(1);
		const auto r2 = m.row(2);
		const auto r3 = m.row(3);
		return basic_mat<T, 4, 4, Sp>{r0, r1, r2, r3};
	}
}	 // namespace sek::math