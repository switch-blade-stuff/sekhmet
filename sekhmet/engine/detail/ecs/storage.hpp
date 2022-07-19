/*
 * Created by switchblade on 19/07/22
 */

#pragma once

#include "sekhmet/detail/event.hpp"

#include "component.hpp"

namespace sek::engine
{
	/** @brief Structure used to manage a pool of components and handle component creation & removal events. */
	template<typename T>
	class component_storage : public component_traits<T>::pool_type
	{
		friend class entity_world;

	public:
		typedef event<void(entity_world &, entity_t)> event_type;

		typedef component_traits<T> traits_type;
		typedef typename traits_type::pool_type base_pool;
		typedef typename base_pool::base_set base_set;

		typedef typename base_pool::allocator_type allocator_type;
		typedef typename base_pool::component_type component_type;
		typedef typename base_pool::value_type value_type;
		typedef typename base_pool::pointer pointer;
		typedef typename base_pool::const_pointer const_pointer;
		typedef typename base_pool::reference reference;
		typedef typename base_pool::const_reference const_reference;
		typedef typename base_pool::iterator iterator;
		typedef typename base_pool::const_iterator const_iterator;
		typedef typename base_pool::reverse_iterator reverse_iterator;
		typedef typename base_pool::const_reverse_iterator const_reverse_iterator;
		typedef typename base_pool::difference_type difference_type;
		typedef typename base_pool::size_type size_type;

	private:
		using set_iterator = typename base_set::iterator;

	public:
		component_storage() = delete;
		component_storage(const component_storage &) = delete;
		component_storage &operator=(const component_storage &) = delete;

		~component_storage() override = default;

		// clang-format off
		constexpr component_storage(component_storage &&)
			noexcept(std::is_nothrow_move_constructible_v<base_pool>&&
					 std::is_nothrow_move_constructible_v<event_type>) = default;
		constexpr component_storage &operator=(component_storage &&)
			noexcept(std::is_nothrow_move_assignable_v<base_pool> &&
			         std::is_nothrow_move_assignable_v<event_type>) = default;
		// clang-format on

		constexpr component_storage(entity_world &world, const allocator_type &alloc = {})
			: base_pool(alloc), m_world(world)
		{
		}
		constexpr component_storage(entity_world &world, size_type n, const allocator_type &alloc = {})
			: base_pool(n, alloc), m_world(world)
		{
		}

		constexpr component_storage(entity_world &world,
									std::initializer_list<entity_t> init_list,
									const allocator_type &alloc = {})
			: base_pool(init_list, alloc), m_world(world)
		{
		}

		/** Moves storage using the provided allocator. */
		constexpr component_storage(component_storage &&other, const allocator_type &alloc)
			: base_pool(std::move(other), alloc),
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

		/** @copydoc base_pool::replace */
		template<typename... Args>
		iterator replace(entity_t e, Args &&...args)
			requires std::constructible_from<T, Args...>
		{
			const auto result = base_pool::replace(e, std::forward<Args>(args)...);
			dispatch_replace(e);
			return result;
		}

		/** @copydoc base_pool::emplace */
		template<typename... Args>
		iterator emplace(entity_t e, Args &&...args)
			requires std::constructible_from<T, Args...>
		{
			const auto result = base_pool::emplace(e, std::forward<Args>(args)...);
			dispatch_create(e);
			return result;
		}
		/** @copydoc base_pool::emplace_or_replace */
		template<typename... Args>
		std::pair<iterator, bool> emplace_or_replace(entity_t e, Args &&...args)
			requires std::constructible_from<T, Args...>
		{
			const auto result = base_pool::emplace_or_replace(e, std::forward<Args>(args)...);
			if (result.second)
				dispatch_create(e);
			else
				dispatch_replace(e);
			return result;
		}

		/** @copydoc base_pool::emplace_back */
		template<typename... Args>
		iterator emplace_back(entity_t e, Args &&...args)
			requires std::constructible_from<T, Args...>
		{
			const auto result = base_pool::emplace_back(e, std::forward<Args>(args)...);
			dispatch_create(e);
			return result;
		}
		/** @copydoc base_pool::emplace_back_or_replace */
		template<typename... Args>
		std::pair<iterator, bool> emplace_back_or_replace(entity_t e, Args &&...args)
			requires std::constructible_from<T, Args...>
		{
			const auto result = base_pool::emplace_back_or_replace(e, std::forward<Args>(args)...);
			if (result.second)
				dispatch_create(e);
			else
				dispatch_replace(e);
			return result;
		}

		/** @copydoc base_pool::insert */
		iterator insert(entity_t e, component_type &&value)
		{
			const auto result = base_pool::insert(e, std::move(value));
			dispatch_create(e);
			return result;
		}
		/** @copydoc base_pool::insert */
		iterator insert(entity_t e, const component_type &value)
		{
			const auto result = base_pool::insert(e, value);
			dispatch_create(e);
			return result;
		}
		/** @copydoc base_pool::insert_or_replace */
		std::pair<iterator, bool> insert_or_replace(entity_t e, component_type &&value)
		{
			const auto result = base_pool::insert_or_replace(e, std::move(value));
			if (result.second)
				dispatch_create(e);
			else
				dispatch_replace(e);
			return result;
		}
		/** @copydoc base_pool::insert_or_replace */
		std::pair<iterator, bool> insert_or_replace(entity_t e, const component_type &value)
		{
			const auto result = base_pool::insert_or_replace(e, value);
			if (result.second)
				dispatch_create(e);
			else
				dispatch_replace(e);
			return result;
		}

		/** @copydoc base_pool::push_back */
		iterator push_back(entity_t e, component_type &&value)
		{
			const auto result = base_pool::push_back(e, std::move(value));
			dispatch_create(e);
			return result;
		}
		/** @copydoc base_pool::push_back */
		iterator push_back(entity_t e, const component_type &value)
		{
			const auto result = base_pool::push_back(e, value);
			dispatch_create(e);
			return result;
		}
		/** @copydoc base_pool::push_back_or_replace */
		std::pair<iterator, bool> push_back_or_replace(entity_t e, component_type &&value)
		{
			const auto result = base_pool::push_back_or_replace(e, std::move(value));
			if (result.second)
				dispatch_create(e);
			else
				dispatch_replace(e);
			return result;
		}
		/** @copydoc base_pool::push_back_or_replace */
		std::pair<iterator, bool> push_back_or_replace(entity_t e, const component_type &value)
		{
			const auto result = base_pool::push_back_or_replace(e, value);
			if (result.second)
				dispatch_create(e);
			else
				dispatch_replace(e);
			return result;
		}

	private:
		constexpr void dispatch_create(entity_t e) const { m_create.dispatch(m_world, e); }
		constexpr void dispatch_replace(entity_t e) const { m_replace.dispatch(m_world, e); }
		constexpr void dispatch_remove(entity_t e) const { m_remove.dispatch(m_world, e); }

		set_iterator erase(set_iterator where) final
		{
			dispatch_remove(*where);
			return base_pool::erase(where);
		}

		entity_world &m_world;
		event_type m_create;
		event_type m_replace;
		event_type m_remove;
	};
}	 // namespace sek::engine