/*
 * Created by switchblade on 2022-03-06
 */

#pragma once

#include "sekhmet/detail/hash.hpp"

#include "fwd.hpp"

namespace sek::math::detail
{
	template<typename T, std::size_t N, policy_t P>
	union vector_data;
	template<typename T, std::size_t N, policy_t P>
	union mask_data;

	template<typename T>
	struct mask_set
	{
		template<typename U>
		constexpr void operator()(T &to, U &&from) const noexcept
		{
			to = T{std::forward<U>(from)};
		}
	};
	template<typename T>
	struct mask_get
	{
		constexpr bool operator()(T &v) const noexcept { return T{v}; }
	};

	template<typename...>
	struct data_policy;
	template<typename T, std::size_t N, policy_t P>
	struct data_policy<vector_data<T, N, P>>
	{
		constexpr static auto value = P;
	};
	template<typename T, std::size_t N, policy_t P>
	struct data_policy<mask_data<T, N, P>>
	{
		constexpr static auto value = P;
	};
	template<typename T>
	constexpr auto data_policy_v = data_policy<T>::value;

	template<policy_t P, policy_t Mask, policy_t Flag>
	using check_policy = std::bool_constant<(P & Mask) == Flag>;
	template<policy_t P, policy_t Mask, policy_t Flag>
	constexpr auto check_policy_v = check_policy<P, Mask, Flag>::value;
	template<policy_t P, policy_t Mask, policy_t Flag>
	constexpr auto set_policy_v = static_cast<policy_t>((P & ~Mask) | Flag);

	template<typename T, std::size_t N, policy_t P>
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::PACKED>
	union vector_data<T, N, P>
	{
		constexpr vector_data() noexcept : values{} {}
		template<std::size_t M = N>
		constexpr vector_data(const T (&data)[M]) noexcept : values{}
		{
			for (std::size_t i = 0; i < std::min(N, M); ++i) values[i] = data[i];
		}
		template<std::convertible_to<T>... Args>
		constexpr vector_data(Args &&...args) noexcept : vector_data({static_cast<T>(std::forward<Args>(args))...})
		{
		}

		constexpr T &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr const T &operator[](std::size_t i) const noexcept { return values[i]; }

		T values[N];
	};
	template<typename T, std::size_t N, policy_t P>
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::PACKED>
	union mask_data<T, N, P>
	{
		constexpr mask_data() noexcept : values{} {}
		template<std::convertible_to<bool> B, std::size_t M = N>
		constexpr mask_data(const B (&data)[M]) noexcept : values{}
		{
			for (std::size_t i = 0; i < std::min(N, M); ++i) values[i] = static_cast<bool>(data[i]);
		}
		template<std::convertible_to<bool>... Args>
		constexpr mask_data(Args &&...args) noexcept : mask_data({static_cast<bool>(std::forward<Args>(args))...})
		{
		}

		constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		bool values[N];
	};

	template<typename T, typename = void>
	struct is_defined
	{
		static constexpr bool value = false;
	};
	template<typename T>
	struct is_defined<T, std::enable_if_t<(sizeof(T) > 0)>>
	{
		static constexpr bool value = true;
	};
	template<typename T>
	constexpr auto is_defined_v = is_defined<T>::value;

	template<typename T, std::size_t N, policy_t P>
	using vector_data_t = std::conditional_t<is_defined_v<vector_data<T, N, P>>,
											 vector_data<T, N, P>,
											 vector_data<T, N, set_policy_v<P, policy_t::STORAGE_MASK, policy_t::PACKED>>>;
	template<typename T, std::size_t N, policy_t P>
	using mask_data_t = std::conditional_t<is_defined_v<mask_data<T, N, P>>,
										   mask_data<T, N, P>,
										   mask_data<T, N, set_policy_v<P, policy_t::STORAGE_MASK, policy_t::PACKED>>>;

	template<typename T>
	class mask_element
	{
		template<typename, std::size_t, policy_t>
		friend union mask_data;

		constexpr mask_element(T &ref) noexcept : m_ptr(&ref) {}

		constexpr static void set(auto &ref, auto &&value) { mask_set<std::remove_const_t<T>>{}(ref, value); }
		constexpr static bool get(auto &ref) { return mask_get<std::remove_const_t<T>>{}(ref); }

	public:
		mask_element() = delete;
		mask_element &operator=(const mask_element &) = delete;
		mask_element &operator=(mask_element &&) = delete;

		constexpr mask_element(const mask_element &) noexcept = default;
		constexpr mask_element(mask_element &&) noexcept = default;

		template<typename U>
		constexpr mask_element &operator=(U value) noexcept
			requires(!std::is_const_v<T>)
		{
			set(*m_ptr, std::move(value));
			return *this;
		}

		[[nodiscard]] constexpr operator bool() const noexcept { return get(*m_ptr); }

	private:
		T *m_ptr;
	};
}	 // namespace sek::math::detail