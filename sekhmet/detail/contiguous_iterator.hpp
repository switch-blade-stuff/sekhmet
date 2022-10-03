/*
 * Created by switchblade on 2021-11-08
 */

#pragma once

#include <iterator>

#include "../define.h"
#include <type_traits>

namespace sek
{
	/** @brief Generic contiguous range_type_iterator. */
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

		constexpr explicit contiguous_iterator(pointer element) noexcept : m_ptr(element) {}

		template<bool OtherConst, typename = std::enable_if_t<IsConst && !OtherConst>>
		constexpr contiguous_iterator(const contiguous_iterator<T, OtherConst> &other) noexcept : m_ptr(other.m_ptr)
		{
		}

		constexpr contiguous_iterator operator++(int) noexcept
		{
			auto temp = *this;
			m_ptr++;
			return temp;
		}
		constexpr contiguous_iterator operator--(int) noexcept
		{
			auto temp = *this;
			m_ptr--;
			return temp;
		}
		constexpr contiguous_iterator &operator++() noexcept
		{
			m_ptr++;
			return *this;
		}
		constexpr contiguous_iterator &operator--() noexcept
		{
			m_ptr--;
			return *this;
		}
		constexpr contiguous_iterator &operator+=(difference_type n) noexcept
		{
			m_ptr += n;
			return *this;
		}
		constexpr contiguous_iterator &operator-=(difference_type n) noexcept
		{
			m_ptr -= n;
			return *this;
		}

		[[nodiscard]] constexpr contiguous_iterator operator+(difference_type n) const noexcept
		{
			return contiguous_iterator{m_ptr + n};
		}
		[[nodiscard]] constexpr contiguous_iterator operator-(difference_type n) const noexcept
		{
			return contiguous_iterator{m_ptr - n};
		}
		[[nodiscard]] constexpr difference_type operator-(const contiguous_iterator &other) const noexcept
		{
			return m_ptr - other.m_ptr;
		}

		[[nodiscard]] friend constexpr contiguous_iterator operator+(difference_type n, const contiguous_iterator &iter) noexcept
		{
			return iter + n;
		}

		/** Returns pointer to the target element. */
		[[nodiscard]] constexpr pointer get() const noexcept { return m_ptr; }
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
		pointer m_ptr;
	};
}	 // namespace sek