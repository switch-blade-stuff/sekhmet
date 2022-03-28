//
// Created by switchblade on 2021-10-25.
//

#pragma once

#include <concepts>
#include <cstring>
#include <memory>
#include <utility>

#include "assert.hpp"
#include "define.h"

namespace sek::detail
{
	/** @brief Default allocator used for internal types. */
	template<typename T>
	class allocator
	{
	public:
		typedef T value_type;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;
		typedef std::false_type propagate_on_container_move_assignment;
		typedef std::false_type propagate_on_container_copy_assignment;
		typedef std::false_type propagate_on_container_swap;
		typedef std::true_type is_always_equal;

	public:
		/** Allocates the specified amount of objects.
		 * @param n Amount of objects to allocate.
		 * @return Pointer to the allocated memory.
		 * @throw std::bad_alloc If allocation has failed. */
		[[nodiscard]] constexpr T *allocate(size_type n) const
		{
			if (auto result = allocate(n, std::nothrow); !result) [[unlikely]]
				throw std::bad_alloc();
			else
				return result;
		}
		/** Allocates the specified amount of objects.
		 * @param n Amount of objects to allocate.
		 * @return Pointer to the allocated memory or nullptr on failure. */
		[[nodiscard]] constexpr T *allocate(size_type n, std::nothrow_t) const noexcept
		{
			auto bytes = sizeof(value_type) * static_cast<std::size_t>(n);
			if constexpr (alignof(value_type) > alignof(std::max_align_t))
				return static_cast<T *>(aligned_alloc(alignof(value_type), bytes));
			else
				return static_cast<T *>(malloc(bytes));
		}
		/** Deallocates the specified amount of objects.
		 * @param p Pointer to the memory previously allocated via allocate or reallocate. */
		constexpr void deallocate(T *p, size_type) const noexcept
		{
			if (p) [[likely]]
				free(p);
		}
	};

	template<typename T>
	[[nodiscard]] constexpr bool operator==(const allocator<T> &, const allocator<T> &) noexcept
	{
		return true;
	}

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
	/* Wrapper around `select_on_container_copy_construction` because that is too long. */
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

	template<typename Alloc>
	constexpr bool allow_reallocate = std::is_same_v<Alloc, allocator<typename std::allocator_traits<Alloc>::value_type>> &&
									  alignof(typename std::allocator_traits<Alloc>::value_type) <= alignof(std::max_align_t) &&
									  std::is_trivially_copyable_v<typename std::allocator_traits<Alloc>::value_type>;

	[[nodiscard]] constexpr auto *allocator_reallocate(auto *ptr, std::size_t new_n, std::nothrow_t)
	{
		using T = std::remove_pointer_t<decltype(ptr)>;
		return static_cast<T *>(realloc(ptr, sizeof(T) * static_cast<std::size_t>(new_n)));
	}
	[[nodiscard]] constexpr auto *allocator_reallocate(auto *ptr, std::size_t new_n)
	{
		if (auto result = allocator_reallocate(ptr, new_n, std::nothrow); !result) [[unlikely]]
			throw std::bad_alloc();
		else
			return result;
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