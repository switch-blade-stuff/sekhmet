/*
 * Created by switchblade on 31/07/22
 */

#pragma once

#include "component_iterator.hpp"

namespace sek::engine
{
	template<typename, typename = optional_t<>, typename = std::allocator<entity_t>>
	class component_view;

	/** @brief Structure used to provide a simple view of components for a set of entities.
	 * @tparam Cs Component types captured by the view.
	 * @tparam Opt Optional components of the view.
	 * @tparam Alloc Allocator used for internal entity set. */
	template<typename... Cs, typename... Opt, typename Alloc>
	class component_view<included_t<Cs...>, optional_t<Opt...>, Alloc>
	{
		using entities_t = std::vector<entity_t, typename std::allocator_traits<Alloc>::template rebind_alloc<entity_t>>;
		template<typename T>
		using set_t = transfer_cv_t<T, component_set<std::remove_cvref_t<T>>>;

	public:
		typedef std::tuple<entity_t, Cs...> value_type;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;
		typedef typename entities_t::allocator_type allocator_type;

	private:
		struct iterator_base
		{
			constexpr iterator_base() noexcept = default;
			constexpr iterator_base(const entities_t *ptr, size_type pos) noexcept
				: m_ptr(ptr), m_pos(static_cast<difference_type>(pos))
			{
			}

			constexpr iterator_base &operator+=(difference_type n) noexcept
			{
				m_pos += n;
				return *this;
			}
			constexpr iterator_base &operator-=(difference_type n) noexcept
			{
				m_pos -= n;
				return *this;
			}
			[[nodiscard]] constexpr iterator_base operator+(difference_type n) const noexcept
			{
				return iterator_base{m_ptr, m_pos + n};
			}
			[[nodiscard]] constexpr iterator_base operator-(difference_type n) const noexcept
			{
				return iterator_base{m_ptr, m_pos - n};
			}
			[[nodiscard]] constexpr difference_type operator-(const iterator_base &other) const noexcept
			{
				return m_pos - other.m_pos;
			}

			[[nodiscard]] constexpr const entity_t *get() const noexcept { return m_ptr->data() + m_pos; }

			template<template_type_instance<component_set> S>
			[[nodiscard]] constexpr const typename S::component_type *get(const S *ptr) const noexcept
			{
				const auto entity = *get();
				if constexpr (is_in_v<typename S::component_type, std::remove_cv_t<Opt>...>)
				{
					if (ptr == nullptr) [[unlikely]]
						return nullptr;

					const auto pos = ptr->find(entity);
					if (pos == ptr->end()) [[unlikely]]
						return nullptr;

					return std::addressof(pos->second);
				}
				else
					return std::addressof(ptr->get(entity));
			}
			template<template_type_instance<component_set> S>
			[[nodiscard]] constexpr typename S::component_type *get(S *ptr) const noexcept
			{
				const auto entity = *get();
				if constexpr (is_in_v<typename S::component_type, std::remove_cv_t<Opt>...>)
				{
					if (ptr == nullptr) [[unlikely]]
						return nullptr;

					const auto pos = ptr->find(entity);
					if (pos == ptr->end()) [[unlikely]]
						return nullptr;

					return std::addressof(pos->second);
				}
				else
					return std::addressof(ptr->get(entity));
			}

			[[nodiscard]] constexpr auto operator<=>(const iterator_base &other) const noexcept
			{
				return m_pos <=> other.m_pos;
			}
			[[nodiscard]] constexpr bool operator==(const iterator_base &other) const noexcept
			{
				return m_pos == other.m_pos;
			}

			constexpr void swap(iterator_base &other) noexcept
			{
				std::swap(m_ptr, other.m_ptr);
				std::swap(m_pos, other.m_pos);
			}

			const entities_t *m_ptr = nullptr;
			difference_type m_pos = 0;
		};

	public:
		typedef basic_component_iterator<iterator_base, Cs...> iterator;
		typedef basic_component_iterator<iterator_base, Cs...> const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

		typedef typename iterator::pointer pointer;
		typedef typename const_iterator::pointer const_pointer;
		typedef typename iterator::reference reference;
		typedef typename const_iterator::reference const_reference;

	public:
		component_view() = delete;

		constexpr component_view(const component_view &) = default;
		constexpr component_view &operator=(const component_view &) = default;
		constexpr component_view(component_view &&) noexcept = default;
		constexpr component_view &operator=(component_view &&) noexcept = default;

		/** Initializes component view from a set of entities and pointers to component sets.
		 * @param storage Pointers to component sets, containing viewed components.
		 * @param il Initializer list containing viewed entities. */
		constexpr component_view(set_t<Cs> *...storage, std::initializer_list<entity_t> il)
			: m_storage(storage...), m_entities(il)
		{
		}
		/** @copydoc component_view
		 * @param alloc Allocator used to allocate internal memory. */
		constexpr component_view(set_t<Cs> *...storage, std::initializer_list<entity_t> il, const allocator_type &alloc)
			: m_storage(storage...), m_entities(il, alloc)
		{
		}
		/** Initializes component view from a set of entities and pointers to component sets.
		 * @param storage Pointers to component sets, containing viewed components.
		 * @param first Iterator to the first entity of the view.
		 * @param last Iterator one past the last entity of the view. */
		template<std::forward_iterator I, std::sentinel_for<I> S>
		constexpr component_view(set_t<Cs> *...storage, I first, S last)
			: m_storage(storage...), m_entities(first, last)
		{
		}
		/** @copydoc component_view
		 * @param alloc Allocator used to allocate internal memory. */
		template<std::forward_iterator I, std::sentinel_for<I> S>
		constexpr component_view(set_t<Cs> *...storage, I first, S last, const allocator_type &alloc)
			: m_storage(storage...), m_entities(first, last, alloc)
		{
		}

		/** Initializes component view from a vector of entities and pointers to component sets.
		 * @param storage Pointers to component sets, containing viewed components.
		 * @param entities Vector containing viewed entities. */
		constexpr component_view(set_t<Cs> *...storage, const entities_t &entities)
			: m_storage(storage...), m_entities(entities)
		{
		}
		/** @copydoc component_view */
		constexpr component_view(set_t<Cs> *...storage, entities_t &&entities) noexcept
			: m_storage(storage...), m_entities(std::move(entities))
		{
		}
		/** @copydoc component_view
		 * @param alloc Allocator used to allocate internal memory. */
		constexpr component_view(set_t<Cs> *...storage, const entities_t &entities, const allocator_type &alloc)
			: m_storage(storage...), m_entities(entities, alloc)
		{
		}
		/** @copydoc component_view */
		constexpr component_view(set_t<Cs> *...storage, entities_t &&entities, const allocator_type &alloc)
			: m_storage(storage...), m_entities(std::move(entities), alloc)
		{
		}

		/** Returns iterator to the first entity and it's components. */
		[[nodiscard]] constexpr iterator begin() const noexcept { return to_iterator(0); }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
		/** Returns iterator one past the last entity. */
		[[nodiscard]] constexpr iterator end() const noexcept { return to_iterator(size()); }
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }
		/** Returns reverse iterator to the last entity and it's components. */
		[[nodiscard]] constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator{end()}; }
		/** @copydoc rbegin */
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
		/** Returns reverse iterator one past the first entity. */
		[[nodiscard]] constexpr reverse_iterator rend() const noexcept { return reverse_iterator{begin()}; }
		/** @copydoc rend */
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return rend(); }

		/** Returns the size of the view. */
		[[nodiscard]] constexpr size_type size() const noexcept { return m_entities.size(); }
		/** Checks if the view is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_entities.empty(); }

		/** Returns reference to the first entity and it's components. */
		[[nodiscard]] constexpr reference front() const noexcept { return *begin(); }
		/** Returns reference to the last entity and it's components. */
		[[nodiscard]] constexpr reference back() const noexcept { return *std::prev(end()); }
		/** Returns reference to the entity at the specified index and it's components. */
		[[nodiscard]] constexpr reference at(size_type i) const noexcept { return *to_iterator(i); }
		/** @copydoc at */
		[[nodiscard]] constexpr reference operator[](size_type i) const noexcept { return at(i); }

		/** Applies the functor to every entity of the view. */
		template<std::invocable<entity_t, Cs *...> F>
		constexpr void for_each(F &&f) const
		{
			for_each(std::make_index_sequence<sizeof...(Cs)>{}, std::forward<F>(f));
		}

		constexpr void swap(component_view &other) noexcept
		{
			using std::swap;
			swap(m_storage, other.m_storage);
			swap(m_entities, other.m_entities);
		}
		friend constexpr void swap(component_view &a, component_view &b) noexcept { a.swap(b); }

	private:
		template<size_type... Is>
		[[nodiscard]] constexpr const_iterator to_iterator(std::index_sequence<Is...>, size_type i) const noexcept
		{
			using std::get;
			return const_iterator{iterator_base{&m_entities, i}, get<Is>(m_storage)...};
		}
		[[nodiscard]] constexpr const_iterator to_iterator(size_type i) const noexcept
		{
			return to_iterator(std::make_index_sequence<sizeof...(Cs)>{}, i);
		}

		template<size_type... Is, typename F>
		constexpr void for_each(std::index_sequence<Is...>, F &&f) const
		{
			for (auto element = begin(), last = end(); element != last; ++element)
			{
				const auto ptr = element.get();
				if constexpr (std::convertible_to<std::invoke_result_t<F, entity_t, Cs *...>, bool>)
				{
					if (!std::invoke(f, *ptr.template get<0>(), ptr.template get<1 + Is>()...)) [[unlikely]]
						break;
				}
				else
					std::invoke(f, *ptr.template get<0>(), ptr.template get<1 + Is>()...);
			}
		}

		std::tuple<set_t<Cs> *...> m_storage;
		entities_t m_entities;
	};
}	 // namespace sek::engine