/*
 * Created by switchblade on 19/07/22
 */

#pragma once

#include "sekhmet/detail/event.hpp"
#include "sekhmet/detail/meta_util.hpp"

#include "../type_info.hpp"
#include "entity_set.hpp"

namespace sek::engine
{
	/** @brief Structure used to store a set of entities and provide a type-erased access to their components. */
	class basic_component_set : basic_entity_set<std::allocator<entity_t>>
	{
		template<typename>
		friend class component_set;
		friend class entity_world;

		using base_set = basic_entity_set<std::allocator<entity_t>>;

	public:
		typedef event<void(entity_world &, entity_t)> event_type;

		typedef typename base_set::value_type value_type;
		typedef typename base_set::pointer pointer;
		typedef typename base_set::const_pointer const_pointer;
		typedef typename base_set::reference reference;
		typedef typename base_set::const_reference const_reference;
		typedef typename base_set::iterator iterator;
		typedef typename base_set::const_iterator const_iterator;
		typedef typename base_set::reverse_iterator reverse_iterator;
		typedef typename base_set::const_reverse_iterator const_reverse_iterator;
		typedef typename base_set::difference_type difference_type;
		typedef typename base_set::size_type size_type;

	protected:
		constexpr basic_component_set(type_info type, entity_world &world) : m_world(&world), m_type(type) {}
		constexpr basic_component_set(type_info type, entity_world &world, size_type n)
			: base_set(n), m_world(&world), m_type(type)
		{
		}

		// clang-format off
		constexpr basic_component_set(basic_component_set &&)
			noexcept(std::is_nothrow_move_constructible_v<base_set> &&
					 std::is_nothrow_move_constructible_v<event_type>) = default;
		constexpr basic_component_set &operator=(basic_component_set &&)
			noexcept(std::is_nothrow_move_assignable_v<base_set> &&
			         std::is_nothrow_move_assignable_v<event_type>) = default;
		// clang-format on

	public:
		basic_component_set() = delete;
		basic_component_set(const basic_component_set &) = delete;
		basic_component_set &operator=(const basic_component_set &) = delete;

		constexpr virtual ~basic_component_set() = default;

		/** @copydoc base_set::begin */
		[[nodiscard]] constexpr iterator begin() noexcept { return base_set::begin(); }
		/** @copydoc base_set::begin */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return base_set::begin(); }
		/** @copydoc base_set::cbegin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return base_set::cbegin(); }
		/** @copydoc base_set::end */
		[[nodiscard]] constexpr iterator end() noexcept { return base_set::end(); }
		/** @copydoc base_set::end */
		[[nodiscard]] constexpr const_iterator end() const noexcept { return base_set::end(); }
		/** @copydoc base_set::cend */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return base_set::cend(); }
		/** @copydoc base_set::rbegin */
		[[nodiscard]] constexpr reverse_iterator rbegin() noexcept { return base_set::rbegin(); }
		/** @copydoc base_set::rbegin */
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return base_set::rbegin(); }
		/** @copydoc base_set::crbegin */
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return base_set::crbegin(); }
		/** @copydoc base_set::rend */
		[[nodiscard]] constexpr reverse_iterator rend() noexcept { return base_set::rend(); }
		/** @copydoc base_set::rend */
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return base_set::rend(); }
		/** @copydoc base_set::crend */
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return base_set::crend(); }

		/** @copydoc base_set::data */
		[[nodiscard]] constexpr const entity_t *data() const noexcept { return base_set::data(); }
		/** @copydoc base_set::at */
		[[nodiscard]] constexpr reference at(size_type i) const noexcept { return base_set::at(i); }
		/** @copydoc base_set::size */
		[[nodiscard]] constexpr size_type size() const noexcept { return base_set::size(); }
		/** @copydoc base_set::empty */
		[[nodiscard]] constexpr bool empty() const noexcept { return base_set::empty(); }
		/** @copydoc base_set::contains */
		[[nodiscard]] constexpr bool contains(entity_t entity) const noexcept { return base_set::contains(entity); }

		/** @copydoc base_set::find */
		[[nodiscard]] constexpr iterator find(entity_t entity) noexcept { return base_set::find(entity); }
		/** @copydoc base_set::find */
		[[nodiscard]] constexpr const_iterator find(entity_t entity) const noexcept { return base_set::find(entity); }

		/** @copydoc base_set::offset */
		[[nodiscard]] constexpr size_type offset(entity_t entity) const noexcept { return base_set::offset(entity); }
		/** @copydoc base_set::offset */
		[[nodiscard]] constexpr size_type offset(const_iterator which) const noexcept
		{
			return base_set::offset(which);
		}

		/** Returns reference to the parent world. */
		[[nodiscard]] constexpr entity_world &world() const noexcept { return *m_world; }
		/** Returns type info of the stored component type. */
		[[nodiscard]] constexpr type_info type() const noexcept { return m_type; }

		/** Returns `any_ref` reference to the component at the specified offset. */
		[[nodiscard]] virtual any_ref get_any(size_type i) noexcept = 0;
		/** @copydoc get_any */
		[[nodiscard]] virtual any_ref get_any(size_type i) const noexcept = 0;
		/** Returns `any_ref` reference to the component of the specified entity. */
		[[nodiscard]] virtual any_ref get_any(const_iterator which) noexcept = 0;
		/** @copydoc get_any */
		[[nodiscard]] virtual any_ref get_any(const_iterator which) const noexcept = 0;
		/** @copydoc get_any
		 * @warning Using an entity not contained within the set will result in undefined behavior. */
		[[nodiscard]] virtual any_ref get_any(entity_t entity) noexcept = 0;
		/** @copydoc get_any */
		[[nodiscard]] virtual any_ref get_any(entity_t entity) const noexcept = 0;

		/** Rebinds component set to use new world instance. */
		constexpr void rebind(entity_world &world) noexcept { m_world = &world; }

		/** @copydoc base_set::update */
		constexpr void update(entity_t entity) { base_set::update(entity); }
		/** @copydoc base_set::update */
		constexpr void update(entity_t entity, entity_t::generation_type gen) { base_set::update(entity, gen); }

		/** Swaps entities and components of the set. */
		constexpr void swap(size_type a, size_type b) { base_set::swap(a, b); }
		/** @copydoc swap */
		constexpr void swap(const_iterator a, const_iterator b) { base_set::swap(a, b); }
		/** @copydoc swap */
		constexpr void swap(entity_t a, entity_t b) { base_set::swap(a, b); }

		/** Removes tombstones (if any) from the set. */
		constexpr void pack() { base_set::pack(); }
		/** Removes all entities and components from the set. */
		constexpr void clear() { base_set::clear(); }

		/** @copydoc base_set::sort_n */
		template<typename Sort = detail::default_sort, typename... Args>
		constexpr void sort_n(size_type n, Sort sort = {}, Args &&...args)
		{
			base_set::sort_n(n, sort, std::forward<Args>(args)...);
		}
		/** @copydoc base_set::sort */
		template<typename Sort = detail::default_sort, typename... Args>
		constexpr void sort(Sort sort = {}, Args &&...args)
		{
			base_set::sort(sort, std::forward<Args>(args)...);
		}
		/** @copydoc base_set::sort */
		constexpr void sort(const_iterator from, const_iterator to) { base_set::sort(from, to); }
		/** @copydoc base_set::sort */
		template<std::bidirectional_iterator I>
		constexpr void sort(I from, I to)
		{
			base_set::sort(from, to);
		}

		/** Inserts an entity and it's component into the set. Tombstones (if any) are re-used.
		 * @param entity Entity to insert.
		 * @param value `any` containing the value of the component.
		 * @return Iterator to the inserted entity.
		 * @warning Using an entity already contained within the set will result in undefined behavior. */
		virtual iterator insert(entity_t entity, any value) = 0;
		/** Inserts an entity and it's component into the set. Entities are always pushed to the end (tombstones are not re-used).
		 * @param entity Entity to insert.
		 * @param value `any` containing the value of the component.
		 * @return Iterator to the inserted entity.
		 * @warning Using an entity already contained within the set will result in undefined behavior. */
		virtual iterator push_back(entity_t entity, any value) = 0;

		/** Inserts entities in the range `[first, last)` into the set (always at the end).
		 * @warning Using entities already present will result in undefined behavior. */
		template<std::forward_iterator I, std::sentinel_for<I> S>
		constexpr iterator insert(I first, S last)
		{
			for (; first != last; first = std::next(first)) push_back(*first);
			return end();
		}

		/** Erases the entity and it's component from the set using the preferred method (swap & pop if component type is not fixed). */
		constexpr iterator erase(const_iterator which) { return base_set::erase(which); }
		/** @copydoc erase */
		constexpr iterator erase(entity_t entity) { return base_set::erase(entity); }
		/** Erases the entity and it's component from the set in-place, leaving a tombstone. */
		constexpr iterator fixed_erase(const_iterator which) { return base_set::fixed_erase(which); }
		/** @copydoc fixed_erase */
		constexpr iterator fixed_erase(entity_t entity) { return base_set::fixed_erase(entity); }

		/** Returns event proxy for the component replace event. */
		[[nodiscard]] constexpr event_proxy<event_type> on_replace() noexcept { return {m_replace}; }
		/** Returns event proxy for the component create event. */
		[[nodiscard]] constexpr event_proxy<event_type> on_create() noexcept { return {m_create}; }
		/** Returns event proxy for the component remove event. */
		[[nodiscard]] constexpr event_proxy<event_type> on_remove() noexcept { return {m_remove}; }

	protected:
		using base_set::swap;

		using base_set::erase_;
		using base_set::fixed_erase_;
		using base_set::insert_;
		using base_set::move_;
		using base_set::push_back_;
		using base_set::swap_;

	protected:
		constexpr void dispatch_replace(size_type idx) { m_replace(world(), base_set::at(idx)); }
		constexpr void dispatch_create(size_type idx) { m_create(world(), base_set::at(idx)); }
		constexpr void dispatch_remove(size_type idx) { m_remove(world(), base_set::at(idx)); }

	private:
		entity_world *m_world;
		event_type m_replace;
		event_type m_create;
		event_type m_remove;
		type_info m_type;
	};

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
			using alloc_type = typename component_traits<T>::allocator_type;
			using alloc_traits = std::allocator_traits<alloc_type>;
			using alloc_base = ebo_base_helper<alloc_type>;

			using pages_alloc = typename alloc_traits::template rebind_alloc<T *>;
			using pages_data = std::vector<T *, pages_alloc>;

		public:
			component_pool(const component_pool &) = delete;
			component_pool &operator=(const component_pool &) = delete;

			constexpr component_pool() = default;

			// clang-format off
			constexpr component_pool(component_pool &&other)
				noexcept(sek::detail::nothrow_alloc_move_construct<alloc_type> &&
						 std::is_nothrow_move_constructible_v<pages_data>)
				: alloc_base(std::move(other)), m_pages(std::move(other.m_pages))
			{
				assert_alloc(other);
			}
			constexpr component_pool &operator=(component_pool &&other)
				noexcept(sek::detail::nothrow_alloc_move_assign<alloc_type> &&
				         std::is_nothrow_move_assignable_v<pages_data>)
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
				const auto pages = page_idx(n) + 1;
				m_pages.resize(pages, nullptr);
				for (size_type i = 0; i < pages; ++i)
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
				sek::detail::alloc_assert_swap(alloc(), other.alloc());
				sek::detail::alloc_swap(alloc(), other.alloc());
				std::swap(m_pages, other.m_pages);
			}

		private:
			[[nodiscard]] constexpr auto &alloc() noexcept { return *alloc_base::get(); }
			[[nodiscard]] constexpr auto &alloc() const noexcept { return *alloc_base::get(); }
			constexpr void assert_alloc(const component_pool &other [[maybe_unused]])
			{
				SEK_ASSERT(alloc_traits::propagate_on_container_move_assignment::value ||
						   sek::detail::alloc_eq(alloc(), other.alloc()));
			}

			[[nodiscard]] constexpr T *alloc_page() { return alloc_traits::allocate(alloc(), page_size); }
			constexpr void dealloc_page(T *page) { alloc_traits::deallocate(alloc(), page, page_size); }
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
		concept empty_component = std::is_empty_v<T>;
		template<empty_component T>
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
	}	 // namespace detail

	/** @brief Type-specific implementation of component set. */
	template<typename T>
	class component_set final : public basic_component_set
	{
		using pool_t = detail::component_pool<T>;
		using base_t = basic_component_set;
		using base_iter = typename base_t::iterator;

	public:
		typedef typename base_t::event_type event_type;
		typedef typename base_t::difference_type difference_type;
		typedef typename base_t::size_type size_type;

		typedef typename component_traits<T>::allocator_type component_allocator;
		typedef T component_type;

	private:
		template<bool IsConst>
		using ref_t = std::pair<const entity_t &, std::conditional_t<IsConst, std::add_const_t<T>, T> &>;

		template<bool>
		class component_iterator;
		template<bool IsConst>
		class component_ptr
		{
			template<bool>
			friend class component_iterator;

			constexpr explicit component_ptr(ref_t<IsConst> ref) noexcept : m_ref(ref) {}

		public:
			component_ptr() = delete;

			constexpr component_ptr(const component_ptr &) noexcept = default;
			constexpr component_ptr &operator=(const component_ptr &) noexcept = default;
			constexpr component_ptr(component_ptr &&) noexcept = default;
			constexpr component_ptr &operator=(component_ptr &&) noexcept = default;

			[[nodiscard]] constexpr auto operator*() const noexcept { return m_ref; }
			[[nodiscard]] constexpr auto *get() const noexcept { return std::addressof(m_ref); }
			[[nodiscard]] constexpr auto *operator->() const noexcept { return get(); }

			[[nodiscard]] constexpr auto operator<=>(const component_ptr &other) const noexcept
			{
				return m_ref.first <=> other->first;
			}
			[[nodiscard]] constexpr bool operator==(const component_ptr &other) const noexcept
			{
				return m_ref.first == other->first;
			}

		private:
			ref_t<IsConst> m_ref;
		};
		template<bool IsConst>
		class component_iterator
		{
			friend class component_set;

		public:
			typedef std::pair<entity_t, T> value_type;
			typedef component_ptr<IsConst> pointer;
			typedef ref_t<IsConst> reference;
			typedef typename base_t::size_type size_type;
			typedef typename base_t::difference_type difference_type;
			typedef std::random_access_iterator_tag iterator_category;

		private:
			using parent_t = std::conditional_t<IsConst, const component_set<T>, component_set<T>>;

			constexpr explicit component_iterator(parent_t *parent, size_type off = 0) noexcept
				: m_parent(parent), m_off(static_cast<difference_type>(off))
			{
			}
			constexpr component_iterator(parent_t *parent, difference_type off) noexcept : m_parent(parent), m_off(off)
			{
			}

		public:
			constexpr component_iterator() noexcept = default;

			template<bool OtherConst, typename = std::enable_if_t<IsConst && !OtherConst>>
			constexpr component_iterator(const component_iterator<OtherConst> &other) noexcept
				: m_parent(other.m_parent), m_off(other.m_off)
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

			[[nodiscard]] constexpr component_iterator operator+(difference_type n) const noexcept
			{
				return component_iterator{m_parent, m_off - n};
			}
			[[nodiscard]] constexpr component_iterator operator-(difference_type n) const noexcept
			{
				return component_iterator{m_parent, m_off + n};
			}
			[[nodiscard]] constexpr difference_type operator-(const component_iterator &other) const noexcept
			{
				return other.m_off - m_off;
			}

			/** Returns the offset of the iterator from the base. */
			[[nodiscard]] constexpr size_type offset() const noexcept { return static_cast<size_type>(m_off - 1); }

			/** Returns fancy pointer to the a pair of references to the target entity and it's component. */
			[[nodiscard]] constexpr pointer get() const noexcept { return pointer{operator*()}; }
			/** @copydoc get */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
			/** Returns pair of references to the target entity and it's component. */
			[[nodiscard]] constexpr reference operator*() const noexcept { return operator[](0); }
			/** Returns pair of references to the entity at offset `i` from this iterator and it's component. */
			[[nodiscard]] constexpr reference operator[](difference_type i) const noexcept
			{
				const auto off = offset() + static_cast<size_type>(i);
				return reference{m_parent->entity_ref(off), m_parent->component_ref(off)};
			}

			[[nodiscard]] constexpr auto operator<=>(const component_iterator &other) const noexcept
			{
				return m_off <=> other.m_off;
			}
			[[nodiscard]] constexpr bool operator==(const component_iterator &) const noexcept = default;

			constexpr void swap(component_iterator &other) noexcept
			{
				std::swap(m_parent, other.m_parent);
				std::swap(m_off, other.m_off);
			}
			friend constexpr void swap(component_iterator &a, component_iterator &b) noexcept { a.swap(b); }

		private:
			parent_t *m_parent = nullptr;
			difference_type m_off = 0;
		};

	public:
		typedef component_iterator<false> iterator;
		typedef component_iterator<true> const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

		typedef typename iterator::value_type value_type;
		typedef typename iterator::pointer pointer;
		typedef typename const_iterator::pointer const_pointer;
		typedef typename iterator::reference reference;
		typedef typename const_iterator::reference const_reference;

	private:
		constexpr static size_type page_size = component_traits<T>::page_size;
		constexpr static bool is_fixed = fixed_component<T>;

	public:
		component_set() = delete;
		component_set(const component_set &) = delete;
		component_set &operator=(const component_set &) = delete;

		/** Initializes component storage for the specified world. */
		constexpr component_set(entity_world &world) : base_t(type_info::get<T>(), world) {}
		/** Initializes component storage for the specified world and reserves `n` components. */
		constexpr component_set(entity_world &world, size_type n) : base_t(type_info::get<T>(), world, n)
		{
			reserve_impl(n);
		}

		// clang-format off
		constexpr component_set(component_set &&)
			noexcept(std::is_nothrow_move_constructible_v<base_t> &&
					 std::is_nothrow_move_constructible_v<pool_t>) = default;
		constexpr component_set &operator=(component_set &&)
			noexcept(std::is_nothrow_move_assignable_v<base_t> &&
			         std::is_nothrow_move_assignable_v<pool_t>) = default;
		// clang-format on

		constexpr ~component_set() final
		{
			/* Destroy all components before releasing pages. */
			clear();
			m_pool.release_pages();
		}

		/** Returns iterator to the first entity of the set and it's component. */
		[[nodiscard]] constexpr iterator begin() noexcept { return to_iterator(base_t::begin().offset()); }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return to_iterator(base_t::begin().offset()); }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
		/** Returns iterator one past the last entity of the set. */
		[[nodiscard]] constexpr iterator end() noexcept { return to_iterator(base_t::end().offset()); }
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator end() const noexcept { return to_iterator(base_t::end().offset()); }
		/** @copydoc cend */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }
		/** Returns reverse iterator to the last entity of the set and it's component. */
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

		/** Returns iterator to the specified entity and it's component or an end iterator if such entity does
		 * not exist within the set. */
		[[nodiscard]] constexpr iterator find(entity_t entity) noexcept
		{
			return to_iterator(base_t::find(entity).offset());
		}
		/** @copydoc find */
		[[nodiscard]] constexpr const_iterator find(entity_t entity) const noexcept
		{
			return to_iterator(base_t::find(entity).offset());
		}

		/** @copydoc base_t::offset(entity_t) */
		[[nodiscard]] constexpr size_type offset(entity_t entity) const noexcept { return base_t::offset(entity); }
		/** @copydoc base_t::offset(base_iter) */
		[[nodiscard]] constexpr size_type offset(const_iterator which) const noexcept { return which.offset(); }

		/** Returns reference to the component of an entity at the specified offset . */
		[[nodiscard]] constexpr auto &get(size_type i) noexcept { return component_ref(i); }
		/** @copydoc get */
		[[nodiscard]] constexpr auto &get(size_type i) const noexcept { return component_ref(i); }
		/** Returns reference to the component of the specified entity. */
		[[nodiscard]] constexpr auto &get(iterator which) noexcept { return get(offset(which)); }
		/** @copydoc get */
		[[nodiscard]] constexpr auto &get(const_iterator which) const noexcept { return get(offset(which)); }
		/** @copydoc get
		 * @warning Using an entity not contained within the set will result in undefined behavior. */
		[[nodiscard]] constexpr auto &get(entity_t entity) noexcept { return get(offset(entity)); }
		/** @copydoc get */
		[[nodiscard]] constexpr auto &get(entity_t entity) const noexcept { return get(offset(entity)); }

		/** @copydoc base_t::get_any */
		[[nodiscard]] any_ref get_any(size_type i) noexcept final { return forward_any(component_ref(i)); }
		/** @copydoc base_t::get_any */
		[[nodiscard]] any_ref get_any(size_type i) const noexcept final { return forward_any(component_ref(i)); }
		/** @copydoc base_t::get_any */
		[[nodiscard]] any_ref get_any(entity_t entity) noexcept final { return get_any(offset(entity)); }
		/** @copydoc base_t::get_any */
		[[nodiscard]] any_ref get_any(entity_t entity) const noexcept final { return get_any(offset(entity)); }

		/** Reserves space for `n` entities and components. */
		constexpr void reserve(size_type n)
		{
			base_t::reserve(n);
			reserve_impl(n);
		}

		// clang-format off
		/** @brief Replaces component of the specified entity with an in-place constructed instance.
		 * @param entity Entity whose component to replace.
		 * @param args Arguments passed to component's constructor.
		 * @return Iterator to the replaced entity and component.
		 * @warning Using an entity not contained within the set will result in undefined behavior. */
		template<typename... Args>
		constexpr iterator replace(entity_t entity, Args &&...args) requires std::constructible_from<T, Args...>
		{
			return to_iterator(replace_impl(offset(entity), std::forward<Args>(args)...));
		}
		/** @copybrief replace
		 * @param which Iterator to the entity whose component to replace.
		 * @param args Arguments passed to component's constructor.
		 * @return Iterator to the replaced entity and component. */
		template<typename... Args>
		constexpr iterator replace(const_iterator which, Args &&...args) requires std::constructible_from<T, Args...>
		{
			return to_iterator(replace_impl(offset(which), std::forward<Args>(args)...));
		}

		/** @brief Inserts the specified entity into the set and constructs it's component in-place. Tombstones (if any) are re-used.
		 * @param entity Entity to insert.
		 * @param args Arguments passed to component's constructor.
		 * @return Pair of references to the inserted entity and component.
		 * @warning Using an entity already contained within the set will result in undefined behavior. */
		template<typename... Args>
		constexpr reference emplace(entity_t entity, Args &&...args) requires std::constructible_from<T, Args...>
		{
			return *to_iterator(emplace_impl(entity, std::forward<Args>(args)...));
		}
		/** @brief Inserts the specified entity into the set and constructs it's component in-place.
		 * Entities and components are always pushed to the end (tombstones are not re-used).
		 * @copydetails emplace */
		template<typename... Args>
		constexpr reference emplace_back(entity_t entity, Args &&...args) requires std::constructible_from<T, Args...>
		{
			return *to_iterator(emplace_back_impl(entity, std::forward<Args>(args)...));
		}
		// clang-format on

		/** @brief Replaces component of the specified entity with a new value.
		 * @param entity Entity whose component to replace.
		 * @param value New value of the component.
		 * @return Iterator to the replaced entity and component.
		 * @warning Using an entity not contained within the set will result in undefined behavior. */
		constexpr iterator replace(entity_t entity, const T &value)
		{
			return to_iterator(replace_impl(offset(entity), value));
		}
		/** @copydoc replace */
		template<typename U = T>
		constexpr iterator replace(entity_t entity, U &&value)
		{
			return to_iterator(replace_impl(offset(entity), std::forward<U>(value)));
		}
		/** @copybrief replace
		 * @param which Iterator to the entity whose component to replace.
		 * @param value New value of the component.
		 * @return Iterator to the replaced entity and component. */
		template<typename... Args>
		constexpr iterator replace(const_iterator which, const T &value)
		{
			return to_iterator(replace_impl(offset(which), value));
		}
		/** @copydoc replace */
		template<typename U = T>
		constexpr iterator replace(const_iterator which, U &&value)
		{
			return to_iterator(replace_impl(offset(which), std::forward<U>(value)));
		}

		/** @brief Attempts to replace component of the specified entity with an in-place constructed instance.
		 * @param entity Entity whose component to replace.
		 * @param args Arguments passed to component's constructor.
		 * @return Iterator to the replaced entity and component or end iterator if such entity does not exist. */
		template<typename... Args>
		constexpr iterator try_replace(entity_t entity, Args &&...args)
		{
			if (const auto pos = find(entity); pos != end()) [[likely]]
				return replace(pos, std::forward<Args>(args)...);
			else
				return pos;
		}

		/** @brief Attempts to insert the specified entity into the set and construct it's component in-place. Tombstones (if any) are re-used.
		 * @param entity Entity to insert.
		 * @param args Arguments passed to component's constructor.
		 * @return Pair where first is a pair of references to the potentially inserted entity and it's component
		 * and second is a boolean indicating whether the entity was inserted (`true` if inserted, `false` otherwise). */
		template<typename... Args>
		constexpr std::pair<reference, bool> try_emplace(entity_t entity, Args &&...args)
		{
			if (const auto existing = find(entity); existing == end()) [[likely]]
				return {emplace(entity, std::forward<Args>(args)...), true};
			else
				return {*existing, false};
		}
		/** Attempts to insert the specified entity into the set and construct it's component in-place.
		 * Entities and components are always pushed to the end (tombstones are not re-used).
		 * @copydetails try_emplace */
		template<typename... Args>
		constexpr std::pair<reference, bool> try_emplace_back(entity_t entity, Args &&...args)
		{
			if (const auto existing = find(entity); existing == end()) [[likely]]
				return {emplace_back(entity, std::forward<Args>(args)...), true};
			else
				return {*existing, false};
		}

		/** @brief Inserts the specified entity into the set and constructs it's component in-place or
		 * replaces the component if such entity already exists. Tombstones (if any) are re-used.
		 * @param entity Entity to insert.
		 * @param args Arguments passed to component's constructor.
		 * @return Pair where first is a pair of references to the potentially inserted entity and it's component
		 * and second is a boolean indicating whether the entity was inserted (`true` if inserted, `false` if replaced). */
		template<typename... Args>
		constexpr std::pair<reference, bool> emplace_or_replace(entity_t entity, Args &&...args)
		{
			if (const auto existing = find(entity); existing != end()) [[unlikely]]
				return {*replace(existing, std::forward<Args>(args)...), false};
			else
				return {emplace(entity, std::forward<Args>(args)...), true};
		}
		/** Inserts the specified entity into the set and construct it's component in-place or
		 * replaces the component if such entity already exists. Entities and components are always
		 * pushed to the end (tombstones are not re-used).
		 * @copydetails try_emplace */
		template<typename... Args>
		constexpr std::pair<reference, bool> emplace_back_or_replace(entity_t entity, Args &&...args)
		{
			if (const auto existing = find(entity); existing != end()) [[unlikely]]
				return {*replace(existing, std::forward<Args>(args)...), false};
			else
				return {emplace_back(entity, std::forward<Args>(args)...), true};
		}

		/** @brief Inserts the specified entity into the set and default-constructs it's component. Tombstones (if any) are re-used.
		 * @param entity Entity to insert.
		 * @return Iterator to the inserted entity and component.
		 * @warning Using an entity already contained within the set will result in undefined behavior. */
		constexpr iterator insert(entity_t entity) { return to_iterator(emplace_impl(entity)); }
		/** @brief Inserts the specified entity into the set and copy-constructs it's component. Tombstones (if any) are re-used.
		 * @param value Value of the inserted component.
		 * @copydetails insert */
		constexpr iterator insert(entity_t entity, const T &value) { return to_iterator(emplace_impl(entity, value)); }
		/** Inserts the specified entity into the set and move-constructs it's component. Tombstones (if any) are re-used.
		 * @copydetails insert */
		constexpr iterator insert(entity_t entity, T &&value)
		{
			return to_iterator(emplace_impl(entity, std::forward<T>(value)));
		}

		/** @brief Attempts to insert the specified entity into the set and default-constructs it's component. Tombstones (if any) are re-used.
		 * @param entity Entity to insert.
		 * @return Pair where first is an iterator to the potentially inserted entity and component and second is
		 * a boolean indicating whether the entity was inserted (`true` if inserted, `false` otherwise). */
		constexpr std::pair<iterator, bool> try_insert(entity_t entity)
		{
			if (const auto existing = find(entity); existing == end()) [[likely]]
				return {insert(entity), true};
			else
				return {existing, false};
		}
		/** @brief Attempts to insert the specified entity into the set and copy-constructs it's component. Tombstones (if any) are re-used.
		 * @param value Value of the inserted component.
		 * @copydetails try_insert */
		constexpr std::pair<iterator, bool> try_insert(entity_t entity, const T &value)
		{
			if (const auto existing = find(entity); existing == end()) [[likely]]
				return {insert(entity, value), true};
			else
				return {existing, false};
		}
		/** Attempts to insert the specified entity into the set and move-constructs it's component. Tombstones (if any) are re-used.
		 * @copydetails try_insert */
		constexpr std::pair<iterator, bool> try_insert(entity_t entity, T &&value)
		{
			if (const auto existing = find(entity); existing == end()) [[likely]]
				return {insert(entity, std::forward<T>(value)), true};
			else
				return {existing, false};
		}

		/** @brief Inserts the specified entity into the set and default-constructs it's component or
		 * replaces the component if such entity already exists. Tombstones (if any) are re-used.
		 * @param entity Entity to insert.
		 * @return Pair where first is an iterator to the potentially inserted entity and it's component
		 * and second is a boolean indicating whether the entity was inserted (`true` if inserted, `false` if replaced). */
		constexpr std::pair<iterator, bool> insert_or_replace(entity_t entity)
		{
			if (const auto existing = find(entity); existing != end()) [[unlikely]]
				return {replace(existing), false};
			else
				return {insert(entity), true};
		}
		/** @brief Inserts the specified entity into the set and copy-constructs it's component or
		 * replaces the component if such entity already exists. Tombstones (if any) are re-used.
		 * @param value Value of the inserted component.
		 * @copydetails insert_or_replace */
		constexpr std::pair<iterator, bool> insert_or_replace(entity_t entity, const T &value)
		{
			if (const auto existing = find(entity); existing != end()) [[unlikely]]
				return {replace(existing, value), false};
			else
				return {insert(entity, value), true};
		}
		/** @brief Inserts the specified entity into the set and move-constructs it's component or
		 * replaces the component if such entity already exists. Tombstones (if any) are re-used.
		 * @param value Value of the inserted component.
		 * @copydetails insert_or_replace */
		constexpr std::pair<iterator, bool> insert_or_replace(entity_t entity, T &&value)
		{
			if (const auto existing = find(entity); existing != end()) [[unlikely]]
				return {replace(existing, std::forward<T>(value)), false};
			else
				return {insert(entity, std::forward<T>(value)), true};
		}

		/** @brief Inserts the specified entity into the set and default-constructs it's component.
		 * Entities and components are always pushed to the end (tombstones are not re-used).
		 * @param entity Entity to insert.
		 * @return Iterator to the inserted entity and component.
		 * @warning Using an entity already contained within the set will result in undefined behavior. */
		constexpr iterator push_back(entity_t entity) { return to_iterator(emplace_back_impl(entity)); }
		/** @brief Inserts the specified entity into the set and copy-constructs it's component.
		 * Entities and components are always pushed to the end (tombstones are not re-used).
		 * @param value Value of the inserted component.
		 * @copydetails insert */
		constexpr iterator push_back(entity_t entity, const T &value)
		{
			return to_iterator(emplace_back_impl(entity, value));
		}
		/** Inserts the specified entity into the set and move-constructs it's component.
		 * Entities and components are always pushed to the end (tombstones are not re-used).
		 * @copydetails insert */
		constexpr iterator push_back(entity_t entity, T &&value)
		{
			return to_iterator(emplace_back_impl(entity, std::forward<T>(value)));
		}

		/** @brief Attempts to insert the specified entity into the set and default-constructs it's component.
		 * Entities and components are always pushed to the end (tombstones are not re-used).
		 * @param entity Entity to insert.
		 * @return Pair where first is an iterator to the potentially inserted entity and component and second is
		 * a boolean indicating whether the entity was inserted (`true` if inserted, `false` otherwise). */
		constexpr std::pair<iterator, bool> try_push_back(entity_t entity)
		{
			if (const auto existing = find(entity); existing == end()) [[likely]]
				return {push_back(entity), true};
			else
				return {existing, false};
		}
		/** @brief Attempts to insert the specified entity into the set and copy-constructs it's component.
		 * Entities and components are always pushed to the end (tombstones are not re-used).
		 * @param value Value of the inserted component.
		 * @copydetails try_insert */
		constexpr std::pair<iterator, bool> try_push_back(entity_t entity, const T &value)
		{
			if (const auto existing = find(entity); existing == end()) [[likely]]
				return {push_back(entity, value), true};
			else
				return {existing, false};
		}
		/** Attempts to insert the specified entity into the set and move-constructs it's component.
		 * Entities and components are always pushed to the end (tombstones are not re-used).
		 * @copydetails try_insert */
		constexpr std::pair<iterator, bool> try_push_back(entity_t entity, T &&value)
		{
			if (const auto existing = find(entity); existing == end()) [[likely]]
				return {push_back(entity, std::forward<T>(value)), true};
			else
				return {existing, false};
		}

		/** @brief Inserts the specified entity into the set and default-constructs it's component or
		 * replaces the component if such entity already exists. Entities and components are
		 * always pushed to the end (tombstones are not re-used).
		 * @param entity Entity to insert.
		 * @return Pair where first is an iterator to the potentially inserted entity and it's component
		 * and second is a boolean indicating whether the entity was inserted (`true` if inserted, `false` if replaced). */
		constexpr std::pair<iterator, bool> push_back_or_replace(entity_t entity)
		{
			if (const auto existing = find(entity); existing != end()) [[unlikely]]
				return {replace(existing), false};
			else
				return {push_back(entity), true};
		}
		/** @brief Inserts the specified entity into the set and copy-constructs it's component or
		 * replaces the component if such entity already exists.Entities and components are
		 * always pushed to the end (tombstones are not re-used).
		 * @param value Value of the inserted component.
		 * @copydetails insert_or_replace */
		constexpr std::pair<iterator, bool> push_back_or_replace(entity_t entity, const T &value)
		{
			if (const auto existing = find(entity); existing != end()) [[unlikely]]
				return {replace(existing, value), false};
			else
				return {push_back(entity, value), true};
		}
		/** @brief Inserts the specified entity into the set and move-constructs it's component or
		 * replaces the component if such entity already exists.Entities and components are
		 * always pushed to the end (tombstones are not re-used).
		 * @param value Value of the inserted component.
		 * @copydetails insert_or_replace */
		constexpr std::pair<iterator, bool> push_back_or_replace(entity_t entity, T &&value)
		{
			if (const auto existing = find(entity); existing != end()) [[unlikely]]
				return {replace(existing, std::forward<T>(value)), false};
			else
				return {push_back(entity, std::forward<T>(value)), true};
		}

		/** @copydoc base_t::erase(base_iter) */
		constexpr iterator erase(const_iterator which) { return to_iterator(erase_(offset(which))); }
		/** @copydoc base_t::fixed_erase(base_iter) */
		constexpr iterator fixed_erase(const_iterator which) { return to_iterator(fixed_erase_(offset(which))); }
		/** @copydoc base_t::erase(entity_t) */
		constexpr iterator erase(entity_t entity) { return to_iterator(base_t::erase(entity).offset()); }
		/** @copydoc base_t::fixed_erase(entity_t) */
		constexpr iterator fixed_erase(entity_t entity) { return to_iterator(base_t::fixed_erase(entity).offset()); }

		constexpr void swap(component_set &other) noexcept
		{
			base_t::swap(other);
			m_pool.swap(other.m_pool);
		}
		friend constexpr void swap(component_set &a, component_set &b) noexcept { a.swap(b); }

	private:
		[[nodiscard]] constexpr auto to_iterator(size_type i) noexcept { return iterator{this, i + 1}; }
		[[nodiscard]] constexpr auto to_iterator(size_type i) const noexcept { return const_iterator{this, i + 1}; }
		[[nodiscard]] constexpr auto &entity_ref(size_type i) noexcept { return base_t::at(i); }
		[[nodiscard]] constexpr auto &entity_ref(size_type i) const noexcept { return base_t::at(i); }
		[[nodiscard]] constexpr auto &component_ref(size_type i) noexcept { return m_pool.component_ref(i); }
		[[nodiscard]] constexpr auto &component_ref(size_type i) const noexcept { return m_pool.component_ref(i); }

		constexpr void reserve_impl(size_type n)
		{
			if (n != 0) [[likely]]
				m_pool.reserve(n);
		}

		template<typename... Args>
		constexpr size_type replace_impl(size_type idx, Args &&...args)
		{
			auto &ref = component_ref(idx);
			if constexpr (std::is_move_assignable_v<T>)
				ref = T{std::forward<Args>(args)...};
			else
			{
				std::destroy_at(std::addressof(ref));
				std::construct_at(std::addressof(ref), std::forward<Args>(args)...);
			}
			dispatch_replace(idx);
			return idx;
		}
		template<typename U>
		constexpr size_type replace_impl(size_type idx, U &&value)
		{
			auto &ref = component_ref(idx);
			if constexpr (std::is_assignable_v<T, U &&>)
				ref = std::forward<U>(value);
			else if constexpr (std::is_move_assignable_v<T>)
				ref = T{std::forward<U>(value)};
			else
			{
				std::destroy_at(std::addressof(ref));
				std::construct_at(std::addressof(ref), std::forward<U>(value));
			}
			dispatch_replace(idx);
			return idx;
		}

		template<typename... Args>
		constexpr size_type emplace_back_impl(entity_t entity, Args &&...args)
		{
			const auto pos = base_t::push_back_(entity);
			try
			{
				m_pool.emplace(pos, std::forward<Args>(args)...);
			}
			catch (...)
			{
				/* If exceptions were encountered during emplacement, erase the entity.
				 * Always swap & pop, since the component does not exist. */
				base_t::erase_(pos);
				throw;
			}

			/* Component created successfully, dispatch event & return. */
			base_t::dispatch_create(pos);
			return pos;
		}
		template<typename... Args>
		constexpr size_type emplace_impl(entity_t entity, Args &&...args)
		{
			const auto pos = base_t::insert_(entity);
			try
			{
				m_pool.emplace(pos, std::forward<Args>(args)...);
			}
			catch (...)
			{
				/* If exceptions were encountered during emplacement, erase the entity.
				 * Always swap & pop, since the component does not exist. */
				base_t::erase_(pos);
				throw;
			}

			/* Component created successfully, dispatch event & return. */
			base_t::dispatch_create(pos);
			return pos;
		}

		/* basic_entity_set overrides */
		constexpr size_type push_back_(entity_t e) final
		{
			const auto pos = base_t::push_back_(e);
			base_t::dispatch_create(pos);
			return pos;
		}
		constexpr size_type insert_(entity_t e) final
		{
			const auto pos = base_t::insert_(e);
			base_t::dispatch_create(pos);
			return pos;
		}

		constexpr size_type fixed_erase_(size_type idx) final
		{
			/* Fixed components will not be moved by the handler (or at least should not be),
			 * thus no need to re-acquire entity index. */
			base_t::dispatch_remove(idx);
			m_pool.erase(idx);
			return base_t::erase_(idx);
		}
		constexpr size_type erase_(size_type idx) final
		{
			if constexpr (is_fixed)
				return fixed_erase_(idx);
			else
			{
				/* Since component type is not fixed, handler functions may attempt to move it.
				 * Because of this the original index may no longer be valid. */
				const auto e = base_t::at(idx);
				base_t::dispatch_remove(idx);

				const auto last = size() - 1;
				idx = base_t::offset(e);

				/* Move the last component to the erased one, then erase the last to account for swap & pop. */
				m_pool.move_value(idx, last);
				m_pool.erase(last);

				return base_t::erase_(idx);
			}
		}

		constexpr void move_(size_type to, size_type from) final { m_pool.move_value(to, from); }
		constexpr void swap_(size_type lhs, size_type rhs) final { m_pool.swap_value(lhs, rhs); }

		/* basic_component_set overrides */
		[[nodiscard]] any_ref get_any(base_iter entity) noexcept final { return get_any(entity.offset()); }
		[[nodiscard]] any_ref get_any(base_iter entity) const noexcept final { return get_any(entity.offset()); }

		base_iter insert(entity_t entity, any value) final
		{
			const auto &component = value.template cast<const T &>();
			return base_t::end() - insert(entity, component).m_off;
		}
		base_iter push_back(entity_t entity, any value) final
		{
			const auto &component = value.template cast<const T &>();
			return base_t::end() - push_back(entity, component).m_off;
		}

		/* Hide these from public API. */
		using base_t::erase;

		pool_t m_pool;
	};

	/** @brief Structure used to indirectly reference a component through an entity from a component set. */
	template<typename T>
	class component_ptr
	{
	public:
		typedef component_traits<T> traits_type;
		typedef component_set<T> set_type;

		typedef T value_type;
		typedef value_type *pointer;
		typedef value_type &reference;

	private:
		using set_ptr = transfer_cv_t<T, set_type> *;
		using set_ref = transfer_cv_t<T, set_type> &;

	public:
		/** Initializes a null component pointer. */
		constexpr component_ptr() noexcept = default;

		/** Initializes a component pointer for an entity and a set.
		 * @param e Entity who's component to point to.
		 * @param set Pointer to the component set containing the target component. */
		constexpr component_ptr(entity_t e, set_ptr set) noexcept : m_entity(e), m_set(set) {}
		/** Initializes a component pointer for an entity and a set.
		 * @param e Entity who's component to point to.
		 * @param set Reference to the component set containing the target component. */
		constexpr component_ptr(entity_t e, set_ref set) noexcept : component_ptr(e, &set) {}

		/** Checks if the component pointer has points to a valid component of an entity (both entity and set are valid). */
		[[nodiscard]] constexpr bool empty() const noexcept { return !m_entity.is_tombstone() && m_set; }
		/** @copydoc empty */
		[[nodiscard]] constexpr operator bool() const noexcept { return empty(); }

		/** Returns the associated entity. */
		[[nodiscard]] constexpr entity_t entity() const noexcept { return m_entity; }
		/** Returns the bound component set. */
		[[nodiscard]] constexpr set_type *set() const noexcept { return m_set; }

		/** Returns pointer to the associated component. */
		[[nodiscard]] constexpr pointer get() const noexcept { return std::addressof(m_set->find(m_entity)->second); }
		/** @copydoc get */
		[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
		/** Returns reference to the associated component. */
		[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }

		/** Rebinds pointer to use a different entity.
		 * @param entity New entity to point to.
		 * @return Old entity. */
		constexpr entity_t reset(entity_t entity) noexcept { return std::exchange(m_entity, entity); }
		/** Rebinds pointer to use a different component set.
		 * @param set Pointer to the new set.
		 * @return Pointer to the old set. */
		constexpr set_ptr reset(set_ptr set) noexcept { return std::exchange(m_set, set); }
		/** Rebinds pointer to use a different component set and entity.
		 * @param entity New entity to point to.
		 * @param set Pointer to the new set.
		 * @return Pair where first is the old entity and second is pointer to the old set. */
		constexpr std::pair<entity_t, set_ptr> reset(entity_t entity, set_ptr set) noexcept
		{
			return {std::exchange(m_entity, entity), std::exchange(m_set, set)};
		}
		/** Resets pointer to an empty state.
		 * @return Pair where first is the old entity and second is pointer to the old set. */
		constexpr std::pair<entity_t, set_ptr> reset() noexcept { return reset(entity_t::tombstone(), nullptr); }

		[[nodiscard]] constexpr bool operator==(const component_ptr &) const noexcept = default;
		[[nodiscard]] constexpr bool operator!=(const component_ptr &) const noexcept = default;

		constexpr void swap(component_ptr &other) noexcept
		{
			using std::swap;
			swap(m_entity, other.m_entity);
			swap(m_set, other.m_set);
		}
		friend constexpr void swap(component_ptr &a, component_ptr &b) noexcept { a.swap(b); }

	private:
		entity_t m_entity = entity_t::tombstone();
		set_ptr m_set = nullptr;
	};

	template<typename T>
	component_ptr(entity_t, component_set<T> *) -> component_ptr<T>;
	template<typename T>
	component_ptr(entity_t, const component_set<T> *) -> component_ptr<const T>;
	template<typename T>
	component_ptr(entity_t, component_set<T> &) -> component_ptr<T>;
	template<typename T>
	component_ptr(entity_t, const component_set<T> &) -> component_ptr<const T>;
}	 // namespace sek::engine