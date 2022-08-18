/*
 * Created by switchblade on 14/07/22
 */

#pragma once

#include <memory>

#include "sekhmet/dense_map.hpp"

#include "../type_info.hpp"
#include "component_set.hpp"

namespace sek::engine
{
	template<typename... Cs>
	struct owned_t
	{
		using type = type_seq_t<Cs...>;

		constexpr owned_t() noexcept = default;
		constexpr owned_t(type_seq_t<Cs...>) noexcept {}
	};
	template<typename... Cs>
	struct included_t
	{
		using type = type_seq_t<Cs...>;

		constexpr included_t() noexcept = default;
		constexpr included_t(type_seq_t<Cs...>) noexcept {}
	};
	template<typename... Cs>
	struct optional_t
	{
		using type = type_seq_t<Cs...>;

		constexpr optional_t() noexcept = default;
		constexpr optional_t(type_seq_t<Cs...>) noexcept {}
	};
	template<typename... Cs>
	struct excluded_t
	{
		using type = type_seq_t<Cs...>;

		constexpr excluded_t() noexcept = default;
		constexpr excluded_t(type_seq_t<Cs...>) noexcept {}
	};

	namespace detail
	{
		template<typename...>
		struct collection_handler;

		class collection_sorter
		{
		public:
			collection_sorter(const collection_sorter &) = delete;
			collection_sorter &operator=(const collection_sorter &) = delete;

			constexpr collection_sorter(collection_sorter &&other) noexcept { swap(other); }
			constexpr collection_sorter &operator=(collection_sorter &&other) noexcept
			{
				swap(other);
				return *this;
			}

			constexpr ~collection_sorter() { m_delete(m_data); }

			template<typename... C, typename... I, typename... E>
			constexpr explicit collection_sorter(collection_handler<owned_t<C...>, included_t<I...>, excluded_t<E...>> *h)
			{
				type_count = sizeof...(C) + sizeof...(I) + sizeof...(E);

				if constexpr (sizeof...(C) != 0)
					is_collected = +[](type_info info) -> bool { return ((type_info::get<C>() == info) || ...); };
				if constexpr (sizeof...(I) != 0)
					is_included = +[](type_info info) -> bool { return ((type_info::get<I>() == info) || ...); };
				if constexpr (sizeof...(E) != 0)
					is_excluded = +[](type_info info) -> bool { return ((type_info::get<E>() == info) || ...); };

				m_delete = +[](void *ptr) { delete static_cast<decltype(h)>(ptr); };
				m_data = h;
			}

			[[nodiscard]] constexpr void *get() const noexcept { return m_data; }

			constexpr void swap(collection_sorter &other) noexcept
			{
				std::swap(type_count, other.type_count);
				std::swap(is_collected, other.is_collected);
				std::swap(is_included, other.is_included);
				std::swap(is_excluded, other.is_excluded);
				std::swap(m_delete, other.m_delete);
				std::swap(m_data, other.m_data);
			}
			friend constexpr void swap(collection_sorter &a, collection_sorter &b) noexcept { a.swap(b); }

			std::size_t type_count; /* Total amount of collected, included & excluded types. */

			bool (*is_collected)(type_info info) = +[](type_info) -> bool { return false; };
			bool (*is_included)(type_info info) = +[](type_info) -> bool { return false; };
			bool (*is_excluded)(type_info info) = +[](type_info) -> bool { return false; };

		private:
			void (*m_delete)(void *) = +[](void *) {};
			void *m_data = nullptr;
		};
	}	 // namespace detail

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
		template<typename...>
		friend struct detail::collection_handler;

#if !defined(__cpp_lib_constexpr_memory) || __cpp_lib_constexpr_memory < 202202L
		struct storage_ptr
		{
			storage_ptr(const storage_ptr &) = delete;
			storage_ptr &operator=(const storage_ptr &) = delete;

			constexpr storage_ptr() noexcept = default;
			constexpr storage_ptr(storage_ptr &&other) noexcept { swap(other); }
			constexpr storage_ptr &operator=(storage_ptr &&other) noexcept
			{
				swap(other);
				return *this;
			}

			constexpr storage_ptr(generic_component_set *ptr) noexcept : m_ptr(ptr) {}
			constexpr ~storage_ptr() { delete m_ptr; }

			[[nodiscard]] constexpr auto *get() const noexcept { return m_ptr; }
			[[nodiscard]] constexpr auto *operator->() const noexcept { return get(); }
			[[nodiscard]] constexpr auto &operator*() const noexcept { return *get(); }

			constexpr void swap(storage_ptr &other) noexcept { std::swap(m_ptr, other.m_ptr); }
			friend constexpr void swap(storage_ptr &a, storage_ptr &b) noexcept { a.swap(b); }

		private:
			generic_component_set *m_ptr = nullptr;
		};
#else
		using storage_ptr = std::unique_ptr<generic_component_set>;
#endif

		struct storage_hash
		{
			using is_transparent = std::true_type;

			constexpr hash_t operator()(const storage_ptr &ptr) const noexcept { return operator()(ptr->type()); }
			constexpr hash_t operator()(const type_info &ti) const noexcept { return operator()(ti.name()); }
			constexpr hash_t operator()(std::string_view sv) const noexcept { return fnv1a(sv.data(), sv.size()); }
		};
		struct storage_cmp
		{
			using is_transparent = std::true_type;

			constexpr bool operator()(const storage_ptr &a, const storage_ptr &b) const noexcept
			{
				return a->type() == b->type();
			}

			constexpr bool operator()(const storage_ptr &a, const type_info &b) const noexcept
			{
				return a->type() == b;
			}
			constexpr bool operator()(const type_info &a, const storage_ptr &b) const noexcept
			{
				return a == b->type();
			}

			constexpr bool operator()(const storage_ptr &a, std::string_view b) const noexcept
			{
				return a->type().name() == b;
			}
			constexpr bool operator()(std::string_view a, const storage_ptr &b) const noexcept
			{
				return a == b->type().name();
			}

			constexpr bool operator()(const type_info &a, const type_info &b) const noexcept { return a == b; }
			constexpr bool operator()(std::string_view a, std::string_view b) const noexcept { return a == b; }
			constexpr bool operator()(const type_info &a, std::string_view b) const noexcept { return a.name() == b; }
			constexpr bool operator()(std::string_view a, const type_info &b) const noexcept { return a == b.name(); }
		};

		using storage_set = dense_set<storage_ptr, storage_hash, storage_cmp>;
		using sorter_t = detail::collection_sorter;

		template<bool IsConst>
		class storage_view
		{
			friend class entity_world;

			using storage_iter = typename storage_set::iterator;

			class view_iterator
			{
				friend class storage_view;

			public:
				typedef generic_component_set value_type;
				typedef std::conditional_t<IsConst, const value_type, value_type> *pointer;
				typedef std::conditional_t<IsConst, const value_type, value_type> &reference;
				typedef typename storage_iter::size_type size_type;
				typedef typename storage_iter::difference_type difference_type;
				typedef typename storage_iter::iterator_category iterator_category;

			private:
				constexpr explicit view_iterator(storage_iter iter) noexcept : m_iter(iter) {}

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
					++m_iter;
					return *this;
				}
				constexpr view_iterator &operator+=(difference_type n) noexcept
				{
					m_iter += n;
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
					--m_iter;
					return *this;
				}
				constexpr view_iterator &operator-=(difference_type n) noexcept
				{
					m_iter -= n;
					return *this;
				}

				[[nodiscard]] constexpr view_iterator operator+(difference_type n) const noexcept
				{
					return view_iterator{m_iter + n};
				}
				[[nodiscard]] constexpr view_iterator operator-(difference_type n) const noexcept
				{
					return view_iterator{m_iter - n};
				}
				[[nodiscard]] constexpr difference_type operator-(const view_iterator &other) const noexcept
				{
					return m_iter - other.m_iter;
				}

				/** Returns pointer to the target element. */
				[[nodiscard]] constexpr pointer get() const noexcept { return m_iter->get(); }
				/** @copydoc value */
				[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
				/** Returns reference to the target element. */
				[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }
				/** Returns reference to the element at an offset. */
				[[nodiscard]] constexpr reference operator[](difference_type n) const noexcept { return *m_iter[n]; }

				[[nodiscard]] constexpr auto operator<=>(const view_iterator &) const noexcept = default;
				[[nodiscard]] constexpr bool operator==(const view_iterator &) const noexcept = default;

				constexpr void swap(view_iterator &other) noexcept { m_iter.swap(other.m_iter); }
				friend constexpr void swap(view_iterator &a, view_iterator &b) noexcept { a.swap(b); }

			private:
				storage_iter m_iter;
			};

		public:
			typedef generic_component_set value_type;
			typedef view_iterator iterator;
			typedef view_iterator const_iterator;
			typedef std::reverse_iterator<iterator> reverse_iterator;
			typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

			typedef typename iterator::pointer pointer;
			typedef typename iterator::pointer const_pointer;
			typedef typename iterator::reference reference;
			typedef typename iterator::reference const_reference;
			typedef typename iterator::size_type size_type;
			typedef typename iterator::difference_type difference_type;

		private:
			constexpr storage_view(storage_iter first, storage_iter last) noexcept : m_begin(first), m_end(last) {}

		public:
			storage_view() = delete;

			constexpr storage_view(const storage_view &) noexcept = default;
			constexpr storage_view &operator=(const storage_view &) noexcept = default;
			constexpr storage_view(storage_view &&) noexcept = default;
			constexpr storage_view &operator=(storage_view &&) noexcept = default;

			/** Returns iterator to the first element of the view. */
			[[nodiscard]] constexpr iterator begin() const noexcept { return iterator{m_begin}; }
			/** @copydoc begin */
			[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
			/** Returns iterator one past the last element of the view. */
			[[nodiscard]] constexpr iterator end() const noexcept { return iterator{m_end}; }
			/** @copydoc end */
			[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }
			/** Returns reverse iterator to the last element of the view. */
			[[nodiscard]] constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator{end()}; }
			/** @copydoc rbegin */
			[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
			/** Returns iterator one past the last element of the view. */
			[[nodiscard]] constexpr reverse_iterator rend() const noexcept { return reverse_iterator{begin()}; }
			/** @copydoc rend */
			[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return rend(); }

			/** Returns reference to the first element of the view. */
			[[nodiscard]] constexpr reference front() const noexcept { return *begin(); }
			/** Returns reference to the last element of the view. */
			[[nodiscard]] constexpr reference back() const noexcept { return *std::prev(end()); }
			/** Returns reference to the `n`th element of the view. */
			[[nodiscard]] constexpr reference at(size_type i) const noexcept
			{
				return begin()[static_cast<difference_type>(i)];
			}
			/** @copydoc at */
			[[nodiscard]] constexpr reference operator[](size_type i) const noexcept { return at(i); }

			/** Returns the total amount of elements in the view. */
			[[nodiscard]] constexpr size_type size() const noexcept { return static_cast<size_type>(m_end - m_begin); }
			/** Checks if the view is empty. */
			[[nodiscard]] constexpr bool empty() const noexcept { return m_end == m_begin; }

			constexpr void swap(storage_view &other) noexcept
			{
				m_begin.swap(other.m_begin);
				m_end.swap(other.m_end);
			}
			friend constexpr void swap(storage_view &a, storage_view &b) noexcept { a.swap(b); }

		private:
			storage_iter m_begin;
			storage_iter m_end;
		};

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
		typedef event<void(entity_world &, entity_t, type_info)> generic_event_type;
		typedef event<void(entity_world &, entity_t)> event_type;

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

	private:
		template<typename T>
		static void create_listener(entity_world &world, entity_t entity)
		{
			world.m_create(world, entity, type_info::get<T>());
		}
		template<typename T>
		static void modify_listener(entity_world &world, entity_t entity)
		{
			world.m_modify(world, entity, type_info::get<T>());
		}
		template<typename T>
		static void remove_listener(entity_world &world, entity_t entity)
		{
			world.m_remove(world, entity, type_info::get<T>());
		}

	public:
		entity_world(const entity_world &) = delete;
		entity_world &operator=(const entity_world &) = delete;

		constexpr entity_world() = default;
		constexpr ~entity_world() { clear_storage(); }

		constexpr entity_world(entity_world &&other) noexcept
			: m_storage(std::move(other.m_storage)),
			  m_create(std::move(other.m_create)),
			  m_modify(std::move(other.m_modify)),
			  m_remove(std::move(other.m_remove)),
			  m_sorters(std::move(other.m_sorters)),
			  m_entities(std::move(other.m_entities)),
			  m_next(std::exchange(other.m_next, {})),
			  m_size(std::exchange(other.m_size, {}))
		{
			rebind_storage();
		}
		constexpr entity_world &operator=(entity_world &&other) noexcept
		{
			m_storage = std::move(other.m_storage);
			m_create = std::move(other.m_create);
			m_modify = std::move(other.m_modify);
			m_remove = std::move(other.m_remove);
			m_sorters = std::move(other.m_sorters);
			m_entities = std::move(other.m_entities);
			m_next = std::exchange(other.m_next, {});
			m_size = std::exchange(other.m_size, {});
			rebind_storage();
			return *this;
		}

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

		/** Releases all entities, destroys all components.
		 * @note Does not clear component events. */
		constexpr void clear()
		{
			clear_storage();
			m_entities.clear();
			m_next = entity_t::tombstone();
			m_size = 0;
		}
		/** Destroys all components of specified types.
		 * @note Does not clear component events. */
		template<typename... Cs>
		constexpr void clear()
		{
			constexpr auto clear_set = [](auto *set)
			{
				if (set != nullptr) set->clear();
			};
			(clear_set(get_storage<Cs>()), ...);
		}
		/** @copydoc clear */
		constexpr void clear(std::string_view type)
		{
			if (const auto set = m_storage.find(type); set != m_storage.end()) [[likely]]
				set->get()->clear();
		}
		/** @copydoc clear */
		constexpr void clear(type_info type)
		{
			if (const auto set = m_storage.find(type); set != m_storage.end()) [[likely]]
				set->get()->clear();
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
				return storage != m_storage.end() && storage->get()->contains(e);
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
				return storage == m_storage.end() || !storage->get()->contains(e);
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
			for (auto &set : m_storage) result += set->contains(e);
			return result;
		}
		/** @copydoc size */
		[[nodiscard]] constexpr size_type size(const_iterator which) const noexcept { return size(*which); }
		/** Checks if the entity is empty (does not have any components). */
		[[nodiscard]] constexpr bool empty(entity_t e) const noexcept
		{
			return std::ranges::none_of(m_storage, [e](auto &set) { return set->contains(e); });
		}
		/** @copydoc empty */
		[[nodiscard]] constexpr bool empty(const_iterator which) const noexcept { return empty(*which); }

		/** Returns a view of type-erased generic component sets of the world. */
		[[nodiscard]] constexpr storage_view<false> storage() noexcept { return {m_storage.begin(), m_storage.end()}; }
		/** @copydoc storage */
		[[nodiscard]] constexpr storage_view<true> storage() const noexcept
		{
			return {m_storage.begin(), m_storage.end()};
		}

		/** Returns a pointer to the type-erased generic component set for the specified type or `nullptr`. */
		[[nodiscard]] constexpr generic_component_set *storage(std::string_view name) noexcept
		{
			if (const auto set = m_storage.find(name); set != m_storage.end()) [[likely]]
				return set->get();
			return nullptr;
		}
		/** @copydoc storage */
		[[nodiscard]] constexpr generic_component_set *storage(const type_info &type) noexcept
		{
			if (const auto set = m_storage.find(type); set != m_storage.end()) [[likely]]
				return set->get();
			return nullptr;
		}
		/** @copydoc storage */
		[[nodiscard]] constexpr const generic_component_set *storage(std::string_view name) const noexcept
		{
			if (const auto set = m_storage.find(name); set != m_storage.end()) [[likely]]
				return set->get();
			return nullptr;
		}
		/** @copydoc storage */
		[[nodiscard]] constexpr const generic_component_set *storage(const type_info &type) const noexcept
		{
			if (const auto set = m_storage.find(type); set != m_storage.end()) [[likely]]
				return set->get();
			return nullptr;
		}

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

		/** Creates an entity query for this world. */
		[[nodiscard]] constexpr auto query() noexcept;
		/** @copydoc query */
		[[nodiscard]] constexpr auto query() const noexcept;

		/** Returns a component view for the specified components.
		 * @tparam I Components included by the component view.
		 * @tparam E Components excluded by the component view.
		 * @tparam O Optional components of the component view. */
		template<typename... I, typename... E, typename... O>
		[[nodiscard]] constexpr auto view(excluded_t<E...> = excluded_t<>{}, optional_t<O...> = optional_t<>{}) noexcept;
		/** @copydoc view */
		template<typename... I, typename... E, typename... O>
		[[nodiscard]] constexpr auto view(excluded_t<E...> = excluded_t<>{}, optional_t<O...> = optional_t<>{}) const noexcept;

		/** Returns a component collection for the specified components.
		 * @tparam C Components owned (sorted) by the component collection.
		 * @tparam I Components included by the component view.
		 * @tparam E Components excluded by the component view.
		 * @tparam O Optional components of the component view.
		 * @note Collected component types are implicitly included. */
		template<typename... C, typename... I, typename... E, typename... O>
		[[nodiscard]] constexpr auto collection(included_t<I...> = included_t<>{},
												excluded_t<E...> = excluded_t<>{},
												optional_t<O...> = optional_t<>{}) noexcept;

		/** Checks if the specified component types are collected (owned) by a collection.
		 * @return `true` if any of the components are collected (owned) by a collection, `false` otherwise. */
		template<typename... Cs>
		[[nodiscard]] constexpr bool is_collected() const noexcept
		{
			constexpr auto p = [](const sorter_t &sorter) { return (sorter.is_collected(type_info::get<Cs>()) || ...); };
			return std::any_of(m_sorters.begin(), m_sorters.end(), p);
		}
		/** Sorts components according to the specified order. Components will be grouped together in order
		 * to maximize cache performance.
		 *
		 * @tparam Parent Component type who's entity order to use for sorting.
		 * @tparam C First type of the sorted components.
		 * @tparam Cs Other types of sorted components.
		 *
		 * @example
		 * @code{.cpp}
		 * world.sort<cmp_a, cmp_b>();
		 * @endcode
		 * Sorts component sets to group entities with `cmp_a` and `cmp_b` together.
		 *
		 * @note Sorting in-place components will invalidate references to said components.
		 * @warning Components cannot be sorted if a conflicting collection exists for the specified components
		 * (i.e. they are owned by a collection). Sorting such components will result in undefined behavior. */
		template<typename Parent, typename C, typename... Cs>
		constexpr void sort()
		{
			SEK_ASSERT(!is_collected<C>(), "Cannot sort components owned by collections");

			auto &src = reserve<Parent>();
			auto &dst = reserve<C>();

			src.pack();
			dst.sort(src.begin(), src.end());

			if constexpr (sizeof...(Cs) != 0) sort<C, Cs...>();
		}

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
		 *
		 * @note Sorting in-place components will invalidate references to said components.
		 * @warning Components cannot be sorted if a conflicting collection exists  for component type `C`
		 * (i.e. they are owned by a collection). Sorting such components will result in undefined behavior. */
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
		 *
		 * @note Sorting functor iterates over an implementation-defined range. */
		template<typename C, typename S, typename P>
		constexpr void sort(S &&sort, P &&pred) requires(std::is_invocable_r_v<bool, P, const C &, const C &> ||
														 std::is_invocable_r_v<bool, P, entity_t, entity_t>)
		{
			SEK_ASSERT(!is_collected<C>(), "Cannot sort components owned by collections");

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

		/** Removes tombstones (if any) from component sets of the specified component types.
		 * @note Packing in-place components will invalidate references to said components. */
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
		/** Destroys all components belonging to the entity and releases it. */
		constexpr void destroy(entity_t e)
		{
			for (auto &set : m_storage)
			{
				if (const auto pos = set->find(e); pos != set->end()) [[unlikely]]
					set->erase(pos);
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

		/** Applies a functor to component of an entity.
		 *
		 * @param entity Target entity.
		 * @param f Functor to apply to the entity's component.
		 * @return Pair of references to the entity and the replaced component.
		 *
		 * @warning Using an entity that does not exist or already has the specified component will result in undefined behavior. */
		template<typename C, typename F>
		constexpr decltype(auto) apply(entity_t entity, F &&f)
			requires std::invocable<F, entity_t, C &>
		{
			return *reserve_impl<C>().apply(entity, std::forward<F>(f));
		}

		/** Replaces a component for an entity.
		 *
		 * @param entity Entity to emplace component for.
		 * @param args Arguments passed to component's constructor.
		 * @return Pair of references to the entity and the replaced component.
		 *
		 * @warning Using an entity that does not exist or already has the specified component will result in undefined behavior. */
		template<typename C, typename... Args>
		constexpr decltype(auto) replace(entity_t entity, Args &&...args)
			requires std::constructible_from<C, Args...>
		{
			return *reserve_impl<C>().replace(entity, std::forward<Args>(args)...);
		}

		/** @brief Generates a new entity and constructs a component in-place. Tombstones (if any) are re-used.
		 * @param args Arguments passed to component's constructor.
		 * @return Pair of references to the inserted entity and it's replaced component. */
		template<typename C, typename... Args>
		constexpr decltype(auto) emplace(Args &&...args)
		{
			return emplace(generate(), std::forward<Args>(args)...);
		}
		/** @brief Generates a new entity and constructs a component in-place.
		 * Entities and components are always pushed to the end (tombstones are not re-used).
		 *
		 * @copydetails emplace */
		template<typename C, typename... Args>
		constexpr decltype(auto) emplace_back(Args &&...args)
		{
			return emplace_back(generate(), std::forward<Args>(args)...);
		}
		/** @brief Constructs a component for the specified entity in-place. Tombstones (if any) are re-used.
		 *
		 * @copydetails emplace
		 *
		 * @param entity Entity to emplace component for.
		 * @warning Using an entity that does not exist or already has the specified component will result in undefined behavior. */
		template<typename C, typename... Args>
		constexpr decltype(auto) emplace(entity_t entity, Args &&...args)
		{
			return reserve_impl<C>().emplace(entity, std::forward<Args>(args)...);
		}
		/** @brief Constructs a component for the specified entity in-place.
		 * Entities and components are always pushed to the end (tombstones are not re-used).
		 *
		 * @copydetails emplace_back
		 *
		 * @param entity Entity to emplace component for.
		 * @warning Using an entity that does not exist or already has the specified component will result in undefined behavior. */
		template<typename C, typename... Args>
		constexpr decltype(auto) emplace_back(entity_t entity, Args &&...args)
		{
			return reserve_impl<C>().emplace_back(entity, std::forward<Args>(args)...);
		}

		/** @brief Creates or modifies a component for the specified entity. Tombstones (if any) are re-used.
		 *
		 * @param entity Entity to emplace component for.
		 * @param args Arguments passed to component's constructor.
		 * @return Pair where first is a pair of references to the potentially inserted entity and it's component
		 * and second is a boolean indicating whether the entity was inserted (`true` if inserted, `false` if replaced). */
		template<typename C, typename... Args>
		constexpr decltype(auto) emplace_or_replace(entity_t entity, Args &&...args)
		{
			return reserve_impl<C>().emplace_or_replace(entity, std::forward<Args>(args)...);
		}
		/** @brief Creates or modifies a component for the specified entity.
		 * Entities and components are always pushed to the end (tombstones are not re-used).
		 *
		 * @copydetails emplace_or_replace */
		template<typename C, typename... Args>
		constexpr decltype(auto) emplace_back_or_replace(entity_t entity, Args &&...args)
		{
			return reserve_impl<C>().emplace_back_or_replace(entity, std::forward<Args>(args)...);
		}

		/** @brief Generates and inserts an entity with the specified components. Tombstones (if any) are re-used.
		 * @tparam Cs Component types of the entity.
		 * @return Iterator to the inserted entity. */
		template<typename... Cs>
		constexpr iterator insert()
		{
			const auto entity = generate();
			(emplace<Cs>(entity), ...);
			return to_iterator(entity);
		}
		/** @copydoc insert
		 * @param cs Values of inserted components. */
		template<typename... Cs>
		constexpr iterator insert(Cs &&...cs)
		{
			const auto entity = generate();
			(emplace<Cs>(entity, std::forward<Cs>(cs)), ...);
			return to_iterator(entity);
		}
		/** @brief Generates and inserts an entity with the specified components.
		 * Entities and components are always pushed to the end (tombstones are not re-used).
		 *
		 * @copydetails insert() */
		template<typename... Cs>
		constexpr iterator push_back()
		{
			const auto entity = generate();
			(emplace_back<Cs>(entity), ...);
			return to_iterator(entity);
		}
		/** @copydoc push_back
		 * @param cs Values of inserted components. */
		template<typename... Cs>
		constexpr iterator push_back(Cs &&...cs)
		{
			const auto entity = generate();
			(emplace_back<Cs>(entity, std::forward<Cs>(cs)), ...);
			return to_iterator(entity);
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

		/** Returns event proxy for the component creation event.
		 * This event is invoked when new components of type `C` are created and added to entities. */
		template<typename C>
		[[nodiscard]] constexpr event_proxy<event_type> on_create() noexcept
		{
			return storage<C>()->on_create();
		}
		/** Returns event proxy for the component modification event.
		 * This event is invoked when components of type `C` are modified via `replace` or `apply`. */
		template<typename C>
		[[nodiscard]] constexpr event_proxy<event_type> on_modify() noexcept
		{
			return storage<C>()->on_modify();
		}
		/** Returns event proxy for the component removal event.
		 * This event is invoked when components of type `C` are removed from entities and destroyed. */
		template<typename C>
		[[nodiscard]] constexpr event_proxy<event_type> on_remove() noexcept
		{
			return storage<C>()->on_remove();
		}

		/** Returns event proxy for the generic component creation event.
		 * This event is invoked when new components of any type are created and added to entities. */
		[[nodiscard]] constexpr event_proxy<generic_event_type> on_create() noexcept { return {m_create}; }
		/** Returns event proxy for the generic component modification event.
		 * This event is invoked when components of any type are modified via type-specific functions. */
		[[nodiscard]] constexpr event_proxy<generic_event_type> on_modify() noexcept { return {m_modify}; }
		/** Returns event proxy for the generic component removal event.
		 * This event is invoked when components of any type are removed from entities and destroyed. */
		[[nodiscard]] constexpr event_proxy<generic_event_type> on_remove() noexcept { return {m_remove}; }

		constexpr void swap(entity_world &other) noexcept
		{
			using std::swap;
			swap(m_storage, other.m_storage);
			swap(m_create, other.m_create);
			swap(m_modify, other.m_modify);
			swap(m_remove, other.m_remove);
			swap(m_sorters, other.m_sorters);
			swap(m_entities, other.m_entities);
			swap(m_next, other.m_next);
			swap(m_size, other.m_size);

			/* Rebind storage for both worlds. */
			other.rebind_storage();
			rebind_storage();
		}

	private:
		[[nodiscard]] constexpr iterator to_iterator(entity_t e) const noexcept
		{
			return iterator{m_entities.data() + e.index().value()};
		}

		constexpr void rebind_storage()
		{
			/* Component sets store references to this world and have to be notified on move & swap. */
			for (auto &set : m_storage) set->rebind(*this);
		}
		constexpr void clear_storage()
		{
			/* Cannot clear all at once, since collection handlers require valid references. */
			for (auto &set : m_storage) set->clear();
		}

		[[nodiscard]] constexpr entity_t generate_new(entity_t::generation_type gen)
		{
			const auto idx = entity_t::index_type{m_entities.size()};
			return (++m_size, !gen.is_tombstone() ? m_entities.emplace_back(gen, idx) : m_entities.emplace_back(idx));
		}
		[[nodiscard]] constexpr entity_t generate_existing(entity_t::generation_type gen)
		{
			const auto idx = m_next.index();
			auto &target = m_entities[idx.value()];
			m_next = entity_t{entity_t::generation_type::tombstone(), target.index()};
			return target = entity_t{gen.is_tombstone() ? target.generation() : gen, idx};
		}

		template<typename T, typename U = std::remove_cv_t<T>>
		[[nodiscard]] constexpr component_set<T> *get_storage() noexcept
		{
			const auto set = m_storage.find(type_info::get<U>());
			if (set != m_storage.end()) [[likely]]
				return static_cast<component_set<U> *>(set->get());
			else
				return nullptr;
		}
		template<typename T, typename U = std::remove_cv_t<T>>
		[[nodiscard]] constexpr const component_set<T> *get_storage() const noexcept
		{
			const auto set = m_storage.find(type_info::get<U>());
			if (set != m_storage.end()) [[likely]]
				return static_cast<const component_set<U> *>(set->get());
			else
				return nullptr;
		}

		template<typename T, typename U = std::remove_cv_t<T>>
		constexpr component_set<U> &reserve_impl(size_type n = 0)
		{
			component_set<U> *storage;
			if (auto target = m_storage.find(type_info::get<U>()); target != m_storage.end()) [[likely]]
				storage = static_cast<component_set<U> *>(target->get());
			else
			{
				m_storage.emplace(storage = new component_set<U>(*this));

				/* Subscribe to type-specific component events to handle generic event dispatching. */
				storage->on_create() += delegate_func<&entity_world::create_listener<U>>;
				storage->on_modify() += delegate_func<&entity_world::modify_listener<U>>;
				storage->on_remove() += delegate_func<&entity_world::remove_listener<U>>;
			}

			if (n != 0) [[likely]]
				storage->reserve(n);
			return *storage;
		}

		template<typename... Coll, typename... Inc, typename... Exc>
		[[nodiscard]] constexpr auto find_sorter(owned_t<Coll...>, included_t<Inc...>, excluded_t<Exc...>) const noexcept
		{
			constexpr auto pred = [](const sorter_t &sorter) -> bool
			{
				return sorter.type_count == sizeof...(Coll) + sizeof...(Inc) + sizeof...(Exc) &&
					   (sorter.is_collected(type_info::get<Coll>()) && ...) &&
					   (sorter.is_included(type_info::get<Inc>()) && ...) &&
					   (sorter.is_excluded(type_info::get<Exc>()) && ...);
			};
			return std::pair{std::find_if(m_sorters.begin(), m_sorters.end(), pred), m_sorters.end()};
		}
		template<typename... Coll, typename... Inc, typename... Exc>
		[[nodiscard]] constexpr auto next_sorter(owned_t<Coll...>, included_t<Inc...>, excluded_t<Exc...>) const noexcept
		{
			constexpr auto pred = [](const sorter_t &s) -> bool
			{
				return s.type_count > sizeof...(Coll) + sizeof...(Inc) + sizeof...(Exc) &&
					   (s.is_collected(type_info::get<Coll>()) || ...);
			};
			return std::pair{std::find_if(m_sorters.begin(), m_sorters.end(), pred), m_sorters.end()};
		}
		template<typename... Coll, typename... Inc, typename... Exc>
		[[nodiscard]] constexpr auto prev_sorter(owned_t<Coll...>, included_t<Inc...>, excluded_t<Exc...>) const noexcept
		{
			constexpr auto pred = [](const sorter_t &s) -> bool
			{ return (s.is_collected(type_info::get<Coll>()) || ...); };
			return std::pair{std::find_if(m_sorters.begin(), m_sorters.end(), pred), m_sorters.end()};
		}
		template<typename... Coll, typename... Inc, typename... Exc>
		[[nodiscard]] constexpr bool has_conflicts(owned_t<Coll...>, included_t<Inc...>, excluded_t<Exc...>) const noexcept
		{
			constexpr auto pred = [](const sorter_t &s) -> bool
			{
				if (const auto overlap = (0lu + ... + s.is_collected(type_info::get<Coll>())); overlap == 0)
					return false;
				else
				{
					const auto weak = (0lu + ... + s.is_included(type_info::get<Inc>())) +
									  (0lu + ... + s.is_excluded(type_info::get<Exc>()));
					const auto count = weak + overlap;
					return !(count == (sizeof...(Coll) + sizeof...(Inc) + sizeof...(Exc)) || count == s.type_count);
				}
			};
			return std::any_of(m_sorters.begin(), m_sorters.end(), pred);
		}

		storage_set m_storage;
		generic_event_type m_create;
		generic_event_type m_modify;
		generic_event_type m_remove;

		std::vector<sorter_t> m_sorters;
		std::vector<entity_t> m_entities;

		entity_t m_next = entity_t::tombstone();
		size_type m_size = 0; /* Amount of alive entities within the world. */
	};

	namespace detail
	{
		template<typename... C, typename... I, typename... E>
		struct collection_handler<owned_t<C...>, included_t<I...>, excluded_t<E...>>
		{
			[[nodiscard]] static collection_handler *make_handler(entity_world &world)
			{
				auto sorter = world.find_sorter(owned_t<C...>{}, included_t<I...>{}, excluded_t<E...>{});
				if (sorter.first == sorter.second)
				{
					SEK_ASSERT(!world.has_conflicts(owned_t<C...>{}, included_t<I...>{}, excluded_t<E...>{}),
							   "Conflicting collections detected");

					// clang-format off
					[[maybe_unused]] constexpr auto sub_include = []<typename T>(component_set<T> &set, const void *n, const void *p, collection_handler *h)
					{
						set.on_create().subscribe_before(n, delegate{delegate_func_t<&collection_handler::template handle_create<T>>{}, h});
						set.on_remove().subscribe_before(p, delegate{delegate_func_t<&collection_handler::template handle_remove<T>>{}, h});
					};
					[[maybe_unused]] constexpr auto sub_exclude = []<typename T>(component_set<T> &set, const void *n, const void *p, collection_handler *h)
					{
						set.on_create().subscribe_before(p, delegate{delegate_func_t<&collection_handler::template handle_remove<T>>{}, h});
						set.on_remove().subscribe_before(n, delegate{delegate_func_t<&collection_handler::template handle_create<T>>{}, h});
					};
					// clang-format on

					/* Next collection should be the more restricted one, while the previous is the less restricted one.
					 * Since collections sort their components, the most-restricted collection will sort inside the
					 * least-restricted one. */
					const auto next = world.next_sorter(owned_t<C...>{}, included_t<I...>{}, excluded_t<E...>{});
					const auto prev = world.prev_sorter(owned_t<C...>{}, included_t<I...>{}, excluded_t<E...>{});

					const void *next_handler = (next.first == next.second ? nullptr : next.first->get());
					const void *prev_handler = (prev.first == prev.second ? nullptr : prev.first->get());
					auto *handler = new collection_handler{};

					/* Handle addition and removal of new components for both included and excluded types. */
					(sub_include(world.template reserve<C>(), next_handler, prev_handler, handler), ...);
					(sub_include(world.template reserve<I>(), next_handler, prev_handler, handler), ...);
					(sub_exclude(world.template reserve<E>(), next_handler, prev_handler, handler), ...);

					/* Go through all collected entities & sort their components. */
					handler->template sort_entities<C...>(world);

					world.m_sorters.emplace_back(handler);
					return handler;
				}
				return static_cast<collection_handler *>(sorter.first->get());
			}

			template<typename T, typename... Ts>
			constexpr void sort_entities(entity_world &world)
			{
				using std::get;
				const auto storage = std::tuple<component_set<T> *, component_set<Ts> *...>(world.get_storage<T>(),
																							world.get_storage<Ts>()...);
				for (std::size_t count = get<component_set<T> *>(storage)->size(), i = 0; i != count; ++i)
				{
					const auto entity = get<component_set<T> *>(storage)->data()[i];
					const auto accept = (get<component_set<Ts> *>(storage)->contains(entity) && ...) &&
										((std::is_same_v<T, I> || world.get_storage<I>()->contains(entity)) && ...) &&
										((std::is_same_v<T, E> || !world.get_storage<E>()->contains(entity)) && ...);
					if (accept && size <= i)
					{
						constexpr auto swap_elements = []<typename U>(component_set<U> *storage, std::size_t a, entity_t e)
						{
							const auto b = storage->offset(e);
							if (a != b) storage->swap(a, b);
						};
						const auto last_pos = size++;
						(swap_elements(get<component_set<C> *>(storage), last_pos, entity), ...);
					}
				}
			}
			template<typename T>
			void handle_create(entity_world &world, entity_t entity)
			{
				using std::get;
				const auto storage = std::tuple<component_set<C> &...>{world.template reserve<C...>()};
				const auto accept = ((std::is_same_v<T, C> || get<component_set<C> &>(storage).contains(entity)) && ...) &&
									((std::is_same_v<T, I> || world.reserve<I>().contains(entity)) && ...) &&
									((std::is_same_v<T, E> || !world.reserve<E>().contains(entity)) && ...);

				/* If the offset of the accepted entity is greater than the current size of the collection,
				 * it should be appended to the collection. */
				if (accept && size <= get<0>(storage).offset(entity))
				{
					constexpr auto swap_elements = []<typename U>(component_set<U> &storage, std::size_t a, entity_t e)
					{
						const auto b = storage.offset(e);
						if (a != b) storage.swap(a, b);
					};
					const auto last_pos = size++;
					(swap_elements(get<component_set<C> &>(storage), last_pos, entity), ...);
				}
			}
			template<typename T>
			void handle_remove(entity_world &world, entity_t entity)
			{
				using std::get;
				const auto storage = std::tuple<component_set<C> &...>{world.template reserve<C...>()};

				/* If the removed entity is collected by the collection, decrement the collection size and
				 * remove the entity by swapping it to the end. */
				if (get<0>(storage).contains(entity) && get<0>(storage).offset(entity) < size)
				{
					constexpr auto swap_elements = []<typename U>(component_set<U> &storage, std::size_t a, entity_t e)
					{
						const auto b = storage.offset(e);
						if (a != b) storage.swap(a, b);
					};
					const auto last_pos = --size;
					(swap_elements(get<component_set<C> &>(storage), last_pos, entity), ...);
				}
			}

			std::size_t size = 0;
		};
		template<typename... I, typename... E>
		struct collection_handler<owned_t<>, included_t<I...>, excluded_t<E...>>
		{
			[[nodiscard]] static collection_handler *make_handler(entity_world &world)
			{
				auto sorter = world.find_sorter(owned_t<>{}, included_t<I...>{}, excluded_t<E...>{});
				if (sorter.first == sorter.second)
				{
					SEK_ASSERT(!world.has_conflicts(owned_t<>{}, included_t<I...>{}, excluded_t<E...>{}),
							   "Conflicting collections detected");

					[[maybe_unused]] constexpr auto sub_include = []<typename T>(component_set<T> &set, collection_handler *h)
					{
						set.on_create() += delegate{delegate_func_t<&collection_handler::template handle_create<T>>{}, h};
						set.on_remove() += delegate{delegate_func_t<&collection_handler::template handle_remove<T>>{}, h};
					};
					[[maybe_unused]] constexpr auto sub_exclude = []<typename T>(component_set<T> &set, collection_handler *h)
					{
						set.on_create() += delegate{delegate_func_t<&collection_handler::template handle_remove<T>>{}, h};
						set.on_remove() += delegate{delegate_func_t<&collection_handler::template handle_create<T>>{}, h};
					};
					auto *handler = new collection_handler{};

					/* Handle addition and removal of new components for both included and excluded types. */
					(sub_include(world.template reserve<I>(), handler), ...);
					(sub_exclude(world.template reserve<E>(), handler), ...);

					/* Fill the collection with the contents of a view. */
					const auto view = world.template view<I...>(excluded_t<E...>{});
					handler->entities.insert(view.begin(), view.end());

					world.m_sorters.emplace_back(handler);
					return handler;
				}
				else
					return static_cast<collection_handler *>(sorter.first->get());
			}

			template<typename C>
			void handle_create(entity_world &world, entity_t entity)
			{
				/* If the entity is accepted, try to insert it into the set. */
				if (((std::is_same_v<C, I> || world.reserve<I>().contains(entity)) && ...) &&
					((std::is_same_v<C, E> || !world.reserve<E>().contains(entity)) && ...))
					entities.try_insert(entity);
			}
			template<typename C>
			void handle_remove(entity_world &, entity_t entity)
			{
				/* Remove the asset. */
				entities.erase(entity);
			}

			entity_set entities;
		};

		template<typename T>
		constexpr auto get_opt(component_set<T> *set, entity_t e) noexcept -> T *
		{
			if (set == nullptr) [[unlikely]]
				return nullptr;

			const auto pos = set->find(e);
			if (pos == set->end()) [[unlikely]]
				return nullptr;

			return std::addressof(pos->second);
		}
		template<typename T>
		constexpr auto get_opt(const component_set<T> *set, entity_t e) noexcept -> std::add_const_t<T> *
		{
			if (set == nullptr) [[unlikely]]
				return nullptr;

			const auto pos = set->find(e);
			if (pos == set->end()) [[unlikely]]
				return nullptr;

			return std::addressof(pos->second);
		}
	}	 // namespace detail
}	 // namespace sek::engine