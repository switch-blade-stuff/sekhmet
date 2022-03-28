//
// Created by switchblade on 2021-10-25.
//

#pragma once

#include <concepts>
#include <memory>
#include <utility>

#include "assert.hpp"
#include "define.h"

namespace sek::detail
{
	template<typename Alloc, typename T>
	using rebind_alloc_t = typename std::allocator_traits<Alloc>::template rebind_alloc<T>;

	template<typename Alloc>
	[[nodiscard]] constexpr bool alloc_eq(const Alloc &a, const Alloc &b) noexcept
	{
		if constexpr (std::allocator_traits<Alloc>::is_always_equal::value)
			return true;
		else if constexpr (requires { {a == b}; })
			return a == b;
		else
			return false;
	}
	template<typename Alloc>
	[[nodiscard]] constexpr Alloc make_alloc_copy(const Alloc &alloc)
	{
		return std::allocator_traits<Alloc>::select_on_container_copy_construction(alloc);
	}
	template<typename Alloc>
	void alloc_assert_swap([[maybe_unused]] const Alloc &lhs, [[maybe_unused]] const Alloc &rhs) noexcept
	{
		SEK_ASSERT(std::allocator_traits<Alloc>::propagate_on_container_swap::value || alloc_eq(lhs, rhs));
	}
	template<typename Alloc>
	constexpr void alloc_swap(Alloc &lhs, Alloc &rhs) noexcept
	{
		if constexpr (std::allocator_traits<Alloc>::propagate_on_container_swap::value)
		{
			using std::swap;
			swap(lhs, rhs);
		}
	}
	template<typename Alloc>
	constexpr void alloc_move_assign(Alloc &lhs, Alloc &rhs) noexcept
	{
		if constexpr (std::allocator_traits<Alloc>::propagate_on_container_move_assignment::value) lhs = std::move(rhs);
	}
	template<typename Alloc>
	constexpr void alloc_copy_assign(Alloc &lhs, const Alloc &rhs) noexcept
	{
		if constexpr (std::allocator_traits<Alloc>::propagate_on_container_copy_assignment::value) lhs = rhs;
	}

	template<typename... Allocs>
	constexpr bool nothrow_alloc_default_construct = std::conjunction_v<std::is_nothrow_default_constructible<Allocs>...>;
	template<typename... Allocs>
	constexpr bool nothrow_alloc_copy_construct = std::conjunction_v<std::is_nothrow_copy_constructible<Allocs>...>;
	template<typename... Allocs>
	constexpr bool nothrow_alloc_copy_transfer =
		std::conjunction_v<std::bool_constant<std::is_nothrow_copy_constructible_v<Allocs> && std::allocator_traits<Allocs>::is_always_equal::value>...>;
	template<typename Alloc0, typename Alloc1>
	constexpr bool nothrow_alloc_copy_move_transfer =
		nothrow_alloc_copy_transfer<Alloc0> &&std::is_nothrow_move_constructible_v<Alloc1>;
	template<typename... Allocs>
	constexpr bool nothrow_alloc_move_construct = std::conjunction_v<std::is_nothrow_move_constructible<Allocs>...>;
	template<typename... Allocs>
	constexpr bool nothrow_alloc_move_assign =
		std::conjunction_v<std::bool_constant<(std::allocator_traits<Allocs>::propagate_on_container_move_assignment::value &&
											   std::is_nothrow_move_assignable_v<Allocs>) ||
											  std::allocator_traits<Allocs>::is_always_equal::value>...>;
}	 // namespace sek::detail