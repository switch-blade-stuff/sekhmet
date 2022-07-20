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
				using storage_t = component_storage<T>;

				m_destroy = +[](void *ptr, entity_t e)
				{
					const auto storage = static_cast<storage_t *>(ptr);
					if (const auto target = storage->find(e); target != storage->end()) [[unlikely]]
						storage->erase(target);
				};
				m_delete = +[](void *ptr) { delete static_cast<storage_t *>(ptr); };
				m_ptr = static_cast<void *>(new storage_t(std::forward<Args>(args)...));
			}
			constexpr ~storage_entry() { m_delete(m_ptr); }

			constexpr void destroy(entity_t e) { m_destroy(m_ptr, e); }

			template<typename T>
			[[nodiscard]] constexpr auto *get() noexcept
			{
				return static_cast<component_storage<T> *>(m_ptr);
			}
			template<typename T>
			[[nodiscard]] constexpr auto *get() const noexcept
			{
				return static_cast<const component_storage<T> *>(m_ptr);
			}

			constexpr void swap(storage_entry &other) noexcept
			{
				std::swap(m_destroy, other.m_destroy);
				std::swap(m_delete, other.m_delete);
				std::swap(m_ptr, other.m_ptr);
			}
			friend constexpr void swap(storage_entry &a, storage_entry &b) noexcept { a.swap(b); }

		private:
			void (*m_destroy)(void *, entity_t) = +[](void *, entity_t) {};
			void (*m_delete)(void *) = +[](void *) {};
			void *m_ptr = nullptr;
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

		/** Returns component of the specified entity.
		 * @warning Using an entity that does not have the specified component will result in undefined behavior. */
		template<typename C>
		[[nodiscard]] constexpr C &get(entity_t e) noexcept
			requires(!std::is_empty_v<C>)
		{
			return get_storage<C>()->at(e);
		}
		/** @copydoc get */
		template<typename C>
		[[nodiscard]] constexpr const C &get(entity_t e) const noexcept
			requires(!std::is_empty_v<C>)
		{
			return get_storage<C>()->at(e);
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
		/** Destroys all components belonging to the entity & releases it. */
		constexpr void destroy(entity_t e)
		{
			for (auto entry : m_storage) entry.second.destroy(e);
			release(e);
		}

		/** Reserves storage for the specified components.
		 *
		 * @param n Amount of components to reserve. If set to `0`, only creates the storage pools.
		 * @return Tuple of references to component storage. */
		template<typename... Ts>
		std::tuple<component_storage<Ts> &...> reserve(size_type n = 0)
		{
			return std::forward_as_tuple(reserve_impl<Ts>(n)...);
		}

		/** Replaces a component for an entity.
		 *
		 * @param e Entity to emplace component for.
		 * @param args Arguments passed to component's constructor.
		 * @return Reference to the replaced component (or `void`, if component is empty).
		 *
		 * @warning Using an entity that does not have the specified component will result in undefined behavior. */
		template<typename C, typename... Args>
		decltype(auto) replace(entity_t e, Args &&...args)
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
		 * @warning Using an entity that already has the specified component will result in undefined behavior. */
		template<typename C, typename... Args>
		decltype(auto) emplace(entity_t e, Args &&...args)
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
		 * @warning Using an entity that already has the specified component will result in undefined behavior. */
		template<typename C, typename... Args>
		decltype(auto) emplace_back(entity_t e, Args &&...args)
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
		decltype(auto) emplace_or_replace(entity_t e, Args &&...args)
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
		decltype(auto) emplace_back_or_replace(entity_t e, Args &&...args)
			requires std::constructible_from<C, Args...>
		{
			return reserve_impl<C>().emplace_back_or_replace(e, std::forward<Args>(args)...);
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

		template<typename T>
		[[nodiscard]] constexpr auto *get_storage() noexcept
		{
			return m_storage.find(type_info::get<T>())->second.template get<T>();
		}
		template<typename T>
		[[nodiscard]] constexpr auto *get_storage() const noexcept
		{
			return m_storage.find(type_info::get<T>())->second->template get<T>();
		}

		template<typename T>
		component_storage<T> &reserve_impl(size_type n = 0)
		{
			const auto type = type_info::get<T>();
			auto target = m_storage.find(type);
			if (target == m_storage.end()) [[unlikely]]
				target = m_storage
							 .emplace(std::piecewise_construct,
									  std::forward_as_tuple(type),
									  std::forward_as_tuple(type_selector<T>, *this))
							 .first;

			auto &storage = *target->second.template get<T>();
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