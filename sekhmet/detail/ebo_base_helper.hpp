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
			constexpr explicit ebo_base_helper_impl(Args &&...args)
				noexcept(std::is_nothrow_constructible_v<T, Args...>) requires std::is_constructible_v<T, Args...>
				: T(std::forward<Args>(args)...)
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
			constexpr ebo_base_helper_impl()
				noexcept(std::is_nothrow_default_constructible_v<T>) requires std::is_default_constructible_v<T>
			{
				::new (get()) T();
			}
			constexpr ebo_base_helper_impl(const ebo_base_helper_impl &other)
				noexcept(std::is_nothrow_copy_constructible_v<T>) requires std::is_copy_constructible_v<T>
			{
				::new (get()) T{other.value};
			}
			constexpr ebo_base_helper_impl &operator=(const ebo_base_helper_impl &other)
				noexcept(std::is_nothrow_copy_assignable_v<T>) requires std::is_copy_assignable_v<T>
			{
				value = other.value;
				return *this;
			}
			constexpr ebo_base_helper_impl(ebo_base_helper_impl &&other)
				noexcept(std::is_nothrow_move_constructible_v<T>) requires std::is_move_constructible_v<T>
			{
				::new (get()) T{std::move(other.value)};
			}
			constexpr ebo_base_helper_impl &operator=(ebo_base_helper_impl &&other)
				noexcept(std::is_nothrow_move_assignable_v<T>) requires std::is_move_assignable_v<T>
			{
				value = std::move(other.value);
				return *this;
			}
			constexpr ~ebo_base_helper_impl() { get()->~T(); }

			template<typename... Args>
			constexpr explicit ebo_base_helper_impl(Args &&...args)
				noexcept(std::is_nothrow_constructible_v<T, Args...>) requires std::is_constructible_v<T, Args...>
			{
				::new (get()) T{std::forward<Args>(args)...};
			}
			// clang-format on

			[[nodiscard]] constexpr T *get() noexcept { return &value; }
			[[nodiscard]] constexpr const T *get() const noexcept { return &value; }

			constexpr void swap(ebo_base_helper_impl &other) noexcept(std::is_nothrow_swappable_v<T>)
			{
				using std::swap;
				swap(value, other.value);
			}

			union
			{
				type_storage<T> padding = {};
				T value;
			};
		};
	}	 // namespace detail

	/** @brief Helper type used to implement empty base optimization. */
	template<typename T>
	using ebo_base_helper = detail::ebo_base_helper_impl<std::decay_t<T>, std::is_empty_v<std::decay_t<T>>>;

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
}	 // namespace sek