/*
 * Created by switchblade on 22/06/22
 */

#pragma once

#include "../matrix.hpp"
#include "../matrix2x.hpp"
#include "../matrix3x.hpp"
#include "../vector.hpp"
#include "fwd.hpp"

#define SEK_QUATERNION_GENERATE_SHUFFLE(...) SEK_DETAIL_SHUFFLE_4_FUNCS(__VA_ARGS__)

namespace sek::math
{
	template<std::floating_point T, policy_t Policy>
	class basic_quat
	{
	public:
		typedef T value_type;
		typedef basic_vec<T, 4, Policy> vector_type;
		typedef vec_mask<vector_type> mask_type;

		constexpr static auto policy = Policy;

		// clang-format off
		/** Converts a vector of euler angles (pitch, yaw, roll) to quaternion rotation.
		 * @note Euler angles are specified in radians. */
		template<typename U = T, std::size_t N = 3, policy_t P = Policy>
		[[nodiscard]] constexpr static basic_quat from_euler(const basic_vec<U, N, P> &v) noexcept requires(N >= 3);
		/** Converts a rotation matrix to quaternion rotation. */
		template<typename U = T, std::size_t N = 3, std::size_t M = 3, policy_t P = Policy>
		[[nodiscard]] constexpr static basic_quat from_mat(const basic_mat<U, N, M, P> &m) noexcept requires(N >= 3 && M >= 3);
		/** Creates a quaternion from a rotation andle and an axis. */
		template<typename U = T, std::size_t N = 3, policy_t P = Policy>
		[[nodiscard]] constexpr static basic_quat from_angle_axis(U angle, const basic_vec<U, N, P> &axis) noexcept requires(N >= 3);

		/** Creates a quaternion used to rotate towards a direction using left-handed rotation. */
		template<typename U = T, std::size_t N = 3, policy_t P = Policy>
		[[nodiscard]] constexpr static basic_quat look_at_l(const basic_vec<U, N, P> &dir, const basic_vec<U, N, P> &up = {0, 1, 0}) noexcept requires(N >= 3);
		/** Creates a quaternion used to rotate towards a direction using left-handed rotation. */
		template<typename U = T, std::size_t N = 3, policy_t P = Policy>
		[[nodiscard]] constexpr static basic_quat look_at_r(const basic_vec<U, N, P> &dir, const basic_vec<U, N, P> &up = {0, 1, 0}) noexcept requires(N >= 3);
		/** Creates a quaternion used to rotate towards a direction.
		 * @tparam R Whether to use right-handed rotation. */
		template<bool R = false, typename U = T, std::size_t N = 3, policy_t P = Policy>
		[[nodiscard]] constexpr static basic_quat look_at(const basic_vec<U, N, P> &dir, const basic_vec<U, N, P> &up) noexcept requires(N >= 3)
		{
			if constexpr (R)
				return look_at_r(dir, up);
			else
				return look_at_l(dir, up);
		}
		// clang-format on

	public:
		constexpr basic_quat() noexcept = default;

		// clang-format off
		template<policy_t OtherPolicy>
		constexpr explicit basic_quat(const basic_quat<T, OtherPolicy> &other) noexcept requires(OtherPolicy != policy)
			: m_data(other.m_data)
		{
		}
		template<policy_t OtherPolicy>
		constexpr explicit basic_quat(basic_quat<T, OtherPolicy> &&other) noexcept requires(OtherPolicy != policy)
			: m_data(std::move(other.m_data))
		{
		}
		// clang-format on

		constexpr basic_quat(T x, T y, T z, T w) noexcept : m_data(x, y, z, w) {}
		constexpr basic_quat(T x, T y, T z) noexcept : m_data(x, y, z) {}
		constexpr basic_quat(T x, T y) noexcept : m_data(x, y) {}
		constexpr explicit basic_quat(T x) noexcept : m_data(x) {}

		[[nodiscard]] constexpr value_type &operator[](std::size_t i) noexcept { return m_data[i]; }
		[[nodiscard]] constexpr const value_type &operator[](std::size_t i) const noexcept { return m_data[i]; }

		template<policy_t P = policy>
		constexpr basic_quat(const basic_vec<T, 4, P> &vector) noexcept : m_data(vector)
		{
		}
		template<policy_t P = policy>
		constexpr basic_quat(basic_vec<T, 4, P> &&vector) noexcept : m_data(std::move(vector))
		{
		}

		/** Casts quaternion to the underlying vector type. */
		[[nodiscard]] constexpr vector_type vector() const noexcept { return m_data; }
		/** @copydoc vector */
		[[nodiscard]] constexpr operator vector_type() const noexcept { return vector(); }

		[[nodiscard]] constexpr decltype(auto) x() noexcept { return m_data.x(); }
		[[nodiscard]] constexpr decltype(auto) x() const noexcept { return m_data.x(); }
		[[nodiscard]] constexpr decltype(auto) y() noexcept { return m_data.y(); }
		[[nodiscard]] constexpr decltype(auto) y() const noexcept { return m_data.y(); }
		[[nodiscard]] constexpr decltype(auto) z() noexcept { return m_data.z(); }
		[[nodiscard]] constexpr decltype(auto) z() const noexcept { return m_data.z(); }
		[[nodiscard]] constexpr decltype(auto) w() noexcept { return m_data.w(); }
		[[nodiscard]] constexpr decltype(auto) w() const noexcept { return m_data.w(); }

		SEK_QUATERNION_GENERATE_SHUFFLE(x, y, z, w)

		/** Returns euler pitch (x axis) of the quaternion.
		 * @note Euler angles are specified in radians. */
		[[nodiscard]] constexpr T pitch() const noexcept;
		/** Returns euler pitch (y axis) of the quaternion.
		 * @note Euler angles are specified in radians. */
		[[nodiscard]] constexpr T yaw() const noexcept;
		/** Returns euler roll (z axis) of the quaternion.
		 * @note Euler angles are specified in radians. */
		[[nodiscard]] constexpr T roll() const noexcept;

		// clang-format off
		/** Converts the quaternion to euler angles (pitch, yaw, roll).
		 * @note Euler angles are specified in radians. */
		template<std::size_t N = 3, policy_t P = Policy>
		[[nodiscard]] constexpr basic_vec<T, N, P> to_euler() const noexcept requires(N >= 3)
		{
			return basic_vec<T, N, P>(pitch(), yaw(), roll());
		}
		/** Converts the quaternion to a matrix rotation. */
		template<std::size_t N = 3, std::size_t M = 3, policy_t P = Policy>
		[[nodiscard]] constexpr basic_mat<T, N, M, P> to_mat() const noexcept requires(N >= 3 && M >= 3);

		/** Returns quaternion rotation angle. */
		[[nodiscard]] constexpr T angle() const noexcept;
		/** Returns quaternion rotation axis. */
		template<std::size_t N = 3, policy_t P = Policy>
		[[nodiscard]] constexpr basic_vec<T, N, P> axis() const noexcept;

		/* Rotates a quaternion from an axis vector and an angle.
		 * @param angle Angle of the rotation.
		 * @param axis Axis of the rotation. */
		template<std::size_t N = 3, policy_t P = Policy>
		constexpr basic_quat &rotate(T angle, const basic_vec<T, N, P> &axis) noexcept requires(N >= 3);
		// clang-format on

		[[nodiscard]] constexpr auto operator==(const basic_quat &other) const noexcept
		{
			return m_data == other.m_data;
		}
		[[nodiscard]] constexpr auto operator!=(const basic_quat &other) const noexcept
		{
			return m_data != other.m_data;
		}
		[[nodiscard]] constexpr auto operator<(const basic_quat &other) const noexcept { return m_data < other.m_data; }
		[[nodiscard]] constexpr auto operator<=(const basic_quat &other) const noexcept
		{
			return m_data <= other.m_data;
		}
		[[nodiscard]] constexpr auto operator>(const basic_quat &other) const noexcept { return m_data > other.m_data; }
		[[nodiscard]] constexpr auto operator>=(const basic_quat &other) const noexcept
		{
			return m_data >= other.m_data;
		}

		constexpr void swap(basic_quat &other) noexcept { m_data.swap(other.m_data); }
		friend constexpr void swap(basic_quat &a, basic_quat &b) noexcept { a.swap(b); }

	private:
		basic_vec<T, 4, Policy> m_data;
	};

	template<std::floating_point T, policy_t P>
	template<typename U, std::size_t N, policy_t Q>
	constexpr basic_quat<T, P> basic_quat<T, P>::from_euler(const basic_vec<U, N, Q> &v) noexcept
		requires(N >= 3)
	{
		const auto half_v = v * static_cast<T>(0.5);
		const auto c = cos(half_v);
		const auto s = sin(half_v);

		const auto x = s.x() * c.y() * c.z() - c.x() * s.y() * s.z();
		const auto y = c.x() * s.y() * c.z() + s.x() * c.y() * s.z();
		const auto z = c.x() * c.y() * s.z() - s.x() * s.y() * c.z();
		const auto w = c.x() * c.y() * c.z() + s.x() * s.y() * s.z();
		return basic_quat<T, P>{x, y, z, w};
	}
	template<std::floating_point T, policy_t P>
	constexpr T basic_quat<T, P>::pitch() const noexcept
	{
		const auto v2 = vector() * vector();
		const auto a = static_cast<T>(2) * (y() * z() + x() * w());
		const auto b = -v2.x() - v2.y() + v2.z() + v2.w();

		/* Singularity check */
		using fast_vec2 = basic_vec<T, 2, policy_t::FAST_SIMD>;
		if (all(fcmp_eq(fast_vec2{b, a}, fast_vec2{0}, static_cast<T>(0.0001)))) [[unlikely]]
			return static_cast<T>(static_cast<T>(2) * std::atan2(x(), w()));
		return static_cast<T>(std::atan2(a, b));
	}
	template<std::floating_point T, policy_t P>
	constexpr T basic_quat<T, P>::yaw() const noexcept
	{
		return std::asin(clamp(static_cast<T>(-2) * (x() * z() - y() * w()), static_cast<T>(-1), static_cast<T>(1)));
	}
	template<std::floating_point T, policy_t P>
	constexpr T basic_quat<T, P>::roll() const noexcept
	{
		const auto v2 = vector() * vector();
		const auto a = (x() * y() + z() * w());
		const auto b = v2.x() - v2.y() - v2.z() + v2.w();

		return static_cast<T>(std::atan2(static_cast<T>(2) * a, b));
	}

	template<std::floating_point T, policy_t P>
	template<typename U, std::size_t N, std::size_t M, policy_t Q>
	constexpr basic_quat<T, P> basic_quat<T, P>::from_mat(const basic_mat<U, N, M, Q> &m) noexcept
		requires(N >= 3 && M >= 3)
	{
		enum max_select
		{
			MAX_X,
			MAX_Y,
			MAX_Z,
			MAX_W,
		};

		const auto x2m1 = m[0][0] - m[1][1] - m[2][2];
		const auto y2m1 = m[1][1] - m[0][0] - m[2][2];
		const auto z2m1 = m[2][2] - m[0][0] - m[1][1];
		const auto w2m1 = m[0][0] + m[1][1] + m[2][2];

		auto max2m1 = w2m1;
		auto select = max_select::MAX_W;
		if (x2m1 > max2m1)
		{
			max2m1 = x2m1;
			select = max_select::MAX_X;
		}
		if (y2m1 > max2m1)
		{
			max2m1 = y2m1;
			select = max_select::MAX_Y;
		}
		if (z2m1 > max2m1)
		{
			max2m1 = z2m1;
			select = max_select::MAX_Z;
		}

		const auto a = std::sqrt(max2m1 + static_cast<T>(1)) * static_cast<T>(0.5);
		const auto b = static_cast<T>(0.25) / a;
		switch (select)
		{
			case MAX_X: return {a, (m[0][1] + m[1][0]) * b, (m[2][0] + m[0][2]) * b, (m[1][2] - m[2][1]) * b};
			case MAX_Y: return {(m[0][1] + m[1][0]) * b, a, (m[1][2] + m[2][1]) * b, (m[2][0] - m[0][2]) * b};
			case MAX_Z: return {(m[2][0] + m[0][2]) * b, (m[1][2] + m[2][1]) * b, a, (m[0][1] - m[1][0]) * b};
			case MAX_W: return {(m[1][2] - m[2][1]) * b, (m[2][0] - m[0][2]) * b, (m[0][1] - m[1][0]) * b, a};
		}
		return basic_quat<T, P>{std::numeric_limits<T>::quiet_NaN()};
	}
	template<std::floating_point T, policy_t P>
	template<std::size_t N, std::size_t M, policy_t Q>
	constexpr basic_mat<T, N, M, Q> basic_quat<T, P>::to_mat() const noexcept
		requires(N >= 3 && M >= 3)
	{
		using mat_type = basic_mat<T, N, M, Q>;
		using col_type = typename mat_type::col_type;

		const auto a = vector().xyz();
		const auto b = a * a;
		const auto c = vector().xxy() * vector().zyz();
		const auto d = vector().www() * a;

		const auto c0 = col_type{static_cast<T>(1) - (b.y() + b.z()) * static_cast<T>(2),
								 static_cast<T>(2) * (c.y() + d.z()),
								 static_cast<T>(2) * (c.x() - d.y())};
		const auto c1 = col_type{static_cast<T>(2) * (c.y() - d.z()),
								 static_cast<T>(1) - (b.x() + b.z()) * static_cast<T>(2),
								 static_cast<T>(2) * (c.z() + d.x())};
		const auto c2 = col_type{static_cast<T>(2) * (c.x() + d.y()),
								 static_cast<T>(2) * (c.z() - d.x()),
								 static_cast<T>(1) - (b.x() + b.y()) * static_cast<T>(2)};
		return mat_type{c0, c1, c2};
	}

	template<std::floating_point T, policy_t P>
	template<typename U, std::size_t N, policy_t Q>
	constexpr basic_quat<T, P> basic_quat<T, P>::from_angle_axis(U angle, const basic_vec<U, N, Q> &axis) noexcept
		requires(N >= 3)
	{
		const auto half_angle = angle * static_cast<T>(0.5);
		const auto s = std::sin(half_angle);
		const auto c = std::cos(half_angle);
		const auto v = axis * s;
		return basic_quat<T, P>(v.x(), v.y(), v.z(), c);
	}
	template<std::floating_point T, policy_t P>
	constexpr T basic_quat<T, P>::angle() const noexcept
	{
		constexpr auto cos1o2 = static_cast<T>(0.877582561890372716130286068203503191); /* cos(1/2) */
		if (std::abs(w()) > cos1o2)
		{
			const auto v = basic_vec<T, 3, policy_t::FAST_SIMD>{vector().xyz()};
			const auto a = std::asin(magn(v)) * static_cast<T>(2);
			if (w() < static_cast<T>(0))
				return std::numbers::pi_v<T> * static_cast<T>(2) - a;
			else
				return a;
		}
		else
			return std::acos(w()) * static_cast<T>(2);
	}
	template<std::floating_point T, policy_t P>
	template<std::size_t N, policy_t Q>
	constexpr basic_vec<T, N, Q> basic_quat<T, P>::axis() const noexcept
	{
		if (const auto a = static_cast<T>(1) - w() * w(); a <= static_cast<T>(0))
			return basic_vec<T, N, Q>{0, 0, 1};
		else
		{
			const auto v = basic_vec<T, 3, policy_t::FAST_SIMD>{vector().xyz()};
			const auto b = static_cast<T>(1) / std::sqrt(a);
			return basic_vec<T, N, Q>{v * b};
		}
	}

	template<std::floating_point T, policy_t P>
	template<typename U, std::size_t N, policy_t Q>
	constexpr basic_quat<T, P> basic_quat<T, P>::look_at_l(const basic_vec<U, N, Q> &dir, const basic_vec<U, N, Q> &up) noexcept
		requires(N >= 3)
	{
		using fast_mat = basic_mat<T, 3, 3, policy_t::FAST_SIMD>;
		using fast_vec = basic_vec<T, 3, policy_t::FAST_SIMD>;

		const auto dir_f = fast_vec{dir};
		const auto up_f = fast_vec{up};

		const auto right = cross(up_f, dir_f);
		const auto c0 = right * (static_cast<T>(1) / std::sqrt(max(static_cast<T>(0.00001), dot(right, right))));
		const auto c1 = right * cross(dir_f, c0);
		const auto c2 = dir_f;
		return from_mat(fast_mat{c0, c1, c2});
	}
	template<std::floating_point T, policy_t P>
	template<typename U, std::size_t N, policy_t Q>
	constexpr basic_quat<T, P> basic_quat<T, P>::look_at_r(const basic_vec<U, N, Q> &dir, const basic_vec<U, N, Q> &up) noexcept
		requires(N >= 3)
	{
		using fast_mat = basic_mat<T, 3, 3, policy_t::FAST_SIMD>;
		using fast_vec = basic_vec<T, 3, policy_t::FAST_SIMD>;

		const auto dir_f = -fast_vec{dir};
		const auto up_f = fast_vec{up};

		const auto right = cross(up_f, dir_f);
		const auto c0 = right * (static_cast<T>(1) / std::sqrt(max(static_cast<T>(0.00001), dot(right, right))));
		const auto c1 = right * cross(dir_f, c0);
		const auto c2 = dir_f;
		return from_mat(fast_mat{c0, c1, c2});
	}

	template<std::floating_point T, policy_t P>
	template<std::size_t N, policy_t Q>
	constexpr basic_quat<T, P> &basic_quat<T, P>::rotate(T angle, const basic_vec<T, N, Q> &axis) noexcept
		requires(N >= 3)
	{
		auto tmp = basic_vec<T, 4, policy_t::FAST_SIMD>{axis};

		/* Axis must be normalised */
		if (const auto len = magn(tmp); std::abs(len - static_cast<T>(1)) > static_cast<T>(0.001)) [[unlikely]]
			tmp *= static_cast<T>(1) / len;

		const auto half_angle = angle * static_cast<T>(0.5);
		tmp *= std::sin(half_angle);
		tmp.w() = std::cos(half_angle);

		m_data *= tmp;
		return *this;
	}

	/** Shuffles elements of a quaternion according to the provided indices.
	 * @return Result quaternion who's elements are specified by `I`.
	 * @example shuffle<2, 1, 0, 3>({.3, .4, .5, .5}) -> {.5, .4, .3, .5} */
	template<std::size_t Ix, std::size_t Iy, std::size_t Iz, std::size_t Iw, typename T, policy_t P>
	[[nodiscard]] constexpr basic_quat<T, P> shuffle(const basic_quat<T, P> &q) noexcept
	{
		return basic_quat<T, P>{shuffle<Ix, Iy, Iz, Iw>(q.vector())};
	}

	/** Rotates a vector by a quaternion rotation.
	 * @return Result of the rotation. */
	template<typename T, std::size_t N, policy_t P>
	[[nodiscard]] constexpr basic_vec<T, N, P> operator*(const basic_vec<T, N, P> &l, const basic_quat<T, P> &r) noexcept
	{
		const auto v = r.vector().xyz();
		return l + static_cast<T>(2.0) * cross(v, cross(v, l) + l * r.w());
	}

	/** Gets the Ith element of the quaternion. */
	template<std::size_t I, typename T, policy_t Q>
	[[nodiscard]] constexpr T &get(basic_quat<T, Q> &q) noexcept
	{
		return get<I>(q.vector());
	}
	/** @copydoc get */
	template<std::size_t I, typename T, policy_t Q>
	[[nodiscard]] constexpr const T &get(const basic_quat<T, Q> &q) noexcept
	{
		return get<I>(q.vector());
	}
}	 // namespace sek::math

template<typename T, sek::math::policy_t Q>
struct std::tuple_size<sek::math::basic_quat<T, Q>> : std::integral_constant<std::size_t, 4>
{
};
template<std::size_t I, typename T, sek::math::policy_t Q>
struct std::tuple_element<I, sek::math::basic_quat<T, Q>>
{
	using type = T;
};
