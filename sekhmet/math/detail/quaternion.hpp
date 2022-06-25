/*
 * Created by switchblade on 22/06/22
 */

#pragma once

#include "sekhmet/detail/assert.hpp"

#include "matrix.hpp"
#include "matrix2x.hpp"
#include "matrix3x.hpp"
#include "vector.hpp"

#define SEK_QUATERNION_GENERATE_SHUFFLE(...) SEK_DETAIL_SHUFFLE_4(SEK_DETAIL_Q_SHUFFLE, SEK_DETAIL_V_TYPE, __VA_ARGS__)

namespace sek::math
{
	template<std::floating_point T, storage_policy Policy = storage_policy::OPTIMAL>
	class basic_quat
	{
	public:
		typedef T value_type;
		typedef basic_vec<T, 4, Policy> vector_type;

		constexpr static auto policy = Policy;

	public:
		constexpr basic_quat() noexcept = default;

		// clang-format off
		template<storage_policy OtherPolicy>
		constexpr explicit basic_quat(const basic_quat<T, OtherPolicy> &other) noexcept requires(OtherPolicy != policy)
			: m_data(other.m_data)
		{
		}
		template<storage_policy OtherPolicy>
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

		template<storage_policy P = policy>
		constexpr basic_quat(const basic_vec<T, 4, P> &vector) noexcept : m_data(vector)
		{
		}
		template<storage_policy P = policy>
		constexpr basic_quat(basic_vec<T, 4, P> &&vector) noexcept : m_data(std::move(vector))
		{
		}

		/** Casts quaternion to the underlying vector type. */
		[[nodiscard]] constexpr vector_type &vector() noexcept { return m_data; }
		/** @copydoc vector */
		[[nodiscard]] constexpr operator vector_type &() noexcept { return vector(); }
		/** @copydoc vector */
		[[nodiscard]] constexpr const vector_type &vector() const noexcept { return m_data; }
		/** @copydoc vector */
		[[nodiscard]] constexpr operator const vector_type &() const noexcept { return vector(); }

		[[nodiscard]] constexpr decltype(auto) x() noexcept { return m_data.x(); }
		[[nodiscard]] constexpr decltype(auto) x() const noexcept { return m_data.x(); }
		[[nodiscard]] constexpr decltype(auto) y() noexcept { return m_data.y(); }
		[[nodiscard]] constexpr decltype(auto) y() const noexcept { return m_data.y(); }
		[[nodiscard]] constexpr decltype(auto) z() noexcept { return m_data.z(); }
		[[nodiscard]] constexpr decltype(auto) z() const noexcept { return m_data.z(); }
		[[nodiscard]] constexpr decltype(auto) w() noexcept { return m_data.w(); }
		[[nodiscard]] constexpr decltype(auto) w() const noexcept { return m_data.w(); }

		SEK_QUATERNION_GENERATE_SHUFFLE(x, y, z, w)

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

	/** Gets the Ith element of the quaternion. */
	template<std::size_t I, typename T, storage_policy Sp>
	[[nodiscard]] constexpr T &get(basic_quat<T, Sp> &q) noexcept
	{
		return get<I>(q.vector());
	}
	/** @copydoc get */
	template<std::size_t I, typename T, storage_policy Sp>
	[[nodiscard]] constexpr const T &get(const basic_quat<T, Sp> &q) noexcept
	{
		return get<I>(q.vector());
	}
}	 // namespace sek::math

template<typename T, sek::math::storage_policy Sp>
struct std::tuple_size<sek::math::basic_quat<T, Sp>> : std::integral_constant<std::size_t, 4>
{
};
template<std::size_t I, typename T, sek::math::storage_policy Sp>
struct std::tuple_element<I, sek::math::basic_quat<T, Sp>>
{
	using type = T;
};
