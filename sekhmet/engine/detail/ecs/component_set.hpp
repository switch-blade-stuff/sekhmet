/*
 * Created by switchblade on 19/07/22
 */

#pragma once

#include "sekhmet/detail/event.hpp"
#include "sekhmet/detail/meta_util.hpp"

#include "entity_set.hpp"

namespace sek::engine
{
	/** @brief Structure used to manage a set of components and handle component creation, removal and modification events. */
	template<typename T>
	class component_set : public basic_entity_set<T, component_set<T>>
	{
		friend class basic_entity_set<T, component_set<T>>;
		friend class entity_world;

		using base_set = basic_entity_set<T, component_set>;

	public:
		typedef event<void(entity_world &, entity_t)> event_type;

		typedef component_traits<T> traits_type;

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
		component_set() = delete;
		component_set(const component_set &) = delete;
		component_set &operator=(const component_set &) = delete;

		constexpr ~component_set() = default;

		// clang-format off
		constexpr component_set(component_set &&)
			noexcept(std::is_nothrow_move_constructible_v<base_set> &&
					 std::is_nothrow_move_constructible_v<event_type>) = default;
		constexpr component_set &operator=(component_set &&)
			noexcept(std::is_nothrow_move_assignable_v<base_set> &&
			         std::is_nothrow_move_assignable_v<event_type>) = default;
		// clang-format on

		constexpr component_set(entity_world &world, const allocator_type &alloc = {}) : base_set(alloc), m_world(world)
		{
		}
		constexpr component_set(entity_world &world, size_type n, const allocator_type &alloc = {})
			: base_set(n, alloc), m_world(world)
		{
		}

		/** Moves storage using the provided allocator. */
		constexpr component_set(component_set &&other, const allocator_type &alloc)
			: base_set(std::move(other), alloc),
			  m_world(other.m_world),
			  m_create(std::move(other.m_create)),
			  m_remove(std::move(other.m_remove))
		{
		}

		/** Returns reference to the parent world of the storage. */
		[[nodiscard]] constexpr entity_world &world() const noexcept { return m_world; }

		/** Returns event proxy for the component create event. */
		[[nodiscard]] constexpr event_proxy<event_type> on_create() noexcept { return {m_create}; }
		/** Returns event proxy for the component replace event. */
		[[nodiscard]] constexpr event_proxy<event_type> on_replace() noexcept { return {m_replace}; }
		/** Returns event proxy for the component remove event. */
		[[nodiscard]] constexpr event_proxy<event_type> on_remove() noexcept { return {m_remove}; }

	private:
		constexpr void replace_(entity_t e) const { m_replace.dispatch(m_world, e); }
		constexpr void insert_(entity_t e) const { m_create.dispatch(m_world, e); }
		constexpr void erase_(entity_t e) const { m_remove.dispatch(m_world, e); }

		entity_world &m_world;
		event_type m_create;
		event_type m_replace;
		event_type m_remove;
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