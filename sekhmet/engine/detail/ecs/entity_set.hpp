/*
 * Created by switchblade on 24/07/22
 */

#pragma once

#include "entity.hpp"
#include "traits.hpp"

namespace sek::engine
{
	template<typename = std::allocator<entity_t>>
	class basic_entity_set;

	namespace detail
	{
		struct default_sort
		{
			template<typename... Args>
			constexpr void operator()(auto first, auto last, Args &&...args)
			{
				std::sort(first, last, std::forward<Args>(args)...);
			}
		};
	}	 // namespace detail

	/** @brief Structure used to store a unique set of entities with `O(1)` lookup, insertion & deletion complexity.
	 * @tparam Alloc Allocator used for allocation of entity set memory. */
	template<typename Alloc>
	class basic_entity_set : ebo_base_helper<typename std::allocator_traits<Alloc>::template rebind_alloc<entity_t>>
	{
		constexpr static auto page_size = SEK_KB(8) / sizeof(entity_t);

		using alloc_traits = typename std::allocator_traits<Alloc>;

	public:
		typedef typename alloc_traits::template rebind_alloc<entity_t> allocator_type;

	private:
		using sparse_alloc = typename alloc_traits::template rebind_alloc<entity_t *>;
		using sparse_data = std::vector<entity_t *, sparse_alloc>;
		using dense_data = std::vector<entity_t, allocator_type>;
		using alloc_base = ebo_base_helper<allocator_type>;

		[[nodiscard]] constexpr static auto sparse_idx(auto n) noexcept { return n / page_size; }
		[[nodiscard]] constexpr static auto sparse_off(auto n) noexcept { return n % page_size; }

		class set_iterator
		{
			friend class basic_entity_set;

		public:
			typedef entity_t value_type;
			typedef const entity_t &reference;
			typedef const entity_t *pointer;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;
			typedef std::random_access_iterator_tag iterator_category;

		private:
			constexpr explicit set_iterator(const basic_entity_set *parent, size_type off = 0) noexcept
				: m_parent(parent), m_off(static_cast<difference_type>(off))
			{
			}
			constexpr set_iterator(const basic_entity_set *parent, difference_type off) noexcept
				: m_parent(parent), m_off(off)
			{
			}

		public:
			constexpr set_iterator() noexcept = default;

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

			[[nodiscard]] constexpr set_iterator operator+(difference_type n) const noexcept
			{
				return set_iterator{m_parent, m_off - n};
			}
			[[nodiscard]] constexpr set_iterator operator-(difference_type n) const noexcept
			{
				return set_iterator{m_parent, m_off + n};
			}
			[[nodiscard]] constexpr difference_type operator-(const set_iterator &other) const noexcept
			{
				return other.m_off - m_off;
			}

			/** Returns the offset of the iterator from the base. */
			[[nodiscard]] constexpr size_type offset() const noexcept { return static_cast<size_type>(m_off - 1); }

			/** Returns pointer to the target entity. */
			[[nodiscard]] constexpr pointer get() const noexcept { return std::addressof(operator*()); }
			/** @copydoc get */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
			/** Returns reference to the target entity. */
			[[nodiscard]] constexpr reference operator*() const noexcept { return operator[](0); }
			/** Returns reference to the entity at offset `i` from this iterator. */
			[[nodiscard]] constexpr reference operator[](difference_type i) const noexcept
			{
				const auto off = offset() + static_cast<size_type>(i);
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
		typedef set_iterator iterator;
		typedef set_iterator const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

		typedef typename iterator::value_type value_type;
		typedef typename iterator::reference reference;
		typedef typename const_iterator::reference const_reference;
		typedef typename iterator::pointer pointer;
		typedef typename const_iterator::pointer const_pointer;
		typedef typename iterator::size_type size_type;
		typedef typename iterator::difference_type difference_type;

	public:
		constexpr virtual ~basic_entity_set() { release_pages(); }

		constexpr basic_entity_set(const basic_entity_set &other)
			: alloc_base(sek::detail::make_alloc_copy(other.alloc())),
			  m_sparse(other.m_sparse),
			  m_dense(other.m_dense),
			  m_next(other.m_next)
		{
			copy_sparse(other);
		}
		constexpr basic_entity_set(const basic_entity_set &other, const allocator_type &alloc)
			: alloc_base(alloc), m_sparse(other.m_sparse, alloc), m_dense(other.m_dense, alloc), m_next(other.m_next)
		{
			copy_sparse(other);
		}

		constexpr basic_entity_set &operator=(const basic_entity_set &other)
		{
			if (this != &other)
			{
				release_pages();
				sek::detail::alloc_copy_assign(alloc(), other.alloc());
				m_sparse = other.m_sparse;
				m_dense = other.m_dense;
				m_next = other.m_next;
				copy_sparse(other);
			}
			return *this;
		}

		constexpr basic_entity_set(basic_entity_set &&other) noexcept(sek::detail::nothrow_alloc_move_construct<allocator_type>)
			: alloc_base(std::move(other)),
			  m_sparse(std::move(other.m_sparse)),
			  m_dense(std::move(other.m_dense)),
			  m_next(std::move(other.m_next))
		{
			if (!sek::detail::alloc_eq(this->alloc(), other.alloc())) [[unlikely]]
				move_sparse(other);
		}
		constexpr basic_entity_set(basic_entity_set &&other, const allocator_type &alloc)
			: alloc_base(alloc),
			  m_sparse(std::move(other.m_sparse), alloc),
			  m_dense(std::move(other.m_dense), alloc),
			  m_next(std::move(other.m_next))
		{
			if (!sek::detail::alloc_eq(this->alloc(), other.alloc())) [[unlikely]]
				move_sparse(other);
		}

		constexpr basic_entity_set &operator=(basic_entity_set &&other) noexcept(sek::detail::nothrow_alloc_move_assign<allocator_type>)
		{
			sek::detail::alloc_move_assign(alloc(), other.alloc());
			m_sparse = std::move(other.m_sparse);
			m_dense = std::move(other.m_dense);
			m_next = std::move(other.m_next);

			if (!sek::detail::alloc_eq(alloc(), other.alloc())) [[unlikely]]
				move_sparse(other);

			return *this;
		}

		/** Initializes an empty entity set. */
		constexpr basic_entity_set() noexcept(sek::detail::nothrow_alloc_default_construct<allocator_type>) = default;
		/** @copydoc basic_entity_set
		 * @alloc Allocator used to allocate memory of the entity set. */
		constexpr basic_entity_set(const allocator_type &alloc) noexcept(sek::detail::nothrow_alloc_copy_construct<allocator_type>)
			: alloc_base(alloc), m_sparse(alloc), m_dense(alloc)
		{
		}

		/** Initializes an empty set and reserves `n` entities.
		 * @param n Amount of entities to reserve. */
		constexpr explicit basic_entity_set(size_type n) { reserve(n); }
		/** @copydoc basic_entity_set
		 * @alloc Allocator used to allocate memory of the entity set. */
		constexpr basic_entity_set(size_type n, const allocator_type &alloc) : basic_entity_set(alloc) { reserve(n); }
		/** Initializes entity set from a range of entities.
		 * @param first Iterator to the first entity.
		 * @param last Sentinel for the `first` iterator. */
		template<std::forward_iterator I, std::sentinel_for<I> S>
		constexpr basic_entity_set(I first, S last)
		{
			insert(first, last);
		}
		/** @copydoc basic_entity_set */
		template<std::random_access_iterator I, std::sentinel_for<I> S>
		constexpr basic_entity_set(I first, S last) : basic_entity_set(static_cast<size_type>(last - first))
		{
			insert(first, last);
		}
		/** @copydoc basic_entity_set
		 * @alloc Allocator used to allocate memory of the entity set. */
		template<std::forward_iterator I, std::sentinel_for<I> S>
		constexpr basic_entity_set(I first, S last, const allocator_type &alloc) : basic_entity_set(alloc)
		{
			insert(first, last);
		}
		/** @copydoc basic_entity_set */
		template<std::random_access_iterator I, std::sentinel_for<I> S>
		constexpr basic_entity_set(I first, S last, const allocator_type &alloc)
			: basic_entity_set(static_cast<size_type>(last - first), alloc)
		{
			insert(first, last);
		}
		/** Initializes entity set from an initializer list of entities.
		 * @param il Initializer list containing entities of the set. */
		constexpr basic_entity_set(std::initializer_list<entity_t> il) : basic_entity_set(il.begin(), il.end()) {}
		/** @copydoc basic_entity_set
		 * @alloc Allocator used to allocate memory of the entity set. */
		constexpr basic_entity_set(std::initializer_list<entity_t> il, const allocator_type &alloc)
			: basic_entity_set(il.begin(), il.end(), alloc)
		{
		}

		/** Returns iterator to the first entity of the set. */
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
		/** Returns reverse iterator to the last entity of the set. */
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

		/** Returns pointer to the dense array of entities.
		 * @note Dense array may include tombstones (if any). */
		[[nodiscard]] constexpr const entity_t *data() const noexcept { return m_dense.data(); }
		/** Returns reference to the entity located at the specified dense offset. */
		[[nodiscard]] constexpr reference at(size_type i) const noexcept { return data()[i]; }

		/** Returns the amount of entities contained within the set.
		 * @note Size may include the number of tombstones (if any). */
		[[nodiscard]] constexpr size_type size() const noexcept { return m_dense.size(); }
		/** Checks if the size of the set is `0`.
		 * @note Result may be incorrect if any tombstones are present. */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_dense.empty(); }

		/** Checks if the set contains the specified entity. */
		[[nodiscard]] constexpr bool contains(entity_t entity) const noexcept
		{
			const auto sparse = sparse_ptr(entity.index().value());
			return sparse != nullptr && !sparse->is_tombstone();
		}

		/** Returns iterator to the specified entity or an end iterator. */
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
		[[nodiscard]] constexpr size_type offset(const_iterator which) const noexcept { return which.offset(); }

		/** Reserves space for `n` entities. */
		constexpr void reserve(size_type n)
		{
			if (n != 0) [[likely]]
			{
				m_sparse.resize(sparse_idx(n) + 1, nullptr);
				m_dense.reserve(n);
			}
		}
		/** Removes all entities from the set. */
		constexpr void clear()
		{
			for (auto item = begin(), last = end(); item != last; ++item)
			{
				if (item->is_tombstone()) [[unlikely]]
					continue;

				fixed_erase_(item.offset());
			}
			m_dense.clear();
			m_next = entity_t::tombstone();
		}

		/** Updates generation of an entity contained within the set.
		 * @param entity Entity to update generation of. */
		constexpr void update(entity_t entity) { update(entity, entity.generation()); }
		/** @copydoc update
		 * @param gen Generation value to use. */
		constexpr void update(entity_t entity, entity_t::generation_type gen)
		{
			const auto idx = entity.index();
			auto &slot = sparse_ref(idx.value());
			slot = entity_t{gen, slot.index()};
			m_dense[slot.index().value()] = entity_t{gen, idx};
		}

		/** Swaps entities of the entity set. */
		constexpr void swap(size_type a, size_type b)
		{
			swap_(a, b);
			auto &lhs = m_dense[a];
			auto &rhs = m_dense[b];
			std::swap(sparse_ref(lhs.index().value()), sparse_ref(rhs.index().value()));
			std::swap(lhs, rhs);
		}
		/** @copydoc swap */
		constexpr void swap(const_iterator a, const_iterator b) { swap(a.offset(), b.offset()); }
		/** @copydoc swap */
		constexpr void swap(entity_t a, entity_t b) { swap(offset(a), offset(b)); }

		/** Removes tombstones (if any) from the set. */
		constexpr void pack()
		{
			// clang-format off
			size_type from = size(), to;
			auto skip_base = [&]() { while (m_dense[from - 1].is_tombstone()) --from; };
			// clang-format on

			skip_base();
			for (auto *ptr = &this->m_next; ptr->index() != entity_t::index_type::tombstone(); ptr = &m_dense[to])
			{
				to = ptr->index().value();
				if (to < from)
				{
					move_(to, --from);

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
		 * @param args Arguments passed to the sort functor.
		 *
		 * @note Sorting functor iterates over entities of the set. */
		template<typename Sort = detail::default_sort, typename... Args>
		constexpr void sort_n(size_type n, Sort sort = {}, Args &&...args)
		{
			SEK_ASSERT(n <= size());
			SEK_ASSERT(m_next.is_tombstone(), "Dense array must be packed for sorting");

			/* Sort dense entity array, then fix sparse entities. */
			invoke_sort(std::forward<Sort>(sort), n, std::forward<Args>(args)...);
			for (auto item = begin(), last = iterator{this, n}; item != last; ++item)
			{
				auto &slot = sparse_ref(to_entity(item).index().value());
				const auto old_pos = slot.index().value();
				const auto new_pos = item.offset();

				/* Let any derived type know we are swapping entities & update the sparse index. */
				swap_(old_pos, new_pos);
				slot = entity_t{slot.generation(), entity_t::index_type{new_pos}};
			}
		}
		/** Sorts entities of the set.
		 * @param sort Functor to use for sorting. `std::sort` used by default.
		 * @param args Arguments passed to the sort functor.
		 *
		 * @note Sorting functor iterates over entities of the set. */
		template<typename Sort = detail::default_sort, typename... Args>
		constexpr void sort(Sort sort = {}, Args &&...args)
		{
			pack();
			sort_n(size(), sort, std::forward<Args>(args)...);
		}
		/** Sorts entities of the set according to the provided order. */
		constexpr void sort(const_iterator from, const_iterator to)
		{
			pack();
			for (auto i = size() - 1; i && to-- != from;)
				if (const auto other = *to; contains(other))
				{
					const auto off = i--;
					const auto self = data()[off];
					if (other != self) swap(off, to.offset());
				}
		}
		/** @copydoc sort */
		template<std::bidirectional_iterator I>
		constexpr void sort(I from, I to)
		{
			pack();
			for (auto i = size() - 1; i && to-- != from;)
				if (const auto self = data()[i], other = *to; contains(other))
				{
					if (other != self) swap(self, other);
					--i;
				}
		}

		/** Inserts an entity into the set. Tombstones (if any) are re-used.
		 * @param entity Entity to insert.
		 * @return Iterator to the inserted entity.
		 * @warning Using an entity already contained within the set will result in undefined behavior. */
		constexpr iterator insert(entity_t entity) { return to_iterator(insert_(entity)); }
		/** Attempts to insert an entity into the set. Tombstones (if any) are re-used.
		 * @param entity Entity to insert.
		 * @return Pair where first is iterator to the potentially inserted entity and second is a boolean
		 * indicating whether the entity was inserted (`true` if inserted, `false` otherwise). */
		constexpr std::pair<iterator, bool> try_insert(entity_t entity)
		{
			if (const auto existing = find(entity); existing == end())
				return {insert(entity), true};
			else
				return {existing, false};
		}
		/** Inserts entities in the range `[first, last)` into the set (always at the end).
		 * @warning Using entities already present will result in undefined behavior. */
		template<std::forward_iterator I, std::sentinel_for<I> S>
		constexpr iterator insert(I first, S last)
		{
			for (; first != last; first = std::next(first)) push_back(*first);
			return end();
		}

		/** Inserts an entity into the set. Entities are always pushed to the end (tombstones are not re-used).
		 * @param entity Entity to insert.
		 * @return Iterator to the inserted entity.
		 * @warning Using an entity already contained within the set will result in undefined behavior. */
		constexpr iterator push_back(entity_t entity) { return iterator{this, push_back_(entity) + 1}; }
		/** Attempts to insert an entity into the set. Entities are always pushed to the end (tombstones are not re-used).
		 * @param entity Entity to insert.
		 * @return Pair where first is iterator to the potentially inserted entity and second is a boolean
		 * indicating whether the entity was inserted (`true` if inserted, `false` otherwise). */
		constexpr std::pair<iterator, bool> try_push_back(entity_t entity)
		{
			if (const auto existing = find(entity); existing == end())
				return {push_back(entity), true};
			else
				return {existing, false};
		}

		/** Erases the entity from the set using swap & pop (without leaving tombstones). */
		constexpr iterator erase(const_iterator which) { return to_iterator(erase_(offset(which))); }
		/** @copydoc erase */
		constexpr iterator erase(entity_t entity) { return to_iterator(erase_(offset(entity))); }
		/** Erases the entity from the set in-place, leaving a tombstone. */
		constexpr iterator fixed_erase(const_iterator which) { return to_iterator(fixed_erase_(offset(which))); }
		/** @copydoc fixed_erase */
		constexpr iterator fixed_erase(entity_t entity) { return to_iterator(fixed_erase_(offset(entity))); }

		/** Returns allocator used for the entity set. */
		[[nodiscard]] constexpr allocator_type get_allocator() const noexcept { return *alloc_base::get(); }

		constexpr void swap(basic_entity_set &other) noexcept
		{
			sek::detail::alloc_assert_swap(alloc(), other.alloc());
			sek::detail::alloc_swap(alloc(), other.alloc());

			using std::swap;
			swap(m_sparse, other.m_sparse);
			swap(m_dense, other.m_dense);
			swap(m_next, other.m_next);
		}
		friend constexpr void swap(basic_entity_set &a, basic_entity_set &b) noexcept { a.swap(b); }

	private:
		[[nodiscard]] constexpr auto to_iterator(size_type offset) noexcept { return iterator{this, offset + 1}; }
		[[nodiscard]] constexpr auto to_iterator(size_type offset) const noexcept
		{
			return const_iterator{this, offset + 1};
		}

		[[nodiscard]] constexpr auto &alloc() noexcept { return *alloc_base::get(); }
		[[nodiscard]] constexpr auto &alloc() const noexcept { return *alloc_base::get(); }

		[[nodiscard]] constexpr entity_t *alloc_page() { return alloc_traits::allocate(alloc(), page_size); }
		constexpr void dealloc_page(entity_t *page) { alloc_traits::deallocate(alloc(), page, page_size); }

		[[nodiscard]] constexpr entity_t *make_page()
		{
			auto page = alloc_page();
			std::fill_n(page, page_size, entity_t::tombstone());
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
				page = make_page();

			auto &slot = page[sparse_off(i)];
			SEK_ASSERT(slot.is_tombstone(), "Sparse slot already in use");
			return slot;
		}
		constexpr void release_pages()
		{
			for (auto page : m_sparse) dealloc_page(page);
		}

		constexpr void copy_sparse(const basic_entity_set &other)
		{
			for (size_type i = 0; auto src : other.m_sparse)
			{
				auto dst = m_sparse[i++] = alloc_page();
				std::uninitialized_copy_n(src, page_size, dst);
			}
		}
		constexpr void move_sparse(basic_entity_set &other)
		{
			for (size_type i = 0; auto src : m_sparse)
			{
				auto dst = alloc_page();
				std::uninitialized_copy_n(src, page_size, dst);
				other.dealloc_page(std::exchange(m_sparse[i++], dst));
			}
		}

		template<typename S, typename... Args>
		constexpr void invoke_sort(S &&s, size_type n, Args &&...args)
		{
			const auto first = m_dense.begin();
			const auto last = first + static_cast<std::ptrdiff_t>(n);
			std::invoke(std::forward<S>(s), first, last, std::forward<Args>(args)...);
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

	protected:
		constexpr virtual size_type push_back_(entity_t e)
		{
			const auto pos = m_dense.size();

			auto &slot = insert_sparse(e.index().value());
			m_dense.push_back(e);
			slot = entity_t{e.generation(), entity_t::index_type{pos}};

			return pos;
		}
		constexpr virtual size_type insert_(entity_t e)
		{
			/* Reuse an existing dense position if possible. */
			if (m_next.is_tombstone() && m_next.index() == entity_t::index_type::tombstone())
				return push_back_(e);
			else
			{
				const auto idx = m_next.index();
				const auto pos = idx.value();

				auto &slot = insert_sparse(e.index().value());
				slot = entity_t(e.generation(), idx);
				m_next = std::exchange(m_dense[pos], e);

				return pos;
			}
		}

		constexpr virtual size_type fixed_erase_(size_type idx)
		{
			const auto new_next = entity_t{entity_t::generation_type::tombstone(), entity_t::index_type{idx}};
			auto &dense = m_dense[idx];
			sparse_ref(dense.index().value()) = entity_t::tombstone();
			dense = std::exchange(m_next, new_next);
			return idx + 1;
		}
		constexpr virtual size_type erase_(size_type idx)
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
			}
			else
				sparse_ref(m_dense[last].index().value()) = entity_t::tombstone();
			m_dense.pop_back();
			return idx;
		}

		constexpr virtual void swap_(size_type, size_type) {}
		constexpr virtual void move_(size_type, size_type) {}

		sparse_data m_sparse;
		dense_data m_dense;

		/* Next dense entity available for reuse. */
		entity_t m_next = entity_t::tombstone();
	};

	using entity_set = basic_entity_set<>;
}	 // namespace sek::engine