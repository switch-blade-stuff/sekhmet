/*
 * Created by switchblade on 14/07/22
 */

#pragma once

#include "../type_info.hpp"
#include "entity.hpp"

namespace sek::engine
{
	/** @brief Helper type used to obtain traits of a component type.
	 *
	 * Component traits must contain a compile-time constant of type `std::size_t` named `page_size`, specifying
	 * size of allocation pages used by component pools.
	 *
	 * Optionally, traits may define an `is_fixed` typedef which, if present, will prevent components of this type
	 * from being sorted either by a component pool or ordering queries. */
	template<typename T>
	struct component_traits
	{
		constexpr static std::size_t page_size = 1024;
	};

	namespace detail
	{
		// clang-format off
		template<typename T, typename A>
		using component_entity_set = basic_entity_set<
		    typename std::allocator_traits<A>::template rebind_alloc<entity>,
		    requires{ typename component_traits<T>::is_fixed; }>;
		// clang-format on
	}	 // namespace detail

	/** @brief Structure used to allocate components and associate them with entities.
	 *
	 * Component pools allocate components in pages. Pages are used to reduce the need for re-allocation and copy/move
	 * operations for components. Every component is then indirectly indexed via an entity via an entity set.
	 *
	 * @tparam T Type of components managed by the pool.
	 * @tparam A Allocator used to allocate memory of the pool. */
	template<typename T, typename A = std::allocator<T>>
	class basic_component_pool : detail::component_entity_set<T, A>
	{
		using pages_t = std::vector<T *, typename std::allocator_traits<A>::template rebind_alloc<T *>>;

		// clang-format off
		constexpr static bool is_fixed = requires{ typename component_traits<T>::is_fixed; };
		// clang-format on
		constexpr static std::size_t page_size = component_traits<T>::page_size;

		[[nodiscard]] constexpr static std::size_t page_idx(std::size_t i) noexcept { return i / page_size; }
		[[nodiscard]] constexpr static std::size_t component_idx(std::size_t i) noexcept { return i % page_size; }

	public:
		typedef detail::component_entity_set<T, A> base_set;

		typedef A allocator_type;
		typedef T component_type;

	public:
		/** Returns reference to the set of entities of the component pool. */
		[[nodiscard]] constexpr base_set &entities() noexcept { return *this; }
		/** @copydoc entities */
		[[nodiscard]] constexpr const base_set &entities() const noexcept { return *this; }

	private:
		[[nodiscard]] constexpr auto &pages() noexcept { return m_pages_alloc.first(); }
		[[nodiscard]] constexpr auto &pages() const noexcept { return m_pages_alloc.first(); }
		[[nodiscard]] constexpr auto &page_alloc() noexcept { return m_pages_alloc.second(); }
		[[nodiscard]] constexpr auto &page_alloc() const noexcept { return m_pages_alloc.second(); }

		[[nodiscard]] T *alloc_page() { return std::allocator_traits<A>::allocate(page_alloc(), page_size); }
		void dealloc_page(T *ptr) { std::allocator_traits<A>::deallocate(page_alloc(), ptr, page_size); }

		[[nodiscard]] constexpr auto *get_component(std::size_t i) noexcept
		{
			return pages()[page_idx(i)] + component_idx(i);
		}
		[[nodiscard]] constexpr auto *get_component(std::size_t i) const noexcept
		{
			return pages()[page_idx(i)] + component_idx(i);
		}

		packed_pair<pages_t, A> m_pages_alloc; /* Array of pointers to component pages & page allocator. */
		type_info m_type;					   /* Type of components stored by the pool. */
	};
}	 // namespace sek::engine