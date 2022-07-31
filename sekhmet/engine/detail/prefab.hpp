/*
 * Created by switchblade on 25/07/22
 */

#pragma once

#include "sekhmet/serialization/ubjson.hpp"

#include "ecs/world.hpp"

namespace sek::engine
{
	class prefab_ctx;

	/** @brief Resource used to store a serialized collection of entities and their components. */
	class prefab_resource
	{
		friend class prefab_ctx;

		using id_t = ssize_t;

	public:
		// clang-format off
		typedef sek::serialization::json_tree data_tree;
		typedef sek::serialization::ubj::input_archive input_archive;
		typedef sek::serialization::ubj::basic_output_archive<
		    sek::serialization::ubj::fixed_type |
			sek::serialization::ubj::fixed_size> output_archive;
		// clang-format on

	private:
		struct size_read_proxy
		{
			void deserialize(auto &a) const { a >> res.m_size; }
			prefab_resource &res;
		};

	public:
		prefab_resource(data_tree &&data) : m_data(std::move(data))
		{
			input_archive in{m_data};
			in >> size_read_proxy{*this};
		}

		/** Converts an internal id to an entity. If the entity with such id does not exist within the specified world, generates a new entity. */
		[[nodiscard]] constexpr entity_t resolve(entity_world &world, id_t id)
		{
			/* If the id is greater than size of the entity set, entity does not exist yet and needs to be created. */
			if (auto diff = static_cast<id_t>(m_entities.size()) - id; diff > 0) [[unlikely]]
			{
				m_entities.reserve(static_cast<std::size_t>(id));
				while (diff-- != 0) m_entities.insert(world.generate());
			}
			/* Now, all entities up to `id` exist within the set. */
			return m_entities.at(static_cast<std::size_t>(id));
		}

		/** Creates a component pointer for the specified prefab id. */
		template<typename C>
		[[nodiscard]] constexpr component_ptr<C> make_ptr(entity_world &world, id_t id)
		{
			if (id >= 0) [[likely]]
			{
				auto &storage = world.template storage<C>();
				const auto entity = resolve(world, id);
				return {entity, storage};
			}
			return {};
		}

	private:
		std::size_t m_size; /* Total amount of entities stored within the prefab. */
		data_tree m_data;	/* Serialized data of the prefab. */
	};

	/** @brief Serialization context used for prefab resources. */
	class prefab_ctx
	{
		using id_t = typename prefab_resource::id_t;

	public:
		/** Initializes prefab context for the specified prefab and world. */
		constexpr prefab_ctx(prefab_resource &prefab, entity_world &world) noexcept : m_prefab(prefab), m_world(world)
		{
		}

		/** @copydoc prefab_resource::take */
		[[nodiscard]] constexpr id_t take(entity_t entity) { return m_prefab.take(entity); }
		/** @copydoc prefab_resource::try_take */
		[[nodiscard]] constexpr id_t try_take(entity_t entity) const { return m_prefab.try_take(entity); }

		/** @copydoc prefab_resource::resolve */
		[[nodiscard]] constexpr entity_t resolve(id_t id) { return m_prefab.resolve(m_world, id); }

		/** @copydoc prefab_resource::make_ptr */
		template<typename C>
		[[nodiscard]] constexpr component_ptr<C> make_ptr(id_t id)
		{
			return m_prefab.template make_ptr<C>(m_world, id);
		}

	private:
		prefab_resource &m_prefab;
		entity_world &m_world;
	};

	template<typename T>
	void deserialize(component_ptr<T> &ptr, auto &a, prefab_ctx &ctx, auto &&...)
	{
		ptr = ctx.template make_ptr<T>(a.read(std::in_place_type<ssize_t>));
	}
	template<typename T>
	void serialize(const component_ptr<T> &ptr, auto &a, prefab_ctx &ctx, auto &&...)
	{
		ssize_t id = -1;
		if (!ptr.empty()) [[likely]]
			id = ctx.try_take(ptr.entity());
		a << sek::serialization::keyed_entry("id", id);
	}

	template<typename... Args>
	void deserialize(prefab_resource &prefab, auto &a, Args &&...args)
	{
		using namespace sek::serialization;
		a << keyed_entry("entities", prefab.)
	}
	template<typename... Args>
	void serialize(const prefab_resource &ptr, auto &a, Args &&...args)
	{
		using namespace sek::serialization;
	}

	namespace attributes
	{
		/** @brief Attribute used to enable the use of a component within a prefab. */
		class prefab_component
		{
			template<typename T, typename... Args>
			static auto *make_factory(Args &&...a)
			{
				// clang-format off
				static const auto f = [... args = std::move(a)](entity_world &world, entity_t e) -> decltype(auto)
				{
					return world.template emplace<T>(e, std::forward<Args>(args)...);
				};
				// clang-format on
				return &f;
			}

		public:
			prefab_component() = delete;

			template<typename T, typename... Args>
			explicit prefab_component(type_selector_t<T>, Args &&...args)
			{
				const auto *factory = make_factory<T>(std::forward<Args>(args)...);
				using factory_t = decltype(factory);

				m_contains = +[](const entity_world &world, entity_t e) { return world.template contains_all<T>(e); };
				m_try_insert = +[](const void *ptr, entity_world &world, entity_t e) -> std::pair<any_ref, bool>
				{
					const auto &f = *static_cast<factory_t>(ptr);
					auto &storage = world.template storage<T>();
					if (const auto existing = storage.find(e); existing != storage.end()) [[likely]]
						return {any_ref{forward_any(existing->second)}, false};
					else
						return {any_ref{forward_any(f(world, e))}, true};
				};
				m_insert = +[](const void *ptr, entity_world &world, entity_t e)
				{
					const auto &f = *static_cast<factory_t>(ptr);
					return any_ref{forward_any(f(world, e))};
				};
				m_erase = +[](entity_world &world, entity_t e) { world.template erase<T>(e); };
				m_factory = factory;
			}

			/** Checks if the world contains the bound component.
			 * @param world World to check.
			 * @param entity Entity to check. */
			[[nodiscard]] bool contains(const entity_world &world, entity_t entity) const
			{
				return m_contains(world, entity);
			}

			/** Attempts to insert the bound component into the specified world for the specified entity.
			 * @param world World to insert the component into.
			 * @param entity Entity to insert the component for.
			 * @return Pair where first is `any_ref` reference to the inserted component and second is a boolean
			 * indicating whether the component was inserted (`true` if inserted, `false` otherwise). */
			std::pair<any_ref, bool> try_insert(entity_world &world, entity_t entity) const
			{
				return m_try_insert(m_factory, world, entity);
			}
			/** Inserts the bound component into the specified world for the specified entity.
			 * @param world World to insert the component into.
			 * @param entity Entity to insert the component for.
			 * @return `any_ref` reference to the inserted component.
			 * @warning Using entity that already has the bound component will result in undefined behavior. */
			any_ref insert(entity_world &world, entity_t entity) const { return m_insert(m_factory, world, entity); }
			/** Erases the component from the entity.
			 * @param world World to erase the component from.
			 * @param entity Entity to erase the component from.
			 * @warning Using entity that does not have the bound component will result in undefined behavior. */
			void erase(entity_world &world, entity_t entity) const { m_erase(world, entity); }

		private:
			bool (*m_contains)(const entity_world &, entity_t);

			std::pair<any_ref, bool> (*m_try_insert)(const void *, entity_world &, entity_t);
			any_ref (*m_insert)(const void *, entity_world &, entity_t);
			void (*m_erase)(entity_world &, entity_t);
			const void *m_factory;
		};

		/** Helper function used to create an instance of `prefab_component` attribute for type `T`.
		 * @tparam T Component type to make accessible at runtime.
		 * @param args Optional arguments passed to component's constructor to bind with the runtime factory. */
		template<typename T, typename... Args>
		[[nodiscard]] prefab_component make_prefab_component(Args &&...args) noexcept
		{
			return prefab_component{type_selector<T>, std::forward<Args>(args)...};
		}
	}	 // namespace attributes
}	 // namespace sek::engine