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
 * Created by switchblade on 2021-11-08
 */

#pragma once

#include <iterator>

#include "define.h"
#include <type_traits>

namespace sek
{
	/** @brief Generic contiguous iterator. */
	template<typename T, bool IsConst>
	class contiguous_iterator
	{
		template<typename U, bool B>
		friend class contiguous_iterator;

	public:
		typedef T value_type;
		typedef std::conditional_t<IsConst, const T, T> *pointer;
		typedef std::conditional_t<IsConst, const T, T> &reference;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;
		typedef std::contiguous_iterator_tag iterator_category;
		typedef value_type element_type;

	public:
		constexpr contiguous_iterator() noexcept = default;

		constexpr explicit contiguous_iterator(pointer element) noexcept : ptr(element) {}

		template<bool OtherConst, typename = std::enable_if_t<IsConst && !OtherConst>>
		constexpr contiguous_iterator(const contiguous_iterator<T, OtherConst> &other) noexcept : ptr(other.ptr)
		{
		}

		constexpr contiguous_iterator operator++(int) noexcept
		{
			auto temp = *this;
			ptr++;
			return temp;
		}
		constexpr contiguous_iterator operator--(int) noexcept
		{
			auto temp = *this;
			ptr--;
			return temp;
		}
		constexpr contiguous_iterator &operator++() noexcept
		{
			ptr++;
			return *this;
		}
		constexpr contiguous_iterator &operator--() noexcept
		{
			ptr--;
			return *this;
		}
		constexpr contiguous_iterator &operator+=(difference_type n) noexcept
		{
			ptr += n;
			return *this;
		}
		constexpr contiguous_iterator &operator-=(difference_type n) noexcept
		{
			ptr -= n;
			return *this;
		}

		[[nodiscard]] constexpr contiguous_iterator operator+(difference_type n) const noexcept
		{
			return contiguous_iterator{ptr + n};
		}
		[[nodiscard]] constexpr contiguous_iterator operator-(difference_type n) const noexcept
		{
			return contiguous_iterator{ptr - n};
		}
		[[nodiscard]] constexpr difference_type operator-(const contiguous_iterator &other) const noexcept
		{
			return ptr - other.ptr;
		}

		[[nodiscard]] friend constexpr contiguous_iterator operator+(difference_type n, const contiguous_iterator &iter) noexcept
		{
			return iter + n;
		}

		/** Returns pointer to the target element. */
		[[nodiscard]] constexpr pointer get() const noexcept { return ptr; }
		/** @copydoc value */
		[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
		/** Returns reference to the target element. */
		[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }
		/** Returns reference to the element located at the specified offset. */
		[[nodiscard]] constexpr reference operator[](difference_type n) const noexcept { return *(get() + n); }

		[[nodiscard]] constexpr auto operator<=>(const contiguous_iterator &) const noexcept = default;
		[[nodiscard]] constexpr bool operator==(const contiguous_iterator &) const noexcept = default;

	private:
		/** Pointer into the target sequence. */
		pointer ptr;
	};
}	 // namespace sek