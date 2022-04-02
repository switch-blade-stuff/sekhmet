//
// Created by switchblade on 2022-03-12.
//

#pragma once

#include <ranges>

#include "meta_util.hpp"

namespace sek
{
	namespace detail
	{
		template<typename T, std::size_t N>
		struct static_array_t
		{
			constexpr const T *begin() const noexcept { return data; }
			constexpr const T *end() const noexcept { return data + N; }

			T data[N];
		};
		template<typename T>
		struct static_array_t<T, 0>
		{
			constexpr const T *begin() const noexcept { return nullptr; }
			constexpr const T *end() const noexcept { return nullptr; }
		};
	}	 // namespace detail

	/** @brief Structure used to store compile-time array made from NTTP object pack.
	 * @tparam T Type of objects stored in the array.
	 * @tparam Vals NTTP values to initialize the array with. */
	template<typename T, T... Vals>
	class array_constant
	{
	private:
		using array_t = detail::static_array_t<T, sizeof...(Vals)>;

	public:
		constexpr static array_t value = {Vals...};
	};

	/** @brief Structure used to store an array of constant values initialized from the NTTP parameter pack.
	 * @tparam T Type of values stored in the array.
	 * @tparam Vals Values to store within the array. Any values not convertible to T will be ignored. */
	template<typename T, auto... Vals>
	class filter_array_constant
	{
	private:
		template<std::size_t... Is>
		constexpr static std::size_t count_idx(std::index_sequence<Is...>)
		{
			return sizeof...(Is);
		}
		template<std::size_t, std::size_t... Is>
		constexpr static auto make_filter_sequence_impl(std::index_sequence<Is...>)
		{
			return std::index_sequence<Is...>{};
		}
		template<std::size_t I, auto V, auto... Vs, std::size_t... Is>
		constexpr static auto make_filter_sequence_impl(std::index_sequence<Is...>)
		{
			if constexpr (std::same_as<std::decay_t<decltype(V)>, std::decay_t<T>>)
				return make_filter_sequence_impl<I + 1, Vs...>(std::index_sequence<Is..., I>{});
			else
				return make_filter_sequence_impl<I + 1, Vs...>(std::index_sequence<Is...>{});
		}
		constexpr static auto make_filter_sequence()
		{
			return make_filter_sequence_impl<0, Vals...>(std::index_sequence<>{});
		}

		using array_t = detail::static_array_t<T, count_idx(make_filter_sequence())>;

		template<std::size_t I, std::size_t J, auto Arg, auto... Args>
		constexpr static auto extract_arg() noexcept
		{
			if constexpr (I == J)
				return Arg;
			else
				return extract_arg<I, J + 1, Args...>();
		}
		template<std::size_t... Is>
		constexpr static array_t instantiate(std::index_sequence<Is...>) noexcept
		{
			return array_t{extract_arg<Is, 0, Vals...>()...};
		}

	public:
		constexpr static array_t value = instantiate(make_filter_sequence());
	};

	/** @brief Simple structural view into a range of elements. */
	template<typename T>
	struct meta_view
	{
		typedef T value_type;
		typedef const T *pointer;
		typedef const T *const_pointer;
		typedef const T &reference;
		typedef const T &const_reference;
		typedef const_pointer iterator;
		typedef const_pointer const_iterator;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

		constexpr meta_view() noexcept = default;

		template<std::size_t N>
		constexpr meta_view(const T (&data)[N]) noexcept : data_ptr(data), data_size(N)
		{
		}
		template<std::contiguous_iterator I>
		constexpr meta_view(I f, I l) noexcept
			: data_ptr(std::to_address(f)), data_size(static_cast<size_type>(std::distance(f, l)))
		{
		}
		template<std::ranges::contiguous_range R>
		constexpr meta_view(const R &r) noexcept : meta_view(std::ranges::begin(r), std::ranges::end(r))
		{
		}
		constexpr meta_view(const_pointer data, size_type size) noexcept : meta_view(data, data + size) {}

		constexpr const_iterator begin() const noexcept { return data_ptr; }
		constexpr const_iterator end() const noexcept { return data_ptr + data_size; }
		constexpr const_iterator cbegin() const noexcept { return data_ptr; }
		constexpr const_iterator cend() const noexcept { return data_ptr + data_size; }

		constexpr const_pointer data() const noexcept { return data_ptr; }
		constexpr size_type size() const noexcept { return data_size; }

		[[nodiscard]] friend constexpr auto operator<=>(const meta_view &lhs, const meta_view &rhs) noexcept
		{
			return std::lexicographical_compare_three_way(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
		}
		[[nodiscard]] friend constexpr bool operator==(const meta_view &lhs, const meta_view &rhs) noexcept
		{
			return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
		}

		const T *data_ptr = nullptr;
		size_type data_size = 0;
	};
}	 // namespace sek
