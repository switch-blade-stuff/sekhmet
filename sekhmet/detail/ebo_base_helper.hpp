//
// Created by switchblade on 2021-12-05.
//

#pragma once

#include "aligned_storage.hpp"
#include <type_traits>

namespace sek::detail
{
	template<typename T, bool = true>
	struct ebo_base_helper_impl : T
	{
		constexpr ebo_base_helper_impl() noexcept(std::is_nothrow_default_constructible_v<T>) = default;
		constexpr ebo_base_helper_impl(const ebo_base_helper_impl &) noexcept(std::is_nothrow_copy_constructible_v<T>) = default;
		constexpr ebo_base_helper_impl &
			operator=(const ebo_base_helper_impl &) noexcept(std::is_nothrow_copy_assignable_v<T>) = default;
		constexpr ebo_base_helper_impl(ebo_base_helper_impl &&) noexcept(std::is_nothrow_move_constructible_v<T>) = default;
		constexpr ebo_base_helper_impl &operator=(ebo_base_helper_impl &&) noexcept(std::is_nothrow_move_assignable_v<T>) = default;

		template<typename... Args>
		constexpr explicit ebo_base_helper_impl(Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
			: T(std::forward<Args>(args)...)
		{
		}

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
		constexpr ebo_base_helper_impl() noexcept(std::is_nothrow_default_constructible_v<T>) requires std::is_default_constructible_v<T>
		{
			::new (get()) T();
		}
		constexpr ebo_base_helper_impl(const ebo_base_helper_impl &other) noexcept(
			std::is_nothrow_copy_constructible_v<T>) requires std::is_copy_constructible_v<T>
		{
			::new (get()) T{*other.get()};
		}
		constexpr ebo_base_helper_impl &operator=(const ebo_base_helper_impl &other) noexcept(
			std::is_nothrow_copy_assignable_v<T>) requires std::is_copy_assignable_v<T>
		{
			*get() = *other.get();
			return *this;
		}
		constexpr ebo_base_helper_impl(ebo_base_helper_impl &&other) noexcept(
			std::is_nothrow_move_constructible_v<T>) requires std::is_move_constructible_v<T>
		{
			::new (get()) T{std::move(*other.get())};
		}
		constexpr ebo_base_helper_impl &operator=(ebo_base_helper_impl &&other) noexcept(
			std::is_nothrow_move_assignable_v<T>) requires std::is_move_assignable_v<T>
		{
			*get() = std::move(*other.get());
			return *this;
		}
		constexpr ~ebo_base_helper_impl() { get()->~T(); }

		template<typename... Args>
		constexpr explicit ebo_base_helper_impl(Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
		{
			::new (get()) T{std::forward<Args>(args)...};
		}

		[[nodiscard]] constexpr T *get() noexcept { return obj_data.template get<T>(); }
		[[nodiscard]] constexpr const T *get() const noexcept { return obj_data.template get<T>(); }

		constexpr void swap(ebo_base_helper_impl &other) noexcept(std::is_nothrow_swappable_v<T>)
		{
			using std::swap;
			swap(*get(), *other.get());
		}

		type_storage<T> obj_data = {};
	};

	/** @brief Helper type used to implement empty base optimization. */
	template<typename T>
	using ebo_base_helper = ebo_base_helper_impl<std::decay_t<T>, std::is_empty_v<std::decay_t<T>>>;

	template<typename T>
	constexpr void swap(ebo_base_helper<T> &a, ebo_base_helper<T> &b) noexcept(std::is_nothrow_swappable_v<T>)
	{
		a.swap(b);
	}
	template<typename T>
	[[nodiscard]] constexpr int operator<=>(const ebo_base_helper<T> &a, const ebo_base_helper<T> &b) noexcept
	{
		return *a.get() <=> *b.get();
	}
	template<typename T>
	[[nodiscard]] constexpr bool operator==(const ebo_base_helper<T> &a, const ebo_base_helper<T> &b) noexcept
	{
		return *a.get() == *b.get();
	}
}	 // namespace sek::detail