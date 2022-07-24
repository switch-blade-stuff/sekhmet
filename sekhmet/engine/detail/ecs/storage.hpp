/*
 * Created by switchblade on 19/07/22
 */

#pragma once

#include "sekhmet/detail/event.hpp"

#include "entity_set.hpp"

namespace sek::engine
{
	/** @brief Structure used to manage a pool of components and handle component creation & removal events. */
	template<typename T>
	class component_storage : public component_set<T>
	{
		friend class entity_world;

		using base_set = component_set<T>;

	public:
		typedef event<void(entity_world &, entity_t)> event_type;

		typedef component_traits<T> traits_type;
		typedef component_set<T> set_type;

		typedef typename base_set::allocator_type allocator_type;
		typedef typename base_set::component_type component_type;
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

	public:
		component_storage() = delete;
		component_storage(const component_storage &) = delete;
		component_storage &operator=(const component_storage &) = delete;

		constexpr ~component_storage() = default;

		// clang-format off
		constexpr component_storage(component_storage &&)
			noexcept(std::is_nothrow_move_constructible_v<base_set> &&
					 std::is_nothrow_move_constructible_v<event_type>) = default;
		constexpr component_storage &operator=(component_storage &&)
			noexcept(std::is_nothrow_move_assignable_v<base_set> &&
			         std::is_nothrow_move_assignable_v<event_type>) = default;
		// clang-format on

		constexpr component_storage(entity_world &world, const allocator_type &alloc = {})
			: base_set(alloc), m_world(world)
		{
		}
		constexpr component_storage(entity_world &world, size_type n, const allocator_type &alloc = {})
			: base_set(n, alloc), m_world(world)
		{
		}

		/** Moves storage using the provided allocator. */
		constexpr component_storage(component_storage &&other, const allocator_type &alloc)
			: base_set(std::move(other), alloc),
			  m_world(other.m_world),
			  m_create(std::move(other.m_create)),
			  m_remove(std::move(other.m_remove))
		{
		}

		/** Returns reference to the parent world of the storage. */
		[[nodiscard]] constexpr entity_world &world() const noexcept { return m_world; }

		/** Returns event proxy for the component creation event. */
		[[nodiscard]] constexpr event_proxy<event_type> on_create() noexcept { return {m_create}; }
		/** Returns event proxy for the component removal event. */
		[[nodiscard]] constexpr event_proxy<event_type> on_remove() noexcept { return {m_remove}; }

		/** @copydoc base_set::replace */
		template<typename... Args>
		constexpr decltype(auto) replace(entity_t e, Args &&...args)
			requires std::constructible_from<T, Args...>
		{
			auto &result = base_set::replace(e, std::forward<Args>(args)...);
			dispatch_replace(e);
			return result;
		}
		/** @copydoc base_set::emplace */
		template<typename... Args>
		constexpr decltype(auto) emplace(entity_t e, Args &&...args)
			requires std::constructible_from<T, Args...>
		{
			auto &result = base_set::emplace(e, std::forward<Args>(args)...);
			dispatch_create(e);
			return result;
		}
		/** @copydoc base_set::emplace_back */
		template<typename... Args>
		constexpr decltype(auto) emplace_back(entity_t e, Args &&...args)
			requires std::constructible_from<T, Args...>
		{
			auto &result = base_set::emplace_back(e, std::forward<Args>(args)...);
			dispatch_create(e);
			return result;
		}
		/** Emplaces or modifies a component for the specified entity (re-using slots if component type requires fixed storage).
		 *
		 * @param e Entity to emplace component for.
		 * @param args Arguments passed to component's constructor.
		 * @return Reference to the component (or `void`, if component is empty). */
		template<typename... Args>
		constexpr decltype(auto) emplace_or_replace(entity_t e, Args &&...args)
			requires std::constructible_from<T, Args...>
		{
			if (!base_set::contains(e))
				return emplace(e, std::forward<Args>(args)...);
			else
				return replace(e, std::forward<Args>(args)...);
		}
		/** Emplaces or modifies a component for the specified entity (always at the end).
		 *
		 * @param e Entity to emplace component for.
		 * @param args Arguments passed to component's constructor.
		 * @return Reference to the component (or `void`, if component is empty). */
		template<typename... Args>
		constexpr decltype(auto) emplace_back_or_replace(entity_t e, Args &&...args)
			requires std::constructible_from<T, Args...>
		{
			if (!base_set::contains(e))
				return emplace_back(e, std::forward<Args>(args)...);
			else
				return replace(e, std::forward<Args>(args)...);
		}

		/** @copydoc base_set::insert */
		constexpr iterator insert(entity_t e, component_type &&value)
		{
			const auto result = base_set::insert(e, std::move(value));
			dispatch_create(e);
			return result;
		}
		/** @copydoc base_set::insert */
		constexpr iterator insert(entity_t e, const component_type &value)
		{
			const auto result = base_set::insert(e, value);
			dispatch_create(e);
			return result;
		}
		/** @copydoc base_set::insert_or_replace */
		constexpr std::pair<iterator, bool> insert_or_replace(entity_t e, component_type &&value)
		{
			const auto result = base_set::insert_or_replace(e, std::move(value));
			if (result.second)
				dispatch_create(e);
			else
				dispatch_replace(e);
			return result;
		}
		/** @copydoc base_set::insert_or_replace */
		constexpr std::pair<iterator, bool> insert_or_replace(entity_t e, const component_type &value)
		{
			const auto result = base_set::insert_or_replace(e, value);
			if (result.second)
				dispatch_create(e);
			else
				dispatch_replace(e);
			return result;
		}
		/** @copydoc base_set::push_back */
		constexpr iterator push_back(entity_t e, component_type &&value)
		{
			const auto result = base_set::push_back(e, std::move(value));
			dispatch_create(e);
			return result;
		}
		/** @copydoc base_set::push_back */
		constexpr iterator push_back(entity_t e, const component_type &value)
		{
			const auto result = base_set::push_back(e, value);
			dispatch_create(e);
			return result;
		}
		/** @copydoc base_set::push_back_or_replace */
		constexpr std::pair<iterator, bool> push_back_or_replace(entity_t e, component_type &&value)
		{
			const auto result = base_set::push_back_or_replace(e, std::move(value));
			if (result.second)
				dispatch_create(e);
			else
				dispatch_replace(e);
			return result;
		}
		/** @copydoc base_set::push_back_or_replace */
		constexpr std::pair<iterator, bool> push_back_or_replace(entity_t e, const component_type &value)
		{
			const auto result = base_set::push_back_or_replace(e, value);
			if (result.second)
				dispatch_create(e);
			else
				dispatch_replace(e);
			return result;
		}

		using base_set::erase;

	private:
		constexpr void dispatch_create(entity_t e) const { m_create.dispatch(m_world, e); }
		constexpr void dispatch_replace(entity_t e) const { m_replace.dispatch(m_world, e); }
		constexpr void dispatch_remove(entity_t e) const { m_remove.dispatch(m_world, e); }

		entity_world &m_world;
		event_type m_create;
		event_type m_replace;
		event_type m_remove;
	};

	/** @brief Structure used to indirectly reference a component through an entity. */
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

		/** Initializes a component pointer for an entity and a pool.
		 * @param e Entity who's component to point to.
		 * @param pool Pointer to the component pool containing the target component. */
		constexpr component_ptr(entity_t e, set_ptr pool) noexcept : m_entity(e), m_pool(pool) {}
		/** Initializes a component pointer for an entity and a pool.
		 * @param e Entity who's component to point to.
		 * @param pool Reference to the component pool containing the target component. */
		constexpr component_ptr(entity_t e, set_ref pool) noexcept : component_ptr(e, &pool) {}

		/** Checks if the component pointer has a a bound entity and pool. */
		[[nodiscard]] constexpr bool empty() const noexcept { return !m_entity.is_tombstone() && m_pool; }
		/** @copydoc empty */
		[[nodiscard]] constexpr operator bool() const noexcept { return empty(); }

		/** Returns the associated entity. */
		[[nodiscard]] constexpr entity_t entity() const noexcept { return m_entity; }
		/** Returns the bound component pool. */
		[[nodiscard]] constexpr set_type *pool() const noexcept { return m_pool; }

		/** Returns pointer to the associated component. */
		[[nodiscard]] constexpr pointer get() const noexcept { return std::addressof(m_pool->find(m_entity)->second); }
		/** @copydoc get */
		[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
		/** Returns reference to the associated component. */
		[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }

		/** Rebinds pointer to use a different component pool.
		 * @param pool Pointer to the new pool.
		 * @return Pointer to the old pool. */
		constexpr set_ptr reset(set_ptr pool) noexcept { return std::exchange(m_pool, pool); }
		/** Resets pointer to a null state.
		 * @return Pointer to the old pool. */
		constexpr set_ptr reset() noexcept { return reset(nullptr); }

		[[nodiscard]] constexpr bool operator==(const component_ptr &) const noexcept = default;
		[[nodiscard]] constexpr bool operator!=(const component_ptr &) const noexcept = default;

		constexpr void swap(component_ptr &other) noexcept
		{
			using std::swap;
			swap(m_entity, other.m_entity);
			swap(m_pool, other.m_pool);
		}
		friend constexpr void swap(component_ptr &a, component_ptr &b) noexcept { a.swap(b); }

	private:
		entity_t m_entity = entity_t::tombstone();
		set_ptr m_pool = nullptr;
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