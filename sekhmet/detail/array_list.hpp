/*
 * Created by switchblade on 2021-11-30
 */

#pragma once

#include <iterator>

#include "alloc_util.hpp"
#include "assert.hpp"
#include "ebo_base_helper.hpp"

namespace sek
{
	namespace detail
	{
		template<typename T, typename Alloc>
		using list_node_allocator_type = rebind_alloc_t<Alloc, T *>;
		template<typename T, typename Alloc>
		using list_node_allocator_base = ebo_base_helper<list_node_allocator_type<T, Alloc>>;
	}	 // namespace detail

	/** Dynamic array of pointers to objects.
	 * @tparam T Type of objects stored in the list.
	 * @tparam Alloc Allocator used for the list. */
	template<typename T, typename Alloc = std::allocator<T>>
	class array_list : ebo_base_helper<Alloc>, detail::list_node_allocator_base<T, Alloc>
	{
	public:
		typedef Alloc allocator_type;
		typedef detail::list_node_allocator_type<T, Alloc> node_allocator_type;

		typedef T value_type;
		typedef T *pointer;
		typedef const T *const_pointer;
		typedef T &reference;
		typedef const T &const_reference;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

	private:
		using node_ebo_base = detail::list_node_allocator_base<T, Alloc>;
		using node_alloc_traits = std::allocator_traits<node_allocator_type>;
		using value_ebo_base = ebo_base_helper<Alloc>;
		using value_alloc_traits = std::allocator_traits<Alloc>;

		using list_node = T *;

		template<bool IsConst>
		class list_iterator
		{
			template<bool B>
			friend class list_iterator;

			friend class array_list;

		public:
			typedef T value_type;
			typedef std::conditional_t<IsConst, const T, T> *pointer;
			typedef std::conditional_t<IsConst, const T, T> &reference;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;
			typedef std::random_access_iterator_tag iterator_category;

		private:
			using node_pointer = list_node *;

			constexpr explicit list_iterator(node_pointer node_ptr) noexcept : m_ptr(node_ptr) {}

		public:
			constexpr list_iterator() noexcept = default;
			template<bool OtherConst, typename = std::enable_if_t<IsConst && !OtherConst>>
			constexpr list_iterator(const list_iterator<OtherConst> &other) noexcept : m_ptr(other.m_ptr)
			{
			}

			constexpr list_iterator operator++(int) noexcept
			{
				auto temp = *this;
				m_ptr++;
				return temp;
			}
			constexpr list_iterator operator--(int) noexcept
			{
				auto temp = *this;
				m_ptr--;
				return temp;
			}
			constexpr list_iterator &operator++() noexcept
			{
				m_ptr++;
				return *this;
			}
			constexpr list_iterator &operator--() noexcept
			{
				m_ptr--;
				return *this;
			}
			constexpr list_iterator &operator+=(difference_type n) noexcept
			{
				m_ptr += n;
				return *this;
			}
			constexpr list_iterator &operator-=(difference_type n) noexcept
			{
				m_ptr -= n;
				return *this;
			}

			[[nodiscard]] constexpr list_iterator operator+(difference_type n) const noexcept
			{
				return list_iterator{m_ptr + n};
			}
			[[nodiscard]] constexpr list_iterator operator-(difference_type n) const noexcept
			{
				return list_iterator{m_ptr - n};
			}
			[[nodiscard]] constexpr difference_type operator-(const list_iterator &other) const noexcept
			{
				return m_ptr - other.m_ptr;
			}

			[[nodiscard]] friend constexpr list_iterator operator+(difference_type n, const list_iterator &iter) noexcept
			{
				return iter + n;
			}

			/** Returns pointer to the target element. */
			[[nodiscard]] constexpr pointer get() const noexcept { return *m_ptr; }
			/** @copydoc value */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
			/** Returns reference to the target element. */
			[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }
			/** Returns reference to the element located at the specified
			 * offset. */
			[[nodiscard]] constexpr reference operator[](difference_type n) const noexcept { return *(*this + n); }

			[[nodiscard]] constexpr auto operator<=>(const list_iterator &) const noexcept = default;
			[[nodiscard]] constexpr bool operator==(const list_iterator &) const noexcept = default;

			friend constexpr void swap(list_iterator &a, list_iterator &b) noexcept
			{
				using std::swap;
				swap(a.m_ptr, b.m_ptr);
			}

		private:
			/** Pointer into the list's internal node array. */
			node_pointer m_ptr;
		};

	public:
		typedef list_iterator<false> iterator;
		typedef list_iterator<true> const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

		class node_handle : ebo_base_helper<allocator_type>
		{
			friend class array_list;

			using ebo_base = ebo_base_helper<allocator_type>;

			constexpr node_handle(list_node node, const allocator_type &alloc) noexcept : ebo_base(alloc), node(node) {}

		public:
			node_handle(const node_handle &) = delete;
			node_handle &operator=(const node_handle &) = delete;

			constexpr node_handle() noexcept = default;
			constexpr ~node_handle() { destroy(); }

			constexpr node_handle(node_handle &&other) noexcept
				: ebo_base(std::move(other.get_allocator())), node(other.reset())
			{
			}
			constexpr node_handle &operator=(node_handle &&other) noexcept
			{
				destroy();
				get_allocator() = std::move(other.get_allocator());
				node = other.reset();
				return *this;
			}

			/** Checks if the handle points to a valid node. */
			[[nodiscard]] constexpr bool empty() const noexcept { return node == nullptr; }
			/** Returns reference to the value the handle points to.
			 * @warning If node is empty, causes nullptr dereference. */
			[[nodiscard]] constexpr reference value() const noexcept { return *node; }

			[[nodiscard]] constexpr allocator_type &get_allocator() noexcept { return *ebo_base::get(); }
			[[nodiscard]] constexpr const allocator_type &get_allocator() const noexcept { return *ebo_base::get(); }

			constexpr void swap(node_handle &other) noexcept
			{
				using std::swap;
				ebo_base::swap(other);
				swap(node, other.node);
			}

			friend constexpr void swap(node_handle &a, node_handle &b) noexcept { a.swap(b); }

		private:
			constexpr void destroy()
			{
				if (node)
				{
					std::destroy_at(node);
					get_allocator().deallocate(node, 1);
				}
			}

			constexpr list_node reset() noexcept { return std::exchange(node, nullptr); }

			list_node node = nullptr;
		};

	public:
		constexpr array_list() noexcept(detail::nothrow_alloc_default_construct<allocator_type, node_allocator_type>) = default;

		/** Constructs the list with the specified allocators.
		 * @param value_alloc Allocator used to allocate list's elements.
		 * @param node_alloc Allocator used to allocate list's internal node array. */
		constexpr explicit array_list(const allocator_type &value_alloc, const node_allocator_type &node_alloc) noexcept(
			detail::nothrow_alloc_copy_construct<allocator_type, node_allocator_type>)
			: value_ebo_base(value_alloc), node_ebo_base(node_alloc)
		{
		}

		/** Constructs the list with the specified capacity and allocators.
		 * @param n Capacity of the list.
		 * @param value_alloc Allocator used to allocate list's elements.
		 * @param node_alloc Allocator used to allocate list's internal node array. */
		constexpr explicit array_list(size_type n,
									  const allocator_type &value_alloc = allocator_type{},
									  const node_allocator_type &node_alloc = node_allocator_type{})
			: array_list(value_alloc, node_alloc)
		{
			init_impl(n);
		}
		/** Constructs the list with the specified size and allocators.
		 * @param n Size of the list.
		 * @param value Value used to initialize elements of the list.
		 * @param value_alloc Allocator used to allocate list's elements.
		 * @param node_alloc Allocator used to allocate list's internal node array. */
		constexpr explicit array_list(size_type n,
									  const value_type &value,
									  const allocator_type &value_alloc = allocator_type{},
									  const node_allocator_type &node_alloc = node_allocator_type{})
			: array_list(n, value_alloc, node_alloc)
		{
			push_back(n, value);
		}
		/** Constructs the list using specified allocators and a sequence of elements.
		 * @param first Start of the source sequence.
		 * @param last End of the source sequence.
		 * @param value_alloc Allocator used to allocate list's elements.
		 * @param node_alloc Allocator used to allocate list's internal node array. */
		template<std::forward_iterator Iterator>
		constexpr array_list(Iterator first,
							 Iterator last,
							 const allocator_type &value_alloc = allocator_type{},
							 const node_allocator_type &node_alloc = node_allocator_type{})
			: array_list(value_alloc, node_alloc)
		{
			push_back(first, last);
		}
		/** Constructs the list using specified allocators and an initializer list.
		 * @param il List containing elements of the list.
		 * @param value_alloc Allocator used to allocate list's elements.
		 * @param node_alloc Allocator used to allocate list's internal node array. */
		template<typename U>
		constexpr array_list(std::initializer_list<U> il,
							 const allocator_type &value_alloc = allocator_type{},
							 const node_allocator_type &node_alloc = node_allocator_type{})
			: array_list(il.begin(), il.end(), value_alloc, node_alloc)
		{
		}

		/** Copy-constructs the list. Both allocators are copied via `select_on_container_copy_construction`.
		 * @param other List to copy elements and allocators from. */
		constexpr array_list(const array_list &other)
			: array_list(other,
						 detail::make_alloc_copy(other.get_allocator()),
						 detail::make_alloc_copy(other.get_node_allocator()))
		{
		}
		/** Copy-constructs the list. Node allocator is copied via `select_on_container_copy_construction`.
		 * @param other List to copy elements and node allocator from.
		 * @param value_alloc Allocator used to allocate list's elements. */
		constexpr array_list(const array_list &other, const allocator_type &value_alloc)
			: array_list(other, value_alloc, detail::make_alloc_copy(other.get_node_allocator()))
		{
		}
		/** Copy-constructs the list.
		 * @param other List to copy elements from.
		 * @param value_alloc Allocator used to allocate list's elements.
		 * @param node_alloc Allocator used to allocate list's internal node array. */
		constexpr array_list(const array_list &other, const allocator_type &value_alloc, const node_allocator_type &node_alloc)
			: array_list(other.size(), value_alloc, node_alloc)
		{
			push_back(other.begin(), other.end());
		}

		/** Move-constructs the list. Both allocators are move-constructed.
		 * @param other List to move elements and allocators from. */
		constexpr array_list(array_list &&other) noexcept(detail::nothrow_alloc_move_construct<allocator_type, node_allocator_type>)
			: value_ebo_base(std::move(other.get_allocator())), node_ebo_base(std::move(other.get_node_allocator()))
		{
			take_data(std::move(other));
		}
		/** Move-constructs the list. Node allocator is move-constructed.
		 * @param other List to move elements and node allocator from.
		 * @param value_alloc Allocator used to allocate list's elements. */
		constexpr array_list(array_list &&other, const allocator_type &value_alloc) noexcept(
			detail::nothrow_alloc_copy_move_transfer<allocator_type, node_allocator_type>)
			: value_ebo_base(value_alloc), node_ebo_base(std::move(other.get_node_allocator()))
		{
			if (detail::alloc_eq(get_allocator(), other.get_allocator()))
				take_data(std::move(other));
			else
				move_values(std::move(other));
		}
		/** Copy-constructs the list.
		 * @param other List to copy elements from.
		 * @param value_alloc Allocator used to allocate list's elements.
		 * @param node_alloc Allocator used to allocate list's internal node array. */
		constexpr array_list(array_list &&other, const allocator_type &value_alloc, const node_allocator_type &node_alloc) noexcept(
			detail::nothrow_alloc_copy_transfer<allocator_type, node_allocator_type>)
			: array_list(value_alloc, node_alloc)
		{
			if (detail::alloc_eq(get_allocator(), other.get_allocator()) &&
				detail::alloc_eq(get_node_allocator(), other.get_node_allocator()))
				take_data(std::move(other));
			else
				move_values(std::move(other));
		}

		/** Copy-assigns the list.
		 * @param other List to copy elements from. */
		constexpr array_list &operator=(const array_list &other)
		{
			if (this != &other) copy_assign_impl(other);
			return *this;
		}
		/** Move-assigns the list.
		 * @param other List to move elements from. */
		constexpr array_list &
			operator=(array_list &&other) noexcept(detail::nothrow_alloc_move_assign<allocator_type, node_allocator_type>)
		{
			move_assign_impl(std::move(other));
			return *this;
		}

		constexpr ~array_list()
		{
			clear();
			get_node_allocator().deallocate(m_begin, m_capacity);
		}

		/** Returns iterator to the start of the list. */
		[[nodiscard]] constexpr iterator begin() noexcept { return iterator{m_begin}; }
		/** Returns iterator to the end of the list. */
		[[nodiscard]] constexpr iterator end() noexcept { return iterator{m_end}; }
		/** Returns const iterator to the start of the list. */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return const_iterator{m_begin}; }
		/** Returns const iterator to the end of the list. */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return const_iterator{m_end}; }
		/** @copydoc cbegin */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return cbegin(); }
		/** @copydoc cend */
		[[nodiscard]] constexpr const_iterator end() const noexcept { return cend(); }
		/** Returns reverse iterator to the end of the list. */
		[[nodiscard]] constexpr reverse_iterator rbegin() noexcept { return reverse_iterator{end()}; }
		/** Returns reverse iterator to the start of the list. */
		[[nodiscard]] constexpr reverse_iterator rend() noexcept { return reverse_iterator{begin()}; }
		/** Returns const reverse iterator to the end of the list. */
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept
		{
			return const_reverse_iterator{end()};
		}
		/** Returns const reverse iterator to the start of the list. */
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept
		{
			return const_reverse_iterator{begin()};
		}
		/** @copydoc crbegin */
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return crend(); }
		/** @copydoc crend */
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return crbegin(); }

		/** Returns pointer to the internal pointer array. */
		[[nodiscard]] constexpr pointer *data() noexcept { return m_begin; }
		/** @copydoc data */
		[[nodiscard]] constexpr const const_pointer *data() const noexcept { return m_begin; }

		/** Returns reference to the element at the specified index. */
		[[nodiscard]] constexpr reference at(size_type i) noexcept
		{
			SEK_ASSERT(i < size());
			return *(m_begin[i]);
		}
		/** @copydoc at */
		[[nodiscard]] constexpr const_reference at(size_type i) const noexcept
		{
			SEK_ASSERT(i < size());
			return *(m_begin[i]);
		}
		/** @copydoc at */
		[[nodiscard]] constexpr reference operator[](size_type i) noexcept { return at(i); }
		/** @copydoc at */
		[[nodiscard]] constexpr const_reference operator[](size_type i) const noexcept { return at(i); }
		/** Returns reference to the start of the list. */
		[[nodiscard]] constexpr reference front() noexcept { return at(0); }
		/** @copydoc front */
		[[nodiscard]] constexpr const_reference front() const noexcept { return at(0); }
		/** Returns reference to the end of the list. */
		[[nodiscard]] constexpr reference back() noexcept { return at(size() - 1); }
		/** @copydoc back */
		[[nodiscard]] constexpr const_reference back() const noexcept { return at(size() - 1); }

		/** Removes all elements from the list. Does not reserve the internal array. */
		constexpr void clear()
		{
			for (auto node = m_begin; node != m_end; ++node)
			{
				std::destroy_at(*node);
				get_allocator().deallocate(*node, 1);
			}
			m_end = m_begin;
		}
		/** Removes all elements from the list and destroys the internal node array. */
		constexpr void purge()
		{
			clear();
			get_node_allocator().deallocate(m_begin, m_capacity);
			m_end = m_begin = nullptr;
			m_capacity = 0;
		}

		/** Shrinks the list to only occupy the required amount of space for it's size. */
		constexpr void narrow() { resize_impl(size()); }
		/** @copydoc narrow */
		constexpr void shrink_to_fit() { narrow(); }

		/** Resizes internal node array to fit at least n elements. */
		constexpr void reserve(size_type n)
		{
			if (n > capacity()) [[likely]]
				resize_impl(n);
		}
		/** Resizes the list to store n elements. If new size is less than the current one,
		 * excess elements are destroyed but the internal array is not resized.
		 * @param n New size of the list.
		 * @param args Arguments used to construct the new elements. */
		template<typename... Args>
		constexpr void resize(size_type n, Args &&...args)
		{
			reserve(n);
			if (n < size())
				erase_impl(m_begin + n, m_end);
			else
				emplace_impl(end(), n - size(), std::forward<Args>(args)...);
		}

		/** Emplaces a single element at the specified position.
		 * @param where Iterator to the target position.
		 * @param args Arguments used to construct the new element.
		 * @return Iterator to the emplaced element. */
		template<typename... Args>
		constexpr iterator emplace(const_iterator where, Args &&...args)
		{
			return emplace_impl(where, std::forward<Args>(args)...);
		}
		/** Emplaces a single element at the end of the list.
		 * @param args Arguments used to construct the new element.
		 * @return Iterator to the emplaced element. */
		template<typename... Args>
		constexpr iterator emplace_back(Args &&...args)
		{
			return emplace(cend(), std::forward<Args>(args)...);
		}
		/** Emplaces a single element at the start of the list.
		 * @param args Arguments used to construct the new element.
		 * @return Iterator to the emplaced element. */
		template<typename... Args>
		constexpr iterator emplace_front(Args &&...args)
		{
			return emplace(cbegin(), std::forward<Args>(args)...);
		}

		/** Inserts a single element at the specified position using the copy constructor.
		 * @param where Iterator to the target position.
		 * @param value Value to insert.
		 * @return Iterator to the inserted element. */
		constexpr iterator insert(const_iterator where, const_reference value) { return insert(where, 1, value); }
		/** Inserts a multiple elements at the specified position using the copy constructor.
		 * @param where Iterator to the target position.
		 * @param amount Amount of elements to insert.
		 * @param value Value to insert.
		 * @return Iterator to the inserted element. */
		constexpr iterator insert(const_iterator where, size_type amount, const_reference value)
		{
			return emplace_impl(where, amount, value);
		}
		/** Inserts a single element at the specified position using the move constructor.
		 * @param where Iterator to the target position.
		 * @param value Value to insert.
		 * @return Iterator to the inserted element. */
		constexpr iterator insert(const_iterator where, T &&value)
		{
			return emplace_impl(where, 1, std::forward<T>(value));
		}
		/** Inserts a sequence of elements at the specified position.
		 * @param where Iterator to the target position.
		 * @param il Initializer list containing elements to be inserted.
		 * @return Iterator to the first element of the inserted sequence. */
		template<typename U>
		constexpr iterator insert(const_iterator where, std::initializer_list<U> il)
		{
			return insert(where, il.begin(), il.end());
		}
		/** Inserts a sequence of elements at the specified position.
		 * @param where Iterator to the target position.
		 * @param first Start of the source sequence.
		 * @param last End of the source sequence.
		 * @return Iterator to the first element of the inserted sequence. */
		template<std::forward_iterator Iterator>
		constexpr iterator insert(const_iterator where, Iterator first, Iterator last)
		{
			return insert_range_impl(where, first, last);
		}

		/** Inserts a single element at the end of the list using the copy constructor.
		 * @param value Value to insert.
		 * @return Iterator to the inserted element. */
		constexpr iterator push_back(const_reference value) { return insert(cend(), value); }
		/** Inserts a multiple elements at the end of the list using the copy constructor.
		 * @param amount Amount of elements to insert.
		 * @param value Value to insert.
		 * @return Iterator to the inserted element. */
		constexpr iterator push_back(size_type amount, const_reference value) { return insert(cend(), amount, value); }
		/** Inserts a single element at the end of the list using the copy constructor.
		 * @param value Value to insert.
		 * @return Iterator to the inserted element. */
		constexpr iterator push_back(T &&value) { return insert(cend(), std::forward<T>(value)); }
		/** Inserts a sequence of elements at the end of the list.
		 * @param il Initializer list containing elements to be inserted.
		 * @return Iterator to the first element of the inserted sequence. */
		template<typename U>
		constexpr iterator push_back(std::initializer_list<U> il)
		{
			return push_back(il.begin(), il.end());
		}
		/** Inserts a sequence of elements at the end of the list.
		 * @param first Start of the source sequence.
		 * @param last End of the source sequence.
		 * @return Iterator to the first element of the inserted sequence. */
		template<std::forward_iterator Iterator>
		constexpr iterator push_back(Iterator first, Iterator last)
		{
			return insert(cend(), first, last);
		}

		/** Inserts a single element at the start of the list using the copy constructor.
		 * @param value Value to insert.
		 * @return Iterator to the inserted element. */
		constexpr iterator push_front(const_reference value) { return insert(cbegin(), value); }
		/** Inserts a multiple elements at the start of the list using the copy constructor.
		 * @param amount Amount of elements to insert.
		 * @param value Value to insert.
		 * @return Iterator to the inserted element. */
		constexpr iterator push_front(size_type amount, const_reference value)
		{
			return insert(cbegin(), amount, value);
		}
		/** Inserts a single element at the start of the list using the copy constructor.
		 * @param value Value to insert.
		 * @return Iterator to the inserted element. */
		constexpr iterator push_front(T &&value) { return insert(cbegin(), std::forward<T>(value)); }
		/** Inserts a sequence of elements at the start of the list.
		 * @param il Initializer list containing elements to be inserted.
		 * @return Iterator to the first element of the inserted sequence. */
		template<typename U>
		constexpr iterator push_front(std::initializer_list<U> il)
		{
			return insert(cbegin(), il.begin(), il.end());
		}
		/** Inserts a sequence of elements at the start of the list.
		 * @param first Start of the source sequence.
		 * @param last End of the source sequence.
		 * @return Iterator to the first element of the inserted sequence. */
		template<std::forward_iterator Iterator>
		constexpr iterator push_front(Iterator first, Iterator last)
		{
			return insert(cbegin(), first, last);
		}

		/** Removes a single element at the specified position.
		 * @param where Iterator to the target element.
		 * @return Iterator to the element after the erased one. */
		constexpr iterator erase(const_iterator where) { return erase(where, where + 1); }
		/** Removes all elements between [first, last) from the list.
		 * @param first Start of the target sequence.
		 * @param last End of the target sequence.
		 * @return Iterator to the element after the erased sequence. */
		constexpr iterator erase(const_iterator first, const_iterator last)
		{
			if (first == last) [[unlikely]]
				return begin() + std::distance(first, last);
			else
				return iterator{erase_impl(first.m_ptr, last.m_ptr)};
		}

		/** Extracts a single element at the specified position.
		 * @param where Iterator to the target element.
		 * @return Node handle to the extracted element. */
		constexpr node_handle extract(const_iterator where)
		{
			SEK_ASSERT(where >= begin() && where <= end());

			auto extract_pos = where - begin();
			auto result = node_handle{m_begin[extract_pos], get_allocator()};
			m_end = std::move(m_begin + extract_pos + 1, m_end, m_begin + extract_pos);

			return result;
		}
		/** Inserts a node element at the specified position.
		 * @param where Iterator to the target element.
		 * @param node Node to be inserted.
		 * @return Iterator to the inserted element. */
		constexpr iterator insert(const_iterator where, node_handle &&node)
		{
			SEK_ASSERT(where >= begin() && where <= end());

			auto insert_pos = where.m_ptr - data();
			make_space(insert_pos, 1);
			m_begin[insert_pos] = node.reset();
			return iterator{m_begin + insert_pos};
		}

		/** Returns current size of the list. */
		[[nodiscard]] constexpr size_type size() const noexcept
		{
			return static_cast<size_type>(m_end - m_begin);
		}
		/** Returns max size of the list. */
		[[nodiscard]] constexpr size_type max_size() const noexcept
		{
			constexpr auto absolute_max = static_cast<size_type>(std::numeric_limits<difference_type>::max());
			const auto alloc_max = static_cast<size_type>(value_alloc_traits::max_size(get_allocator()));
			return std::min(absolute_max, alloc_max) / sizeof(value_type);
		}
		/** Returns current capacity of the list. */
		[[nodiscard]] constexpr size_type capacity() const noexcept { return m_capacity; }
		/** Checks if the list is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_begin == m_end; }

		[[nodiscard]] constexpr allocator_type &get_allocator() noexcept { return *value_ebo_base::get(); }
		[[nodiscard]] constexpr const allocator_type &get_allocator() const noexcept { return *value_ebo_base::get(); }
		[[nodiscard]] constexpr node_allocator_type &get_node_allocator() noexcept { return *node_ebo_base::get(); }
		[[nodiscard]] constexpr const node_allocator_type &get_node_allocator() const noexcept
		{
			return *node_ebo_base::get();
		}

		[[nodiscard]] constexpr auto operator<=>(const array_list &other) const noexcept
		{
			return std::lexicographical_compare_three_way(begin(), end(), other.begin(), other.end());
		}
		[[nodiscard]] constexpr bool operator==(const array_list &other) const noexcept
		{
			return std::equal(begin(), end(), other.begin(), other.end());
		}

		constexpr void swap(array_list &other) noexcept
		{
			detail::alloc_assert_swap(get_allocator(), other.get_allocator());
			detail::alloc_assert_swap(get_node_allocator(), other.get_node_allocator());

			swap_data(other);
			detail::alloc_swap(get_allocator(), other.get_allocator());
			detail::alloc_swap(get_node_allocator(), other.get_node_allocator());
		}

		friend constexpr void swap(array_list &a, array_list &b) noexcept { a.swap(b); }

	private:
		[[nodiscard]] constexpr size_type next_capacity() const noexcept { return m_capacity * 2; }

		constexpr void take_data(array_list &&other) noexcept
		{
			m_begin = std::exchange(other.m_begin, nullptr);
			m_end = std::exchange(other.m_end, nullptr);
			m_capacity = std::exchange(other.m_capacity, 0);
		}
		constexpr void swap_data(array_list &other) noexcept
		{
			using std::swap;
			swap(m_begin, other.m_begin);
			swap(m_end, other.m_end);
			swap(m_capacity, other.m_capacity);
		}
		constexpr void move_values(array_list &&other)
		{
			auto new_size = other.size();
			reserve(new_size);

			for (size_type i = new_size; --i > 0;)
			{
				auto **dst_node = m_begin + i;
				auto **src_node = other.m_begin + i;
				if (dst_node < m_end) /* Occupied nodes must not be null. */
					**dst_node = std::move(**src_node);
				else
					*dst_node = make_node(std::move(**src_node));
			}
			auto new_end = m_begin + new_size;
			if (new_end < m_end) erase_impl(new_end, m_end);
			m_end = new_end;
			other.clear();
		}
		constexpr void move_assign_impl(array_list &&other)
		{
			if ((value_alloc_traits::propagate_on_container_move_assignment::value ||
				 detail::alloc_eq(get_allocator(), other.get_allocator())) &&
				(node_alloc_traits::propagate_on_container_move_assignment::value ||
				 detail::alloc_eq(get_node_allocator(), other.get_node_allocator())))
			{
				array_list tmp{get_allocator(), get_node_allocator()};
				swap_data(other);
				tmp.swap_data(other);
				detail::alloc_move_assign(get_allocator(), other.get_allocator());
				detail::alloc_move_assign(get_node_allocator(), other.get_node_allocator());
			}
			else
				move_values(std::move(other));
		}
		constexpr void copy_assign_impl(const array_list &other)
		{

			if constexpr (value_alloc_traits::propagate_on_container_copy_assignment::value ||
						  !value_alloc_traits::is_always_equal::value ||
						  node_alloc_traits::propagate_on_container_copy_assignment::value ||
						  !node_alloc_traits::is_always_equal::value)
			{
				purge();
				alloc_copy_assign(get_allocator(), other.get_allocator());
				alloc_copy_assign(get_node_allocator(), other.get_node_allocator());
			}

			if (auto new_size = other.size(); !new_size)
				clear();
			else
			{
				auto old_size = size();
				reserve(new_size);

				if (new_size < old_size)
				{
					std::copy_n(other.begin(), new_size, begin());
					erase_impl(m_begin + new_size, m_end);
				}
				else
				{
					std::copy_n(other.begin(), old_size, begin());
					push_back(const_iterator{m_begin + old_size}, other.cend());
				}
			}
		}

		constexpr void init_impl(size_type new_capacity)
		{
			if (new_capacity)
			{
				m_begin = get_node_allocator().allocate(new_capacity);
				m_capacity = new_capacity;
				m_end = m_begin;
			}
		}
		constexpr void resize_impl(size_type new_capacity)
		{
			auto old_capacity = m_capacity;
			auto old_data = m_begin;
			auto old_end = m_end;

			m_begin = get_node_allocator().allocate(m_capacity = new_capacity);
			m_end = std::copy(old_data, old_end, m_begin);
			get_node_allocator().deallocate(old_data, old_capacity);
		}

		template<typename... Args>
		constexpr list_node make_node(Args &&...args)
		{
			return std::construct_at(get_allocator().allocate(1), std::forward<Args>(args)...);
		}
		constexpr void destroy_node(list_node node)
		{
			std::destroy_at(node);
			get_allocator().deallocate(node, 1);
		}

		constexpr void make_space(difference_type pos, size_type amount)
		{
			auto new_size = size() + amount;
			if (new_size > capacity()) [[unlikely]]
				resize_impl(math::max(new_size, next_capacity()));

			auto new_end = m_begin + new_size;
			std::move_backward(m_begin + pos, m_end, new_end);
			m_end = new_end;
		}
		template<typename... Args>
		constexpr iterator emplace_impl(const_iterator where, size_type amount, Args &&...args)
		{
			SEK_ASSERT(where >= begin() && where <= end());

			auto insert_pos = where - begin();
			make_space(insert_pos, amount);

			for (auto elem = m_begin + insert_pos, last = elem + amount; elem != last; ++elem)
				*elem = make_node(std::forward<Args>(args)...);
			return begin() + insert_pos;
		}
		template<std::random_access_iterator Iterator>
		constexpr iterator insert_range_impl(const_iterator where, Iterator first, Iterator last)
		{
			SEK_ASSERT(where >= begin() && where <= end());
			SEK_ASSERT(first < last);

			auto insert_pos = where - begin();
			make_space(insert_pos, static_cast<size_type>(std::distance(first, last)));

			for (auto elem = m_begin + insert_pos; first != last; ++elem, ++first) *elem = make_node(*first);
			return begin() + insert_pos;
		}
		template<std::forward_iterator Iterator>
		constexpr iterator insert_range_impl(const_iterator where, Iterator first, Iterator last)
		{
			SEK_ASSERT(where >= begin() && where <= end());

			auto insert_pos = where - begin();
			while (first != last) emplace_impl(where, *first++);
			return begin() + insert_pos;
		}

		constexpr list_node *erase_impl(list_node *first, list_node *last)
		{
			SEK_ASSERT(first >= m_begin && last <= m_end);

			for (; first < last; ++first) destroy_node(*first);
			m_end = std::move(last, m_end, first);
			return first;
		}

		/** Pointer to the start of list's data array. */
		list_node *m_begin = nullptr;
		/** Pointer to the end of list's data array. */
		list_node *m_end = nullptr;
		/** Capacity of the list. */
		size_type m_capacity = 0;
	};

	template<std::forward_iterator Iterator>
	array_list(Iterator, Iterator) -> array_list<typename std::iterator_traits<Iterator>::value_type>;
	template<std::forward_iterator Iterator, typename A0>
	array_list(Iterator, Iterator, const A0 &) -> array_list<typename std::iterator_traits<Iterator>::value_type, A0>;
	template<std::forward_iterator Iterator, typename A0, typename A1>
	array_list(Iterator, Iterator, const A0 &, const A1 &)
		-> array_list<typename std::iterator_traits<Iterator>::value_type, A0>;

	template<typename T>
	array_list(std::initializer_list<T>) -> array_list<T>;
	template<typename T, typename A0>
	array_list(std::initializer_list<T>, const A0 &) -> array_list<T, A0>;
	template<typename T, typename A0, typename A1>
	array_list(std::initializer_list<T>, const A0 &, const A1 &) -> array_list<T, A0>;
}	 // namespace sek
