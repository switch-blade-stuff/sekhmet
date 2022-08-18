/*
 * Created by switchblade on 31/07/22
 */

#pragma once

#include <functional>

#include "world.hpp"

namespace sek::engine
{
	template<typename, typename = included_t<>, typename = excluded_t<>, typename = optional_t<>>
	class component_collection;

	/** @brief Structure used to collect and provide a view of components for a set of entities.
	 *
	 * Component collections act as "strong" references to a group of component sets. Iterating a component collection
	 * will iterate over entities of it's owned (collected) included and optional sets, discarding any entities from the
	 * excluded sets. Collections track component events on relevant component types in order to provide more efficient
	 * iteration than a `component_view`.
	 *
	 * Owning collections will automatically sort components in order to achieve better cache locality than a
	 * `component_view` and avoid multiple indirection for owned components, as well as to track modification to
	 * relevant component sets. This, however, comes at a cost of restricting allowed operations on owned component sets
	 * - any external sorting of components will leave the collection in undefined state, thus no sorting may be
	 * preformed for owned components. While creating multiple owning collections for the same component type is
	 * allowed, such collections must either have the same owned components or be a "specialized" version
	 * of one another. For example, if owned components of collection `A` are `int, float`, collection `B` can only
	 * own one of the following: `int`, `int, float` or `int, float, Ts...` where `Ts...` is a sequence of other
	 * component types. Specialized collections are allowed to exist, since sort order of a more-specialized collection
	 * will always satisfy a less-specialized one.
	 *
	 * If no owned components are specified, collections will act as an event-aware view which tracks modifications of
	 * relevant component sets.
	 *
	 * @note While non-owning collections do not create any side-effects for the underlying component sets, they will
	 * still be more expensive than a `component_view`, since they still keep track of component events.
	 *
	 * @note Use of owning collections is not recommended for commonly used component types, as they will prevent
	 * creation of other collections (or regular sorting) of owned types and will potentially result in frequent
	 * sorting of component sets.
	 *
	 * @tparam C Component types collected (owned) by the collection.
	 * @tparam I Component types captured by the collection.
	 * @tparam E Component types excluded from the collection.
	 * @tparam O Optional components of the collection.
	 * @tparam Alloc Allocator used for internal entity set. */
	template<typename... C, typename... I, typename... E, typename... O>
	class component_collection<owned_t<C...>, included_t<I...>, excluded_t<E...>, optional_t<O...>>
	{
		template<typename, typename, typename, typename, typename>
		friend class entity_query;

		using handler_t = detail::collection_handler<owned_t<C...>, included_t<I...>, excluded_t<E...>>;

		template<typename T>
		using set_ptr_t = transfer_cv_t<T, component_set<std::remove_cv_t<T>>> *;
		using coll_ptr = std::tuple<set_ptr_t<C>...>;
		using inc_ptr = std::tuple<set_ptr_t<I>...>;
		using opt_ptr = std::tuple<set_ptr_t<O>...>;

		template<typename T>
		constexpr static bool is_fixed = fixed_component<std::remove_cv_t<T>>;
		template<typename T>
		constexpr static bool is_coll = is_in_v<std::remove_cv_t<T>, std::remove_cv_t<C>...>;
		template<typename T>
		constexpr static bool is_inc = is_in_v<std::remove_cv_t<T>, std::remove_cv_t<I>...>;
		template<typename T>
		constexpr static bool is_opt = is_in_v<std::remove_cv_t<T>, std::remove_cv_t<O>...>;

		static_assert(!((is_coll<E> || is_inc<E> || is_opt<E>) || ...),
					  "Excluded, included and collected component types must not intersect");
		static_assert(!(is_fixed<C> || ...), "Cannot collect fixed-storage components");

		class collection_iterator
		{
			friend class component_collection;

		public:
			typedef entity_t value_type;
			typedef const entity_t &reference;
			typedef const entity_t *pointer;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;
			typedef std::random_access_iterator_tag iterator_category;

		private:
			constexpr explicit collection_iterator(const component_collection *coll, size_type off = 0) noexcept
				: collection_iterator(coll, static_cast<difference_type>(off))
			{
			}
			constexpr collection_iterator(const component_collection *coll, difference_type off) noexcept
				: m_coll(coll), m_off(off)
			{
			}

		public:
			constexpr collection_iterator() noexcept = default;

			constexpr collection_iterator operator++(int) noexcept
			{
				auto temp = *this;
				++(*this);
				return temp;
			}
			constexpr collection_iterator &operator++() noexcept
			{
				++m_off;
				return *this;
			}
			constexpr collection_iterator &operator+=(difference_type n) noexcept
			{
				m_off += n;
				return *this;
			}
			constexpr collection_iterator operator--(int) noexcept
			{
				auto temp = *this;
				--(*this);
				return temp;
			}
			constexpr collection_iterator &operator--() noexcept
			{
				--m_off;
				return *this;
			}
			constexpr collection_iterator &operator-=(difference_type n) noexcept
			{
				m_off -= n;
				return *this;
			}

			[[nodiscard]] constexpr collection_iterator operator+(difference_type n) const noexcept
			{
				return collection_iterator{m_coll, m_off + n};
			}
			[[nodiscard]] constexpr collection_iterator operator-(difference_type n) const noexcept
			{
				return collection_iterator{m_coll, m_off - n};
			}
			[[nodiscard]] constexpr difference_type operator-(const collection_iterator &other) const noexcept
			{
				return m_off - other.m_off;
			}

			/** Returns the offset of the iterator within the collection. */
			[[nodiscard]] constexpr size_type offset() const noexcept { return static_cast<size_type>(m_off); }

			/** Returns pointer to the target entity. */
			[[nodiscard]] constexpr pointer get() const noexcept { return std::addressof(operator*()); }
			/** @copydoc get */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
			/** Returns reference to the target entity. */
			[[nodiscard]] constexpr reference operator*() const noexcept { return operator[](0); }
			/** Returns reference to the entity at offset `i` from this iterator. */
			[[nodiscard]] constexpr reference operator[](difference_type i) const noexcept
			{
				return m_coll->at(static_cast<size_type>(m_off + i));
			}

			[[nodiscard]] constexpr auto operator<=>(const collection_iterator &other) const noexcept
			{
				return m_off <=> other.m_off;
			}
			[[nodiscard]] constexpr bool operator==(const collection_iterator &) const noexcept = default;

			constexpr void swap(collection_iterator &other) noexcept
			{
				std::swap(m_coll, other.m_coll);
				std::swap(m_off, other.m_off);
			}
			friend constexpr void swap(collection_iterator &a, collection_iterator &b) noexcept { a.swap(b); }

		private:
			const component_collection *m_coll = nullptr;
			difference_type m_off = 0;
		};

	public:
		typedef collection_iterator iterator;
		typedef collection_iterator const_iterator;
		typedef std::reverse_iterator<collection_iterator> reverse_iterator;
		typedef std::reverse_iterator<collection_iterator> const_reverse_iterator;

		typedef typename iterator::value_type value_type;
		typedef typename iterator::pointer pointer;
		typedef typename iterator::pointer const_pointer;
		typedef typename iterator::reference reference;
		typedef typename iterator::reference const_reference;
		typedef typename iterator::size_type size_type;
		typedef typename iterator::difference_type difference_type;

	private:
		constexpr component_collection(handler_t *h, set_ptr_t<C>... c, set_ptr_t<I>... i, set_ptr_t<O>... o) noexcept
			: m_handler(h), m_collected(c...), m_included(i...), m_optional(o...)
		{
			SEK_ASSERT(h != nullptr, "Collection handler must not be null");
			SEK_ASSERT(((c != nullptr) && ...), "Collected component sets must not be null");
			SEK_ASSERT(((i != nullptr) && ...), "Included component sets must not be null");
		}

	public:
		/** Initializes an empty collection. */
		constexpr component_collection() noexcept = default;

		constexpr component_collection(const component_collection &) = default;
		constexpr component_collection &operator=(const component_collection &) = default;
		constexpr component_collection(component_collection &&other) noexcept { swap(other); }
		constexpr component_collection &operator=(component_collection &&other) noexcept
		{
			swap(other);
			return *this;
		}

		/** Returns iterator to the first entity. */
		[[nodiscard]] constexpr iterator begin() const noexcept { return iterator{this}; }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
		/** Returns iterator one past the last entity. */
		[[nodiscard]] constexpr iterator end() const noexcept { return iterator{this, size()}; }
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }
		/** Returns reverse iterator to the last entity. */
		[[nodiscard]] constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator{end()}; }
		/** @copydoc rbegin */
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
		/** Returns reverse iterator one past the first entity. */
		[[nodiscard]] constexpr reverse_iterator rend() const noexcept { return reverse_iterator{begin()}; }
		/** @copydoc rend */
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return rend(); }

		/** Returns reference to the first entity. */
		[[nodiscard]] constexpr reference front() const noexcept { return *begin(); }
		/** Returns reference to the last entity. */
		[[nodiscard]] constexpr reference back() const noexcept { return *std::prev(end()); }
		/** Returns reference the entity located at the specified offset within the collection. */
		[[nodiscard]] constexpr reference at(size_type i) const noexcept { return std::get<0>(m_collected)->at(i); }
		/** @copydoc at */
		[[nodiscard]] constexpr reference operator[](size_type i) const noexcept { return at(i); }

		/** Returns the size of the collection. */
		[[nodiscard]] constexpr size_type size() const noexcept { return m_handler != nullptr ? m_handler->size : 0; }
		/** Checks if the the collection is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_handler == nullptr || m_handler->size == 0; }

		/** Returns offset of the specified entity within the collection. */
		[[nodiscard]] constexpr size_type offset(const_iterator which) const noexcept { return which.offset(); }
		/** @copydoc offset
		 * @warning Using entity that does not belong to the collection will result in undefined behavior. */
		[[nodiscard]] constexpr size_type offset(entity_t entity) const noexcept
		{
			/* Collections are always sorted from the first set, meaning index of the entity in the first set
			 * is the same for all sorted (owned) component sets . */
			return std::get<0>(m_collected)->offset(entity);
		}

		/** Checks if the the collection contains the specified entity. */
		[[nodiscard]] constexpr bool contains(entity_t entity) const noexcept
		{
			if (const auto coll = std::get<0>(m_collected); coll == nullptr) [[unlikely]]
				return false;
			else
			{
				/* Entity must be present within the first set and must be located at an offset that is
				 * within the range of collection's offsets. */
				const auto pos = coll->find(entity);
				return pos != coll->end() && pos.offset() < m_handler->size;
			}
		}
		/** Returns iterator to the specified entity, or an end iterator if the entity does not belong to the collection. */
		[[nodiscard]] constexpr iterator find(entity_t entity) const noexcept
		{
			if (const auto coll = std::get<0>(m_collected); coll == nullptr) [[unlikely]]
				return iterator{this};
			else if (const auto pos = coll->find(entity); pos != coll->end() && pos.offset() < m_handler->size) [[likely]]
				return iterator{this, pos.offset()};
			else
				return end();
		}

		/** Returns pointers to components associated with the entity at the specific collection offset.
		 *
		 * @param i Offset of the target entity within the collection.
		 * @return Tuple of pointers to entity components, or a single pointer if only one component is specified.
		 * @note Pointers to optional components may be null. */
		template<typename... Ts>
		[[nodiscard]] constexpr decltype(auto) get(size_type i) const noexcept
		{
			return get<Ts...>(iterator{this, i});
		}
		/** Returns pointers to components associated with the entity pointed to by the iterator.
		 *
		 * @param which Iterator to the entity to get components for.
		 * @return Tuple of pointers to entity components, or a single pointer if only one component is specified.
		 * @note Pointers to optional components may be null. */
		template<typename... Ts>
		[[nodiscard]] constexpr decltype(auto) get(const_iterator which) const noexcept
		{
			return get_impl<Ts...>(which);
		}
		/** Returns pointers to components associated with the specified entity.
		 *
		 * @param entity Entity to get components for.
		 * @return Tuple of pointers to entity components, or a single pointer if only one component is specified.
		 *
		 * @note Pointers to optional components may be null.
		 * @warning Using an entity not belonging to the collection results in undefined behavior. */
		template<typename... Ts>
		[[nodiscard]] constexpr decltype(auto) get(entity_t entity) const noexcept
		{
			return get_impl<Ts...>(entity);
		}

		/** Applies the functor to every entity of the collection.
		 * Functor may optionally return a value, which if evaluated to `false`, prematurely terminates iteration. */
		template<std::invocable<entity_t, C *..., I *..., O *...> F>
		constexpr void for_each(F &&f) const
		{
			for (auto first = begin(), last = end(); first != last; ++first)
			{
				using std::get;
				const auto cmp = std::tuple<C *..., I *..., O *...>{this->get<C..., I..., O...>(first)};
				const auto e = *first;

				if constexpr (std::convertible_to<std::invoke_result_t<F, entity_t, C *..., I *..., O *...>, bool>)
				{
					if (!std::invoke(std::forward<F>(f), e, get<C *>(cmp)..., get<I *>(cmp)..., get<O *>(cmp)...)) [[unlikely]]
						break;
				}
				else
					std::invoke(std::forward<F>(f), e, get<C *>(cmp)..., get<I *>(cmp)..., get<O *>(cmp)...);
			}
		}

		constexpr void swap(component_collection &other) noexcept
		{
			std::swap(m_handler, other.m_handler);
			std::swap(m_collected, other.m_collected);
			std::swap(m_included, other.m_included);
			std::swap(m_optional, other.m_optional);
		}
		friend constexpr void swap(component_collection &a, component_collection &b) noexcept { a.swap(b); }

	private:
		template<typename T0, typename T1, typename... Ts, typename U>
		[[nodiscard]] constexpr std::tuple<T0 *, T1 *, Ts *...> get_impl(U &&arg) const noexcept
		{
			return std::tuple<T0 *, T1 *, Ts *...>{get_impl<T0>(arg), get_impl<T1>(arg), get_impl<Ts>(arg)...};
		}
		template<typename T>
		[[nodiscard]] constexpr T *get_impl(const_iterator which) const noexcept
		{
			if constexpr (is_coll<T>)
				return std::addressof(std::get<set_ptr_t<T>>(m_collected)->get(offset(which)));
			else
				return get_impl<T>(*which);
		}
		template<typename T>
		[[nodiscard]] constexpr T *get_impl(entity_t e) const noexcept
		{
			using std::get;
			if constexpr (is_coll<T>)
				return std::addressof(get<set_ptr_t<T>>(m_collected)->get(offset(e)));
			else if constexpr (is_inc<T>)
				return std::addressof(get<set_ptr_t<T>>(m_included)->get(e));
			else
				return detail::get_opt(get<set_ptr_t<T>>(m_optional), e);
		}

		handler_t *m_handler = nullptr;
		coll_ptr m_collected = {};
		inc_ptr m_included = {};
		opt_ptr m_optional = {};
	};

	/** @brief Non-owning specialization of `component_collection`. */
	template<typename... I, typename... E, typename... O>
	class component_collection<owned_t<>, included_t<I...>, excluded_t<E...>, optional_t<O...>>
	{
		template<typename, typename, typename, typename, typename>
		friend class entity_query;

		using handler_t = detail::collection_handler<owned_t<>, included_t<I...>, excluded_t<E...>>;

		template<typename T>
		using set_ptr_t = transfer_cv_t<T, component_set<std::remove_cv_t<T>>> *;
		using inc_ptr = std::tuple<set_ptr_t<I>...>;
		using exc_ptr = std::tuple<set_ptr_t<E>...>;
		using opt_ptr = std::tuple<set_ptr_t<O>...>;

		template<typename T>
		constexpr static bool is_fixed = fixed_component<std::remove_cv_t<T>>;
		template<typename T>
		constexpr static bool is_inc = is_in_v<std::remove_cv_t<T>, std::remove_cv_t<I>...>;
		template<typename T>
		constexpr static bool is_opt = is_in_v<std::remove_cv_t<T>, std::remove_cv_t<O>...>;

		static_assert(!((is_inc<E> || is_opt<E>) || ...), "Excluded and included component types must not intersect");
		static_assert(sizeof...(I) != 0, "Collection must include at least 1 component type");

		using set_iter = typename entity_set::iterator;

		class collection_iterator
		{
			friend class component_collection;

		public:
			typedef typename set_iter::value_type value_type;
			typedef typename set_iter::reference reference;
			typedef typename set_iter::pointer pointer;
			typedef typename set_iter::size_type size_type;
			typedef typename set_iter::difference_type difference_type;
			typedef typename set_iter::iterator_category iterator_category;

		private:
			constexpr explicit collection_iterator(set_iter iter) noexcept : m_iter(iter) {}

		public:
			constexpr collection_iterator() noexcept = default;

			constexpr collection_iterator operator++(int) noexcept
			{
				auto temp = *this;
				++(*this);
				return temp;
			}
			constexpr collection_iterator &operator++() noexcept
			{
				++m_iter;
				return *this;
			}
			constexpr collection_iterator &operator+=(difference_type n) noexcept
			{
				m_iter += n;
				return *this;
			}
			constexpr collection_iterator operator--(int) noexcept
			{
				auto temp = *this;
				--(*this);
				return temp;
			}
			constexpr collection_iterator &operator--() noexcept
			{
				--m_iter;
				return *this;
			}
			constexpr collection_iterator &operator-=(difference_type n) noexcept
			{
				m_iter -= n;
				return *this;
			}

			[[nodiscard]] constexpr collection_iterator operator+(difference_type n) const noexcept
			{
				return collection_iterator{m_iter + n};
			}
			[[nodiscard]] constexpr collection_iterator operator-(difference_type n) const noexcept
			{
				return collection_iterator{m_iter - n};
			}
			[[nodiscard]] constexpr difference_type operator-(const collection_iterator &other) const noexcept
			{
				return m_iter - other.m_iter;
			}

			/** Returns the offset of the iterator within the collection. */
			[[nodiscard]] constexpr size_type offset() const noexcept { return m_iter.offset(); }

			/** Returns pointer to the target entity. */
			[[nodiscard]] constexpr pointer get() const noexcept { return std::addressof(operator*()); }
			/** @copydoc get */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
			/** Returns reference to the target entity. */
			[[nodiscard]] constexpr reference operator*() const noexcept { return operator[](0); }
			/** Returns reference to the entity at offset `i` from this iterator. */
			[[nodiscard]] constexpr reference operator[](difference_type i) const noexcept { return m_iter[i]; }

			[[nodiscard]] constexpr auto operator<=>(const collection_iterator &) const noexcept = default;
			[[nodiscard]] constexpr bool operator==(const collection_iterator &) const noexcept = default;

			constexpr void swap(collection_iterator &other) noexcept { m_iter.swap(other.m_iter); }
			friend constexpr void swap(collection_iterator &a, collection_iterator &b) noexcept { a.swap(b); }

		private:
			set_iter m_iter;
		};

	public:
		typedef collection_iterator iterator;
		typedef collection_iterator const_iterator;
		typedef std::reverse_iterator<collection_iterator> reverse_iterator;
		typedef std::reverse_iterator<collection_iterator> const_reverse_iterator;

		typedef typename iterator::value_type value_type;
		typedef typename iterator::pointer pointer;
		typedef typename iterator::pointer const_pointer;
		typedef typename iterator::reference reference;
		typedef typename iterator::reference const_reference;
		typedef typename iterator::size_type size_type;
		typedef typename iterator::difference_type difference_type;

	private:
		constexpr component_collection(handler_t *h, set_ptr_t<I>... i, set_ptr_t<O>... o) noexcept
			: m_handler(h), m_included(i...), m_optional(o...)
		{
			SEK_ASSERT(h != nullptr, "Collection handler must not be null");
			SEK_ASSERT(((i != nullptr) && ...), "Included component sets must not be null");
		}

	public:
		/** Initializes an empty collection. */
		constexpr component_collection() noexcept = default;

		constexpr component_collection(const component_collection &) = default;
		constexpr component_collection &operator=(const component_collection &) = default;
		constexpr component_collection(component_collection &&other) noexcept { swap(other); }
		constexpr component_collection &operator=(component_collection &&other) noexcept
		{
			swap(other);
			return *this;
		}

		/** Returns iterator to the first entity. */
		[[nodiscard]] constexpr iterator begin() const noexcept
		{
			return m_handler != nullptr ? iterator{m_handler->entities.begin()} : iterator{};
		}
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
		/** Returns iterator one past the last entity. */
		[[nodiscard]] constexpr iterator end() const noexcept
		{
			return m_handler != nullptr ? iterator{m_handler->entities.end()} : iterator{};
		}
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }
		/** Returns reverse iterator to the last entity. */
		[[nodiscard]] constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator{end()}; }
		/** @copydoc rbegin */
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
		/** Returns reverse iterator one past the first entity. */
		[[nodiscard]] constexpr reverse_iterator rend() const noexcept { return reverse_iterator{begin()}; }
		/** @copydoc rend */
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return rend(); }

		/** Returns reference to the first entity. */
		[[nodiscard]] constexpr reference front() const noexcept { return *begin(); }
		/** Returns reference to the last entity. */
		[[nodiscard]] constexpr reference back() const noexcept { return *std::prev(end()); }
		/** Returns reference the entity located at the specified offset within the collection. */
		[[nodiscard]] constexpr reference at(size_type i) const noexcept { return m_handler->entities.at(i); }
		/** @copydoc at */
		[[nodiscard]] constexpr reference operator[](size_type i) const noexcept { return at(i); }

		/** Returns the size of the collection. */
		[[nodiscard]] constexpr size_type size() const noexcept
		{
			return m_handler != nullptr ? m_handler->entities.size() : 0;
		}
		/** Checks if the the collection is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept
		{
			return m_handler == nullptr || m_handler->entities.empty();
		}

		/** Returns offset of the specified entity within the collection. */
		[[nodiscard]] constexpr size_type offset(const_iterator which) const noexcept { return which.offset(); }
		/** @copydoc offset
		 * @warning Using entity that does not belong to the collection will result in undefined behavior. */
		[[nodiscard]] constexpr size_type offset(entity_t entity) const noexcept
		{
			return m_handler->entities.offset(entity);
		}

		/** Checks if the the collection contains the specified entity. */
		[[nodiscard]] constexpr bool contains(entity_t entity) const noexcept
		{
			return m_handler != nullptr && m_handler->entities.contains(entity);
		}
		/** Returns iterator to the specified entity, or an end iterator if the entity does not belong to the collection. */
		[[nodiscard]] constexpr iterator find(entity_t entity) const noexcept
		{
			return m_handler != nullptr ? iterator{m_handler->entities.find(entity)} : iterator{};
		}

		/** Returns pointers to components associated with the entity at the specific collection offset.
		 *
		 * @param i Offset of the target entity within the collection.
		 * @return Tuple of pointers to entity components, or a single pointer if only one component is specified.
		 * @note Pointers to optional components may be null. */
		template<typename... Ts>
		[[nodiscard]] constexpr decltype(auto) get(size_type i) const noexcept
		{
			return get_impl<Ts...>(at(i));
		}
		/** Returns pointers to components associated with the entity pointed to by the iterator.
		 *
		 * @param which Iterator to the entity to get components for.
		 * @return Tuple of pointers to entity components, or a single pointer if only one component is specified.
		 * @note Pointers to optional components may be null. */
		template<typename... Ts>
		[[nodiscard]] constexpr decltype(auto) get(const_iterator which) const noexcept
		{
			return get_impl<Ts...>(*which);
		}
		/** Returns pointers to components associated with the specified entity.
		 *
		 * @param entity Entity to get components for.
		 * @return Tuple of pointers to entity components, or a single pointer if only one component is specified.
		 *
		 * @note Pointers to optional components may be null.
		 * @warning Using an entity not belonging to the collection results in undefined behavior. */
		template<typename... Ts>
		[[nodiscard]] constexpr decltype(auto) get(entity_t entity) const noexcept
		{
			return get_impl<Ts...>(entity);
		}

		/** Applies the functor to every entity of the collection.
		 * Functor may optionally return a value, which if evaluated to `false`, prematurely terminates iteration. */
		template<std::invocable<entity_t, I *..., O *...> F>
		constexpr void for_each(F &&f) const
		{
			for (auto first = begin(), last = end(); first != last; ++first)
			{
				using std::get;
				const auto cmp = std::tuple<I *..., O *...>{this->get<I..., O...>(first)};
				const auto e = *first;

				if constexpr (std::convertible_to<std::invoke_result_t<F, entity_t, I *..., O *...>, bool>)
				{
					if (!std::invoke(std::forward<F>(f), e, get<I *>(cmp)..., get<O *>(cmp)...)) [[unlikely]]
						break;
				}
				else
					std::invoke(std::forward<F>(f), e, get<I *>(cmp)..., get<O *>(cmp)...);
			}
		}

		constexpr void swap(component_collection &other) noexcept
		{
			std::swap(m_handler, other.m_handler);
			std::swap(m_included, other.m_included);
			std::swap(m_optional, other.m_optional);
		}
		friend constexpr void swap(component_collection &a, component_collection &b) noexcept { a.swap(b); }

	private:
		template<typename T0, typename T1, typename... Ts>
		[[nodiscard]] constexpr std::tuple<T0 *, T1 *, Ts *...> get_impl(entity_t e) const noexcept
		{
			return std::tuple<T0 *, T1 *, Ts *...>{get_impl<T0>(e), get_impl<T1>(e), get_impl<Ts>(e)...};
		}
		template<typename T>
		[[nodiscard]] constexpr T *get_impl(entity_t e) const noexcept
		{
			using std::get;
			if constexpr (is_inc<T>)
				return std::addressof(get<set_ptr_t<T>>(m_included)->get(e));
			else
				return detail::get_opt(get<set_ptr_t<T>>(m_optional), e);
		}

		handler_t *m_handler = nullptr;
		inc_ptr m_included = {};
		opt_ptr m_optional = {};
	};
}	 // namespace sek::engine