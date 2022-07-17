/*
 * Created by switchblade on 14/07/22
 */

#pragma once

#include "../type_info.hpp"
#include "entity.hpp"

namespace sek::engine
{
	/** @brief Helper type used to obtain traits of a component type.
	 *
	 * Component traits must contain a compile-time constant of type `std::size_t` named `page_size`, specifying
	 * size of allocation pages used by component pools.
	 *
	 * Optionally, traits may define an `is_fixed` typedef which, if present, will prevent components of this type
	 * from being sorted either by a component pool or ordering queries. */
	template<typename T>
	struct component_traits
	{
		constexpr static std::size_t page_size = 1024;
	};

	namespace detail
	{
		// clang-format off
		template<typename T, typename A>
		using component_entity_set = basic_entity_set<
		    typename std::allocator_traits<A>::template rebind_alloc<entity>,
		    requires{ typename component_traits<T>::is_fixed; }>;
		// clang-format on

		template<typename T, typename A, bool Empty>
		struct component_pool_impl : public detail::component_entity_set<T, A>, ebo_base_helper<A>
		{
			using alloc_base = ebo_base_helper<A>;
			using alloc_traits = std::allocator_traits<A>;
			using pages_t = std::vector<T *, typename alloc_traits::template rebind_alloc<T *>>;
			using base_set = detail::component_entity_set<T, A>;
			using set_iterator = typename base_set::iterator;

			constexpr static std::size_t page_size = component_traits<T>::page_size;

			[[nodiscard]] constexpr static std::size_t page_idx(std::size_t i) noexcept { return i / page_size; }
			[[nodiscard]] constexpr static std::size_t page_off(std::size_t i) noexcept { return i % page_size; }

			constexpr component_pool_impl() noexcept = default;
			constexpr ~component_pool_impl() { release_pages(); }

			// clang-format off
			constexpr component_pool_impl(component_pool_impl &&other)
				noexcept(std::is_nothrow_move_constructible_v<base_set> &&
						 std::is_nothrow_move_constructible_v<pages_t>)
				: base_set(std::move(other)), m_pages(std::move(other.m_pages))
			{
				SEK_ASSERT(alloc_traits::propagate_on_container_move_assignment::value ||
						   sek::detail::alloc_eq(get_allocator(), other.get_allocator()));
			}
			constexpr component_pool_impl &operator=(component_pool_impl &&other)
				noexcept(std::is_nothrow_move_assignable_v<base_set>&&
						 std::is_nothrow_move_assignable_v<pages_t>)
			{
				SEK_ASSERT(alloc_traits::propagate_on_container_move_assignment::value ||
						   sek::detail::alloc_eq(get_allocator(), other.get_allocator()));

				base_set::operator=(std::move(other));
				m_pages = std::move(other.m_pages);

				return *this;
			}
			// clang-format on

			constexpr explicit component_pool_impl(const A &alloc) : alloc_base(alloc) {}
			constexpr component_pool_impl(std::size_t n, const A &alloc) : base_set(n), alloc_base(alloc)
			{
				reserve_impl(n);
			}

			constexpr component_pool_impl(component_pool_impl &&other, const A &alloc)
				: base_set(std::move(other)), alloc_base(alloc), m_pages(std::move(other.m_pages))
			{
				SEK_ASSERT(alloc_traits::propagate_on_container_move_assignment::value ||
						   sek::detail::alloc_eq(get_allocator(), other.get_allocator()));
			}

			constexpr void reserve_impl(std::size_t n) { m_pages.resize(page_idx(n), nullptr); }
			constexpr void purge_impl()
			{
				release_pages();
				m_pages.clear();
				m_pages.shrink_to_fit();
			}

			template<typename... Args>
			set_iterator emplace_impl(entity e, Args &&...args)
			{
				const auto pos = base_set::insert(e);
				try
				{
					std::construct_at(alloc_component(pos.offset()), std::forward<Args>(args)...);
				}
				catch (...)
				{
					base_set::erase(pos);
					throw;
				}
				return pos;
			}
			template<typename... Args>
			set_iterator push_impl(entity e, Args &&...args)
			{
				const auto pos = base_set::push_back(e);
				try
				{
					std::construct_at(alloc_component(pos.offset()), std::forward<Args>(args)...);
				}
				catch (...)
				{
					base_set::erase(pos);
					throw;
				}
				return pos;
			}

			set_iterator erase_impl(set_iterator where)
			{
				auto *last = component_ptr(base_set::size() - 1);
				component_ref(where.offset()) = std::move(*last);
				std::destroy_at(last);

				return base_set::erase(where);
			}

			void dense_move(std::size_t from, std::size_t to) final
			{
				auto *from_ptr = component_ptr(from);
				component_ref(to) = std::move(*from_ptr);
				std::destroy_at(from_ptr);
			}
			void dense_swap(std::size_t lhs, std::size_t rhs) final
			{
				using std::swap;
				swap(component_ref(lhs), component_ref(rhs));
			}

			[[nodiscard]] constexpr auto &get_allocator() noexcept { return *alloc_base::get(); }
			[[nodiscard]] constexpr auto &get_allocator() const noexcept { return *alloc_base::get(); }

			[[nodiscard]] constexpr T *alloc_page() { return alloc_traits::allocate(get_allocator(), page_size); }
			constexpr void dealloc_page(T *ptr) { alloc_traits::deallocate(get_allocator(), ptr, page_size); }
			constexpr void release_pages()
			{
				for (auto page : m_pages)
					if (page != nullptr) dealloc_page(page);
			}

			[[nodiscard]] constexpr T *const *data() const noexcept { return m_pages.data(); }

			[[nodiscard]] constexpr T *component_ptr(std::size_t i) const noexcept
			{
				const auto idx = page_idx(i);
				const auto off = page_off(i);

				if (idx >= m_pages.size() || m_pages[idx] == nullptr) [[unlikely]]
					return nullptr;
				return m_pages[idx] + off;
			}
			[[nodiscard]] constexpr T &component_ref(std::size_t i) const noexcept
			{
				auto ptr = component_ptr(i);
				SEK_ASSERT(ptr != nullptr, "Pool index must be valid");
				return *ptr;
			}
			[[nodiscard]] constexpr T *alloc_component(std::size_t i)
			{
				const auto idx = page_idx(i);
				const auto off = page_off(i);

				/* Make sure the page exists. */
				m_pages.resize(idx + 1, nullptr);
				auto &page = m_pages[idx];
				if (page == nullptr) [[unlikely]]
					page = alloc_page();
				return page + off;
			}

			constexpr void swap(component_pool_impl &other)
			{
				base_set::swap(other);

				sek::detail::alloc_assert_swap(get_allocator(), other.get_allocator());
				sek::detail::alloc_swap(get_allocator(), other.get_allocator());

				m_pages.swap(other.m_pages);
			}

			pages_t m_pages; /* Array of pointers to component pages & page allocator. */
		};
		template<typename T, typename A>
		struct component_pool_impl<T, A, true> : public detail::component_entity_set<T, A>
		{
			using base_set = detail::component_entity_set<T, A>;
			using set_iterator = typename base_set::iterator;

			[[nodiscard]] constexpr static std::size_t page_idx(std::size_t) noexcept { return 0; }
			[[nodiscard]] constexpr static std::size_t page_off(std::size_t) noexcept { return 0; }

			constexpr component_pool_impl() noexcept = default;
			constexpr ~component_pool_impl() = default;

			// clang-format off
			constexpr component_pool_impl(component_pool_impl &&)
				noexcept(std::is_nothrow_move_constructible_v<base_set>) = default;
			constexpr component_pool_impl &operator=(component_pool_impl &&)
				noexcept(std::is_nothrow_move_assignable_v<base_set>) = default;
			// clang-format on

			constexpr explicit component_pool_impl(const A &) {}
			constexpr component_pool_impl(std::size_t n, const A &) : base_set(n) {}
			constexpr component_pool_impl(component_pool_impl &&other, const A &) : base_set(std::move(other)) {}

			constexpr void reserve_impl(std::size_t) {}
			constexpr void purge_impl() {}

			template<typename... Args>
			set_iterator emplace_impl(entity e, Args &&...)
			{
				return base_set::insert(e);
			}
			template<typename... Args>
			set_iterator push_impl(entity e, Args &&...)
			{
				return base_set::push_back(e);
			}

			set_iterator erase_impl(set_iterator where) { return base_set::erase(where); }

			void dense_move(std::size_t, std::size_t) final {}
			void dense_swap(std::size_t, std::size_t) final {}

			[[nodiscard]] constexpr T *const *data() const noexcept { return nullptr; }

			constexpr void component_ref(std::size_t) const noexcept {}
			constexpr void component_ptr(std::size_t) const noexcept {}

			constexpr void swap(component_pool_impl &other) { base_set::swap(other); }
		};
	}	 // namespace detail

	template<typename Set, typename... Ts>
	class component_view;

	/** @brief Structure used to allocate components and associate them with entities.
	 *
	 * Component pools allocate components in pages. Pages are used to reduce the need for re-allocation and copy/move
	 * operations for components. Every component is then indirectly indexed via an entity via an entity set.
	 *
	 * @tparam T Type of components managed by the pool.
	 * @tparam A Allocator used to allocate memory of the pool. */
	template<typename T, typename A = std::allocator<T>>
	class basic_component_pool : detail::component_pool_impl<T, A, std::is_empty_v<T>>
	{
		using base_t = detail::component_pool_impl<T, A, std::is_empty_v<T>>;

		template<typename, typename...>
		friend class component_view;

	public:
		typedef A allocator_type;
		typedef T component_type;

		typedef T value_type;
		typedef value_type *pointer;
		typedef const value_type *const_pointer;
		typedef value_type &reference;
		typedef const value_type &const_reference;

		typedef typename base_t::base_set base_set;
		typedef typename base_set::difference_type difference_type;
		typedef typename base_set::size_type size_type;

	private:
		// clang-format off
		constexpr static bool is_fixed = requires{ typename component_traits<T>::is_fixed; };
		// clang-format on

		using set_iterator = typename base_t::set_iterator;
		using pages_t = T *const *;

		[[nodiscard]] constexpr static size_type page_idx(size_type i) noexcept { return base_t::page_idx(i); }
		[[nodiscard]] constexpr static size_type page_off(size_type i) noexcept { return base_t::page_off(i); }

		template<bool IsConst>
		class component_iterator
		{
			template<bool>
			friend class component_iterator;
			friend class basic_component_pool;

		public:
			typedef T value_type;
			typedef std::conditional_t<IsConst, const T, T> *pointer;
			typedef std::conditional_t<IsConst, const T, T> &reference;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;
			typedef std::random_access_iterator_tag iterator_category;

		private:
			constexpr explicit component_iterator(pages_t pages, size_type off = 0) noexcept
				: m_pages(pages), m_off(static_cast<difference_type>(off))
			{
			}

		public:
			constexpr component_iterator() noexcept = default;
			template<bool OtherConst, typename = std::enable_if_t<IsConst && !OtherConst>>
			constexpr component_iterator(const component_iterator<OtherConst> &other) noexcept
				: m_pages(other.m_pages), m_off(other.m_off)
			{
			}

			constexpr component_iterator operator++(int) noexcept
			{
				auto temp = *this;
				++(*this);
				return temp;
			}
			constexpr component_iterator &operator++() noexcept
			{
				--m_off;
				return *this;
			}
			constexpr component_iterator &operator+=(difference_type n) noexcept
			{
				m_off -= n;
				return *this;
			}
			constexpr component_iterator operator--(int) noexcept
			{
				auto temp = *this;
				--(*this);
				return temp;
			}
			constexpr component_iterator &operator--() noexcept
			{
				++m_off;
				return *this;
			}
			constexpr component_iterator &operator-=(difference_type n) noexcept
			{
				m_off += n;
				return *this;
			}

			constexpr component_iterator operator+(difference_type n) const noexcept
			{
				return component_iterator{m_pages, static_cast<size_type>(m_off - n)};
			}
			constexpr component_iterator operator-(difference_type n) const noexcept
			{
				return component_iterator{m_pages, static_cast<size_type>(m_off + n)};
			}
			constexpr difference_type operator-(const component_iterator &other) const noexcept
			{
				return other.m_off - m_off;
			}

			/** Returns offset of the iterator from the base. */
			[[nodiscard]] constexpr size_type offset() const noexcept { return static_cast<size_type>(m_off - 1); }

			/** Returns pointer to the target component. */
			[[nodiscard]] constexpr pointer get() const noexcept
			{
				const auto off = page_off(offset());
				const auto idx = page_idx(offset());
				return m_pages[idx] + off;
			}
			/** @copydoc value */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }

			/** Returns reference to the component at an offset. */
			[[nodiscard]] constexpr reference operator[](difference_type n) const noexcept
			{
				const auto off = page_off(offset() + n);
				const auto idx = page_idx(offset() + n);
				return m_pages[idx] + off;
			}
			/** Returns reference to the target component. */
			[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }

			[[nodiscard]] constexpr auto operator<=>(const component_iterator &other) const noexcept
			{
				return get() <=> other.get();
			}
			[[nodiscard]] constexpr bool operator==(const component_iterator &other) const noexcept
			{
				return get() == other.get();
			}

			constexpr void swap(component_iterator &other) noexcept
			{
				std::swap(m_pages, other.m_pages);
				std::swap(m_off, other.m_off);
			}
			friend constexpr void swap(component_iterator &a, component_iterator &b) noexcept { a.swap(b); }

		private:
			pages_t m_pages = {};
			difference_type m_off = 0;
		};

	public:
		typedef component_iterator<false> iterator;
		typedef component_iterator<true> const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

	public:
		basic_component_pool(const basic_component_pool &) = delete;
		basic_component_pool &operator=(const basic_component_pool &) = delete;

		constexpr basic_component_pool() noexcept = default;
		constexpr ~basic_component_pool() = default;

		// clang-format off
		constexpr basic_component_pool(basic_component_pool &&)
			noexcept(std::is_nothrow_move_constructible_v<base_t>) = default;
		constexpr basic_component_pool &operator=(basic_component_pool &&)
			noexcept(std::is_nothrow_move_assignable_v<base_t>) = default;
		// clang-format on

		/** Initializes component pool using the provided allocator. */
		constexpr explicit basic_component_pool(const allocator_type &alloc) : base_t(alloc) {}
		/** Initializes component pool using the provided allocator & reserves n elements. */
		constexpr basic_component_pool(size_type n, const allocator_type &alloc = {}) : base_t(n, alloc) {}
		/** Moves component pool using the provided allocator. */
		constexpr basic_component_pool(basic_component_pool &&other, const allocator_type &alloc)
			: base_t(std::move(other), alloc)
		{
		}

		/** Returns iterator to the first component in the pool. */
		[[nodiscard]] constexpr auto begin() noexcept { return iterator{data(), size()}; }
		/** @copydoc begin */
		[[nodiscard]] constexpr auto cbegin() const noexcept { return const_iterator{data(), size()}; }
		/** @copydoc begin */
		[[nodiscard]] constexpr auto begin() const noexcept { return cbegin(); }
		/** Returns iterator one past the last component in the pool. */
		[[nodiscard]] constexpr auto end() noexcept { return iterator{data()}; }
		/** @copydoc end */
		[[nodiscard]] constexpr auto cend() const noexcept { return const_iterator{data()}; }
		/** @copydoc end */
		[[nodiscard]] constexpr auto end() const noexcept { return cend(); }

		/** Returns reverse iterator to the last component in the pool. */
		[[nodiscard]] constexpr auto rbegin() noexcept { return reverse_iterator{end()}; }
		/** @copydoc rbegin */
		[[nodiscard]] constexpr auto crbegin() const noexcept { return const_reverse_iterator{cend()}; }
		/** @copydoc rbegin */
		[[nodiscard]] constexpr auto rbegin() const noexcept { return crbegin(); }
		/** Returns reverse iterator one past the first component in the pool. */
		[[nodiscard]] constexpr auto rend() noexcept { return reverse_iterator{begin()}; }
		/** @copydoc rend */
		[[nodiscard]] constexpr auto crend() const noexcept { return const_reverse_iterator{cbegin()}; }
		/** @copydoc rend */
		[[nodiscard]] constexpr auto rend() const noexcept { return crend(); }

		/** Returns reference to the set of entities of the component pool. */
		[[nodiscard]] constexpr base_set &entities() noexcept { return *this; }
		/** @copydoc entities */
		[[nodiscard]] constexpr const base_set &entities() const noexcept { return *this; }

		/** Returns the amount of entities associated with the pool.
		 * @note If the component type requires fixed storage, size will include the number of tombstones. */
		[[nodiscard]] constexpr size_type size() const noexcept { return base_set::size(); }
		/** Checks if no entities are associated with the pool.
		 * @note If the component type requires fixed storage, `empty` may return erroneous result. */
		[[nodiscard]] constexpr bool empty() const noexcept { return base_set::empty(); }
		/** Checks if the specified entity is associated with the pool. */
		[[nodiscard]] constexpr bool contains(entity e) const noexcept { return base_set::contains(e); }
		/** Returns an iterator to the component associated with the specified entity, or an end iterator
		 * if the entity is not associated with the pool. */
		[[nodiscard]] constexpr iterator find(entity e) noexcept { return to_iterator(base_set::find(e).offset()); }
		/** @copydoc find */
		[[nodiscard]] constexpr const_iterator find(entity e) const noexcept
		{
			return to_iterator(base_set::find(e).offset());
		}

		/** Returns component located at index `i`. */
		[[nodiscard]] constexpr decltype(auto) at(size_type i) noexcept { return base_t::component_ref(i); }
		/** @copydoc at */
		[[nodiscard]] constexpr decltype(auto) at(size_type i) const noexcept { return base_t::component_ref(i); }
		/** @copydoc at */
		[[nodiscard]] constexpr decltype(auto) operator[](size_type i) noexcept { return at(i); }
		/** @copydoc at */
		[[nodiscard]] constexpr decltype(auto) operator[](size_type i) const noexcept { return at(i); }

		/** Reserves space for `n` components. */
		void reserve(size_type n) final
		{
			base_set::reserve(n);
			base_t::reserve_impl(n);
		}

		/** Clears component pool. */
		void clear() { base_set::clear(); }
		/** Clears component pool and deallocates internal storage. */
		void purge()
		{
			base_set::purge();
			base_t::purge_impl();
		}

		/** Constructs a component for the specified entity in-place (re-using slots if
		 * component type requires fixed storage) and returns an iterator to it. */
		template<typename... Args>
		iterator emplace(entity e, Args &&...args)
			requires std::constructible_from<T, Args...>
		{
			return to_iterator(base_t::emplace_impl(e, std::forward<Args>(args)...).offset());
		}
		/** Constructs a component for the specified entity in-place (always at the end)
		 * and returns an iterator to it. */
		template<typename... Args>
		iterator emplace_back(entity e, Args &&...args)
			requires std::constructible_from<T, Args...>
		{
			return to_iterator(base_t::push_impl(e, std::forward<Args>(args)...).offset());
		}
		/** Inserts a component for the specified entity (re-using slots if component type
		 * requires fixed storage) and returns an iterator to it. */
		iterator insert(entity e, component_type &&value) { return emplace(e, std::forward<T>(value)); }
		/** @copydoc insert */
		iterator insert(entity e, const component_type &value) { return emplace(e, value); }
		/** Inserts a component for the specified entity (always at the end) and returns an iterator to it. */
		iterator push_back(entity e, component_type &&value) { return emplace_back(e, std::forward<T>(value)); }
		/** @copydoc push_back */
		iterator push_back(entity e, const component_type &value) { return emplace_back(e, value); }

		/** Erases a component from the pool and returns iterator to the next component. */
		iterator erase(const_iterator where)
		{
			const auto base_pos = base_set::begin() + static_cast<difference_type>(where.offset());
			return to_iterator(erase(base_pos).offset());
		}
		/** Erases a component associated with the specified entity from the pool and returns iterator to the next component.
		 * @note Will cause undefined behavior if the entity is not associated with the pool. */
		iterator erase(entity e) { return erase(find(e)); }
		/** Erases all components between `[first, last)`. */
		void erase(const_iterator first, const_iterator last)
		{
			while (first != last) erase(first++);
		}

		/** Swaps components of the given entities. */
		void swap(entity a, entity b) { base_set::swap(a, b); }
		/** Swaps components of the pool. */
		void swap(size_type a_idx, size_type b_idx) { base_set::swap(a_idx, b_idx); }
		/** @copydoc swap */
		void swap(const_iterator a, const_iterator b) { swap(a.offset(), b.offset()); }
		/** Removes tombstone entities (if any) from the pool.
		 * @note Pointer stability of components is not guaranteed. */
		void pack() { base_set::pack(); }

		constexpr void swap(basic_component_pool &other) { base_t::swap(other); }
		friend constexpr void swap(basic_component_pool &a, basic_component_pool &b) { a.swap(b); }

	protected:
		set_iterator insert(entity e) override
		{
			if constexpr (!std::is_default_constructible_v<T>)
				return base_set::end();
			else
				return base_t::emplace_impl(e);
		}
		set_iterator push_back(entity e) override
		{
			if constexpr (!std::is_default_constructible_v<T>)
				return base_set::end();
			else
				return base_t::push_impl(e);
		}
		set_iterator erase(set_iterator where) override { return base_t::erase_impl(where); }

	private:
		[[nodiscard]] constexpr pages_t data() const noexcept { return base_t::data(); }

		[[nodiscard]] constexpr iterator to_iterator(size_type i) noexcept { return iterator{data(), i + 1}; }
		[[nodiscard]] constexpr const_iterator to_iterator(size_type i) const noexcept
		{
			return const_iterator{data(), i + 1};
		}
	};

	/** @brief Helper structure used to iterate over multiple component pools.
	 * @tparam Set Entity set used to define the range of the view.
	 * @tparam Ts Sequence of component pool types to iterate over. */
	template<typename Set, typename... Ts>
	class component_view
	{
		using set_iterator = typename Set::iterator;

		template<typename T>
		using component_t = typename T::value_type;
		template<typename T>
		using page_t = component_t<T> *const *;
		using pages_t = std::tuple<page_t<Ts>...>;

	public:
		typedef std::tuple<component_t<Ts>...> value_type;
		typedef std::tuple<component_t<Ts> *...> pointer;
		typedef std::tuple<const component_t<Ts> *...> const_pointer;
		typedef std::tuple<component_t<Ts> &...> reference;
		typedef std::tuple<const component_t<Ts> &...> const_reference;
		typedef std::common_type_t<typename Ts::size_type...> size_type;
		typedef std::common_type_t<typename Ts::difference_type...> difference_type;

	private:
		template<typename T>
		[[nodiscard]] constexpr static size_type page_idx(size_type i) noexcept
		{
			return T::page_idx(i);
		}
		template<typename T>
		[[nodiscard]] constexpr static size_type page_off(size_type i) noexcept
		{
			return T::page_off(i);
		}

		template<bool IsConst>
		class view_iterator
		{
			friend class component_view;

		public:
			typedef std::tuple<component_t<Ts>...> value_type;
			typedef std::tuple<std::conditional_t<IsConst, const component_t<Ts>, component_t<Ts>> *...> pointer;
			typedef std::tuple<std::conditional_t<IsConst, const component_t<Ts>, component_t<Ts>> &...> reference;

			typedef std::common_type_t<typename Ts::size_type...> size_type;
			typedef std::common_type_t<typename Ts::difference_type...> difference_type;
			typedef std::random_access_iterator_tag iterator_category;

		private:
			constexpr explicit view_iterator(page_t<Ts>... pages, size_type off = 0) noexcept
				: view_iterator(std::forward_as_tuple(pages...), off)
			{
			}
			constexpr explicit view_iterator(pages_t pages, size_type off = 0) noexcept
				: m_pages(pages), m_off(static_cast<difference_type>(off))
			{
			}

			template<size_type... Is, bool OtherConst>
			constexpr view_iterator(std::index_sequence<Is...>, const view_iterator<OtherConst> &other) noexcept
				: view_iterator(static_cast<std::tuple_element_t<Is, pages_t>>(other.m_pages)..., other.m_off)
			{
			}

		public:
			constexpr view_iterator() noexcept = default;
			template<bool OtherConst, typename = std::enable_if_t<IsConst && !OtherConst>>
			constexpr view_iterator(const view_iterator<OtherConst> &other) noexcept
				: view_iterator(std::make_index_sequence<sizeof...(Ts)>{}, other.m_off)
			{
			}

			constexpr view_iterator operator++(int) noexcept
			{
				auto temp = *this;
				++(*this);
				return temp;
			}
			constexpr view_iterator &operator++() noexcept
			{
				--m_off;
				return *this;
			}
			constexpr view_iterator &operator+=(difference_type n) noexcept
			{
				m_off -= n;
				return *this;
			}
			constexpr view_iterator operator--(int) noexcept
			{
				auto temp = *this;
				--(*this);
				return temp;
			}
			constexpr view_iterator &operator--() noexcept
			{
				++m_off;
				return *this;
			}
			constexpr view_iterator &operator-=(difference_type n) noexcept
			{
				m_off += n;
				return *this;
			}

			constexpr view_iterator operator+(difference_type n) const noexcept
			{
				return view_iterator{m_pages, static_cast<size_type>(m_off - n)};
			}
			constexpr view_iterator operator-(difference_type n) const noexcept
			{
				return view_iterator{m_pages, static_cast<size_type>(m_off + n)};
			}
			constexpr difference_type operator-(const view_iterator &other) const noexcept
			{
				return m_off - other.m_off;
			}

			/** Returns offset of the iterator from the base. */
			[[nodiscard]] constexpr size_type offset() const noexcept { return static_cast<size_type>(m_off - 1); }

			/** Returns pointer to the target component. */
			[[nodiscard]] constexpr pointer get() const noexcept
			{
				return page_ptr(std::make_index_sequence<sizeof...(Ts)>{}, m_off - 1);
			}
			/** @copydoc value */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }

			/** Returns reference to the component at an offset. */
			[[nodiscard]] constexpr decltype(auto) operator[](difference_type n) const noexcept
			{
				return page_ref(std::make_index_sequence<sizeof...(Ts)>{}, offset() + n);
			}
			/** Returns reference to the target component. */
			[[nodiscard]] constexpr decltype(auto) operator*() const noexcept
			{
				return page_ref(std::make_index_sequence<sizeof...(Ts)>{}, m_off - 1);
			}

			[[nodiscard]] constexpr auto operator<=>(const view_iterator &other) const noexcept
			{
				return m_off <=> other.m_off;
			}
			[[nodiscard]] constexpr bool operator==(const view_iterator &other) const noexcept
			{
				return m_off == other.m_off;
			}

			constexpr void swap(view_iterator &other) noexcept
			{
				std::swap(m_pages, other.m_pages);
				std::swap(m_off, other.m_off);
			}
			friend constexpr void swap(view_iterator &a, view_iterator &b) noexcept { a.swap(b); }

		private:
			template<size_type I>
			[[nodiscard]] constexpr auto page_ptr(difference_type i) const noexcept
			{
				using pool_t = pack_member_t<type_seq_t<Ts...>, I>;

				const auto off = page_off<pool_t>(static_cast<typename pool_t::size_type>(i));
				const auto idx = page_idx<pool_t>(static_cast<typename pool_t::size_type>(i));
				return std::get<I>(m_pages)[idx] + off;
			}
			template<size_type... Is>
			[[nodiscard]] constexpr pointer page_ptr(std::index_sequence<Is...>, difference_type i) const noexcept
			{
				return std::forward_as_tuple(page_ptr<Is>(i)...);
			}
			template<size_type... Is>
			[[nodiscard]] constexpr reference page_ref(std::index_sequence<Is...>, difference_type i) const noexcept
			{
				return std::forward_as_tuple(*page_ptr<Is>(i)...);
			}

			pages_t m_pages;
			difference_type m_off;
		};

	public:
		typedef view_iterator<false> iterator;
		typedef view_iterator<true> const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

	public:
		constexpr component_view() noexcept = default;

		/** Initializes component view from a range of entity iterators and pointers to pools. */
		constexpr component_view(set_iterator first, set_iterator last, const Ts *...pools) noexcept
			: m_begin(first), m_end(last), m_ptr(pools...)
		{
		}
		/** Initializes component view from an entity set and pointers to pools. */
		constexpr component_view(const Set &set, const Ts *...pools) noexcept
			: component_view(set.begin(), set.end(), pools...)
		{
		}
		/** Initializes component view from a range of entity iterators and references to pools. */
		constexpr component_view(set_iterator first, set_iterator last, const Ts &...pools) noexcept
			: component_view(first, last, std::addressof(pools)...)
		{
		}
		/** Initializes component view from an entity set and references to pools. */
		constexpr component_view(const Set &set, const Ts &...pools) noexcept
			: component_view(set.begin(), set.end(), std::addressof(pools)...)
		{
		}

		/** Returns iterator to the first set of components in the view. */
		[[nodiscard]] constexpr auto begin() noexcept { return to_iterator(m_begin.offset()); }
		/** @copydoc begin */
		[[nodiscard]] constexpr auto cbegin() const noexcept { return to_iterator(m_begin.offset()); }
		/** @copydoc begin */
		[[nodiscard]] constexpr auto begin() const noexcept { return cbegin(); }
		/** Returns iterator one past the last set of components in the view. */
		[[nodiscard]] constexpr auto end() noexcept { return to_iterator(m_end.offset()); }
		/** @copydoc end */
		[[nodiscard]] constexpr auto cend() const noexcept { return to_iterator(m_end.offset()); }
		/** @copydoc end */
		[[nodiscard]] constexpr auto end() const noexcept { return cend(); }

		/** Returns reverse iterator to the last set of components in the view. */
		[[nodiscard]] constexpr auto rbegin() noexcept { return reverse_iterator{end()}; }
		/** @copydoc rbegin */
		[[nodiscard]] constexpr auto crbegin() const noexcept { return const_reverse_iterator{cend()}; }
		/** @copydoc rbegin */
		[[nodiscard]] constexpr auto rbegin() const noexcept { return crbegin(); }
		/** Returns reverse iterator one past the first set of components in the view. */
		[[nodiscard]] constexpr auto rend() noexcept { return reverse_iterator{begin()}; }
		/** @copydoc rend */
		[[nodiscard]] constexpr auto crend() const noexcept { return const_reverse_iterator{cbegin()}; }
		/** @copydoc rend */
		[[nodiscard]] constexpr auto rend() const noexcept { return crend(); }

		/** Returns a set of components located at index `i`. */
		[[nodiscard]] constexpr reference at(size_type i) noexcept { return *to_iterator(i); }
		/** @copydoc at */
		[[nodiscard]] constexpr const_reference at(size_type i) const noexcept { return *to_iterator(i); }
		/** @copydoc at */
		[[nodiscard]] constexpr reference operator[](size_type i) noexcept { return at(i); }
		/** @copydoc at */
		[[nodiscard]] constexpr const_reference operator[](size_type i) const noexcept { return at(i); }

		/** Returns the size of the component view. */
		[[nodiscard]] constexpr size_type size() const noexcept
		{
			return static_cast<size_type>(std::distance(m_begin, m_end));
		}
		/** Checks if the component view is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_begin == m_end; }

		constexpr void swap(component_view &other) noexcept
		{
			using std::swap;
			swap(m_begin, other.m_begin);
			swap(m_end, other.m_end);
			swap(m_ptr, other.m_ptr);
		}
		friend constexpr void swap(component_view &a, component_view &b) noexcept { a.swap(b); }

	private:
		template<size_type I>
		[[nodiscard]] constexpr auto data() const noexcept
		{
			return std::get<I>(m_ptr)->data();
		}
		template<size_type... Is>
		[[nodiscard]] constexpr pages_t data(std::index_sequence<Is...>) const noexcept
		{
			return pages_t{data<Is>()...};
		}

		[[nodiscard]] constexpr iterator to_iterator(size_type i) noexcept
		{
			return iterator{data(std::make_index_sequence<sizeof...(Ts)>{}), i + 1};
		}
		[[nodiscard]] constexpr const_iterator to_iterator(size_type i) const noexcept
		{
			return const_iterator{data(std::make_index_sequence<sizeof...(Ts)>{}), i + 1};
		}

		set_iterator m_begin, m_end;
		std::tuple<const Ts *...> m_ptr;
	};

	template<typename S, typename... Ts>
	component_view(const S &, const Ts *...) -> component_view<S, Ts...>;
	template<typename S, typename... Ts>
	component_view(const S &, const Ts &...) -> component_view<S, Ts...>;
}	 // namespace sek::engine