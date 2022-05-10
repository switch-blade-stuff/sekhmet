//
// Created by switchblade on 2022-01-25.
//

#pragma once

#include <algorithm>
#include <limits>

#include "contiguous_iterator.hpp"
#include "hash.hpp"
#include "string_util.hpp"
#include <string_view>

namespace sek
{
	/** @brief Fixed-size string that can be used for NTTP. */
	template<typename C, std::size_t N, typename Traits = std::char_traits<C>>
	struct basic_static_string
	{
	public:
		typedef Traits traits_type;
		typedef C value_type;
		typedef value_type *pointer;
		typedef const value_type *const_pointer;
		typedef value_type &reference;
		typedef const value_type &const_reference;
		typedef contiguous_iterator<value_type, false> iterator;
		typedef contiguous_iterator<value_type, true> const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

		constexpr static size_type npos = std::numeric_limits<size_type>::max();

	public:
		constexpr basic_static_string() = default;
		constexpr basic_static_string(const C (&str)[N]) { std::copy_n(str, N, value); }
		constexpr basic_static_string(const C *str, size_type n) : basic_static_string(str, str + n) {}
		template<forward_range_for<value_type>, typename R>
		constexpr basic_static_string(const R &range)
			: basic_static_string(std::ranges::begin(range), std::ranges::end(range))
		{
		}
		template<forward_iterator_for<value_type>, typename Iterator>
		constexpr basic_static_string(Iterator first, Iterator last)
		{
			std::copy(first, last, value);
		}

		/** Returns iterator to the start of the string. */
		[[nodiscard]] constexpr iterator begin() noexcept { return iterator{value}; }
		/** Returns iterator to the end of the string. */
		[[nodiscard]] constexpr iterator end() noexcept { return iterator{value + size()}; }
		/** Returns const iterator to the start of the string. */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return const_iterator{value}; }
		/** Returns const iterator to the end of the string. */
		[[nodiscard]] constexpr const_iterator end() const noexcept { return const_iterator{value + size()}; }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }
		/** Returns reverse iterator to the end of the string. */
		[[nodiscard]] constexpr reverse_iterator rbegin() noexcept { return reverse_iterator{end()}; }
		/** Returns reverse iterator to the start of the string. */
		[[nodiscard]] constexpr reverse_iterator rend() noexcept { return reverse_iterator{begin()}; }
		/** Returns const reverse iterator to the end of the string. */
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator{end()}; }
		/** Returns const reverse iterator to the start of the string. */
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator{begin()}; }
		/** @copydoc rbegin */
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
		/** @copydoc rend */
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return rend(); }

		/** Returns pointer to the string's data. */
		[[nodiscard]] constexpr pointer data() noexcept { return value; }
		/** @copydoc data */
		[[nodiscard]] constexpr const_pointer data() const noexcept { return value; }
		/** Returns reference to the element at the specified offset within the string. */
		[[nodiscard]] constexpr reference at(size_type i) noexcept { return value[i]; }
		/** @copydoc at */
		[[nodiscard]] constexpr reference operator[](size_type i) noexcept { return at(i); }
		/** Returns constant reference to the element at the specified offset within the string. */
		[[nodiscard]] constexpr const_reference at(size_type i) const noexcept { return value[i]; }
		/** @copydoc at */
		[[nodiscard]] constexpr const_reference operator[](size_type i) const noexcept { return at(i); }
		/** Returns reference to the element at the start of the string. */
		[[nodiscard]] constexpr reference front() noexcept { return value[0]; }
		/** Returns constant reference to the element at the start of the string. */
		[[nodiscard]] constexpr const_reference front() const noexcept { return value[0]; }
		/** Returns constant reference to the element at the end of the string. */
		[[nodiscard]] constexpr reference back() noexcept { return value[size() - 1]; }
		/** Returns constant reference to the element at the end of the string. */
		[[nodiscard]] constexpr const_reference back() const noexcept { return value[size() - 1]; }

		/** Returns size of the string (amount of value_type units). */
		[[nodiscard]] constexpr size_type size() const noexcept { return detail::str_length(value, N); }
		/** @copydoc size */
		[[nodiscard]] constexpr size_type length() const noexcept { return size(); }
		/** Returns maximum value for size. */
		[[nodiscard]] constexpr size_type max_size() const noexcept
		{
			return std::numeric_limits<size_type>::max() - 1;
		}
		/** Checks if the string is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }

		/** Converts the static string to a string view. */
		[[nodiscard]] constexpr operator std::basic_string_view<C, Traits>() const noexcept
		{
			return std::basic_string_view<C, Traits>{data(), size()};
		}

		/** Finds left-most location of a sequence of character within the string. */
		template<forward_iterator_for<value_type> Iterator>
		[[nodiscard]] constexpr size_type find_first(Iterator first, Iterator last) const
		{
			return detail::find_first<npos>(begin(), end(), first, last);
		}
		/** Finds left-most location of a raw character array within the string. */
		[[nodiscard]] constexpr size_type find_first(const value_type *str, size_type len) const noexcept
		{
			return find_first(str, str + len);
		}
		/** Finds left-most location of a c-style substring within the string. */
		[[nodiscard]] constexpr size_type find_first(const value_type *str) const noexcept
		{
			return find_first(str, detail::str_length(str));
		}
		/** Finds left-most location of a character range within the string. */
		template<forward_range_for<value_type> R>
		[[nodiscard]] constexpr size_type find_first(const R &r) const
		{
			return find_first(std::ranges::begin(r), std::ranges::end(r));
		}
		/** Finds left-most location of a character within the string. */
		[[nodiscard]] constexpr size_type find_first(value_type c) const noexcept
		{
			for (auto character = begin(), last = end(); character != last; character++)
				if (*character == c) return static_cast<size_type>(character - begin());
			return npos;
		}

		/** Finds right-most location of a sequence of character within the string. */
		template<forward_iterator_for<value_type> Iterator>
		[[nodiscard]] constexpr size_type find_last(Iterator first, Iterator last) const
		{
			return detail::find_last<npos>(begin(), end(), first, last);
		}
		/** Finds right-most location of a raw character array within the string. */
		[[nodiscard]] constexpr size_type find_last(const value_type *str, size_type len) const noexcept
		{
			return find_last(str, str + len);
		}
		/** Finds right-most location of a c-style substring within the string. */
		[[nodiscard]] constexpr size_type find_last(const value_type *str) const noexcept
		{
			return find_last(str, detail::str_length(str));
		}
		/** Finds right-most location of a character range within the string. */
		template<forward_range_for<value_type> R>
		[[nodiscard]] constexpr size_type find_last(const R &r) const
		{
			return find_last(std::ranges::begin(r), std::ranges::end(r));
		}
		/** Finds right-most location of a character within the string. */
		[[nodiscard]] constexpr size_type find_last(value_type c) const noexcept
		{
			for (auto first = begin(), character = end(); character != first;)
				if (*(--character) == c) return static_cast<size_type>(character - begin());
			return npos;
		}

		/** Finds left-most location of a character from a sequence within the string. */
		template<forward_iterator_for<value_type> Iterator>
		[[nodiscard]] constexpr size_type find_first_of(Iterator first, Iterator last) const
		{
			return detail::find_first_of<npos>(begin(), end(), first, last);
		}
		/** Finds left-most location of a character from an initializer list within the string. */
		[[nodiscard]] constexpr size_type find_first_of(std::initializer_list<value_type> list) const noexcept
		{
			return find_first_of(list.begin(), list.end());
		}
		/** Finds left-most location of a character from a raw character array within the string. */
		[[nodiscard]] constexpr size_type find_first_of(const value_type *str, size_type len) const noexcept
		{
			return find_first_of(str, str + len);
		}
		/** Finds left-most location of a character from a c-style string within the string. */
		[[nodiscard]] constexpr size_type find_first_of(const value_type *str) const noexcept
		{
			return find_first_of(str, detail::str_length(str));
		}
		/** Finds left-most location of a character from a range within the string. */
		template<forward_range_for<value_type> R>
		[[nodiscard]] constexpr size_type find_first_of(const R &r) const
		{
			return find_first_of(std::ranges::begin(r), std::ranges::end(r));
		}

		/** Finds right-most location of a character from a sequence within the string. */
		template<forward_iterator_for<value_type> Iterator>
		[[nodiscard]] constexpr size_type find_last_of(Iterator first, Iterator last) const
		{
			return detail::find_last_of<npos>(begin(), end(), first, last);
		}
		/** Finds right-most location of a character from an initializer list within the string. */
		[[nodiscard]] constexpr size_type find_last_of(std::initializer_list<value_type> list) const noexcept
		{
			return find_last_of(list.begin(), list.end());
		}
		/** Finds right-most location of a character from a raw character array within the string. */
		[[nodiscard]] constexpr size_type find_last_of(const value_type *str, size_type len) const noexcept
		{
			return find_last_of(str, str + len);
		}
		/** Finds right-most location of a character from a c-style string within the string. */
		[[nodiscard]] constexpr size_type find_last_of(const value_type *str) const noexcept
		{
			return find_last_of(str, detail::str_length(str));
		}
		/** Finds right-most location of a character from a range within the string. */
		template<forward_range_for<value_type> R>
		[[nodiscard]] constexpr size_type find_last_of(const R &r) const
		{
			return find_last_of(std::ranges::begin(r), std::ranges::end(r));
		}

		/** Finds left-most location of a character not from a sequence within the string. */
		template<forward_iterator_for<value_type> Iterator>
		[[nodiscard]] constexpr size_type find_first_not_of(Iterator first, Iterator last) const
		{
			return detail::find_first_not_of<npos>(begin(), end(), first, last);
		}
		/** Finds left-most location of a character not from an initializer list within the string. */
		[[nodiscard]] constexpr size_type find_first_not_of(std::initializer_list<value_type> list) const noexcept
		{
			return find_first_not_of(list.begin(), list.end());
		}
		/** Finds left-most location of a character not from a c-style string within the string. */
		[[nodiscard]] constexpr size_type find_first_not_of(const value_type *str) const noexcept
		{
			return find_first_not_of(str, detail::str_length(str));
		}
		/** Finds left-most location of a character not from a raw character array within the string. */
		[[nodiscard]] constexpr size_type find_first_not_of(const value_type *str, size_type len) const noexcept
		{
			return find_first_not_of(str, str + len);
		}
		/** Finds left-most location of a character not from a range within the string. */
		template<forward_range_for<value_type> R>
		[[nodiscard]] constexpr size_type find_first_not_of(const R &r) const
		{
			return find_first_not_of(std::ranges::begin(r), std::ranges::end(r));
		}

		/** Finds right-most location of a character not from a sequence within the string. */
		template<forward_iterator_for<value_type> Iterator>
		[[nodiscard]] constexpr size_type find_last_not_of(Iterator first, Iterator last) const
		{
			return detail::find_last_not_of<npos>(begin(), end(), first, last);
		}
		/** Finds right-most location of a character not from an initializer list within the string. */
		[[nodiscard]] constexpr size_type find_last_not_of(std::initializer_list<value_type> list) const noexcept
		{
			return find_last_not_of(list.begin(), list.end());
		}
		/** Finds right-most location of a character not from a c-style string within the string. */
		[[nodiscard]] constexpr size_type find_last_not_of(const value_type *str) const noexcept
		{
			return find_last_not_of(str, detail::str_length(str));
		}
		/** Finds right-most location of a character not from a raw character array within the string. */
		[[nodiscard]] constexpr size_type find_last_not_of(const value_type *str, size_type len) const noexcept
		{
			return find_last_not_of(str, str + len);
		}
		/** Finds right-most location of a character not from a range within the string. */
		template<forward_range_for<value_type> R>
		[[nodiscard]] constexpr size_type find_last_not_of(const R &r) const
		{
			return find_last_not_of(std::ranges::begin(r), std::ranges::end(r));
		}

		/** Checks if a substring is present within the string. */
		template<forward_iterator_for<value_type> Iterator>
		[[nodiscard]] constexpr bool contains(Iterator first, Iterator last) const
		{
			return find_first(first, last) != npos;
		}
		/** Checks if a substring is present within the string. */
		[[nodiscard]] constexpr bool contains(const value_type *str, size_type len) const noexcept
		{
			return contains(str, str + len);
		}
		/** Checks if a substring is present within the string. */
		[[nodiscard]] constexpr bool contains(const value_type *str) const noexcept
		{
			return contains(str, detail::str_length(str));
		}
		/** Checks if a range of characters is present within the string. */
		template<forward_range_for<value_type> R>
		[[nodiscard]] constexpr bool contains(const R &r) const
		{
			return contains(std::ranges::begin(r), std::ranges::end(r));
		}
		/** Checks if a character is present within the string. */
		[[nodiscard]] constexpr bool contains(value_type c) const noexcept { return find_first(c) != npos; }

		/** Checks if a substring is located at the start of the string. */
		template<forward_iterator_for<value_type> Iterator>
		[[nodiscard]] constexpr bool has_prefix(Iterator first, Iterator last) const
		{
			return detail::has_prefix(begin(), end(), first, last);
		}
		/** Checks if a substring is located at the start of the string. */
		[[nodiscard]] constexpr bool has_prefix(const value_type *str) const noexcept
		{
			return has_prefix(str, detail::str_length(str));
		}
		/** Checks if a substring is located at the start of the string. */
		[[nodiscard]] constexpr bool has_prefix(const value_type *str, size_type len) const noexcept
		{
			return has_prefix(str, str + len);
		}
		/** Checks if a range of characters is located at the start of the string. */
		template<forward_range_for<value_type> R>
		[[nodiscard]] constexpr bool has_prefix(const R &r) const
		{
			return has_prefix(std::ranges::begin(r), std::ranges::end(r));
		}
		/** Checks if a character is located at the start of the string. */
		[[nodiscard]] constexpr bool has_prefix(value_type c) const noexcept { return front() == c; }

		/** Checks if a substring is located at the end of the string. */
		template<forward_iterator_for<value_type> Iterator>
		[[nodiscard]] constexpr bool has_postfix(Iterator first, Iterator last) const
		{
			return detail::has_postfix(begin(), end(), first, last);
		}
		/** Checks if a substring is located at the end of the string. */
		[[nodiscard]] constexpr bool has_postfix(const value_type *str) const noexcept
		{
			return has_postfix(str, detail::str_length(str));
		}
		/** Checks if a substring is located at the end of the string. */
		[[nodiscard]] constexpr bool has_postfix(const value_type *str, size_type len) const noexcept
		{
			return has_postfix(str, str + len);
		}
		/** Checks if a range of characters is located at the end of the string. */
		template<forward_range_for<value_type> R>
		[[nodiscard]] constexpr bool has_postfix(const R &r) const
		{
			return has_postfix(std::ranges::begin(r), std::ranges::end(r));
		}
		/** Checks if a character is located at the end of the string. */
		[[nodiscard]] constexpr bool has_postfix(value_type c) const noexcept { return back() == c; }

		[[nodiscard]] friend constexpr auto operator<=>(const basic_static_string &,
														const basic_static_string &) noexcept = default;
		[[nodiscard]] friend constexpr bool operator==(const basic_static_string &, const basic_static_string &) noexcept = default;

		C value[N] = {0};
	};

	template<typename C, std::size_t N, typename T>
	constexpr void swap(basic_static_string<C, N, T> &a, basic_static_string<C, N, T> &b) noexcept
	{
		using std::swap;
		swap(a.value, b.value);
	}

	template<typename C, std::size_t N, typename T>
	[[nodiscard]] constexpr hash_t hash(const basic_static_string<C, N, T> &s) noexcept
	{
		return fnv1a(s.value, s.size());
	}
}	 // namespace sek

template<typename C, std::size_t N, typename T>
struct std::hash<sek::basic_static_string<C, N, T>>
{
	[[nodiscard]] constexpr sek::hash_t operator()(const sek::basic_static_string<C, N, T> &s) const noexcept
	{
		return sek::hash(s);
	}
};