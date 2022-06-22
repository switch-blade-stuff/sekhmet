/*
 * Created by switchblade on 02/05/22
 */

#pragma once

#include "matrix.hpp"

namespace sek::math
{
	template<arithmetic T, storage_policy P>
	struct basic_matrix<T, 2, 2, P>
	{
		SEK_MATH_MATRIX_COMMON(T, 2, 2, P)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1) noexcept : m_data{c0, c1} {}
		template<storage_policy Sp>
		constexpr basic_matrix(const basic_vector<T, 2, Sp> &c0, const basic_vector<T, 2, Sp> &c1) noexcept
			: m_data{col_type{c0}, col_type{c1}}
		{
		}
		constexpr basic_matrix(T xx, T yx, T xy, T yy) noexcept : basic_matrix({xx, xy}, {yx, yy}) {}

		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 2, 3, Sp> &other) noexcept
			: basic_matrix{other[0], other[1]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 2, 4, Sp> &other) noexcept
			: basic_matrix{other[0], other[1]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 3, 2, Sp> &other) noexcept
			: basic_matrix{other[0], other[1]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 3, 3, Sp> &other) noexcept
			: basic_matrix{other[0], other[1]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 3, 4, Sp> &other) noexcept
			: basic_matrix{other[0], other[1]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 4, 2, Sp> &other) noexcept
			: basic_matrix{other[0], other[1]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 4, 3, Sp> &other) noexcept
			: basic_matrix{other[0], other[1]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 4, 4, Sp> &other) noexcept
			: basic_matrix{other[0], other[1]}
		{
		}
	};

	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_matrix<T, 2, 2, P> operator*(const basic_matrix<T, 2, 2, P> &l,
															   const basic_matrix<T, 2, 2, P> &r) noexcept
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
	[[nodiscard]] constexpr basic_matrix<T, 3, 2, P> operator*(const basic_matrix<T, 2, 2, P> &l,
															   const basic_matrix<T, 3, 2, P> &r) noexcept
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
	[[nodiscard]] constexpr basic_matrix<T, 4, 2, P> operator*(const basic_matrix<T, 2, 2, P> &l,
															   const basic_matrix<T, 4, 2, P> &r) noexcept
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
	[[nodiscard]] constexpr basic_vector<T, 2, P> operator*(const basic_matrix<T, 2, 2, P> &m,
															const basic_vector<T, 2, P> &v) noexcept
	{
		return m[0] * v[0] + m[1] * v[1];
	}
	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_vector<T, 2, P> operator*(const basic_vector<T, 2, P> &v,
															const basic_matrix<T, 2, 2, P> &m) noexcept
	{
		return basic_vector<T, 2, P>{dot(v, m[0]), dot(v, m[1])};
	}

	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_matrix<T, 2, 2, Sp> transpose(const basic_matrix<T, 2, 2, Sp> &m) noexcept
	{
		const auto r0 = m.row(0);
		const auto r1 = m.row(1);
		return basic_matrix<T, 2, 2, Sp>{r0, r1};
	}

	template<arithmetic T, storage_policy P>
	struct basic_matrix<T, 2, 3, P>
	{
		SEK_MATH_MATRIX_COMMON(T, 2, 3, P)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1) noexcept : m_data{c0, c1} {}
		template<storage_policy Sp>
		constexpr basic_matrix(const basic_vector<T, 3, Sp> &c0, const basic_vector<T, 3, Sp> &c1) noexcept
			: m_data{col_type{c0}, col_type{c1}}
		{
		}
		constexpr basic_matrix(T xx, T yx, T xy, T yy, T xz, T yz) noexcept : basic_matrix({xx, xy, xz}, {yx, yy, yz})
		{
		}

		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 2, 2, Sp> &other) noexcept
			: basic_matrix{other[0], other[1]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 2, 4, Sp> &other) noexcept
			: basic_matrix{other[0], other[1]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 3, 2, Sp> &other) noexcept
			: basic_matrix{other[0], other[1]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 3, 3, Sp> &other) noexcept
			: basic_matrix{other[0], other[1]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 3, 4, Sp> &other) noexcept
			: basic_matrix{other[0], other[1]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 4, 2, Sp> &other) noexcept
			: basic_matrix{other[0], other[1]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 4, 3, Sp> &other) noexcept
			: basic_matrix{other[0], other[1]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 4, 4, Sp> &other) noexcept
			: basic_matrix{other[0], other[1]}
		{
		}
	};

	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_matrix<T, 2, 3, P> operator*(const basic_matrix<T, 2, 3, P> &l,
															   const basic_matrix<T, 2, 2, P> &r) noexcept
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
	[[nodiscard]] constexpr basic_matrix<T, 3, 3, P> operator*(const basic_matrix<T, 2, 3, P> &l,
															   const basic_matrix<T, 3, 2, P> &r) noexcept
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
	[[nodiscard]] constexpr basic_matrix<T, 4, 3, P> operator*(const basic_matrix<T, 2, 3, P> &l,
															   const basic_matrix<T, 4, 2, P> &r) noexcept
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
	[[nodiscard]] constexpr basic_vector<T, 3, P> operator*(const basic_matrix<T, 2, 3, P> &m,
															const basic_vector<T, 2, P> &v) noexcept
	{
		return m[0] * v[0] + m[1] * v[1];
	}
	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_vector<T, 2, P> operator*(const basic_vector<T, 3, P> &v,
															const basic_matrix<T, 2, 3, P> &m) noexcept
	{
		return basic_vector<T, 2, P>{dot(v, m[0]), dot(v, m[1])};
	}

	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_matrix<T, 3, 2, Sp> transpose(const basic_matrix<T, 2, 3, Sp> &m) noexcept
	{
		const auto r0 = m.row(0);
		const auto r1 = m.row(1);
		const auto r2 = m.row(2);
		return basic_matrix<T, 3, 2, Sp>{r0, r1, r2};
	}

	template<arithmetic T, storage_policy P>
	struct basic_matrix<T, 2, 4, P>
	{
		SEK_MATH_MATRIX_COMMON(T, 2, 4, P)

	public:
		constexpr basic_matrix(const col_type &c0, const col_type &c1) noexcept : m_data{c0, c1} {}
		template<storage_policy Sp>
		constexpr basic_matrix(const basic_vector<T, 4, Sp> &c0, const basic_vector<T, 4, Sp> &c1) noexcept
			: m_data{col_type{c0}, col_type{c1}}
		{
		}
		constexpr basic_matrix(T xx, T yx, T xy, T yy, T xz, T yz, T xw, T yw) noexcept
			: basic_matrix({xx, xy, xz, xw}, {yx, yy, yz, yw})
		{
		}

		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 2, 2, Sp> &other) noexcept
			: basic_matrix{other[0], other[1]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 2, 3, Sp> &other) noexcept
			: basic_matrix{other[0], other[1]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 3, 2, Sp> &other) noexcept
			: basic_matrix{other[0], other[1]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 3, 3, Sp> &other) noexcept
			: basic_matrix{other[0], other[1]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 3, 4, Sp> &other) noexcept
			: basic_matrix{other[0], other[1]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 4, 2, Sp> &other) noexcept
			: basic_matrix{other[0], other[1]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 4, 3, Sp> &other) noexcept
			: basic_matrix{other[0], other[1]}
		{
		}
		template<storage_policy Sp>
		constexpr explicit basic_matrix(const basic_matrix<T, 4, 4, Sp> &other) noexcept
			: basic_matrix{other[0], other[1]}
		{
		}
	};

	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_matrix<T, 2, 4, P> operator*(const basic_matrix<T, 2, 4, P> &l,
															   const basic_matrix<T, 2, 2, P> &r) noexcept
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
	[[nodiscard]] constexpr basic_matrix<T, 3, 4, P> operator*(const basic_matrix<T, 2, 4, P> &l,
															   const basic_matrix<T, 3, 2, P> &r) noexcept
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
	[[nodiscard]] constexpr basic_matrix<T, 4, 4, P> operator*(const basic_matrix<T, 2, 4, P> &l,
															   const basic_matrix<T, 4, 2, P> &r) noexcept
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
	[[nodiscard]] constexpr basic_vector<T, 4, P> operator*(const basic_matrix<T, 2, 4, P> &m,
															const basic_vector<T, 2, P> &v) noexcept
	{
		return m[0] * v[0] + m[1] * v[1];
	}
	template<typename T, storage_policy P>
	[[nodiscard]] constexpr basic_vector<T, 2, P> operator*(const basic_vector<T, 4, P> &v,
															const basic_matrix<T, 2, 4, P> &m) noexcept
	{
		return basic_vector<T, 2, P>{dot(v, m[0]), dot(v, m[1])};
	}

	template<typename T, std::size_t N, std::size_t M, storage_policy Sp>
	[[nodiscard]] constexpr basic_matrix<T, 4, 2, Sp> transpose(const basic_matrix<T, 2, 4, Sp> &m) noexcept
	{
		const auto r0 = m.row(0);
		const auto r1 = m.row(1);
		const auto r2 = m.row(2);
		const auto r3 = m.row(3);
		return basic_matrix<T, 4, 2, Sp>{r0, r1, r2, r3};
	}
}	 // namespace sek::math