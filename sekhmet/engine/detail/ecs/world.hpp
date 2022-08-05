/*
 * Created by switchblade on 14/07/22
 */

#pragma once

#include <memory>

#include "sekhmet/detail/dense_map.hpp"

#include "../type_info.hpp"
#include "query.hpp"

namespace sek::engine
{
	/** @brief A world is a special container used to associate entities with their components.
	 *
	 * Internally, a world contains a table of component pools (and dense index arrays) indexed by their type,
	 * and a sparse array of entities used to associate component indexes to their entities.
	 *
	 * Worlds also support component events, allowing the user to execute code when a components are created,
	 * removed or modified.
	 *
	 * @warning Asynchronous operations on entity worlds must be synchronized externally (ex. through an access guard). */
	class entity_world
	{
		class storage_entry
		{
		public:
			storage_entry(const storage_entry &) = delete;
			storage_entry &operator=(const storage_entry &) = delete;

			constexpr storage_entry() noexcept = default;
			constexpr storage_entry(storage_entry &&other) noexcept { swap(other); }
			constexpr storage_entry &operator=(storage_entry &&other) noexcept
			{
				swap(other);
				return *this;
			}

			template<typename T, typename... Args>
			constexpr explicit storage_entry(type_selector_t<T>, Args &&...args)
			{
				using storage_t = component_set<T>;

				m_ptr = static_cast<void *>(new storage_t(std::forward<Args>(args)...));
				m_delete = +[](void *ptr) { delete static_cast<storage_t *>(ptr); };

				m_contains = +[](void *ptr, entity_t e) { return static_cast<storage_t *>(ptr)->contains(e); };
				m_erase = +[](void *ptr, entity_t e) { static_cast<storage_t *>(ptr)->erase(e); };
			}
			constexpr ~storage_entry() { m_delete(m_ptr); }

			template<typename T>
			[[nodiscard]] constexpr auto *get() noexcept
			{
				return static_cast<component_set<T> *>(m_ptr);
			}
			template<typename T>
			[[nodiscard]] constexpr auto *get() const noexcept
			{
				return static_cast<const component_set<T> *>(m_ptr);
			}

			[[nodiscard]] constexpr bool contains(entity_t e) const noexcept { return m_contains(m_ptr, e); }

			constexpr void erase(entity_t e) { m_erase(m_ptr, e); }

			constexpr void swap(storage_entry &other) noexcept
			{
				std::swap(m_contains, other.m_contains);
				std::swap(m_erase, other.m_erase);
				std::swap(m_delete, other.m_delete);
				std::swap(m_ptr, other.m_ptr);
			}
			friend constexpr void swap(storage_entry &a, storage_entry &b) noexcept { a.swap(b); }

		private:
			void *m_ptr = nullptr;
			void (*m_delete)(void *) = +[](void *) {};
			bool (*m_contains)(void *, entity_t);
			void (*m_erase)(void *, entity_t);
		};
		struct table_hash
		{
			using is_transparent = std::true_type;

			constexpr hash_t operator()(std::string_view sv) const noexcept { return fnv1a(sv.data(), sv.size()); }
			constexpr hash_t operator()(const type_info &ti) const noexcept { return operator()(ti.name()); }
		};
		struct table_cmp
		{
			using is_transparent = std::true_type;

			constexpr bool operator()(std::string_view a, std::string_view b) const noexcept { return a == b; }
			constexpr bool operator()(const type_info &a, const type_info &b) const noexcept { return a == b; }
			constexpr bool operator()(const type_info &a, std::string_view b) const noexcept { return a.name() == b; }
			constexpr bool operator()(std::string_view a, const type_info &b) const noexcept { return a == b.name(); }
		};

		using storage_table = dense_map<type_info, storage_entry, table_hash, table_cmp>;

		class entity_iterator
		{
			friend class entity_world;

		public:
			typedef entity_t value_type;
			typedef const entity_t *pointer;
			typedef const entity_t &reference;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;
			typedef std::bidirectional_iterator_tag iterator_category;

		private:
			constexpr explicit entity_iterator(pointer ptr) noexcept : m_ptr(ptr) { skip_tombstones(1); }

		public:
			constexpr entity_iterator() noexcept = default;

			constexpr entity_iterator operator++(int) noexcept
			{
				auto temp = *this;
				++(*this);
				return temp;
			}
			constexpr entity_iterator &operator++() noexcept
			{
				++m_ptr;
				return skip_tombstones(1);
			}
			constexpr entity_iterator operator--(int) noexcept
			{
				auto temp = *this;
				--(*this);
				return temp;
			}
			constexpr entity_iterator &operator--() noexcept
			{
				--m_ptr;
				return skip_tombstones(-1);
			}

			/** Returns pointer to the target entity. */
			[[nodiscard]] constexpr pointer get() const noexcept { return m_ptr; }
			/** @copydoc value */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
			/** Returns reference to the target entity. */
			[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }

			[[nodiscard]] constexpr auto operator<=>(const entity_iterator &) const noexcept = default;
			[[nodiscard]] constexpr bool operator==(const entity_iterator &) const noexcept = default;

			constexpr void swap(entity_iterator &other) noexcept { std::swap(m_ptr, other.m_ptr); }
			friend constexpr void swap(entity_iterator &a, entity_iterator &b) noexcept { a.swap(b); }

		private:
			constexpr entity_iterator &skip_tombstones(difference_type offset) noexcept
			{
				while (m_ptr->is_tombstone()) m_ptr += offset;
				return *this;
			}

			pointer m_ptr = nullptr;
		};

	public:
		typedef entity_t value_type;
		typedef const entity_t *pointer;
		typedef const entity_t *const_pointer;
		typedef const entity_t &reference;
		typedef const entity_t &const_reference;
		typedef entity_iterator iterator;
		typedef entity_iterator const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

	public:
		constexpr entity_world() = default;
		constexpr ~entity_world() = default;

		/** Returns iterator to the first entity in the world. */
		[[nodiscard]] constexpr auto begin() const noexcept { return iterator{m_entities.data()}; }
		/** @copydoc begin */
		[[nodiscard]] constexpr auto cbegin() const noexcept { return const_iterator{m_entities.data()}; }
		/** Returns iterator one past the last entity in the world. */
		[[nodiscard]] constexpr auto end() const noexcept { return iterator{m_entities.data() + size()}; }
		/** @copydoc end */
		[[nodiscard]] constexpr auto cend() const noexcept { return const_iterator{m_entities.data() + size()}; }
		/** Returns reverse iterator to the last entity in the world. */
		[[nodiscard]] constexpr auto rbegin() const noexcept { return reverse_iterator{end()}; }
		/** @copydoc rbegin */
		[[nodiscard]] constexpr auto crbegin() const noexcept { return const_reverse_iterator{cend()}; }
		/** Returns reverse iterator one past the first entity in the world. */
		[[nodiscard]] constexpr auto rend() const noexcept { return reverse_iterator{begin()}; }
		/** @copydoc rend */
		[[nodiscard]] constexpr auto crend() const noexcept { return const_reverse_iterator{cbegin()}; }

		/** Returns the size of the world (amount of alive entities). */
		[[nodiscard]] constexpr size_type size() const noexcept { return m_size; }
		/** Checks if the world is empty (does not contain alive entities). */
		[[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }
		/** Returns the max size of the world (absolute maximum of alive entities). */
		[[nodiscard]] constexpr size_type max_size() const noexcept { return m_entities.max_size(); }
		/** Returns the capacity of the world (current maximum of alive entities) */
		[[nodiscard]] constexpr size_type capacity() const noexcept { return m_entities.capacity(); }

		/** Releases all entities and destroys all component sets of the world.
		 * @warning References to components and entities (including collections and views) will be invalidated. */
		constexpr void clear()
		{
			m_storage.clear();
			m_entities.clear();
			m_next = entity_t::tombstone();
			m_size = 0;
		}

		/** Returns iterator to the specified entity or end iterator if the entity does not exist in the world. */
		[[nodiscard]] constexpr iterator find(entity_t e) const noexcept
		{
			const auto idx = e.index().value();
			return idx < m_entities.size() && m_entities[idx] == e ? iterator{m_entities.data() + idx} : end();
		}
		/** Checks if the world contains the specified entity. */
		[[nodiscard]] constexpr bool contains(entity_t e) const noexcept
		{
			const auto idx = e.index().value();
			return idx < m_entities.size() && m_entities[idx] == e;
		}

		/** Checks if the world contains an entity with all of the specified components. */
		template<typename T, typename... Ts>
		[[nodiscard]] constexpr bool contains_all(entity_t e) const noexcept
		{
			if constexpr (sizeof...(Ts) == 0)
			{
				const auto storage = m_storage.find(type_info::get<T>());
				return storage != m_storage.end() && storage->second.template get<T>()->contains(e);
			}
			else
				return contains_all<T>(e) && (contains_all<Ts>(e) && ...);
		}
		/** Checks if the entity contains all of the specified components. */
		template<typename T, typename... Ts>
		[[nodiscard]] constexpr bool contains_all(const_iterator which) const noexcept
		{
			return contains_all<T, Ts...>(*which);
		}
		/** Checks if the world contains an entity with any of the specified components. */
		template<typename T, typename... Ts>
		[[nodiscard]] constexpr bool contains_any(entity_t e) const noexcept
		{
			return contains_all<T>(e) || (contains_all<Ts>(e) || ...);
		}
		/** Checks if the entity contains any of the specified components. */
		template<typename T, typename... Ts>
		[[nodiscard]] constexpr bool contains_any(const_iterator which) const noexcept
		{
			return contains_any<T, Ts...>(*which);
		}
		/** Checks if the world contains an entity with none of the specified components. */
		template<typename T, typename... Ts>
		[[nodiscard]] constexpr bool contains_none(entity_t e) const noexcept
		{
			if constexpr (sizeof...(Ts) == 0)
			{
				const auto storage = m_storage.find(type_info::get<T>());
				return storage == m_storage.end() || !storage->second.template get<T>()->contains(e);
			}
			else
				return contains_none<T>(e) && (contains_none<Ts>(e) && ...);
		}
		/** Checks if the entity contains none of the specified components. */
		template<typename T, typename... Ts>
		[[nodiscard]] constexpr bool contains_none(const_iterator which) const noexcept
		{
			return contains_none<T, Ts...>(*which);
		}

		/** Returns the total amount of components of the entity. */
		[[nodiscard]] constexpr size_type size(entity_t e) const noexcept
		{
			size_type result = 0;
			for (auto entry : m_storage) result += entry.second.contains(e);
			return result;
		}
		/** @copydoc size */
		[[nodiscard]] constexpr size_type size(const_iterator which) const noexcept { return size(*which); }
		/** Checks if the entity is empty (does not have any components). */
		[[nodiscard]] constexpr bool empty(entity_t e) const noexcept
		{
			for (auto entry : m_storage)
			{
				if (entry.second.contains(e)) [[unlikely]]
					return false;
			}
			return true;
		}
		/** @copydoc empty */
		[[nodiscard]] constexpr bool empty(const_iterator which) const noexcept { return empty(*which); }

		/** Returns pointer to the component set for the specified component.
		 * @note If such storage does not exist, creates it. */
		template<typename C>
		[[nodiscard]] constexpr auto *storage() noexcept
		{
			return std::addressof(reserve_impl<C>());
		}
		/** Returns pointer to the component set for the specified component or `nullptr`. */
		template<typename C>
		[[nodiscard]] constexpr const auto *storage() const noexcept
		{
			return get_storage<C>();
		}

		/** Returns component of the specified entity. */
		template<typename C>
		[[nodiscard]] constexpr C &get(const_iterator which) noexcept
		{
			return get<C>(*which);
		}
		/** @copydoc get */
		template<typename C>
		[[nodiscard]] constexpr const C &get(const_iterator which) const noexcept
		{
			return get<C>(*which);
		}
		/** @copydoc get
		 * @warning Using an entity that does not have the specified component will result in undefined behavior. */
		template<typename C>
		[[nodiscard]] constexpr C &get(entity_t e) noexcept
		{
			return get_storage<C>()->get(e);
		}
		/** @copydoc get */
		template<typename C>
		[[nodiscard]] constexpr const C &get(entity_t e) const noexcept
		{
			return get_storage<C>()->get(e);
		}

		//		/** Creates an entity query for this world. */
		//		[[nodiscard]] constexpr auto query() noexcept { return entity_query{*this}; }
		//		/** @copydoc query */
		//		[[nodiscard]] constexpr auto query() const noexcept { return entity_query{*this}; }
		//

		/** Returns a component view for the specified components.
		 * @tparam Inc Components included by the component view.
		 * @tparam Exc Components excluded by the component view.
		 * @tparam Opt Optional components of the component view. */
		template<typename... Inc, typename... Exc, typename... Opt>
		[[nodiscard]] constexpr auto view(excluded_t<Exc...> = excluded_t<>{}, optional_t<Opt...> = optional_t<>{}) noexcept
		{
			using V = component_view<included_t<Inc...>, excluded_t<Exc...>, optional_t<Opt...>>;
			return V{storage<Inc>()..., storage<Exc>()...};
		}
		/** @copydoc view */
		template<typename... Inc, typename... Exc, typename... Opt>
		[[nodiscard]] constexpr auto view(excluded_t<Exc...> = excluded_t<>{}, optional_t<Opt...> = optional_t<>{}) const noexcept
		{
			// clang-format off
			using V = component_view<included_t<std::add_const_t<Inc>...>, excluded_t<std::add_const_t<Exc>...>, optional_t<std::add_const_t<Opt>...>>;
			return V{storage<Inc>()..., storage<Exc>()...};
			// clang-format on
		}

		/* TODO: Implement component collections. */

		/* TODO: Implement component based sorting. */

		/** Checks if the specified component types are sortable.
		 * @return `false` if any of the components are fixed or ordered by a collection, `true` otherwise.
		 *
		 * @note If more than one component type is specified, verifies that the components are sortable in
		 * the specified order (as if sorted via a collection or `sort<Cs...>()`). */
		template<typename... Cs>
		[[nodiscard]] constexpr bool sortable() const noexcept;
		/** @brief Sorts components according to the specified order. Components will be grouped together in order
		 * to maximize cache performance.
		 *
		 * @example
		 * @code{.cpp}
		 * world.sort<cmp_a, cmp_b>();
		 * @endcode
		 * Sorts component sets to group entities with `cmp_a` and `cmp_b` together.
		 *
		 * @warning Components cannot be sorted if a conflicting collection exists for the specified components
		 * or any of the component types are fixed. Sorting such components will result in undefined behavior. */
		template<typename... Cs>
		constexpr void sort();

		// clang-format off
		/** @brief Sorts components of type `C` using `std::sort` using the passed predicate.
		 *
		 * Sorting predicate should have one of the following signatures:
		 * @code{.cpp}
		 * bool(entity_t, entity_t)
		 * bool(const C &, const C &)
		 * @endcode
		 *
		 * @param pred Predicate used for sorting.
		 * @warning Components cannot be sorted if a conflicting collection exists for component type `C`
		 * or the component type is fixed. Sorting such components will result in undefined behavior. */
		template<typename C, typename P>
		constexpr void sort(P &&pred) requires(std::is_invocable_r_v<bool, P, const C &, const C &> ||
											   std::is_invocable_r_v<bool, P, entity_t, entity_t>)
		{
			constexpr auto default_sort = []<typename Pred>(auto first, auto last, Pred &&pred)
			{
				std::sort(first, last, std::forward<Pred>(pred));
			};
			sort(default_sort, std::forward<P>(pred));
		}
		/** @brief Sorts components of type `C` using the passed sort functor.
		 *
		 * Sort functor must define an invoke operator (`operator()`) with the following signature:
		 * @code{.cpp}
		 * bool(Iter first, Iter last, Pred pred)
		 * @endcode
		 * Where `Iter` is a random-access iterator and `Pred` is an implementation-defined predicate.
		 *
		 * @copydetails sort
		 * @param sort Functor used for sorting.
		 * @note Sorting functor iterates over an implementation-defined range. */
		template<typename C, typename S, typename P>
		constexpr void sort(S &&sort, P &&pred) requires(std::is_invocable_r_v<bool, P, const C &, const C &> ||
														 std::is_invocable_r_v<bool, P, entity_t, entity_t>)
		{
			auto *storage = get_storage<C>();
			if (storage == nullptr) [[unlikely]]
				return;

			const auto sort_proxy = [storage, &pred](entity_t a, entity_t b) -> bool
			{
				if constexpr(std::invocable<P, const C &, const C &>)
					return pred(storage->get(a), storage->get(b));
				else
					return pred(a, b);
			};
			storage->sort(std::forward<S>(sort), sort_proxy);
		}
		// clang-format on

		/** Removes tombstones (if any) from component sets of the specified component types. */
		template<typename C, typename... Cs>
		constexpr void pack()
		{
			if constexpr (sizeof...(Cs) != 0) pack<Cs...>();
			if (auto *storage = get_storage<C>(); storage != nullptr) [[likely]]
				storage->pack();
		}

		/** Generates a new entity.
		 * @param gen Optional generation to use for the entity.
		 * @return Value of the generated entity. */
		[[nodiscard]] constexpr entity_t generate(entity_t::generation_type gen = entity_t::generation_type::tombstone())
		{
			return m_next.index().is_tombstone() ? generate_new(gen) : generate_existing(gen);
		}

		/** Releases an entity.
		 * @warning Releasing an entity that contains components will result in stale references. Use `destroy` instead. */
		constexpr void release(entity_t e)
		{
			const auto next_gen = entity_t::generation_type{e.generation().value() + 1};
			const auto idx = e.index();
			m_entities[idx.value()] = entity_t{next_gen, m_next.index()};
			m_next = entity_t{entity_t::generation_type::tombstone(), idx};
			--m_size;
		}
		/** @copydoc release */
		constexpr void release(const_iterator which) { release(*which); }
		/** Destroys all components belonging to the entity & releases it. */
		constexpr void destroy(entity_t e)
		{
			for (auto entry : m_storage)
			{
				if (entry.second.contains(e)) [[unlikely]]
					entry.second.erase(e);
			}
			release(e);
		}
		/** @copydoc destroy */
		constexpr void destroy(const_iterator which) { destroy(*which); }

		/** Reserves storage for the specified component.
		 * @param n Amount of components to reserve. If set to `0`, only creates the storage pool.
		 * @return Reference to component storagefor type `C`. */
		template<typename C>
		constexpr component_set<std::remove_cv_t<C>> &reserve(size_type n = 0)
		{
			return reserve_impl<C>(n);
		}
		/** Reserves storage for the specified components.
		 * @param n Amount of components to reserve. If set to `0`, only creates the storage pools.
		 * @return Tuple of references to component storage. */
		template<typename... Cs>
		constexpr std::tuple<component_set<std::remove_cv_t<Cs>> &...> reserve(size_type n = 0)
			requires(sizeof...(Cs) > 1)
		{
			return std::forward_as_tuple(reserve<Cs>(n)...);
		}

		/** Replaces a component for an entity.
		 *
		 * @param e Entity to emplace component for.
		 * @param args Arguments passed to component's constructor.
		 * @return Reference to the replaced component (or `void`, if component is empty).
		 *
		 * @warning Using an entity that does not exist or already has the specified component will result in undefined behavior. */
		template<typename C, typename... Args>
		constexpr decltype(auto) replace(entity_t e, Args &&...args)
			requires std::constructible_from<C, Args...>
		{
			return reserve_impl<C>().replace(e, std::forward<Args>(args)...);
		}
		/** Constructs a component for the specified entity in-place (re-using slots if component type requires fixed storage).
		 *
		 * @param e Entity to emplace component for.
		 * @param args Arguments passed to component's constructor.
		 * @return Reference to the emplaced component (or `void`, if component is empty).
		 *
		 * @warning Using an entity that does not exist or already has the specified component will result in undefined behavior. */
		template<typename C, typename... Args>
		constexpr decltype(auto) emplace(entity_t e, Args &&...args)
			requires std::constructible_from<C, Args...>
		{
			return reserve_impl<C>().emplace(e, std::forward<Args>(args)...);
		}
		/** Constructs a component for the specified entity in-place (always at the end).
		 *
		 * @param e Entity to emplace component for.
		 * @param args Arguments passed to component's constructor.
		 * @return Reference to the emplaced component (or `void`, if component is empty).
		 *
		 * @warning Using an entity that does not exist or already has the specified component will result in undefined behavior. */
		template<typename C, typename... Args>
		constexpr decltype(auto) emplace_back(entity_t e, Args &&...args)
			requires std::constructible_from<C, Args...>
		{
			return reserve_impl<C>().emplace(e, std::forward<Args>(args)...);
		}
		/** Emplaces or modifies a component for the specified entity (re-using slots if component type requires fixed storage).
		 *
		 * @param e Entity to emplace component for.
		 * @param args Arguments passed to component's constructor.
		 * @return Reference to the component (or `void`, if component is empty). */
		template<typename C, typename... Args>
		constexpr decltype(auto) emplace_or_replace(entity_t e, Args &&...args)
			requires std::constructible_from<C, Args...>
		{
			return reserve_impl<C>().emplace_or_replace(e, std::forward<Args>(args)...);
		}
		/** Emplaces or modifies a component for the specified entity (always at the end).
		 *
		 * @param e Entity to emplace component for.
		 * @param args Arguments passed to component's constructor.
		 * @return Reference to the component (or `void`, if component is empty). */
		template<typename C, typename... Args>
		constexpr decltype(auto) emplace_back_or_replace(entity_t e, Args &&...args)
			requires std::constructible_from<C, Args...>
		{
			return reserve_impl<C>().emplace_back_or_replace(e, std::forward<Args>(args)...);
		}

		/** Generates and inserts an entity with the specified components (re-using slots if component type requires fixed storage).
		 * @tparam Cs Component types of the entity.
		 * @return Iterator to the inserted entity. */
		template<typename... Cs>
		constexpr iterator insert()
		{
			return insert(Cs{}...);
		}
		/** @copydoc insert
		 * @param cs Values of inserted components. */
		template<typename... Cs>
		constexpr iterator insert(Cs &&...cs)
		{
			const auto entity = generate();
			(emplace<Cs>(entity, std::forward<Cs>(cs)), ...);
			return iterator{m_entities.data() + entity.index().value()};
		}
		/** Generates and inserts an entity with the specified components (always at the end).
		 * @tparam Cs Component types of the entity.
		 * @return Iterator to the inserted entity. */
		template<typename... Cs>
		constexpr iterator push_back()
		{
			return push_back(Cs{}...);
		}
		/** @copydoc push_back
		 * @param cs Values of inserted components. */
		template<typename... Cs>
		constexpr iterator push_back(Cs &&...cs)
		{
			const auto entity = generate();
			(emplace_back<Cs>(entity, std::forward<Cs>(cs)), ...);
			return iterator{m_entities.data() + entity.index().value()};
		}

		/** Removes a component from the specified entity.
		 * @warning Using an entity that does not have the specified component will result in undefined behavior. */
		template<typename C>
		constexpr void erase(entity_t e)
		{
			get_storage<C>()->erase(e);
		}
		/** @copydoc erase */
		template<typename C>
		constexpr void erase(const_iterator which)
		{
			erase<C>(*which);
		}
		/** @copydoc erase
		 * If the last component was erased, releases the entity.
		 * @return `true` if the entity was released, `false` otherwise. */
		template<typename C>
		constexpr bool erase_and_release(entity_t e)
		{
			erase<C>(e);
			const auto is_empty = empty(e);
			if (is_empty) [[unlikely]]
				release(e);
			return is_empty;
		}
		/** @copydoc erase_and_release */
		template<typename C>
		constexpr bool erase_and_release(const_iterator which)
		{
			return erase_and_release<C>(*which);
		}

	private:
		[[nodiscard]] constexpr entity_t generate_existing(entity_t::generation_type gen)
		{
			const auto idx = m_next.index();
			auto &target = m_entities[idx.value()];
			m_next = entity_t{entity_t::generation_type::tombstone(), target.index()};
			return target = entity_t{gen.is_tombstone() ? target.generation() : gen, idx};
		}
		[[nodiscard]] constexpr entity_t generate_new(entity_t::generation_type gen)
		{
			const auto idx = entity_t::index_type{m_entities.size()};
			return (++m_size, !gen.is_tombstone() ? m_entities.emplace_back(gen, idx) : m_entities.emplace_back(idx));
		}

		template<typename T, typename U = std::remove_cv_t<T>>
		[[nodiscard]] constexpr component_set<U> *get_storage() noexcept
		{
			const auto set = m_storage.find(type_info::get<U>());
			if (set != m_storage.end()) [[likely]]
				return set->second.template get<U>();
			else
				return nullptr;
		}
		template<typename T, typename U = std::remove_cv_t<T>>
		[[nodiscard]] constexpr const component_set<U> *get_storage() const noexcept
		{
			const auto set = m_storage.find(type_info::get<U>());
			if (set != m_storage.end()) [[likely]]
				return set->second.template get<U>();
			else
				return nullptr;
		}

		template<typename T, typename U = std::remove_cv_t<T>>
		constexpr component_set<U> &reserve_impl(size_type n = 0)
		{
			const auto type = type_info::get<U>();
			auto target = m_storage.find(type);
			if (target == m_storage.end()) [[unlikely]]
				target = m_storage
							 .emplace(std::piecewise_construct,
									  std::forward_as_tuple(type),
									  std::forward_as_tuple(type_selector<U>, *this))
							 .first;

			auto &storage = *target->second.template get<U>();
			if (n != 0) [[likely]]
				storage.reserve(n);
			return storage;
		}

		storage_table m_storage;
		std::vector<entity_t> m_entities;
		entity_t m_next = entity_t::tombstone();
		size_type m_size = 0; /* Amount of alive entities within the world. */
	};
}	 // namespace sek::engine