/*
 * Created by switchblade on 31/07/22
 */

#pragma once

#include "component_set.hpp"

namespace sek::engine
{
	template<typename, typename = included_t<>, typename = excluded_t<>, typename = optional_t<>>
	class component_collection;

	namespace detail
	{
		template<typename...>
		struct collection_handler;
	}	 // namespace detail

	/** @brief Structure used to provide a simple view of components for a set of entities.
	 * @tparam C Component types collected (sorted) by the collection.
	 * @tparam I Component types captured by the collection.
	 * @tparam E Component types excluded from the collection.
	 * @tparam O Optional components of the collection.
	 * @tparam Alloc Allocator used for internal entity set. */
	template<typename... C, typename... I, typename... E, typename... O>
	class component_collection<collected_t<C...>, included_t<I...>, excluded_t<E...>, optional_t<O...>>
	{
		template<typename, typename, typename, typename, typename>
		friend class entity_query;

		using handler_t = detail::collection_handler<collected_t<C...>, included_t<I...>, excluded_t<E...>>;

		template<typename T>
		using set_ptr_t = transfer_cv_t<T, component_set<std::remove_cv_t<T>>> *;
		using coll_ptr = std::tuple<set_ptr_t<C>...>;
		using inc_ptr = std::tuple<set_ptr_t<I>...>;
		using exc_ptr = std::tuple<set_ptr_t<E>...>;
		using opt_ptr = std::tuple<set_ptr_t<O>...>;

		constexpr static bool view_like = sizeof...(C) == 0;

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
		static_assert(sizeof...(C) + sizeof...(I) != 0, "Collection must include or collect at least 1 component");

		class collection_iterator
		{
			friend class component_collection;

		public:
			typedef entity_t value_type;
			typedef const entity_t &reference;
			typedef const entity_t *pointer;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;
			typedef std::bidirectional_iterator_tag iterator_category;

		private:
			constexpr explicit collection_iterator(const collection_iterator *coll) noexcept : m_coll(coll) {}
			constexpr collection_iterator(const collection_iterator *coll, size_type off) noexcept
				: collection_iterator(coll, static_cast<difference_type>(off))
			{
			}
			constexpr collection_iterator(const collection_iterator *coll, difference_type off) noexcept
				: m_coll(coll), m_off(next_valid(off))
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
				m_off = next_valid(m_off - 1);
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
				for (const auto n = m_coll->size(); m_off != n && !valid(m_off);) ++m_off;
				return *this;
			}

			/** Returns pointer to the target entity. */
			[[nodiscard]] constexpr pointer get() const noexcept { return get(m_off); }
			/** @copydoc get */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
			/** Returns reference to the target entity */
			[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }

			[[nodiscard]] constexpr auto operator<=>(const collection_iterator &other) const noexcept
			{
				return m_off <=> other.m_off;
			}
			[[nodiscard]] constexpr bool operator==(const collection_iterator &other) const noexcept
			{
				return m_off == other.m_off;
			}

			constexpr void swap(collection_iterator &other) noexcept
			{
				std::swap(m_coll, other.m_coll);
				std::swap(m_off, other.m_off);
			}
			friend constexpr void swap(collection_iterator &a, collection_iterator &b) noexcept { a.swap(b); }

		private:
			[[nodiscard]] constexpr difference_type next_valid(difference_type i) const noexcept
			{
				while (i != 0 && !valid(i)) --i;
				return i;
			}
			[[nodiscard]] constexpr pointer get(difference_type i) const noexcept { return m_coll->get_entity(i - 1); }
			[[nodiscard]] constexpr bool valid(difference_type i) const noexcept { return m_coll->contains(*get(i)); }

			const collection_iterator *m_coll = nullptr;
			difference_type m_off = 0;
		};

	public:
		typedef typename collection_iterator::value_type value_type;
		typedef typename collection_iterator::pointer pointer;
		typedef typename collection_iterator::pointer const_pointer;
		typedef typename collection_iterator::reference reference;
		typedef typename collection_iterator::reference const_reference;
		typedef collection_iterator iterator;
		typedef collection_iterator const_iterator;
		typedef std::reverse_iterator<collection_iterator> reverse_iterator;
		typedef std::reverse_iterator<collection_iterator> const_reverse_iterator;
		typedef typename collection_iterator::size_type size_type;
		typedef typename collection_iterator::difference_type difference_type;

	private:
		constexpr component_collection(handler_t *h, set_ptr_t<C>... c, set_ptr_t<I>... i, set_ptr_t<E>... e, set_ptr_t<O>... o) noexcept
			: m_handler(h), m_collected(c...), m_included(i...), m_excluded(e...), m_optional(o...)
		{
			SEK_ASSERT(h != nullptr, "Collection handler must not be null");
			SEK_ASSERT(((c != nullptr) && ...), "Collected component sets must not be null");
			SEK_ASSERT(((i != nullptr) && ...), "Included component sets must not be null");
			SEK_ASSERT(((e != nullptr) && ...), "Excluded component sets must not be null");
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
		[[nodiscard]] constexpr iterator begin() const noexcept { return iterator{this, size()}; }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
		/** Returns iterator one past the last entity. */
		[[nodiscard]] constexpr iterator end() const noexcept { return iterator{this}; }
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
		/** Returns the size of the collection. */
		[[nodiscard]] constexpr size_type size() const noexcept;
		/** Checks if the the collection is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept;

		/** Checks if the the collection contains the specified entity. */
		[[nodiscard]] constexpr bool contains(entity_t entity) const noexcept;
		/** Returns iterator to the specified entity, or an end iterator if the entity does not belong to the collection. */
		[[nodiscard]] constexpr iterator find(entity_t entity) const noexcept;

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
			std::swap(m_excluded, other.m_excluded);
			std::swap(m_optional, other.m_optional);
		}
		friend constexpr void swap(component_collection &a, component_collection &b) noexcept { a.swap(b); }

	private:
		template<typename T>
		[[nodiscard]] constexpr auto *get_collected() const noexcept
		{
			return std::get<set_ptr_t<T>>(m_collected);
		}
		template<typename T>
		[[nodiscard]] constexpr auto *get_included() const noexcept
		{
			return std::get<set_ptr_t<T>>(m_included);
		}
		template<typename T>
		[[nodiscard]] constexpr auto *get_optional() const noexcept
		{
			return std::get<set_ptr_t<T>>(m_optional);
		}

		template<typename T, typename... Ts>
		[[nodiscard]] constexpr decltype(auto) get_impl(const_iterator which) const noexcept
		{
			if constexpr (sizeof...(Ts) != 0)
				return std::tuple<T *, Ts *...>{get_impl<T>(which), get_impl<Ts>(which)...};
			else if constexpr (is_coll<T>)
			{
				/* Iterator offset is within the local range, need to scale for the component set's range. */
				const auto set = get_collected<T>();
				const auto pos = set->size() - (size() - which.m_off + 1);
				return static_cast<T *>(std::addressof(set->get(pos)));
			}
			else
				return get_impl<T>(*which);
		}
		template<typename T, typename... Ts>
		[[nodiscard]] constexpr decltype(auto) get_impl(entity_t e) const noexcept
		{
			if constexpr (sizeof...(Ts) != 0)
				return std::tuple<T *, Ts *...>{get_impl<T>(e), get_impl<Ts>(e)...};
			else if constexpr (is_coll<T>)
				return static_cast<T *>(std::addressof(get_collected<T>()->get(get_offset(e))));
			else if constexpr (is_inc<T>)
				return static_cast<T *>(std::addressof(get_included<T>()->get(e)));
			else
			{
				const auto set = get_optional<T>();
				if (set == nullptr) [[unlikely]]
					return static_cast<T *>(nullptr);
				const auto pos = set->find(e);
				if (pos == set->end()) [[unlikely]]
					return static_cast<T *>(nullptr);
				return static_cast<T *>(std::addressof(pos->second));
			}
		}

		[[nodiscard]] constexpr pointer get_entity(difference_type i) const noexcept;
		[[nodiscard]] constexpr size_type get_offset(entity_t e) const noexcept;

		handler_t *m_handler = nullptr;
		coll_ptr m_collected = {};
		inc_ptr m_included = {};
		exc_ptr m_excluded = {};
		opt_ptr m_optional = {};
	};
}	 // namespace sek::engine