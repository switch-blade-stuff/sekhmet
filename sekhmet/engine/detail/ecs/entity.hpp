/*
 * Created by switchblade on 30/05/22
 */

#pragma once

#include <algorithm>
#include <compare>
#include <vector>

#include "sekhmet/detail/alloc_util.hpp"
#include "sekhmet/detail/assert.hpp"
#include "sekhmet/detail/ebo_base_helper.hpp"

#include "fwd.hpp"

namespace sek::engine
{
	/** @brief An entity is an internal ID used to refer to a group of components.
	 *
	 * Entities have an index, used to uniquely identify an entity, and a generation,
	 * used to disambiguate entities that have been previously "deleted" from their world.
	 * Entities that do not represent a valid group of components are "tombstone" entities.
	 * Tombstone entities always compare equal to each other. */
	class entity_t
	{
	public:
		typedef std::size_t value_type;

		/** @brief Structure used to represent an entity generation. */
		class generation_type
		{
			friend class entity_t;

			constexpr static value_type mask = sizeof(value_type) >= sizeof(std::uint64_t) ? 0xff'ffff : 0xffff;
			constexpr static value_type offset = sizeof(value_type) >= sizeof(std::uint64_t) ? 40 : 16;

		public:
			/** Returns tombstone value of entity generation. */
			[[nodiscard]] constexpr static generation_type tombstone() noexcept { return generation_type{mask}; }
			/** Returns maximum valid value of entity generation. */
			[[nodiscard]] constexpr static generation_type max() noexcept { return generation_type{mask - 1}; }

		public:
			constexpr generation_type() noexcept = default;

			/** Initializes an entity generation from an underlying value type.
			 * @note Value must be 24-bit max. */
			constexpr explicit generation_type(value_type value) noexcept : m_value(value << offset) {}

			/** Checks if the entity generation is a tombstone. */
			[[nodiscard]] constexpr bool is_tombstone() const noexcept { return *this == tombstone(); }

			/** Checks if the entity generation is valid. */
			[[nodiscard]] constexpr bool valid() const noexcept { return (m_value & mask) == mask; }
			/** Returns the underlying integer value of the generation. */
			[[nodiscard]] constexpr value_type value() const noexcept { return m_value >> offset; }

			[[nodiscard]] constexpr auto operator<=>(const generation_type &) const noexcept = default;
			[[nodiscard]] constexpr bool operator==(const generation_type &) const noexcept = default;

		private:
			value_type m_value = 0;
		};
		/** @brief Structure used to represent an entity index. */
		class index_type
		{
			friend class entity_t;

			constexpr static value_type mask = sizeof(value_type) == sizeof(std::uint64_t) ? 0xff'ffff'ffff : 0xffff;

		public:
			/** Returns tombstone value of entity index. */
			[[nodiscard]] constexpr static index_type tombstone() noexcept { return index_type{mask}; }
			/** Returns maximum valid value of entity index. */
			[[nodiscard]] constexpr static index_type max() noexcept { return index_type{mask - 1}; }

		public:
			constexpr index_type() noexcept = default;

			/** Initializes an entity index from an underlying value type.
			 * @note Value must be 40-bit max. */
			constexpr explicit index_type(value_type value) noexcept : m_value(value) {}

			/** Checks if the entity index is a tombstone. */
			[[nodiscard]] constexpr bool is_tombstone() const noexcept { return *this == tombstone(); }

			/** Returns the underlying integer value of the index. */
			[[nodiscard]] constexpr value_type value() const noexcept { return m_value; }

			[[nodiscard]] constexpr auto operator<=>(const index_type &) const noexcept = default;
			[[nodiscard]] constexpr bool operator==(const index_type &) const noexcept = default;

		private:
			value_type m_value = 0;
		};

		/** Returns value of an invalid entity. */
		[[nodiscard]] constexpr static entity_t tombstone() noexcept;

	public:
		/** Initializes an invalid entity. */
		constexpr entity_t() noexcept = default;
		/** Initializes an entity from an index and the default generation (0). */
		constexpr entity_t(index_type idx) noexcept : m_value(idx.m_value) {}
		/** Initializes an entity from a generation and an index. */
		constexpr entity_t(generation_type gen, index_type idx) noexcept : m_value(gen.m_value | idx.m_value) {}

		/** Checks if the entity is a tombstone. */
		[[nodiscard]] constexpr bool is_tombstone() const noexcept
		{
			return generation() == generation_type::tombstone();
		}

		/** Returns generation of the entity. */
		[[nodiscard]] constexpr generation_type generation() const noexcept
		{
			generation_type result;
			result.m_value = m_value & (generation_type::mask << generation_type::offset);
			return result;
		}
		/** Returns index of the entity. */
		[[nodiscard]] constexpr index_type index() const noexcept { return index_type{m_value & index_type::mask}; }
		/** Returns the underlying integer value of the entity. */
		[[nodiscard]] constexpr value_type value() const noexcept { return m_value; }

		[[nodiscard]] constexpr auto operator<=>(const entity_t &other) const noexcept
		{
			if (((m_value & other.m_value) >> generation_type::offset) == generation_type::mask)
				return std::strong_ordering::equivalent;
			else
				return m_value <=> other.m_value;
		}
		[[nodiscard]] constexpr bool operator==(const entity_t &other) const noexcept
		{
			return ((m_value & other.m_value) >> generation_type::offset) == generation_type::mask || m_value == other.m_value;
		}

	private:
		value_type m_value = 0;
	};

	constexpr entity_t entity_t::tombstone() noexcept
	{
		return {generation_type::tombstone(), index_type::tombstone()};
	}

	[[nodiscard]] constexpr hash_t hash(entity_t e) noexcept { return e.value(); }

	namespace detail
	{
		template<typename Alloc, bool IsFixed>
		struct entity_set_impl : ebo_base_helper<Alloc>
		{
			using alloc_base = ebo_base_helper<Alloc>;
			using alloc_traits = std::allocator_traits<Alloc>;
			using sparse_alloc = typename alloc_traits::template rebind_alloc<entity_t *>;
			using sparse_t = std::vector<entity_t *, sparse_alloc>;
			using dense_t = std::vector<entity_t, Alloc>;
			using size_type = std::size_t;

			/* Sparse entities are allocated in pages to avoid wasting memory for un-used slots. */
			constexpr static std::size_t page_size = 1024;

			[[nodiscard]] constexpr static auto page_idx(auto n) noexcept { return n / page_size; }
			[[nodiscard]] constexpr static auto page_off(auto n) noexcept { return n % page_size; }

			constexpr entity_set_impl() noexcept = default;
			constexpr ~entity_set_impl() { release_pages(); }

			constexpr explicit entity_set_impl(const Alloc &alloc) : alloc_base(alloc), m_sparse(), m_dense(alloc) {}
			constexpr entity_set_impl(size_type n, const Alloc &alloc) : entity_set_impl(alloc) { reserve_impl(n); }
			constexpr entity_set_impl(const entity_set_impl &other)
				: alloc_base(sek::detail::make_alloc_copy(other.get_allocator())),
				  m_sparse(other.m_sparse),
				  m_dense(other.m_dense)
			{
				copy_pages(other);
			}
			constexpr entity_set_impl(const entity_set_impl &other, const Alloc &alloc)
				: alloc_base(alloc), m_sparse(other.m_sparse, alloc), m_dense(other.m_dense, alloc)
			{
				copy_pages(other);
			}
			constexpr entity_set_impl &operator=(const entity_set_impl &other)
			{
				if (this != &other) [[likely]]
				{
					release_pages();
					m_sparse = other.m_sparse;
					m_dense = other.m_dense;
					copy_data(other);
				}
				return *this;
			}

			constexpr entity_set_impl(entity_set_impl &&other)
				: m_sparse(std::move(other.m_sparse)), m_dense(std::move(other.m_dense))
			{
				if (alloc_traits::propagate_on_container_move_assignment::value ||
					sek::detail::alloc_eq(get_allocator(), other.get_allocator()))
					move_pages(other);
			}
			constexpr entity_set_impl(entity_set_impl &&other, const Alloc &alloc)
				: alloc_base(alloc), m_sparse(std::move(other.m_sparse), alloc), m_dense(std::move(other.m_dense), alloc)
			{
				if (alloc_traits::propagate_on_container_move_assignment::value ||
					sek::detail::alloc_eq(get_allocator(), other.get_allocator()))
					move_pages(other);
			}
			constexpr entity_set_impl &operator=(entity_set_impl &&other)
			{
				m_sparse = std::move(other.m_sparse);

				/* Copy pages & dense vector if the dense allocator does not allow move assignment. */
				if (alloc_traits::propagate_on_container_move_assignment::value ||
					sek::detail::alloc_eq(get_allocator(), other.get_allocator()))
				{
					m_dense = other.m_dense;
					move_pages(other);
				}
				else
					m_dense = std::move(other.m_dense);
				return *this;
			}

			[[nodiscard]] constexpr entity_t *alloc_page(size_type n = page_size)
			{
				return alloc_traits::allocate(get_allocator(), n);
			}
			[[nodiscard]] constexpr entity_t *make_page(size_type n = page_size)
			{
				auto page = alloc_page(n);
				std::uninitialized_fill_n(page, n, entity_t::tombstone());
				return page;
			}
			constexpr void dealloc_page(entity_t *page, size_type n = page_size)
			{
				alloc_traits::deallocate(get_allocator(), page, n);
			}

			[[nodiscard]] constexpr entity_t *get_sparse(size_type i) const noexcept
			{
				const auto idx = page_idx(i);
				const auto off = page_off(i);

				if (idx >= m_sparse.size() || m_sparse[idx] == nullptr) [[unlikely]]
					return nullptr;
				return m_sparse[idx] + off;
			}

			constexpr size_type insert_impl(entity_t e)
			{
				auto &slot = insert_sparse(e.index().value());
				const auto pos = m_dense.size();
				m_dense.push_back(e);
				slot = entity_t{e.generation(), entity_t::index_type{pos}};
				return pos;
			}
			constexpr size_type push_back_impl(entity_t e) { return insert_impl(e); }
			constexpr size_type erase_impl(size_type idx)
			{
				/* Swap with the last one & pop. */
				if (const auto last = m_dense.size() - 1; idx < last) [[likely]]
				{
					auto &from = m_dense[last];
					auto &to = m_dense[idx];
					*get_sparse(from.index().value()) = entity_t{from.generation(), entity_t::index_type{idx}};
					*get_sparse(to.index().value()) = entity_t::tombstone();
					to = from;
				}
				else
					*get_sparse(m_dense[last].index().value()) = entity_t::tombstone();
				m_dense.pop_back();
				return idx;
			}

			constexpr void swap(entity_set_impl &other) noexcept
			{
				sek::detail::alloc_assert_swap(get_allocator(), other.get_allocator());
				sek::detail::alloc_swap(get_allocator(), other.get_allocator());
				m_sparse.swap(other.m_sparse);
				m_dense.swap(other.m_dense);
			}

		protected:
			[[nodiscard]] constexpr auto &get_allocator() noexcept { return *alloc_base::get(); }
			[[nodiscard]] constexpr auto &get_allocator() const noexcept { return *alloc_base::get(); }

			[[nodiscard]] constexpr entity_t &alloc_sparse(size_type i)
			{
				const auto idx = page_idx(i);

				/* Make sure sparse page list has enough space. */
				m_sparse.resize(idx + 1, nullptr);

				/* Allocate the page if it is empty. */
				auto &page = m_sparse[idx];
				if (page == nullptr) [[unlikely]]
					page = make_page();
				auto &slot = page[page_off(i)];
				return slot;
			}
			[[nodiscard]] constexpr entity_t &insert_sparse(size_type i)
			{
				auto &slot = alloc_sparse(i);
				SEK_ASSERT(slot.is_tombstone(), "Sparse entity slot already in use.");
				return slot;
			}

			constexpr void move_pages(entity_set_impl &other)
			{
				/* Page pointers would have already been moved to out sparse set. */
				for (std::size_t i = 0; auto src : m_sparse)
				{
					auto dst = alloc_page();
					std::uninitialized_copy_n(src, page_size, dst);
					other.dealloc_page(std::exchange(m_sparse[i++], dst));
				}
			}
			constexpr void copy_pages(const entity_set_impl &other)
			{
				for (std::size_t i = 0; auto src : other.m_sparse)
				{
					auto dst = m_sparse[i++] = alloc_page();
					std::uninitialized_copy_n(src, page_size, dst);
				}
			}

		public:
			constexpr void reserve_impl(std::size_t n)
			{
				m_dense.reserve(n);
				m_sparse.resize(page_idx(n), nullptr);
			}
			constexpr void release_pages()
			{
				for (auto page : m_sparse)
					if (page != nullptr) dealloc_page(page);
			}

			sparse_t m_sparse; /* Array of sparse entity pages. */
			dense_t m_dense;   /* Dense array of entities. */
		};
		template<typename Alloc>
		struct entity_set_impl<Alloc, true> : entity_set_impl<Alloc, false>
		{
		private:
			using base_t = entity_set_impl<Alloc, false>;

		public:
			using alloc_traits = typename base_t::alloc_traits;
			using sparse_alloc = typename base_t::sparse_alloc;
			using sparse_t = typename base_t::sparse_t;
			using dense_t = typename base_t::dense_t;
			using size_type = typename base_t::size_type;

			constexpr entity_set_impl() noexcept = default;
			constexpr ~entity_set_impl() = default;

			constexpr entity_set_impl(const entity_set_impl &) = default;
			constexpr entity_set_impl &operator=(const entity_set_impl &) = default;
			constexpr entity_set_impl(entity_set_impl &&) = default;
			constexpr entity_set_impl &operator=(entity_set_impl &&) = default;

			constexpr explicit entity_set_impl(const Alloc &alloc) : base_t(alloc) {}
			constexpr entity_set_impl(size_type n, const Alloc &alloc) : base_t(n, alloc) {}
			constexpr entity_set_impl(const entity_set_impl &other, const Alloc &alloc)
				: base_t(other, alloc), m_next(other.m_next)
			{
			}
			constexpr entity_set_impl(entity_set_impl &&other, const Alloc &alloc)
				: base_t(std::move(other), alloc), m_next(other.m_next)
			{
			}

			constexpr size_type insert_impl(entity_t e)
			{
				/* Reuse an existing dense position if possible. */
				if (m_next.is_tombstone() && m_next.index() == entity_t::index_type::tombstone()) [[unlikely]]
					return base_t::insert_impl(e);
				else
				{
					auto &slot = base_t::insert_sparse(e.index().value());
					const auto idx = m_next.index();
					const auto pos = idx.value();
					slot = entity_t(e.generation(), idx);
					m_next = std::exchange(base_t::m_dense[pos], e);
					return pos;
				}
			}
			constexpr size_type erase_impl(size_type idx)
			{
				const auto new_next = entity_t{entity_t::generation_type::tombstone(), entity_t::index_type{idx}};
				base_t::m_dense[idx] = std::exchange(m_next, new_next);
				return idx + 1;
			}

			constexpr void swap(entity_set_impl &other) noexcept
			{
				base_t::swap(other);
				std::swap(m_next, other.m_next);
			}

			entity_t m_next = entity_t::tombstone(); /* Next dense entity available for reuse. */
		};

		struct default_sort
		{
			template<typename... Args>
			constexpr decltype(auto) operator()(Args &&...args) const
			{
				return std::sort(std::forward<Args>(args)...);
			}
		};
	}	 // namespace detail

	/** @brief Interface used to associate entities with component indices.
	 * @tparam Alloc Allocator used to allocate memory of the entity set.
	 * @tparam IsFixed Whether the entity set should use fixed dense indices, primarily used for component storage. */
	template<typename Alloc, bool IsFixed>
	class basic_entity_set_old : detail::entity_set_impl<Alloc, IsFixed>
	{
		using base_t = detail::entity_set_impl<Alloc, IsFixed>;
		using alloc_traits = typename base_t::alloc_traits;
		using sparse_alloc = typename base_t::sparse_alloc;
		using sparse_t = typename base_t::sparse_t;
		using dense_t = typename base_t::dense_t;

		class entity_iterator
		{
			friend class basic_entity_set_old;

		public:
			typedef entity_t value_type;
			typedef const entity_t *pointer;
			typedef const entity_t &reference;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;
			typedef std::random_access_iterator_tag iterator_category;

		private:
			constexpr explicit entity_iterator(pointer base, size_type off = 0) noexcept
				: m_base(base), m_off(static_cast<difference_type>(off))
			{
			}

		public:
			constexpr entity_iterator() noexcept = default;
			constexpr entity_iterator operator++(int) noexcept
			{
				auto temp = *this;
				++(*this);
				return temp;
			}
			constexpr entity_iterator &operator++() noexcept
			{
				--m_off;
				return *this;
			}
			constexpr entity_iterator &operator+=(difference_type n) noexcept
			{
				m_off -= n;
				return *this;
			}
			constexpr entity_iterator operator--(int) noexcept
			{
				auto temp = *this;
				--(*this);
				return temp;
			}
			constexpr entity_iterator &operator--() noexcept
			{
				++m_off;
				return *this;
			}
			constexpr entity_iterator &operator-=(difference_type n) noexcept
			{
				m_off += n;
				return *this;
			}

			constexpr entity_iterator operator+(difference_type n) const noexcept
			{
				return entity_iterator{m_base, static_cast<size_type>(m_off - n)};
			}
			constexpr entity_iterator operator-(difference_type n) const noexcept
			{
				return entity_iterator{m_base, static_cast<size_type>(m_off + n)};
			}
			constexpr difference_type operator-(const entity_iterator &other) const noexcept
			{
				return other.m_off - m_off;
			}

			/** Returns offset of the iterator from the base. */
			[[nodiscard]] constexpr size_type offset() const noexcept { return static_cast<size_type>(m_off - 1); }

			/** Returns pointer to the target entity. */
			[[nodiscard]] constexpr pointer get() const noexcept { return m_base + offset(); }
			/** @copydoc value */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }

			/** Returns reference to the entity at index `n` from the iterator.
			 * Equivalent to `*(*this + n)`. */
			[[nodiscard]] constexpr reference operator[](difference_type n) const noexcept { return *(*this + n); }
			/** Returns reference to the target entity. */
			[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }

			[[nodiscard]] constexpr auto operator<=>(const entity_iterator &other) const noexcept
			{
				return get() <=> other.get();
			}
			[[nodiscard]] constexpr bool operator==(const entity_iterator &other) const noexcept
			{
				return get() == other.get();
			}

			constexpr void swap(entity_iterator &other) noexcept
			{
				std::swap(m_base, other.m_base);
				std::swap(m_off, other.m_off);
			}
			friend constexpr void swap(entity_iterator &a, entity_iterator &b) noexcept { a.swap(b); }

		private:
			pointer m_base = {};
			difference_type m_off = 0;
		};

		constexpr static entity_t *assert_exists(entity_t *slot) noexcept
		{
			SEK_ASSERT(slot && !slot->is_tombstone(), "Entity must be present within the set");
			return slot;
		}

	public:
		typedef Alloc allocator_type;

		typedef entity_t value_type;
		typedef const entity_t *pointer;
		typedef const entity_t *const_pointer;
		typedef const entity_t &reference;
		typedef const entity_t &const_reference;
		typedef entity_iterator iterator;
		typedef entity_iterator const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

		typedef typename base_t::size_type size_type;
		typedef std::ptrdiff_t difference_type;

	public:
		constexpr basic_entity_set_old() = default;
		virtual ~basic_entity_set_old() = default;

		constexpr basic_entity_set_old(const basic_entity_set_old &) = default;
		constexpr basic_entity_set_old &operator=(const basic_entity_set_old &) = default;
		// clang-format off
		constexpr basic_entity_set_old(basic_entity_set_old &&) noexcept(sek::detail::nothrow_alloc_move_construct<Alloc, sparse_alloc>) = default;
		constexpr basic_entity_set_old &operator=(basic_entity_set_old &&) noexcept(sek::detail::nothrow_alloc_move_assign<Alloc, sparse_alloc>) = default;
		// clang-format on

		/** Initializes an entity set from an allocator. */
		constexpr explicit basic_entity_set_old(const allocator_type &alloc) : base_t(alloc) {}
		/** Initializes an entity set from an allocator and reserves n elements. */
		constexpr basic_entity_set_old(size_type n, const allocator_type &alloc = {}) : base_t(n, alloc) {}

		template<std::forward_iterator I, std::sentinel_for<I> S>
		constexpr basic_entity_set_old(I first, S last, const allocator_type &alloc = {}) : basic_entity_set_old(alloc)
		{
			insert(first, last);
		}
		template<std::random_access_iterator I, std::sentinel_for<I> S>
		constexpr basic_entity_set_old(I first, S last, const allocator_type &alloc = {})
			: basic_entity_set_old(static_cast<size_type>(std::distance(first, last)), alloc)
		{
			insert(first, last);
		}
		constexpr basic_entity_set_old(std::initializer_list<entity_t> init_list, const allocator_type &alloc = {})
			: basic_entity_set_old(init_list.size(), alloc)
		{
			insert(init_list.begin(), init_list.end());
		}

		/** Copy-constructs an entity set using the provided allocator. */
		constexpr basic_entity_set_old(const basic_entity_set_old &other, const allocator_type &alloc) : base_t(other, alloc) {}
		/** Move-constructs an entity set using the provided allocator. */
		constexpr basic_entity_set_old(basic_entity_set_old &&other, const allocator_type &alloc)
			: base_t(std::move(other.m_sparse), alloc)
		{
		}

		/** Returns iterator to the first entity in the set. */
		[[nodiscard]] constexpr auto begin() const noexcept { return iterator{data(), size()}; }
		/** @copydoc begin */
		[[nodiscard]] constexpr auto cbegin() const noexcept { return const_iterator{data(), size()}; }
		/** Returns iterator one past the last entity in the set. */
		[[nodiscard]] constexpr auto end() const noexcept { return iterator{data()}; }
		/** @copydoc end */
		[[nodiscard]] constexpr auto cend() const noexcept { return const_iterator{data()}; }
		/** Returns reverse iterator to the last entity in the set. */
		[[nodiscard]] constexpr auto rbegin() const noexcept { return reverse_iterator{end()}; }
		/** @copydoc rbegin */
		[[nodiscard]] constexpr auto crbegin() const noexcept { return const_reverse_iterator{cend()}; }
		/** Returns reverse iterator one past the first entity in the set. */
		[[nodiscard]] constexpr auto rend() const noexcept { return reverse_iterator{begin()}; }
		/** @copydoc rend */
		[[nodiscard]] constexpr auto crend() const noexcept { return const_reverse_iterator{cbegin()}; }

		/** Returns the size of the dense entity array.
		 * @note If the set supports fixed storage, dense array size will include the number of tombstones. */
		[[nodiscard]] constexpr size_type size() const noexcept { return base_t::m_dense.size(); }
		/** Checks if the dense entity array is empty.
		 * @note If the set supports fixed storage and there are any tombstones, `empty` may return erroneous result. */
		[[nodiscard]] constexpr bool empty() const noexcept { return base_t::m_dense.empty(); }
		/** Returns pointer to the dense entity array. */
		[[nodiscard]] constexpr pointer data() const noexcept { return base_t::m_dense.data(); }
		/** Returns reference to the first entity in the dense array. */
		[[nodiscard]] constexpr reference front() const noexcept { return base_t::m_dense.front(); }
		/** Returns reference to the last entity in the dense array. */
		[[nodiscard]] constexpr reference back() const noexcept { return base_t::m_dense.back(); }
		/** Returns reference to the entity in the dense array located at offset `i`. */
		[[nodiscard]] constexpr reference at(size_type i) const noexcept { return base_t::m_dense[i]; }
		/** @copydoc at */
		[[nodiscard]] constexpr reference operator[](size_type i) const noexcept { return at(i); }

		/** Checks if the entity set contains an entity. */
		[[nodiscard]] constexpr bool contains(entity_t e) const noexcept
		{
			const auto *slot = base_t::get_sparse(e.index().value());
			return slot && !slot->is_tombstone();
		}
		/** Returns an iterator to the specified entity, or an end iterator if the entity is not present within the set. */
		[[nodiscard]] constexpr iterator find(entity_t e) const noexcept
		{
			const auto *slot = base_t::get_sparse(e.index().value());
			return slot && !slot->is_tombstone() ? to_iterator(slot->index().value()) : end();
		}
		/** Returns the dense array offset of an entity iterator. */
		[[nodiscard]] constexpr size_type offset(const_iterator i) const noexcept { return i.offset(); }
		/** Returns the dense array offset of an entity.
		 * @note Will cause undefined behavior if the entity is not present within the set. */
		[[nodiscard]] constexpr size_type offset(entity_t e) const noexcept
		{
			return assert_exists(base_t::get_sparse(e.index().value()))->index().value();
		}

		/** Reserves space for `n` entities. */
		void reserve(std::size_t n) { reserve_(n); }

		/** Removes all entities from the set. */
		void clear()
		{
			erase(begin(), end());
			base_t::m_dense.clear();
		}
		/** Removes all entities from the set and de-allocates internal storage. */
		void purge()
		{
			clear();
			base_t::release_pages();
			base_t::m_sparse.clear();
			base_t::m_sparse.shrink_to_fit();
			base_t::m_dense.shrink_to_fit();
		}

		// clang-format off
		/** @copydoc insert
		 * @param args Arguments passed to constructor of `entity`. */
		template<typename... Args>
		iterator emplace(Args &&...args) requires std::constructible_from<entity_t, Args...>
		{
			return insert(entity_t{std::forward<Args>(args)...});
		}
		// clang-format on

		/** Pushes an entity into the set (at the end of the dense array) and returns iterator to it. */
		iterator push_back(entity_t e) { return push_back_(e); }

		/** Inserts an entity into the set (re-using slots if fixed storage is enabled) and returns iterator to it. */
		iterator insert(entity_t e) { return insert_(e); }
		/** Inserts a range of entities between `[first, last)` into the set (always at the end) and returns iterator to the first entity inserted. */
		template<std::input_iterator I, std::sentinel_for<I> S>
		iterator insert(I first, S last)
		{
			const auto offset = static_cast<difference_type>(base_t::m_dense.size());
			for (; first != last; first = std::next(first)) push_back(*first);
			return to_iterator(offset);
		}

		/** Erases an entity from the set and returns iterator to the next entity. */
		iterator erase(const_iterator which) { return erase_(which); }
		/** @copydoc erase
		 * @note Will cause undefined behavior if the entity is not present within the set. */
		iterator erase(entity_t e) { return erase(find(e)); }
		/** Erases all entities between `[first, last)`. */
		void erase(const_iterator first, const_iterator last)
		{
			while (first != last)
			{
				if constexpr (IsFixed) /* Skip tombstones. */
					if (first->is_tombstone()) continue;
				erase(first++);
			}
		}

		/** Updates generation of an entity contained within the set.
		 * @param Entity to update generation of. */
		constexpr void update(entity_t e) { update(e, e.generation()); }
		/** @copydoc update
		 * @param gen Generation value to use. */
		constexpr void update(entity_t e, entity_t::generation_type gen)
		{
			const auto idx = e.index();
			auto slot = assert_exists(base_t::get_sparse(idx.value()));
			*slot = entity_t{gen, slot->index()};
			base_t::m_dense[slot->index().value()] = entity_t{gen, idx};
		}

		/** Swaps entities of the entity set. */
		void swap(size_type a_idx, size_type b_idx)
		{
			swap_(a_idx, b_idx);
			auto &e_lhs = base_t::m_dense[a_idx];
			auto &e_rhs = base_t::m_dense[b_idx];
			std::swap(*base_t::get_sparse(e_lhs.index().value()), *base_t::get_sparse(e_rhs.index().value()));
			std::swap(e_lhs, e_rhs);
		}
		/** @copydoc swap */
		void swap(const_iterator a, const_iterator b) { swap(a.offset(), b.offset()); }
		/** @copydoc swap */
		void swap(entity_t a, entity_t b) { swap(offset(a), offset(b)); }

		/** Removes tombstone entities (if any) from the set. */
		void pack()
		{
			if constexpr (IsFixed)
			{
				size_type from = size(), to;
				auto skip_base = [&]()
				{
					while (base_t::m_dense[from - 1].is_tombstone()) --from;
				};
				skip_base();
				for (auto *ptr = &this->m_next; ptr->index() != entity_t::index_type::tombstone(); ptr = &base_t::m_dense[to])
				{
					to = ptr->index().value();
					if (to < from)
					{
						move_(--from, to);

						auto &e_from = base_t::m_dense[from];
						auto &e_to = base_t::m_dense[to];
						std::swap(e_from, e_to);

						*base_t::get_sparse(e_to.index().value()) = entity_t{e_to.generation(), entity_t::index_type{to}};
						*ptr = entity_t{entity_t::generation_type::tombstone(), entity_t::index_type{from}};
						skip_base();
					}
				}
				base_t::m_next = entity_t::tombstone();
				base_t::m_dense.resize(from);
			}
		}

		/** Sorts entities `[0, n)` of the set.
		 * @param n Amount of elements to sort.
		 * @param sort Functor to use for sorting. `std::sort` used by default.
		 * @param args Arguments passed to the sort functor. */
		template<typename Sort = detail::default_sort, typename... Args>
		void sort_n(size_type n, Sort sort = {}, Args &&...args)
		{
			SEK_ASSERT(n <= size());
			if constexpr (IsFixed)
			{
				SEK_ASSERT(base_t::m_next.is_tombstone(), "Dense array must be packed for sorting");
			}

			/* Sort dense entity array, then fix sparse entities. */
			sort(base_t::m_dense.begin(), base_t::m_dense.begin() + static_cast<std::ptrdiff_t>(n), std::forward<Args>(args)...);
			for (auto item = begin(), last = iterator{data(), n}; item != last; ++item)
			{
				entity_t &slot = *base_t::get_sparse(item->index().value());
				const auto old_pos = slot.index().value();
				const auto new_pos = item.offset();

				/* Let any derived type know we are swapping entities & update the sparse index. */
				swap_(old_pos, new_pos);
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
				if (const auto self = at(i), other = *to; contains(other))
				{
					if (other != self) swap(self, other);
					--i;
				}
		}

		constexpr void swap(basic_entity_set_old &other) noexcept { base_t::swap(other); }
		friend constexpr void swap(basic_entity_set_old &a, basic_entity_set_old &b) noexcept { a.swap(b); }

	protected:
		virtual iterator push_back_(entity_t e) { return to_iterator(base_t::push_back_impl(e)); }
		virtual iterator insert_(entity_t e) { return to_iterator(base_t::insert_impl(e)); }
		virtual iterator erase_(const_iterator which) { return to_iterator(base_t::erase_impl(which.offset())); }

		virtual void reserve_(size_type n) { base_t::reserve_impl(n); }

		virtual void move_(size_type, size_type) {}
		virtual void swap_(size_type, size_type) {}

	private:
		[[nodiscard]] constexpr iterator to_iterator(size_type i) const noexcept { return iterator{data(), i + 1}; }
	};

	using entity_set = basic_entity_set_old<>;
}	 // namespace sek::engine

template<>
struct std::hash<sek::engine::entity_t>
{
	[[nodiscard]] constexpr sek::hash_t operator()(sek::engine::entity_t e) noexcept { return sek::engine::hash(e); }
};