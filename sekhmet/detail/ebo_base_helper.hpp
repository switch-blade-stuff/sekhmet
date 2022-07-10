/*
 * Created by switchblade on 2021-12-05
 */

#pragma once

#include "aligned_storage.hpp"
#include <type_traits>

namespace sek
{
	namespace detail
	{
		template<typename T, bool = true>
		struct ebo_base_helper_impl : T
		{
			// clang-format off
			constexpr ebo_base_helper_impl() noexcept(std::is_nothrow_default_constructible_v<T>) = default;
			constexpr ebo_base_helper_impl(const ebo_base_helper_impl &) noexcept(std::is_nothrow_copy_constructible_v<T>) = default;
			constexpr ebo_base_helper_impl &operator=(const ebo_base_helper_impl &) noexcept(std::is_nothrow_copy_assignable_v<T>) = default;
			constexpr ebo_base_helper_impl(ebo_base_helper_impl &&) noexcept(std::is_nothrow_move_constructible_v<T>) = default;
			constexpr ebo_base_helper_impl &operator=(ebo_base_helper_impl &&) noexcept(std::is_nothrow_move_assignable_v<T>) = default;

			template<typename... Args>
			constexpr explicit ebo_base_helper_impl(Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
				requires std::is_constructible_v<T, Args...> : T(std::forward<Args>(args)...)
			{
			}
			// clang-format on

			[[nodiscard]] constexpr T *get() noexcept { return static_cast<T *>(this); }
			[[nodiscard]] constexpr const T *get() const noexcept { return static_cast<const T *>(this); }

			constexpr void swap(ebo_base_helper_impl &other) noexcept(std::is_nothrow_swappable_v<T>)
			{
				using std::swap;
				swap(*get(), *other.get());
			}
		};
		template<>
		struct ebo_base_helper_impl<void, false>
		{
			[[nodiscard]] constexpr void *get() noexcept { return nullptr; }
			[[nodiscard]] constexpr const void *get() const noexcept { return nullptr; }

			constexpr void swap(ebo_base_helper_impl &) noexcept {}
		};
		template<typename T>
		struct ebo_base_helper_impl<T, false>
		{
			// clang-format off
			constexpr ebo_base_helper_impl() noexcept(std::is_nothrow_default_constructible_v<T>)
				requires std::is_default_constructible_v<T> : value()
			{
			}
			constexpr ebo_base_helper_impl(const ebo_base_helper_impl &other) noexcept(std::is_nothrow_copy_constructible_v<T>)
				requires std::is_copy_constructible_v<T> : value(other.value)
			{
			}
			constexpr ebo_base_helper_impl &operator=(const ebo_base_helper_impl &other)
				noexcept(std::is_nothrow_copy_assignable_v<T>) requires std::is_copy_assignable_v<T>
			{
				value = other.value;
				return *this;
			}
			constexpr ebo_base_helper_impl(ebo_base_helper_impl &&other) noexcept(std::is_nothrow_move_constructible_v<T>)
				requires std::is_move_constructible_v<T> : value(std::move(other.value))
			{
			}
			constexpr ebo_base_helper_impl &operator=(ebo_base_helper_impl &&other)
				noexcept(std::is_nothrow_move_assignable_v<T>) requires std::is_move_assignable_v<T>
			{
				value = std::move(other.value);
				return *this;
			}
			constexpr ~ebo_base_helper_impl() = default;

			template<typename... Args>
			constexpr explicit ebo_base_helper_impl(Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
				requires std::is_constructible_v<T, Args...> : value{std::forward<Args>(args)...}
			{
			}
			// clang-format on

			[[nodiscard]] constexpr T *get() noexcept { return std::addressof(value); }
			[[nodiscard]] constexpr const T *get() const noexcept { return std::addressof(value); }

			constexpr void swap(ebo_base_helper_impl &other) noexcept(std::is_nothrow_swappable_v<T>)
			{
				using std::swap;
				swap(value, other.value);
			}

			T value;
		};

		// clang-format off
		template<typename T>
		constexpr bool ebo_candidate = std::is_object_v<T> && std::is_empty_v<T>;
		// clang-format on
	}	 // namespace detail

	/** @brief Helper type used to implement empty base optimization. */
	template<typename T>
	using ebo_base_helper = detail::ebo_base_helper_impl<std::decay_t<T>, detail::ebo_candidate<std::decay_t<T>>>;

	template<typename T>
	constexpr void swap(ebo_base_helper<T> &a, ebo_base_helper<T> &b) noexcept(std::is_nothrow_swappable_v<T>)
	{
		a.swap(b);
	}

	// clang-format off
	template<typename T>
	[[nodiscard]] constexpr int operator<=>(const ebo_base_helper<T> &a, const ebo_base_helper<T> &b) noexcept
		requires(requires{ *a.get() <=> *b.get(); })
	{
		return *a.get() <=> *b.get();
	}
	template<typename T>
	[[nodiscard]] constexpr bool operator==(const ebo_base_helper<T> &a, const ebo_base_helper<T> &b) noexcept
		requires(requires{ *a.get() == *b.get(); })
	{
		return *a.get() == *b.get();
	}
	// clang-format on
}	 // namespace sek