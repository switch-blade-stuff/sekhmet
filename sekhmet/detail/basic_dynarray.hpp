//
// Created by switchblade on 2022-03-27.
//

#pragma once

#include <bit>
#include <concepts>
#include <cstdlib>
#include <limits>
#include <stdexcept>

#include "contiguous_iterator.hpp"
#include <initializer_list>

#ifdef SEK_OS_WIN
#include <malloc.h>
#endif

namespace sek
{
	/** @brief Internal utility structure used to wrap malloc/free/realloc to manage dynamic arrays of trivially copyable types.
	 * This *may* be preferable over `std::vector` since vector always does copy on resize,
	 * while `realloc` may simply expand the used heap chunk or mmap more memory. */
	template<typename T>
	requires std::is_trivially_copyable_v<T>
	class basic_dynarray
	{
	public:
		typedef T value_type;
		typedef T *pointer;
		typedef const T *const_pointer;
		typedef T &reference;
		typedef const T &const_reference;
		typedef contiguous_iterator<value_type, false> iterator;
		typedef contiguous_iterator<value_type, true> const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

	private:
		constexpr static void *align_ptr(void *p, ssize_t align) noexcept
		{
			auto int_ptr = std::bit_cast<std::intptr_t>(p);
			int_ptr = static_cast<std::intptr_t>(int_ptr + (align - 1)) & static_cast<std::intptr_t>(-align);
			return std::bit_cast<void *>(int_ptr);
		}
		constexpr static void *do_aligned_alloc(size_type n) noexcept
		{
			if constexpr (alignof(T) <= alignof(std::max_align_t))
				return malloc(sizeof(T) * n);
			else
			{
#ifdef SEK_OS_WIN
				return _aligned_malloc(sizeof(T) * n, alignof(T));
#else
				return std::aligned_alloc(alignof(T), sizeof(T) * n);
#endif
			}
		}
		constexpr static void *do_aligned_realloc(void *ptr, size_type n) noexcept
		{
			if constexpr (alignof(T) <= alignof(std::max_align_t))
				return realloc(ptr, sizeof(T) * n);
			else
			{
#ifdef SEK_OS_WIN
				return _aligned_realloc(ptr, sizeof(T) * n, alignof(T));
#else
				return align_ptr(realloc(ptr, sizeof(T) * n + alignof(T) - 1), static_cast<ssize_t>(alignof(T)));
#endif
			}
		}
		constexpr static void do_aligned_free(void *p) noexcept
		{
#ifdef SEK_OS_WIN
			if constexpr (alignof(T) > alignof(std::max_align_t))
				_aligned_free(p);
			else
#endif
				free(p);
		}

	public:
		constexpr basic_dynarray() noexcept = default;
		template<typename... Args>
		constexpr basic_dynarray(size_type n, Args &&...args) requires std::constructible_from<T, Args...> : data_size(n)
		{
			init_impl(n);
			while (n-- > 0) ::new (data_begin + n) value_type{std::forward<Args>(args)...};
		}
		template<std::forward_iterator I>
		constexpr basic_dynarray(I first, I last) requires std::constructible_from<T, std::iter_value_t<I>>
		{
			insert(end(), first, last);
		}
		template<std::ranges::forward_range R>
		constexpr basic_dynarray(const R &r) : basic_dynarray(std::ranges::begin(r), std::ranges::end(r))
		{
		}
		constexpr basic_dynarray(std::initializer_list<value_type> init_list)
			: basic_dynarray(init_list.begin(), init_list.end())
		{
		}

		constexpr basic_dynarray(const basic_dynarray &other)
		{
			init_impl(data_size = other.data_size);
			std::copy_n(other.data_begin, data_size, data_begin);
		}
		constexpr basic_dynarray &operator=(const basic_dynarray &other)
		{
			if (this != &other)
			{
				reserve(data_size = other.data_size);
				std::copy_n(other.data_begin, data_size, data_begin);
			}
			return *this;
		}
		constexpr basic_dynarray(basic_dynarray &&other) noexcept { take_data(std::forward<basic_dynarray>(other)); }
		constexpr basic_dynarray &operator=(basic_dynarray &&other) noexcept
		{
			swap(other);
			return *this;
		}

		constexpr ~basic_dynarray() { destroy_impl(); }

		constexpr void clear() { data_size = 0; }
		constexpr void shrink_to_fit() { resize_impl(data_size); }
		constexpr void reserve(size_type n)
		{
			if (n > data_capacity) resize_impl(n);
		}
		template<typename... Args>
		constexpr void resize(size_type n, Args &&...args) requires std::constructible_from<T, Args...>
		{
			resize_impl(n);
			if (n > data_size)
			{
				for (auto elem = data_begin + data_size, end = data_begin + n; elem < end; ++elem)
					::new (elem) value_type{std::forward<Args>(args)...};
			}
			data_size = n;
		}

		[[nodiscard]] constexpr size_type size() const noexcept { return data_size; }
		[[nodiscard]] constexpr size_type max_size() const noexcept { return std::numeric_limits<size_type>::max(); }
		[[nodiscard]] constexpr size_type capacity() const noexcept { return data_capacity; }
		[[nodiscard]] constexpr size_type empty() const noexcept { return size() == 0; }

		[[nodiscard]] constexpr pointer data() noexcept { return data_begin; }
		[[nodiscard]] constexpr const_pointer data() const noexcept { return data_begin; }

		[[nodiscard]] constexpr reference at(size_type i) noexcept { return data()[i]; }
		[[nodiscard]] constexpr const_reference at(size_type i) const noexcept { return data()[i]; }
		[[nodiscard]] constexpr reference operator[](size_type i) noexcept { return at(i); }
		[[nodiscard]] constexpr const_reference operator[](size_type i) const noexcept { return at(i); }

		[[nodiscard]] constexpr reference front() noexcept { return at(0); }
		[[nodiscard]] constexpr const_reference front() const noexcept { return at(0); }
		[[nodiscard]] constexpr reference back() noexcept { return at(size() - 1); }
		[[nodiscard]] constexpr const_reference back() const noexcept { return at(size() - 1); }

		[[nodiscard]] constexpr iterator begin() noexcept { return iterator{data()}; }
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return const_iterator{data()}; }
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
		[[nodiscard]] constexpr iterator end() noexcept { return iterator{data_begin + size()}; }
		[[nodiscard]] constexpr const_iterator end() const noexcept { return const_iterator{data_begin + size()}; }
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }

		template<typename... Args>
		constexpr iterator emplace(const_iterator where, Args &&...args) requires std::constructible_from<T, Args...>
		{
			return emplace_impl(where, 1, std::forward<Args>(args)...);
		}

		constexpr iterator insert(const_iterator where, size_type amount, const value_type &value)
		{
			return emplace_impl(where, amount, value);
		}
		constexpr iterator insert(const_iterator where, const value_type &value) { return emplace(where, value); }
		constexpr iterator insert(const_iterator where, value_type &&value)
		{
			return emplace(where, std::forward<value_type>(value));
		}
		template<std::forward_iterator I>
		constexpr iterator insert(const_iterator where, I first, I last)
		{
			auto insert_pos = where.get() - data_begin;

			if constexpr (std::random_access_iterator<I>)
			{
				make_space(insert_pos, static_cast<size_type>(std::distance(first, last)));

				for (auto elem = data_begin + insert_pos; first != last; ++elem, ++first)
					::new (elem) value_type(*first);
			}
			else
			{
				for (auto dest = iterator{data_begin + insert_pos}; first != last; ++first)
					dest = std::next(insert(dest, *first));
			}

			return iterator{data_begin + insert_pos};
		}

		constexpr void push_back(const value_type &value) { insert(end(), value); }
		constexpr void push_back(value_type &&value) { insert(end(), std::forward<value_type>(value)); }
		constexpr void push_front(const value_type &value) { insert(begin(), value); }
		constexpr void push_front(value_type &&value) { insert(begin(), std::forward<value_type>(value)); }

		constexpr iterator erase(const_iterator first, const_iterator last)
		{
			SEK_ASSERT(last < end() && first >= begin());
			SEK_ASSERT(first < last);

			auto amount = last - first;
			auto first_pos = first - begin(), last_pos = last - begin();
			std::move(data_begin + last_pos, data_begin + data_size, data_begin + first_pos);
			data_size -= static_cast<size_type>(amount);

			return iterator{data_begin + first_pos};
		}
		constexpr iterator erase(const_iterator where) { return erase(where, std::next(where)); }

		[[nodiscard]] constexpr auto operator<=>(const basic_dynarray &other) const noexcept
		{
			return std::lexicographical_compare_three_way(begin(), end(), other.begin(), other.end());
		}
		[[nodiscard]] constexpr bool operator==(const basic_dynarray &other) const noexcept
		{
			return std::equal(begin(), end(), other.begin(), other.end());
		}

		constexpr void swap(basic_dynarray &other) noexcept
		{
			using std::swap;
			swap(data_begin, other.data_begin);
			swap(data_size, other.data_size);
			swap(data_capacity, other.data_capacity);
		}

		friend constexpr void swap(basic_dynarray &a, basic_dynarray &b) noexcept { a.swap(b); }

	private:
		constexpr void take_data(basic_dynarray &&other) noexcept
		{
			data_begin = other.data_begin;
			data_size = other.data_size;
			data_capacity = other.data_capacity;

			other.data_begin = nullptr;
			other.data_size = 0;
			other.data_capacity = 0;
		}

		constexpr void destroy_impl()
		{
			if (data_begin) do_aligned_free(data_begin);
		}
		constexpr void init_impl(size_type n)
		{
			data_begin = static_cast<pointer>(do_aligned_alloc(n));
			if (!data_begin) [[unlikely]]
				throw std::bad_alloc();
			data_capacity = n;
		}
		constexpr void resize_impl(size_type n)
		{
			if (!n) [[unlikely]]
				destroy_impl();
			else
			{
				data_begin = static_cast<pointer>(do_aligned_realloc(static_cast<void *>(data_begin), n));
				if (!data_begin) [[unlikely]]
					throw std::bad_alloc();
				data_capacity = n;
			}
		}

		constexpr void make_space(difference_type pos, size_type amount)
		{
			auto new_size = data_size + amount;
			reserve(new_size);

			std::move_backward(data_begin + pos, data_begin + data_size, data_begin + new_size);
			data_size = new_size;
		}
		template<typename... Args>
		constexpr iterator emplace_impl(const_iterator where, size_type amount, Args &&...args)
		{
			SEK_ASSERT(where < end() && where >= begin());

			auto insert_pos = where.get() - data_begin;
			make_space(insert_pos, amount);

			for (auto elem = data_begin + insert_pos, last = elem + amount; elem != last; ++elem)
				::new (elem) value_type{std::forward<Args>(args)...};
			return iterator{data_begin + insert_pos};
		}

		pointer data_begin = nullptr;
		size_type data_size = 0;
		size_type data_capacity = 0;
	};
}	 // namespace sek