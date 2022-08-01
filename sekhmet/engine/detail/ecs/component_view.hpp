/*
 * Created by switchblade on 31/07/22
 */

#pragma once

#include "component_set.hpp"

namespace sek::engine
{
	template<typename, typename = excluded_t<>, typename = optional_t<>>
	class component_view;

	/** @brief Structure used to provide a simple view of components for a set of entities.
	 * @tparam Inc Component types captured by the view.
	 * @tparam Inc Component types excluded from the view.
	 * @tparam Opt Optional components of the view.
	 * @tparam Alloc Allocator used for internal entity set. */
	template<typename... Inc, typename... Exc, typename... Opt>
	class component_view<included_t<Inc...>, excluded_t<Exc...>, optional_t<Opt...>>
	{
		using common_set = detail::entity_set_base;

		template<typename T>
		using set_t = transfer_cv_t<T, component_set<std::remove_cv_t<T>>>;
		template<typename T>
		using set_ptr_t = set_t<T> *;

		using inc_ptr = std::tuple<set_ptr_t<Inc>...>;
		using exc_ptr = std::tuple<set_ptr_t<Exc>...>;

		template<typename T>
		using is_opt = is_in<std::remove_cv_t<T>, std::remove_cv_t<Opt>...>;
		template<typename T>
		constexpr static bool is_opt_v = is_opt<T>::value;

		template<typename T>
		[[nodiscard]] constexpr static bool accept(entity_t e, const inc_ptr &inc) noexcept
		{
			return std::get<set_ptr_t<T>>(inc)->contains(e);
		}
		template<typename T>
		[[nodiscard]] constexpr static bool reject(entity_t e, const exc_ptr &exc) noexcept
		{
			return std::get<set_ptr_t<T>>(exc)->contains(e);
		}

		class view_iterator
		{
			friend class component_view;

		public:
			typedef entity_t value_type;
			typedef const entity_t &reference;
			typedef const entity_t *pointer;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;
			typedef std::bidirectional_iterator_tag iterator_category;

		private:
			constexpr explicit view_iterator(const component_view *view) noexcept : m_view(view) {}
			constexpr view_iterator(const component_view *view, difference_type off) noexcept
				: m_view(view), m_off(next_valid(off))
			{
			}
			constexpr view_iterator(const component_view *view, size_type off) noexcept
				: view_iterator(view, static_cast<difference_type>(off))
			{
			}

		public:
			constexpr view_iterator() noexcept = default;

			constexpr view_iterator operator++(int) noexcept
			{
				auto temp = *this;
				++(*this);
				return temp;
			}
			constexpr view_iterator &operator++() noexcept
			{
				m_off = next_valid(m_off - 1);
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
				for (const auto end = m_view->size_hint(); m_off != end && !valid(m_off);) ++m_off;
				return *this;
			}

			/** Returns pointer to the target entity. */
			[[nodiscard]] constexpr pointer get() const noexcept { return get(m_off); }
			/** @copydoc get */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
			/** Returns reference to the target entity */
			[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }

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
				std::swap(m_view, other.m_view);
				std::swap(m_off, other.m_off);
			}
			friend constexpr void swap(view_iterator &a, view_iterator &b) noexcept { a.swap(b); }

		private:
			[[nodiscard]] constexpr difference_type next_valid(difference_type i) const noexcept
			{
				while (i != 0 && !valid(i)) --i;
				return i;
			}
			[[nodiscard]] constexpr pointer get(difference_type i) const noexcept
			{
				return m_view->m_set->data() + i - 1;
			}
			[[nodiscard]] constexpr bool valid(difference_type i) const noexcept { return m_view->contains(*get(i)); }

			const component_view *m_view = nullptr;
			difference_type m_off = 0;
		};

	public:
		typedef typename view_iterator::value_type value_type;
		typedef typename view_iterator::pointer pointer;
		typedef typename view_iterator::pointer const_pointer;
		typedef typename view_iterator::reference reference;
		typedef typename view_iterator::reference const_reference;
		typedef view_iterator iterator;
		typedef view_iterator const_iterator;
		typedef std::reverse_iterator<view_iterator> reverse_iterator;
		typedef std::reverse_iterator<view_iterator> const_reverse_iterator;
		typedef typename view_iterator::size_type size_type;
		typedef typename view_iterator::difference_type difference_type;

	private:
		template<typename T>
		[[nodiscard]] constexpr static const common_set *select_common(T *storage) noexcept
		{
			return detail::to_base_set(storage);
		}
		template<typename T0, typename T1, typename... Ts>
		[[nodiscard]] constexpr static const common_set *select_common(T0 *a, T1 *b, Ts *...rest) noexcept
			requires(is_opt_v<Inc> && ...)
		{
			/* If all included types are optional, use the largest set. */
			return a->size() >= b->size() ? select_common(a, rest...) : select_common(b, rest...);
		}
		template<typename T0, typename T1, typename... Ts>
		[[nodiscard]] constexpr static const common_set *select_common(T0 *a, T1 *b, Ts *...rest) noexcept
		{
			/* If there are non-optional types, use the smallest non-optional set. */
			using S0 = std::remove_cv_t<T0>;
			using S1 = std::remove_cv_t<T1>;

			if constexpr (is_opt_v<typename S1::component_type>)
				return select_common(a, rest...);
			else if constexpr (is_opt_v<typename S0::component_type>)
				return select_common(b, rest...);
			else
				return a->size() < b->size() ? select_common(a, rest...) : select_common(b, rest...);
		}

	public:
		component_view() = delete;

		constexpr component_view(const component_view &) = default;
		constexpr component_view &operator=(const component_view &) = default;
		constexpr component_view(component_view &&) noexcept = default;
		constexpr component_view &operator=(component_view &&) noexcept = default;

		/** Initializes component view from pointers to component sets.
		 * @param inc Pointers to component sets of included components.
		 * @param exc Pointers to component sets of excluded components.
		 * @note The smallest component set will be used as the main set. */
		constexpr component_view(set_ptr_t<Inc>... inc, set_ptr_t<Exc>... exc)
			: m_set(select_common(inc...)), m_included(inc...), m_excluded(exc...)
		{
		}

		/** Rebinds view to use the specified component set as the main set. */
		template<typename C>
		constexpr component_view &rebind() noexcept
		{
			m_set = std::get<set_ptr_t<C>>(m_included);
			return *this;
		}

		/** Returns iterator to the first entity. */
		[[nodiscard]] constexpr iterator begin() const noexcept { return iterator{this, size_hint()}; }
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
		/** Checks if the the view is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return begin() == end(); }
		/** Checks if the the view contains the specified entity. */
		[[nodiscard]] constexpr bool contains(entity_t entity) const noexcept
		{
			const auto &inc = m_included;
			const auto &exc = m_excluded;
			return ((is_opt_v<Inc> || accept<Inc>(entity, inc)) && ...) && !(reject<Exc>(entity, exc) || ...);
		}

		/** Returns the size of the of the main set (approximate size of the view). */
		[[nodiscard]] constexpr size_type size_hint() const noexcept { return m_set->size(); }

		/** Returns pointers to components associated with the specified entity.
		 *
		 * @param entity Entity to get components for.
		 * @return Tuple of pointers to entity components, or a single pointer if only one component is specified.
		 *
		 * @note Pointers to optional components may be null.
		 * @warning Using an entity not belonging to the view results in undefined behavior. */
		template<typename... Ts>
		[[nodiscard]] constexpr decltype(auto) get(entity_t entity) const noexcept
		{
			return get_impl<Ts...>(entity);
		}
		/** Returns pointers to components associated with the entity pointed to by the iterator.
		 *
		 * @param which Iterator to the entity to get components for.
		 * @return Tuple of pointers to entity components, or a single pointer if only one component is specified.
		 * @note Pointers to optional components may be null. */
		template<typename... Ts>
		[[nodiscard]] constexpr decltype(auto) get(const_iterator which) const noexcept
		{
			return get<Ts...>(*which);
		}

		/** Applies the functor to every entity of the view.
		 * Functor may optionally return a value, which if evaluated to `false`, prematurely terminates iteration. */
		template<std::invocable<entity_t, Inc *...> F>
		constexpr void for_each(F &&f) const
		{
			for (auto entity : *this)
			{
				const auto elements = get<Inc...>(entity);
				if constexpr (std::convertible_to<std::invoke_result_t<F, entity_t, Inc *...>, bool>)
				{
					if (!std::invoke(std::forward<F>(f), entity, std::get<Inc *>(elements)...)) [[unlikely]]
						break;
				}
				else
					std::invoke(std::forward<F>(f), entity, std::get<Inc *>(elements)...);
			}
		}

		constexpr void swap(component_view &other) noexcept
		{
			using std::swap;
			swap(m_included, other.m_included);
			swap(m_excluded, other.m_excluded);
			swap(m_set, other.m_set);
		}
		friend constexpr void swap(component_view &a, component_view &b) noexcept { a.swap(b); }

	private:
		template<typename T>
		[[nodiscard]] constexpr auto *storage() const noexcept
		{
			using std::get;
			return get<set_ptr_t<T>>(m_included);
		}

		template<typename... Ts>
		[[nodiscard]] constexpr std::tuple<Ts *...> get_impl(entity_t entity) const noexcept
			requires(sizeof...(Ts) > 1)
		{
			return std::forward_as_tuple(get_impl<Ts>(entity)...);
		}
		template<typename T>
		[[nodiscard]] constexpr T *get_impl(entity_t entity) const noexcept
		{
			const auto set = storage<T>();
			if constexpr (is_opt_v<T>) /* Optional types need additional null checks. */
			{
				if (set == nullptr) [[unlikely]]
					return nullptr;

				const auto pos = set->find(entity);
				if (pos == set->end()) [[unlikely]]
					return nullptr;

				return std::addressof(pos->second);
			}
			else
				return std::addressof(set->get(entity));
		}

		const common_set *m_set;
		inc_ptr m_included;
		exc_ptr m_excluded;
	};
}	 // namespace sek::engine