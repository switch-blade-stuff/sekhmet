/*
 * Created by switchblade on 02/05/22
 */

#pragma once

#include "matrix.hpp"

namespace sek
{
	template<arithmetic T, policy_t P>
	class basic_mat<T, 3, 2, P>
	{
		SEK_DETAIL_MATRIX_COMMON(T, 3, 2, P)

	public:
		constexpr basic_mat(const col_type &c0, const col_type &c1, const col_type &c2) noexcept : m_data{c0, c1, c2} {}
		template<policy_t Q>
		constexpr basic_mat(const basic_vec<T, 2, Q> &c0, const basic_vec<T, 2, Q> &c1, const basic_vec<T, 2, Q> &c2) noexcept
			: m_data{col_type{c0}, col_type{c1}, col_type{c2}}
		{
		}
		constexpr basic_mat(T xx, T yx, T zx, T xy, T yy, T zy) noexcept : basic_mat({xx, xy}, {yx, yy}, {zx, zy}) {}

		template<policy_t Q>
		constexpr explicit basic_mat(const basic_mat<T, 2, 2, Q> &other) noexcept : basic_mat{other[0], other[1], {}}
		{
		}
		template<policy_t Q>
		constexpr explicit basic_mat(const basic_mat<T, 2, 3, Q> &other) noexcept : basic_mat{other[0], other[1], {}}
		{
		}
		template<policy_t Q>
		constexpr explicit basic_mat(const basic_mat<T, 2, 4, Q> &other) noexcept : basic_mat{other[0], other[1], {}}
		{
		}
		template<policy_t Q>
		constexpr explicit basic_mat(const basic_mat<T, 3, 3, Q> &other) noexcept
			: basic_mat{other[0], other[1], other[2]}
		{
		}
		template<policy_t Q>
		constexpr explicit basic_mat(const basic_mat<T, 3, 4, Q> &other) noexcept
			: basic_mat{other[0], other[1], other[2]}
		{
		}
		template<policy_t Q>
		constexpr explicit basic_mat(const basic_mat<T, 4, 2, Q> &other) noexcept
			: basic_mat{other[0], other[1], other[2]}
		{
		}
		template<policy_t Q>
		constexpr explicit basic_mat(const basic_mat<T, 4, 3, Q> &other) noexcept
			: basic_mat{other[0], other[1], other[2]}
		{
		}
		template<policy_t Q>
		constexpr explicit basic_mat(const basic_mat<T, 4, 4, Q> &other) noexcept
			: basic_mat{other[0], other[1], other[2]}
		{
		}
	};

	template<typename T, policy_t P>
	[[nodiscard]] constexpr basic_mat<T, 2, 2, P> operator*(const basic_mat<T, 3, 2, P> &l, const basic_mat<T, 2, 3, P> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);

		// clang-format off
		return basic_mat<T, 2, 2, P>{{dot(r0, c0), dot(r1, c0)},
									 {dot(r0, c1), dot(r1, c1)}};
		// clang-format on
	}
	template<typename T, policy_t P>
	[[nodiscard]] constexpr basic_mat<T, 3, 2, P> operator*(const basic_mat<T, 3, 2, P> &l, const basic_mat<T, 3, 3, P> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);
		const auto &c2 = r.col(2);

		// clang-format off
		return basic_mat<T, 3, 2, P>{{dot(r0, c0), dot(r1, c0)},
									 {dot(r0, c1), dot(r1, c1)},
									 {dot(r0, c2), dot(r1, c2)}};
		// clang-format on
	}
	template<typename T, policy_t P>
	[[nodiscard]] constexpr basic_mat<T, 4, 2, P> operator*(const basic_mat<T, 3, 2, P> &l, const basic_mat<T, 4, 3, P> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);
		const auto &c2 = r.col(2);
		const auto &c3 = r.col(3);

		// clang-format off
		return basic_mat<T, 4, 2, P>{{dot(r0, c0), dot(r1, c0)},
									 {dot(r0, c1), dot(r1, c1)},
									 {dot(r0, c2), dot(r1, c2)},
									 {dot(r0, c3), dot(r1, c3)}};
		// clang-format on
	}

	template<typename T, policy_t P>
	[[nodiscard]] constexpr basic_vec<T, 2, P> operator*(const basic_mat<T, 3, 2, P> &m, const basic_vec<T, 3, P> &v) noexcept
	{
		return m[0] * v[0] + m[1] * v[1] + m[2] * v[2];
	}
	template<typename T, policy_t P>
	[[nodiscard]] constexpr basic_vec<T, 3, P> operator*(const basic_vec<T, 2, P> &v, const basic_mat<T, 3, 2, P> &m) noexcept
	{
		return basic_vec<T, 3, P>{dot(v, m[0]), dot(v, m[1]), dot(v, m[2])};
	}

	template<typename T, policy_t Q>
	[[nodiscard]] constexpr basic_mat<T, 2, 3, Q> transpose(const basic_mat<T, 3, 2, Q> &m) noexcept
	{
		const auto r0 = m.row(0);
		const auto r1 = m.row(1);
		return basic_mat<T, 2, 3, Q>{r0, r1};
	}

	template<arithmetic T, policy_t P>
	class basic_mat<T, 3, 3, P>
	{
		SEK_DETAIL_MATRIX_COMMON(T, 3, 3, P)

	public:
		constexpr basic_mat(const col_type &c0, const col_type &c1, const col_type &c2) noexcept : m_data{c0, c1, c2} {}
		template<policy_t Q>
		constexpr basic_mat(const basic_vec<T, 3, Q> &c0, const basic_vec<T, 3, Q> &c1, const basic_vec<T, 3, Q> &c2) noexcept
			: m_data{col_type{c0}, col_type{c1}, col_type{c2}}
		{
		}
		constexpr basic_mat(T xx, T yx, T zx, T xy, T yy, T zy, T xz, T yz, T zz) noexcept
			: basic_mat({xx, xy, xz}, {yx, yy, yz}, {zx, zy, zz})
		{
		}

		template<policy_t Q>
		constexpr explicit basic_mat(const basic_mat<T, 2, 2, Q> &other) noexcept
			: basic_mat{other[0], other[1], {0, 0, 1}}
		{
		}
		template<policy_t Q>
		constexpr explicit basic_mat(const basic_mat<T, 2, 3, Q> &other) noexcept
			: basic_mat{other[0], other[1], {0, 0, 1}}
		{
		}
		template<policy_t Q>
		constexpr explicit basic_mat(const basic_mat<T, 2, 4, Q> &other) noexcept
			: basic_mat{other[0], other[1], {0, 0, 1}}
		{
		}
		template<policy_t Q>
		constexpr explicit basic_mat(const basic_mat<T, 3, 2, Q> &other) noexcept
			: basic_mat{other[0], other[1], {other[2][0], other[2][1], 1}}
		{
		}
		template<policy_t Q>
		constexpr explicit basic_mat(const basic_mat<T, 3, 4, Q> &other) noexcept
			: basic_mat{other[0], other[1], other[2]}
		{
		}
		template<policy_t Q>
		constexpr explicit basic_mat(const basic_mat<T, 4, 2, Q> &other) noexcept
			: basic_mat{other[0], other[1], {other[2][0], other[2][1], 1}}
		{
		}
		template<policy_t Q>
		constexpr explicit basic_mat(const basic_mat<T, 4, 3, Q> &other) noexcept
			: basic_mat{other[0], other[1], other[2]}
		{
		}
		template<policy_t Q>
		constexpr explicit basic_mat(const basic_mat<T, 4, 4, Q> &other) noexcept
			: basic_mat{other[0], other[1], other[2]}
		{
		}
	};

	template<typename T, policy_t P>
	[[nodiscard]] constexpr basic_mat<T, 2, 3, P> operator*(const basic_mat<T, 3, 3, P> &l, const basic_mat<T, 2, 3, P> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto r2 = l.row(2);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);

		// clang-format off
		return basic_mat<T, 2, 3, P>{{dot(r0, c0), dot(r1, c0), dot(r2, c0)},
									 {dot(r0, c1), dot(r1, c1), dot(r2, c1)}};
		// clang-format on
	}
	template<typename T, policy_t P>
	[[nodiscard]] constexpr basic_mat<T, 3, 3, P> operator*(const basic_mat<T, 3, 3, P> &l, const basic_mat<T, 3, 3, P> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto r2 = l.row(2);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);
		const auto &c2 = r.col(2);

		// clang-format off
		return basic_mat<T, 3, 3, P>{{dot(r0, c0), dot(r1, c0), dot(r2, c0)},
									 {dot(r0, c1), dot(r1, c1), dot(r2, c1)},
									 {dot(r0, c2), dot(r1, c2), dot(r2, c2)}};
		// clang-format on
	}
	template<typename T, policy_t P>
	[[nodiscard]] constexpr basic_mat<T, 4, 3, P> operator*(const basic_mat<T, 3, 3, P> &l, const basic_mat<T, 4, 3, P> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto r2 = l.row(2);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);
		const auto &c2 = r.col(2);
		const auto &c3 = r.col(3);

		// clang-format off
		return basic_mat<T, 4, 3, P>{{dot(r0, c0), dot(r1, c0), dot(r2, c0)},
									 {dot(r0, c1), dot(r1, c1), dot(r2, c1)},
									 {dot(r0, c2), dot(r1, c2), dot(r2, c2)},
									 {dot(r0, c3), dot(r1, c3), dot(r2, c3)}};
		// clang-format on
	}

	template<typename T, policy_t P>
	[[nodiscard]] constexpr basic_vec<T, 3> operator*(const basic_mat<T, 3, 3> &m, const basic_vec<T, 3> &v) noexcept
	{
		return m[0] * v[0] + m[1] * v[1] + m[2] * v[2];
	}
	template<typename T, policy_t P>
	[[nodiscard]] constexpr basic_vec<T, 3> operator*(const basic_vec<T, 3> &v, const basic_mat<T, 3, 3> &m) noexcept
	{
		return basic_vec<T, 3, P>{dot(v, m[0]), dot(v, m[1]), dot(v, m[2])};
	}

	template<typename T, policy_t Q>
	[[nodiscard]] constexpr basic_mat<T, 3, 3, Q> transpose(const basic_mat<T, 3, 3, Q> &m) noexcept
	{
		const auto r0 = m.row(0);
		const auto r1 = m.row(1);
		const auto r2 = m.row(2);
		return basic_mat<T, 3, 3, Q>{r0, r1, r2};
	}

	template<arithmetic T, policy_t P>
	class basic_mat<T, 3, 4, P>
	{
		SEK_DETAIL_MATRIX_COMMON(T, 3, 4, P)

	public:
		constexpr basic_mat(const col_type &c0, const col_type &c1, const col_type &c2) noexcept : m_data{c0, c1, c2} {}
		template<policy_t Q>
		constexpr basic_mat(const basic_vec<T, 4, Q> &c0, const basic_vec<T, 4, Q> &c1, const basic_vec<T, 4, Q> &c2) noexcept
			: m_data{col_type{c0}, col_type{c1}, col_type{c2}}
		{
		}
		constexpr basic_mat(T xx, T yx, T zx, T xy, T yy, T zy, T xz, T yz, T zz, T xw, T yw, T zw) noexcept
			: basic_mat({xx, xy, xz, xw}, {yx, yy, yz, yw}, {zx, zy, zz, zw})
		{
		}

		template<policy_t Q>
		constexpr explicit basic_mat(const basic_mat<T, 2, 2, Q> &other) noexcept
			: basic_mat{other[0], other[1], {0, 0, 1}}
		{
		}
		template<policy_t Q>
		constexpr explicit basic_mat(const basic_mat<T, 2, 3, Q> &other) noexcept
			: basic_mat{other[0], other[1], {0, 0, 1}}
		{
		}
		template<policy_t Q>
		constexpr explicit basic_mat(const basic_mat<T, 2, 4, Q> &other) noexcept
			: basic_mat{other[0], other[1], {0, 0, 1}}
		{
		}
		template<policy_t Q>
		constexpr explicit basic_mat(const basic_mat<T, 3, 2, Q> &other) noexcept
			: basic_mat{other[0], other[1], {other[2][0], other[2][1], 1}}
		{
		}
		template<policy_t Q>
		constexpr explicit basic_mat(const basic_mat<T, 3, 3, Q> &other) noexcept
			: basic_mat{other[0], other[1], other[2]}
		{
		}
		template<policy_t Q>
		constexpr explicit basic_mat(const basic_mat<T, 4, 2, Q> &other) noexcept
			: basic_mat{other[0], other[1], {other[2][0], other[2][1], 1}}
		{
		}
		template<policy_t Q>
		constexpr explicit basic_mat(const basic_mat<T, 4, 3, Q> &other) noexcept
			: basic_mat{other[0], other[1], other[2]}
		{
		}
		template<policy_t Q>
		constexpr explicit basic_mat(const basic_mat<T, 4, 4, Q> &other) noexcept
			: basic_mat{other[0], other[1], other[2]}
		{
		}
	};

	template<typename T, policy_t P>
	[[nodiscard]] constexpr basic_mat<T, 2, 4, P> operator*(const basic_mat<T, 3, 4, P> &l, const basic_mat<T, 2, 3, P> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto r2 = l.row(2);
		const auto r3 = l.row(3);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);

		// clang-format off
		return basic_mat<T, 2, 4, P>{{dot(r0, c0), dot(r1, c0), dot(r2, c0), dot(r3, c0)},
									 {dot(r0, c1), dot(r1, c1), dot(r2, c1), dot(r3, c1)}};
		// clang-format on
	}
	template<typename T, policy_t P>
	[[nodiscard]] constexpr basic_mat<T, 3, 4, P> operator*(const basic_mat<T, 3, 4, P> &l, const basic_mat<T, 3, 3, P> &r) noexcept
	{
		const auto r0 = l.row(0);
		const auto r1 = l.row(1);
		const auto r2 = l.row(2);
		const auto r3 = l.row(3);
		const auto &c0 = r.col(0);
		const auto &c1 = r.col(1);
		const auto &c2 = r.col(2);

		// clang-format off
		return basic_mat<T, 3, 4, P>{{dot(r0, c0), dot(r1, c0), dot(r2, c0), dot(r3, c0)},
									 {dot(r0, c1), dot(r1, c1), dot(r2, c1), dot(r3, c1)},
									 {dot(r0, c2), dot(r1, c2), dot(r2, c2), dot(r3, c2)}};
		// clang-format on
	}
	template<typename T, policy_t P>
	[[nodiscard]] constexpr basic_mat<T, 4, 4, P> operator*(const basic_mat<T, 3, 4, P> &l, const basic_mat<T, 4, 3, P> &r) noexcept
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
		return basic_mat<T, 4, 4, P>{{dot(r0, c0), dot(r1, c0), dot(r2, c0), dot(r3, c0)},
									 {dot(r0, c1), dot(r1, c1), dot(r2, c1), dot(r3, c1)},
									 {dot(r0, c2), dot(r1, c2), dot(r2, c2), dot(r3, c2)},
									 {dot(r0, c3), dot(r1, c3), dot(r2, c3), dot(r3, c3)}};
		// clang-format on
	}

	template<typename T, policy_t P>
	[[nodiscard]] constexpr basic_vec<T, 4, P> operator*(const basic_mat<T, 3, 4, P> &m, const basic_vec<T, 3, P> &v) noexcept
	{
		return m[0] * v[0] + m[1] * v[1] + m[2] * v[2];
	}
	template<typename T, policy_t P>
	[[nodiscard]] constexpr basic_vec<T, 3, P> operator*(const basic_vec<T, 4, P> &v, const basic_mat<T, 3, 4, P> &m) noexcept
	{
		return basic_vec<T, 3, P>{dot(v, m[0]), dot(v, m[1]), dot(v, m[2])};
	}

	template<typename T, policy_t Q>
	[[nodiscard]] constexpr basic_mat<T, 4, 3, Q> transpose(const basic_mat<T, 3, 4, Q> &m) noexcept
	{
		const auto r0 = m.row(0);
		const auto r1 = m.row(1);
		const auto r2 = m.row(2);
		const auto r3 = m.row(3);
		return basic_mat<T, 4, 3, Q>{r0, r1, r2, r3};
	}
}	 // namespace sek