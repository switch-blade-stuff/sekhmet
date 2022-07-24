/*
 * Created by switchblade on 24/07/22
 */

#pragma once

#include "entity.hpp"
#include "traits.hpp"

namespace sek::engine
{
	namespace detail
	{
		template<typename T>
		class component_pool : ebo_base_helper<typename component_traits<T>::allocator_type>
		{
			constexpr static auto page_size = component_traits<T>::page_size;

			[[nodiscard]] constexpr static auto page_idx(auto n) noexcept { return n / page_size; }
			[[nodiscard]] constexpr static auto page_off(auto n) noexcept { return n % page_size; }

		public:
			using size_type = std::size_t;
			using difference_type = std::ptrdiff_t;

		private:
			using alloc_traits = std::allocator_traits<typename component_traits<T>::allocator_type>;
			using alloc_base = ebo_base_helper<typename component_traits<T>::allocator_type>;

			using pages_alloc = typename alloc_traits::template rebind_alloc<T *>;
			using pages_data = std::vector<T *, pages_alloc>;

		public:
			component_pool(const component_pool &) = delete;
			component_pool &operator=(const component_pool &) = delete;

			constexpr component_pool() = default;

			// clang-format off
			constexpr component_pool(component_pool &&other) : alloc_base(std::move(other)), m_pages(std::move(other.m_pages))
			{
				assert_alloc(other);
			}
			constexpr component_pool &operator=(component_pool &&other)
			{
				assert_alloc(other);
				alloc_base::operator=(std::move(other));
				m_pages = std::move(other.m_pages);
				return *this;
			}
			// clang-format on

			constexpr void release_pages()
			{
				for (auto page : m_pages) dealloc_page(page);
			}

			[[nodiscard]] constexpr T *component_ptr(size_type i) const noexcept
			{
				const auto idx = page_idx(i);
				const auto off = page_off(i);

				if (idx >= m_pages.size() || m_pages[idx] == nullptr) [[unlikely]]
					return nullptr;
				return m_pages[idx] + off;
			}
			[[nodiscard]] constexpr T &component_ref(size_type i) const noexcept
			{
				const auto idx = page_idx(i);
				const auto off = page_off(i);
				return m_pages[idx][off];
			}

			constexpr void reserve(size_type n)
			{
				m_pages.resize(page_idx(n) + 1, nullptr);
				for (size_type i = 0; i < n; ++i)
					if (m_pages[i] == nullptr) m_pages[i] = alloc_page();
			}

			template<typename... Args>
			constexpr T &emplace(size_type i, Args &&...args)
			{
				return *std::construct_at(alloc_entry(i), std::forward<Args>(args)...);
			}
			constexpr void erase(size_type i) { std::destroy_at(component_ptr(i)); }

			constexpr void move_value(size_type to, size_type from)
			{
				component_ref(to) = std::move(component_ref(from));
			}
			constexpr void swap_value(size_type a, size_type b)
			{
				using std::swap;
				swap(component_ref(a), component_ref(b));
			}

			constexpr void swap(component_pool &other) noexcept
			{
				sek::detail::alloc_assert_swap(get_allocator(), other.get_allocator());
				sek::detail::alloc_swap(get_allocator(), other.get_allocator());
				std::swap(m_pages, other.m_pages);
			}

		private:
			[[nodiscard]] constexpr auto &get_allocator() noexcept { return *alloc_base::get(); }
			[[nodiscard]] constexpr auto &get_allocator() const noexcept { return *alloc_base::get(); }

			constexpr void assert_alloc(const component_pool &other)
			{
				SEK_ASSERT(alloc_traits::propagate_on_container_move_assignment::value ||
						   sek::detail::alloc_eq(get_allocator(), other.get_allocator()));
			}

			[[nodiscard]] constexpr T *alloc_page() { return alloc_traits::allocate(get_allocator(), page_size); }
			constexpr void dealloc_page(T *page) { alloc_traits::deallocate(get_allocator(), page, page_size); }

			[[nodiscard]] constexpr T *alloc_entry(size_type i)
			{
				const auto idx = page_idx(i);

				/* Make sure page list has enough space. */
				m_pages.resize(idx + 1, nullptr);

				/* Allocate the page if it is empty. */
				auto &page = m_pages[idx];
				if (page == nullptr) [[unlikely]]
					page = alloc_page();
				return page + page_off(i);
			}

			pages_data m_pages;
		};
		template<typename T>
		struct component_typedef
		{
			typedef T component_type;
			typedef typename component_traits<T>::allocator_type component_allocator;
		};
		// clang-format off
		template<typename T> requires std::is_empty_v<T>
		class component_pool<T> : ebo_base_helper<T>
		{
			using ebo_base = ebo_base_helper<T>;

		public:
			using size_type = std::size_t;
			using difference_type = std::ptrdiff_t;

		public:
			constexpr void release_pages() {}

			[[nodiscard]] constexpr T *component_ptr(size_type) const noexcept
			{
				return const_cast<T *>(ebo_base::get());
			}
			[[nodiscard]] constexpr T &component_ref(size_type) const noexcept
			{
				return *const_cast<T *>(ebo_base::get());
			}

			constexpr void reserve(size_type) {}

			template<typename... Args>
			constexpr T &emplace(size_type i, Args &&...args)
			{
				return *std::construct_at(component_ptr(i), std::forward<Args>(args)...);
			}
			constexpr void erase(size_type i) { std::destroy_at(component_ptr(i)); }

			constexpr void move_value(size_type, size_type) noexcept {}
			constexpr void swap_value(size_type, size_type) noexcept {}

			constexpr void swap(component_pool &) noexcept {}
		};
		template<typename T> requires std::is_empty_v<T>
		struct component_typedef<T>
		{
			typedef T component_type;
		};
		// clang-format on

		template<>
		class component_pool<void>
		{
		public:
			using size_type = std::size_t;
			using difference_type = std::ptrdiff_t;

		public:
			constexpr void release_pages() {}
			constexpr void reserve(size_type) {}
			constexpr void emplace(size_type) {}
			constexpr void erase(size_type) {}
			constexpr void swap(component_pool &) noexcept {}
			constexpr void move_value(size_type, size_type) noexcept {}
			constexpr void swap_value(size_type, size_type) noexcept {}
		};
		template<>
		struct component_typedef<void>
		{
		};

		struct default_sort
		{
			template<typename... Args>
			constexpr void operator()(auto first, auto last, Args &&...args)
			{
				std::sort(first, last, std::forward<Args>(args)...);
			}
		};
	}	 // namespace detail

	/** @brief Structure used to store unique sets of entities and associate entities with components.
	 * @tparam T Optional component type. If set to void, set does not store components.
	 * @tparam Alloc Allocator used to allocate memory of the entity set. */
	template<typename T, typename Alloc>
	class basic_entity_set : detail::component_pool<T>, ebo_base_helper<Alloc>, public detail::component_typedef<T>
	{
		constexpr static auto sparse_page_size = SEK_KB(8) / sizeof(entity_t);
		constexpr static bool entity_only = std::is_void_v<T> || std::is_empty_v<T>;

		using is_fixed = std::bool_constant<fixed_component<T>>;
		using pool_base = detail::component_pool<T>;
		using alloc_base = ebo_base_helper<Alloc>;

	public:
		typedef Alloc allocator_type;
		typedef allocator_type entity_allocator;

	private:
		using alloc_traits = std::allocator_traits<allocator_type>;

		using sparse_alloc = typename alloc_traits::template rebind_alloc<entity_t *>;
		using sparse_data = std::vector<entity_t *, sparse_alloc>;
		using dense_alloc = typename alloc_traits::template rebind_alloc<entity_t>;
		using dense_data = std::vector<entity_t, dense_alloc>;

		template<typename... Args>
		constexpr static bool valid_args = (std::is_void_v<T> && sizeof...(Args) == 0) || std::is_constructible_v<T, Args...>;

		[[nodiscard]] constexpr static auto sparse_idx(auto n) noexcept { return n / sparse_page_size; }
		[[nodiscard]] constexpr static auto sparse_off(auto n) noexcept { return n % sparse_page_size; }

		template<bool IsConst, typename U>
		struct value_selector
		{
			using reference = std::pair<const entity_t &, std::conditional_t<IsConst, const U, U> &>;
			using value = std::pair<entity_t, U>;
		};
		template<bool IsConst>
		struct value_selector<IsConst, void>
		{
			using reference = const entity_t &;
			using value = entity_t;
		};

		// clang-format off
		template<bool IsConst>
		using set_ref = typename value_selector<IsConst, T>::reference;
		using set_value = typename value_selector<false, T>::value;
		// clang-format on

		template<bool>
		class set_iterator;
		template<bool IsConst>
		class set_ptr
		{
			template<bool>
			friend class set_iterator;

			constexpr explicit set_ptr(set_ref<IsConst> ref) noexcept : m_ref(ref) {}

			using ref_t = set_ref<IsConst>;
			using ptr_t = const std::remove_reference_t<ref_t> *;

		public:
			set_ptr() = delete;

			constexpr set_ptr(const set_ptr &) noexcept = default;
			constexpr set_ptr &operator=(const set_ptr &) noexcept = default;
			constexpr set_ptr(set_ptr &&) noexcept = default;
			constexpr set_ptr &operator=(set_ptr &&) noexcept = default;

			[[nodiscard]] constexpr ref_t operator*() const noexcept { return m_ref; }
			[[nodiscard]] constexpr ptr_t get() const noexcept { return std::addressof(m_ref); }
			[[nodiscard]] constexpr ptr_t operator->() const noexcept { return get(); }

			[[nodiscard]] constexpr auto operator<=>(const set_ptr<IsConst> &other) const noexcept
			{
				return entity() <=> other.entity();
			}
			[[nodiscard]] constexpr bool operator==(const set_ptr<IsConst> &other) const noexcept
			{
				return entity() == other.entity();
			}

		private:
			[[nodiscard]] constexpr const entity_t *entity() const noexcept
			{
				if constexpr (std::is_void_v<T>)
					return std::addressof(m_ref);
				else
					return std::addressof(m_ref.first);
			}

			ref_t m_ref;
		};
		template<bool IsConst>
		class set_iterator
		{
			template<bool>
			friend class set_iterator;
			friend class basic_entity_set;

		public:
			typedef set_value value_type;
			typedef set_ref<IsConst> reference;
			typedef set_ptr<IsConst> pointer;
			typedef typename pool_base::size_type size_type;
			typedef typename pool_base::difference_type difference_type;
			typedef std::random_access_iterator_tag iterator_category;

		private:
			constexpr explicit set_iterator(const basic_entity_set *parent, difference_type off) noexcept
				: m_parent(parent), m_off(off)
			{
			}
			constexpr explicit set_iterator(const basic_entity_set *parent, size_type off = 0) noexcept
				: set_iterator(parent, static_cast<difference_type>(off))
			{
			}

		public:
			constexpr set_iterator() noexcept = default;
			template<bool OtherConst, typename = std::enable_if_t<IsConst && !OtherConst>>
			constexpr set_iterator(const set_iterator<OtherConst> &other) noexcept
				: m_parent(other.m_parent), m_off(other.m_off)
			{
			}

			constexpr set_iterator operator++(int) noexcept
			{
				auto temp = *this;
				++(*this);
				return temp;
			}
			constexpr set_iterator &operator++() noexcept
			{
				--m_off;
				return *this;
			}
			constexpr set_iterator &operator+=(difference_type n) noexcept
			{
				m_off -= n;
				return *this;
			}
			constexpr set_iterator operator--(int) noexcept
			{
				auto temp = *this;
				--(*this);
				return temp;
			}
			constexpr set_iterator &operator--() noexcept
			{
				++m_off;
				return *this;
			}
			constexpr set_iterator &operator-=(difference_type n) noexcept
			{
				m_off += n;
				return *this;
			}

			constexpr set_iterator operator+(difference_type n) const noexcept
			{
				return set_iterator{m_parent, m_off - n};
			}
			constexpr set_iterator operator-(difference_type n) const noexcept
			{
				return set_iterator{m_parent, m_off + n};
			}
			constexpr difference_type operator-(const set_iterator &other) const noexcept
			{
				return other.m_off - m_off;
			}

			/** Returns the offset of the iterator from the base. */
			[[nodiscard]] constexpr size_type offset() const noexcept { return static_cast<size_type>(m_off - 1); }

			/** Returns pointer to the target entity (or pointer to the pair of references to entity and it's component). */
			[[nodiscard]] constexpr pointer get() const noexcept { return pointer{operator*()}; }
			/** @copydoc get */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
			/** Returns reference to the target entity (or pair of references to entity and it's component). */
			[[nodiscard]] constexpr reference operator*() const noexcept { return operator[](0); }
			/** Returns reference to the entity at offset `i` from this iterator (or pair of references to entity and it's component). */
			[[nodiscard]] constexpr reference operator[](difference_type i) const noexcept
			{
				const auto off = offset() + static_cast<size_type>(i);
				if constexpr (!std::is_void_v<T>)
					return {m_parent->m_dense[off], m_parent->component_ref(off)};
				else
					return m_parent->m_dense[off];
			}

			[[nodiscard]] constexpr auto operator<=>(const set_iterator &other) const noexcept
			{
				return m_off <=> other.m_off;
			}
			[[nodiscard]] constexpr bool operator==(const set_iterator &) const noexcept = default;

			constexpr void swap(set_iterator &other) noexcept
			{
				std::swap(m_parent, other.m_parent);
				std::swap(m_off, other.m_off);
			}
			friend constexpr void swap(set_iterator &a, set_iterator &b) noexcept { a.swap(b); }

		private:
			const basic_entity_set *m_parent = nullptr;
			difference_type m_off = 0;
		};

	public:
		typedef set_value value_type;

		typedef set_iterator<false> iterator;
		typedef set_iterator<true> const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

		typedef typename iterator::reference reference;
		typedef typename const_iterator::reference const_reference;
		typedef typename iterator::pointer pointer;
		typedef typename const_iterator::pointer const_pointer;

		typedef typename iterator::size_type size_type;
		typedef typename iterator::difference_type difference_type;

	private:
		[[nodiscard]] constexpr static entity_t to_entity(const_iterator i) noexcept
		{
			if constexpr (!std::is_void_v<T>)
				return i->first;
			else
				return *i;
		}
		[[nodiscard]] constexpr static decltype(auto) to_component(iterator i) noexcept
		{
			if constexpr (!std::is_void_v<T>) return i->second;
		}
		[[nodiscard]] constexpr static decltype(auto) to_component(const_iterator i) noexcept
		{
			if constexpr (!std::is_void_v<T>) return i->second;
		}

	public:
		constexpr basic_entity_set() = default;
		constexpr ~basic_entity_set()
		{
			clear();
			release_pages();
		}

		// clang-format off
		constexpr basic_entity_set(const basic_entity_set &other) requires entity_only
			: alloc_base(sek::detail::make_alloc_copy(other.get_allocator())),
			  m_sparse(other.m_sparse),
			  m_dense(other.m_dense)
		{
			copy_sparse(other);
		}
		constexpr basic_entity_set(const basic_entity_set &other, const allocator_type &alloc) requires entity_only
			: alloc_base(alloc), m_sparse(other.m_sparse, alloc), m_dense(other.m_dense, alloc)
		{
			copy_sparse(other);
		}

		constexpr basic_entity_set &operator=(const basic_entity_set &other) requires entity_only
		{
			if (this != &other)
			{
				release_pages();
				m_sparse = other.m_sparse;
				m_dense = other.m_dense;
				copy_sparse(other);
			}
			return *this;
		}
		// clang-format on

		constexpr basic_entity_set(basic_entity_set &&other)
			: pool_base(std::move(other)),
			  alloc_base(std::move(other)),
			  m_sparse(std::move(other.m_sparse)),
			  m_dense(std::move(other.m_dense))
		{
			if (!(alloc_traits::propagate_on_container_move_assignment::value ||
				  sek::detail::alloc_eq(get_allocator(), get_allocator())))
				move_sparse(other);
		}
		constexpr basic_entity_set(basic_entity_set &&other, const allocator_type &alloc)
			: pool_base(std::move(other)),
			  alloc_base(alloc),
			  m_sparse(std::move(other.m_sparse), alloc),
			  m_dense(std::move(other.m_dense), alloc)
		{
			if (!(alloc_traits::propagate_on_container_move_assignment::value ||
				  sek::detail::alloc_eq(get_allocator(), get_allocator())))
				move_sparse(other);
		}
		constexpr basic_entity_set &operator=(basic_entity_set &&other)
		{
			pool_base::operator=(std::move(other));
			m_sparse = std::move(other.m_sparse);

			if (alloc_traits::propagate_on_container_move_assignment::value ||
				sek::detail::alloc_eq(get_allocator(), get_allocator()))
				m_dense = std::move(other.m_dense);
			else
			{
				m_dense = other.m_dense;
				move_sparse(other);
			}
			return *this;
		}

		constexpr explicit basic_entity_set(const allocator_type &alloc)
			: alloc_base(alloc), m_sparse(alloc), m_dense(alloc)
		{
		}
		constexpr explicit basic_entity_set(size_type n, const allocator_type &alloc = {})
			: alloc_base(alloc), m_sparse(alloc), m_dense(alloc)
		{
			reserve(n);
		}

		constexpr basic_entity_set(std::initializer_list<entity_t> init_list, const allocator_type &alloc = {})
			: basic_entity_set(init_list.size(), alloc)
		{
			insert(init_list.begin(), init_list.end());
		}

		/** Returns iterator to the first entity (and it's component, if any) of the set. */
		[[nodiscard]] constexpr iterator begin() noexcept { return iterator{this, size()}; }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return const_iterator{this, size()}; }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
		/** Returns iterator one past the last entity of the set. */
		[[nodiscard]] constexpr iterator end() noexcept { return iterator{this}; }
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator end() const noexcept { return const_iterator{this}; }
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }

		/** Returns reverse iterator to the last entity (and it's component, if any) of the set. */
		[[nodiscard]] constexpr reverse_iterator rbegin() noexcept { return reverse_iterator{end()}; }
		/** @copydoc rbegin */
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator{end()}; }
		/** @copydoc rbegin */
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
		/** Returns reverse iterator one past the first entity of the set. */
		[[nodiscard]] constexpr reverse_iterator rend() noexcept { return reverse_iterator{begin()}; }
		/** @copydoc rend */
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator{begin()}; }
		/** @copydoc rend */
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return rend(); }

		/** Returns the amount of entities contained within the set.
		 * @note If the set stores "fixed" components, size will include the number of tombstones (if any). */
		[[nodiscard]] constexpr size_type size() const noexcept { return m_dense.size(); }
		/** Checks if the size of the set is `0`.
		 * @note If the set stores "fixed" components, may return incorrect value. */
		[[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }

		/** Checks if the set contains the specified entity. */
		[[nodiscard]] constexpr bool contains(entity_t entity) const noexcept
		{
			const auto sparse = sparse_ptr(entity.index().value());
			return sparse != nullptr && !sparse->is_tombstone();
		}

		/** Returns iterator to the specified entity (and it's component, if any) or an end iterator. */
		[[nodiscard]] constexpr iterator find(entity_t entity) noexcept
		{
			const auto sparse = sparse_ptr(entity.index().value());
			if (sparse != nullptr && !sparse->is_tombstone()) [[likely]]
				return to_iterator(sparse->index().value());
			else
				return end();
		}
		/** @copydoc find */
		[[nodiscard]] constexpr const_iterator find(entity_t entity) const noexcept
		{
			const auto sparse = sparse_ptr(entity.index().value());
			if (sparse != nullptr && !sparse->is_tombstone()) [[likely]]
				return to_iterator(sparse->index().value());
			else
				return end();
		}

		/** Returns offset of the specified entity.
		 * @warning Using an entity not contained within the set will result in undefined behavior. */
		[[nodiscard]] constexpr size_type offset(entity_t entity) const noexcept
		{
			return sparse_ref(entity.index().value()).index().value();
		}
		/** Returns offset of the specified iterator. */
		[[nodiscard]] constexpr size_type offset(const_iterator i) const noexcept { return i.offset(); }

		/** Returns reference to the entity located at the specified offset
		 * (or pair of references to the entity and it's component). */
		[[nodiscard]] constexpr decltype(auto) at(size_type i) noexcept { return *to_iterator(i); }
		/** @copydoc at */
		[[nodiscard]] constexpr decltype(auto) at(size_type i) const noexcept { return *to_iterator(i); }

		// clang-format off
		/** Returns reference to the component of the specified entity. */
		[[nodiscard]] constexpr decltype(auto) get(entity_t entity) noexcept requires(!std::is_void_v<T>) { return to_component(find(entity)); }
		/** @copydoc get */
		[[nodiscard]] constexpr decltype(auto) get(entity_t entity) const noexcept requires(!std::is_void_v<T>) { return to_component(find(entity)); }
		// clang-format on

		/** Updates generation of an entity contained within the set.
		 * @param Entity to update generation of. */
		constexpr void update(entity_t e) { update(e, e.generation()); }
		/** @copydoc update
		 * @param gen Generation value to use. */
		constexpr void update(entity_t e, entity_t::generation_type gen)
		{
			const auto idx = e.index();
			auto &slot = sparse_ref(idx.value());
			slot = entity_t{gen, slot.index()};
			m_dense[slot.index().value()] = entity_t{gen, idx};
		}

		/** Replaces component of an entity.
		 * @param Entity to replace component of.
		 * @param value Value of the component. */
		template<typename U>
		constexpr void replace(entity_t e, const U &value)
			requires(!std::is_void_v<T>)
		{
			get(e) = value;
		}
		/** @copydoc update */
		template<typename U>
		constexpr void update(entity_t e, U &&value)
			requires(!std::is_void_v<T>)
		{
			get(e) = std::move(value);
		}

		/** Swaps entities of the entity set. */
		void swap(size_type a, size_type b)
		{
			pool_base::swap_value(a, b);
			auto &lhs = m_dense[a];
			auto &rhs = m_dense[b];
			std::swap(sparse_ref(lhs.index().value()), sparse_ref(rhs.index().value()));
			std::swap(lhs, rhs);
		}
		/** @copydoc swap */
		void swap(const_iterator a, const_iterator b) { swap(a.offset(), b.offset()); }
		/** @copydoc swap */
		void swap(entity_t a, entity_t b) { swap(offset(a), offset(b)); }

		/** Removes tombstones (if any) from the set. */
		void pack()
		{
			size_type from = size(), to;
			auto skip_base = [&]()
			{
				while (m_dense[from - 1].is_tombstone()) --from;
			};
			skip_base();
			for (auto *ptr = &this->m_next; ptr->index() != entity_t::index_type::tombstone(); ptr = &m_dense[to])
			{
				to = ptr->index().value();
				if (to < from)
				{
					pool_base::move_value(to, --from);

					auto &e_from = m_dense[from];
					auto &e_to = m_dense[to];
					std::swap(e_from, e_to);

					sparse_ref(e_to.index().value()) = entity_t{e_to.generation(), entity_t::index_type{to}};
					*ptr = entity_t{entity_t::generation_type::tombstone(), entity_t::index_type{from}};
					skip_base();
				}
			}
			m_next = entity_t::tombstone();
			m_dense.resize(from);
		}

		/** Sorts entities `[0, n)` of the set.
		 * @param n Amount of elements to sort.
		 * @param sort Functor to use for sorting. `std::sort` used by default.
		 * @param args Arguments passed to the sort functor. */
		template<typename Sort = detail::default_sort, typename... Args>
		void sort_n(size_type n, Sort sort = {}, Args &&...args)
		{
			SEK_ASSERT(n <= size());
			SEK_ASSERT(m_next.is_tombstone(), "Dense array must be packed for sorting");

			/* Sort dense entity array, then fix sparse entities. */
			sort(m_dense.begin(), m_dense.begin() + static_cast<std::ptrdiff_t>(n), std::forward<Args>(args)...);
			for (auto item = begin(), last = iterator{this, n}; item != last; ++item)
			{
				auto &slot = sparse_ref(to_entity(item).index().value());
				const auto old_pos = slot.index().value();
				const auto new_pos = item.offset();

				/* Let any derived type know we are swapping entities & update the sparse index. */
				pool_base::swap_value(old_pos, new_pos);
				slot = entity_t{slot.generation(), entity_t::index_type{new_pos}};
			}
		}
		/** Sorts entities of the set.
		 * @param sort Functor to use for sorting. `std::sort` used by default.
		 * @param args Arguments passed to the sort functor. */
		template<typename Sort = detail::default_sort, typename... Args>
		void sort(Sort sort = {}, Args &&...args)
		{
			pack();
			sort_n(size(), sort, std::forward<Args>(args)...);
		}
		/** Sorts entities of the set according to the provided order. */
		template<std::bidirectional_iterator I>
		void sort(I from, I to)
		{
			pack();
			for (auto i = size() - 1; i && to-- != from;)
				if (const auto self = to_entity(to_iterator(i)), other = *to; contains(other))
				{
					if (other != self) swap(self, other);
					--i;
				}
		}

		/** Emplaces the specified entity into the set and constructs it's component in-place (if needed).
		 * Entity slots can be re-used.
		 *
		 * @param entity Entity to insert.
		 * @param args Arguments passed to component's constructor (if any).
		 *
		 * @return Reference to entity's component (or `void`, if the set does not store components).
		 * @warning Using an entity already contained within the set will result in undefined behavior. */
		template<typename... Args>
		constexpr decltype(auto) emplace(entity_t entity, Args &&...args)
			requires valid_args<Args...>
		{
			return component_ref(emplace_impl(entity, std::forward<Args>(args)...));
		}
		/** Emplaces the specified entity into the set and constructs it's component in-place (if needed).
		 * Entities (and components) are always pushed to the end.
		 *
		 * @param entity Entity to insert.
		 * @param args Arguments passed to component's constructor (if any).
		 *
		 * @return Reference to entity's component (or `void`, if the set does not store components).
		 * @warning Using an entity already contained within the set will result in undefined behavior. */
		template<typename... Args>
		constexpr decltype(auto) emplace_back(entity_t entity, Args &&...args)
			requires valid_args<Args...>
		{
			return component_ref(emplace_back_impl(entity, std::forward<Args>(args)...));
		}

		/** Inserts an entity and it's component into the set.
		 * Entity slots can be re-used.
		 *
		 * @param entity Entity to insert.
		 *
		 * @return Iterator to the inserted entity.
		 * @warning Using an entity already contained within the set will result in undefined behavior. */
		constexpr iterator insert(entity_t entity) { return iterator{this, emplace_impl(entity) + 1}; }
		/** Inserts an entity and it's component into the set.
		 * Entity slots can be re-used.
		 *
		 * @param entity Entity to insert.
		 * @param value Value of the component.
		 *
		 * @return Iterator to the inserted entity.
		 * @warning Using an entity already contained within the set will result in undefined behavior. */
		template<typename U>
		constexpr iterator insert(entity_t entity, const U &value)
			requires(!std::is_void_v<T>)
		{
			return iterator{this, emplace_impl(entity, value) + 1};
		}
		/** @copydoc insert */
		template<typename U>
		constexpr iterator insert(entity_t entity, U &&value)
			requires(!std::is_void_v<T>)
		{
			return iterator{this, emplace_impl(entity, std::move(value)) + 1};
		}

		/** Inserts all entities in the range `[first, last)`. Components are default-constructed. */
		template<std::forward_iterator I, std::sentinel_for<I> S>
		constexpr iterator insert(I first, S last)
		{
			const auto pos = m_dense.size();
			for (; first != last; first = std::next(first)) push_back(*first);
			return to_iterator(pos);
		}

		/** Inserts an entity and it's component into the set.
		 * Entities (and components) are always pushed to the end.
		 *
		 * @param entity Entity to insert.
		 *
		 * @return Iterator to the inserted entity.
		 * @warning Using an entity already contained within the set will result in undefined behavior. */
		constexpr iterator push_back(entity_t entity) { return iterator{this, emplace_back_impl(entity) + 1}; }
		/** Inserts an entity and it's component into the set.
		 * Entities (and components) are always pushed to the end.
		 *
		 * @param entity Entity to insert.
		 * @param value Value of the component.
		 *
		 * @return Iterator to the inserted entity.
		 * @warning Using an entity already contained within the set will result in undefined behavior. */
		template<typename U>
		constexpr iterator push_back(entity_t entity, const U &value)
			requires(!std::is_void_v<T>)
		{
			return iterator{this, emplace_back_impl(entity, value) + 1};
		}
		/** @copydoc push_back */
		template<typename U>
		constexpr iterator push_back(entity_t entity, U &&value)
			requires(!std::is_void_v<T>)
		{
			return iterator{this, emplace_back_impl(entity, std::move(value)) + 1};
		}

		/** Erases the entity (and it's component) from the set. */
		constexpr iterator erase(const_iterator which) { return to_iterator(erase_impl(offset(which))); }
		/** @copydoc erase */
		constexpr iterator erase(entity_t entity) { return to_iterator(erase_impl(offset(entity))); }
		/** Erases all entities between `[first, last)`. */
		constexpr void erase(const_iterator first, const_iterator last)
		{
			if (first == begin() && last == end()) [[unlikely]]
				clear();
			else
				for (; first != last; ++first)
				{
					if (to_entity(first).is_tombstone()) [[unlikely]]
						continue;

					erase(first);
				}
		}

		/** Reserves space for `n` entities (and components). */
		constexpr void reserve(size_type n)
		{
			if (n != 0) [[likely]]
			{
				pool_base::reserve(n);
				m_sparse.resize(sparse_idx(n) + 1, nullptr);
				m_dense.reserve(n);
			}
		}
		/** Removes all entities from the set (destroying components, if needed). */
		constexpr void clear()
		{
			for (size_type i = 0; i < 0; ++i)
			{
				const auto dense = m_dense[i];
				if (dense.is_tombstone()) [[unlikely]]
					continue;

				sparse_ref(dense.index().value()) = entity_t::tombstone();
				pool_base::erase(i);
			}
		}

		constexpr void swap(basic_entity_set &other) noexcept
		{
			pool_base::swap(other);

			sek::detail::alloc_assert_swap(get_allocator(), other.get_allocator());
			sek::detail::alloc_swap(get_allocator(), other.get_allocator());

			using std::swap;
			swap(m_sparse, other.m_sparse);
			swap(m_dense, other.m_dense);
			swap(m_next, other.m_next);
		}
		friend constexpr void swap(basic_entity_set &a, basic_entity_set &b) noexcept { a.swap(b); }

	private:
		[[nodiscard]] constexpr auto &get_allocator() noexcept { return *alloc_base::get(); }
		[[nodiscard]] constexpr auto &get_allocator() const noexcept { return *alloc_base::get(); }

		[[nodiscard]] constexpr auto to_iterator(size_type offset) noexcept { return iterator{this, offset + 1}; }
		[[nodiscard]] constexpr auto to_iterator(size_type offset) const noexcept
		{
			return const_iterator{this, offset + 1};
		}

		[[nodiscard]] constexpr entity_t *alloc_sparse_page()
		{
			return alloc_traits::allocate(get_allocator(), sparse_page_size);
		}
		[[nodiscard]] constexpr entity_t *make_sparse_page()
		{
			auto page = alloc_sparse_page();
			std::uninitialized_fill_n(page, sparse_page_size, entity_t::tombstone());
			return page;
		}
		[[nodiscard]] constexpr entity_t &insert_sparse(size_type i)
		{
			const auto idx = sparse_idx(i);

			/* Make sure page list has enough space. */
			m_sparse.resize(idx + 1, nullptr);

			/* Allocate the page if it is empty. */
			auto &page = m_sparse[idx];
			if (page == nullptr) [[unlikely]]
				page = make_sparse_page();

			auto &slot = page[sparse_off(i)];
			SEK_ASSERT(slot.is_tombstone(), "Sparse slot already in use");
			return slot;
		}
		constexpr void dealloc_sparse_page(entity_t *page)
		{
			alloc_traits::deallocate(get_allocator(), page, sparse_page_size);
		}

		constexpr void release_pages()
		{
			pool_base::release_pages();
			for (auto page : m_sparse) dealloc_sparse_page(page);
		}

		[[nodiscard]] constexpr entity_t *sparse_ptr(size_type i) const noexcept
		{
			const auto idx = sparse_idx(i);
			const auto off = sparse_off(i);

			if (idx >= m_sparse.size() || m_sparse[idx] == nullptr) [[unlikely]]
				return nullptr;
			return m_sparse[idx] + off;
		}
		[[nodiscard]] constexpr entity_t &sparse_ref(size_type i) const noexcept
		{
			const auto idx = sparse_idx(i);
			const auto off = sparse_off(i);
			return m_sparse[idx][off];
		}

		[[nodiscard]] constexpr decltype(auto) component_ptr(size_type i) const noexcept
		{
			if constexpr (!std::is_void_v<T>) return pool_base::component_ptr(i);
		}
		[[nodiscard]] constexpr decltype(auto) component_ref(size_type i) const noexcept
		{
			if constexpr (!std::is_void_v<T>) return pool_base::component_ref(i);
		}

		constexpr void copy_sparse(const basic_entity_set &other)
		{
			for (size_type i = 0; auto src : other.m_sparse)
			{
				auto dst = m_sparse[i++] = alloc_sparse_page();
				std::uninitialized_copy_n(src, sparse_page_size, dst);
			}
		}
		constexpr void move_sparse(basic_entity_set &other)
		{
			for (size_type i = 0; auto src : m_sparse)
			{
				auto dst = alloc_sparse_page();
				std::uninitialized_copy_n(src, sparse_page_size, dst);
				other.dealloc_sparse_page(std::exchange(m_sparse[i++], dst));
			}
		}

		template<typename... Args>
		constexpr size_type emplace_impl(std::false_type, entity_t e, Args &&...args)
		{
			const auto pos = m_dense.size();

			pool_base::emplace(pos, std::forward<Args>(args)...);
			auto &slot = insert_sparse(e.index().value());
			m_dense.push_back(e);
			slot = entity_t{e.generation(), entity_t::index_type{pos}};

			return pos;
		}
		template<typename... Args>
		constexpr size_type emplace_impl(std::true_type, entity_t e, Args &&...args)
		{
			/* Reuse an existing dense position if possible. */
			if (m_next.is_tombstone() && m_next.index() == entity_t::index_type::tombstone()) [[unlikely]]
				return emplace_impl(std::false_type{}, e, std::forward<Args>(args)...);
			else
			{
				const auto idx = m_next.index();
				const auto pos = idx.value();

				pool_base::emplace(pos, std::forward<Args>(args)...);
				auto &slot = insert_sparse(e.index().value());

				slot = entity_t(e.generation(), idx);
				m_next = std::exchange(m_dense[pos], e);

				return pos;
			}
		}

		template<typename... Args>
		constexpr size_type emplace_back_impl(entity_t e, Args &&...args)
		{
			return emplace_impl(std::false_type{}, e, std::forward<Args>(args)...);
		}
		template<typename... Args>
		constexpr size_type emplace_impl(entity_t e, Args &&...args)
		{
			return emplace_impl(is_fixed{}, e, std::forward<Args>(args)...);
		}

		constexpr size_type erase_impl(std::false_type, size_type idx)
		{
			/* Swap with the last one & pop. */
			const auto last = m_dense.size() - 1;
			if (idx < last) [[likely]]
			{
				const auto from = m_dense[last].index().value();
				const auto to = m_dense[idx].index().value();

				sparse_ref(from) = entity_t{m_dense[last].generation(), entity_t::index_type{idx}};
				sparse_ref(to) = entity_t::tombstone();
				m_dense[idx] = m_dense[last];

				pool_base::move_value(idx, last);
			}
			else
				sparse_ref(m_dense[last].index().value()) = entity_t::tombstone();
			m_dense.pop_back();
			pool_base::erase(last);
			return idx;
		}
		constexpr size_type erase_impl(std::true_type, size_type idx)
		{
			const auto new_next = entity_t{entity_t::generation_type::tombstone(), entity_t::index_type{idx}};
			m_dense[idx] = std::exchange(m_next, new_next);
			pool_base::erase(idx);
			return idx + 1;
		}
		constexpr size_type erase_impl(size_type idx) { return erase_impl(is_fixed{}, idx); }

		sparse_data m_sparse;
		dense_data m_dense;

		/* Next dense entity available for reuse. */
		entity_t m_next = entity_t::tombstone();
	};

	template<typename T>
	using component_set = basic_entity_set<T>;
	using entity_set = basic_entity_set<void>;
}	 // namespace sek::engine