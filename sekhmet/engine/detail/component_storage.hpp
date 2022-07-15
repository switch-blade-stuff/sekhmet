/*
 * Created by switchblade on 14/07/22
 */

#pragma once

#include "sekhmet/detail/event.hpp"
#include "sekhmet/detail/packed_pair.hpp"

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

	/** @brief Structure used to allocate components and associate them with entities.
	 *
	 * Component pools allocate components in pages. Pages are used to reduce the need for re-allocation and copy/move
	 * operations for components. Every component is then indirectly indexed via an entity. */
	template<typename T, typename Alloc = std::allocator<T>>
	class basic_component_pool
	{
		using dense_alloc = typename std::allocator_traits<Alloc>::template rebind_alloc<entity>;
		using sparse_alloc = typename std::allocator_traits<Alloc>::template rebind_alloc<T *>;
		using dense_t = std::vector<entity, dense_alloc>;
		using sparse_t = std::vector<T *, sparse_alloc>;
		using traits_t = component_traits<T>;

		// clang-format off
		constexpr static bool is_fixed = requires{ typename component_traits<T>::is_fixed; };
		// clang-format on
		constexpr static std::size_t page_size = component_traits<T>::page_size;

	public:
		typedef Alloc allocator_type;
		typedef T component_type;

	private:
		[[nodiscard]] auto &sparse_vector() noexcept { return m_sparse_alloc.first(); }
		[[nodiscard]] auto &sparse_vector() const noexcept { return m_sparse_alloc.first(); }
		[[nodiscard]] auto &page_alloc() noexcept { return m_sparse_alloc.second(); }
		[[nodiscard]] auto &page_alloc() const noexcept { return m_sparse_alloc.second(); }

		[[nodiscard]] T *alloc_page() { return std::allocator_traits<Alloc>::allocate(page_alloc(), page_size); }
		void dealloc_page(T *ptr) { std::allocator_traits<Alloc>::deallocate(page_alloc(), ptr, page_size); }

		packed_pair<sparse_t, Alloc> m_sparse_alloc; /* Sparse array of pointers to component pages & page allocator. */

		dense_t m_dense; /* Dense array of entities. */
		entity m_next;	 /* Next entity available for re-use. */
	};

	/** @brief Helper structure used to obtain a storage for component types. */
	template<typename T>
	struct storage_type
	{
		using type = basic_component_pool<T>;
	};
	template<typename T>
	using storage_type_t = typename storage_type<T>::type;

	/** @brief Proxy type used to add support for creation, modification & removal events for component storage. */
	template<typename T>
	class storage_event_proxy
	{
		using storage_t = storage_type_t<T>;

	public:
	private:
		storage_t m_storage;
	};
}	 // namespace sek::engine