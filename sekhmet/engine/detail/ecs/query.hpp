/*
 * Created by switchblade on 19/07/22
 */

#pragma once

#include "collection.hpp"
#include "view.hpp"

namespace sek::engine
{
	/** @brief Query structure used to build a component collection or a view.
	 *
	 * @tparam W World type used for the query.
	 * @tparam C Component types collected by the query (owned by a collection made from the query).
	 * @tparam I Component types included by the query.
	 * @tparam E Component types excluded from the query.
	 * @tparam O Component types optional to the query (must be included).
	 *
	 * @note Excluded components must not be the same as collected, included and optional.
	 * @note Collected components cannot be fixed-storage (`component_traits::is_fixed` must not be defined).
	 * @note Collecting queries can only be created for non-constant worlds. */
	template<typename W, typename... C, typename... I, typename... E, typename... O>
	class entity_query<W, owned_t<C...>, included_t<I...>, excluded_t<E...>, optional_t<O...>>
	{
		friend class entity_world;

		constexpr static bool is_read_only = std::is_const_v<W>;

		template<typename T>
		constexpr static bool is_fixed = fixed_component<std::remove_cv_t<T>>;
		template<typename T>
		constexpr static bool is_coll = is_in_v<std::remove_cv_t<T>, std::remove_cv_t<C>...>;
		template<typename T>
		constexpr static bool is_inc = is_in_v<std::remove_cv_t<T>, std::remove_cv_t<I>...>;
		template<typename T>
		constexpr static bool is_opt = is_in_v<std::remove_cv_t<T>, std::remove_cv_t<O>...>;

		static_assert(!(is_fixed<C> || ...), "Cannot collect fixed-storage components");

		template<typename... Ts>
		using collect_query =
			entity_query<W, owned_t<C..., Ts...>, included_t<I...>, excluded_t<E...>, optional_t<O...>>;
		template<typename... Ts>
		using include_query =
			entity_query<W, owned_t<C...>, included_t<I..., Ts...>, excluded_t<E...>, optional_t<O...>>;
		template<typename... Ts>
		using exclude_query =
			entity_query<W, owned_t<C...>, included_t<I...>, excluded_t<E..., Ts...>, optional_t<O...>>;
		template<typename... Ts>
		using optional_query =
			entity_query<W, owned_t<C...>, included_t<I...>, excluded_t<E...>, optional_t<O..., Ts...>>;

	public:
		entity_query() = delete;
		entity_query(const entity_query &) = delete;
		entity_query &operator=(const entity_query &) = delete;

		constexpr entity_query(entity_query &&) noexcept = default;
		constexpr entity_query &operator=(entity_query &&) noexcept = default;

		/** Initializes an entity query for the specified parent world.
		 * @param parent Reference to the parent entity world. */
		constexpr explicit entity_query(W &parent) noexcept : m_parent(&parent) {}

		/** Returns a new query with `Cs` components added to the included components list.
		 * @return New query instance. */
		template<typename... Cs>
		constexpr auto include() const
		{
			return include_query<transfer_cv_t<W, Cs>...>{*m_parent};
		}
		/** Returns a new query with `Cs` components added to the excluded components list.
		 * @return New query instance. */
		template<typename... Cs>
		constexpr auto exclude() const
		{
			return exclude_query<transfer_cv_t<W, Cs>...>{*m_parent};
		}
		/** Returns a new query with `Cs` components added to the optional components list.
		 * @return New query instance. */
		template<typename... Cs>
		constexpr auto optional() const
		{
			return optional_query<transfer_cv_t<W, Cs>...>{*m_parent};
		}

		/** Returns a new query with `Cs` components added to the collected (owned) components list.
		 * @return New query instance.
		 * @note Collected components are implicitly included.
		 * @note Collecting queries are only allowed for non-const worlds. */
		template<typename... Cs>
		constexpr auto collect() const
		{
			static_assert(!is_read_only, "Collections are not available for read-only queries");
			return collect_query<Cs...>{*m_parent};
		}
		/** Returns a component collection made using this query.
		 * @note Collections are only allowed for non-const worlds.
		 * @note Collections sort owned components and track any modifications to component sets. */
		[[nodiscard]] constexpr auto collection() const
		{
			static_assert(!is_read_only, "Collections are not available for read-only queries");
			using handler_t = detail::collection_handler<owned_t<C...>, included_t<I...>, excluded_t<E...>>;

			// clang-format off
			return component_collection<owned_t<C...>, included_t<I...>, excluded_t<E...>, optional_t<O...>>{
				handler_t::make_handler(*m_parent),
				m_parent->template storage<C>()...,
				m_parent->template storage<I>()...,
				m_parent->template storage<O>()...
			};
			// clang-format on
		}

		/** Returns a component view made using this query.
		 * @note Views ignore collected components. */
		[[nodiscard]] constexpr auto view() const
		{
			// clang-format off
			return component_view<included_t<I...>, excluded_t<E...>, optional_t<O...>>{
				m_parent->template storage<I>()...,
				m_parent->template storage<E>()...,
				m_parent->template storage<O>()...
			};
			// clang-format on
		}

	private:
		W *m_parent;
	};

	template<typename W>
	entity_query(W &) -> entity_query<W, owned_t<>, included_t<>, excluded_t<>, optional_t<>>;
	template<typename W>
	entity_query(const W &) -> entity_query<const W, owned_t<>, included_t<>, excluded_t<>, optional_t<>>;

	constexpr auto entity_world::query() noexcept { return entity_query{*this}; }
	constexpr auto entity_world::query() const noexcept { return entity_query{*this}; }

	template<typename... I, typename... E, typename... O>
	constexpr auto entity_world::view(excluded_t<E...>, optional_t<O...>) noexcept
	{
		return query().template include<I...>().template exclude<E...>().template optional<O...>().view();
	}
	template<typename... I, typename... E, typename... O>
	constexpr auto entity_world::view(excluded_t<E...>, optional_t<O...>) const noexcept
	{
		return query().template include<I...>().template exclude<E...>().template optional<O...>().view();
	}

	template<typename... C, typename... I, typename... E, typename... O>
	constexpr auto entity_world::collection(included_t<I...>, excluded_t<E...>, optional_t<O...>) noexcept
	{
		return query()
			.template collect<C...>()
			.template include<I...>()
			.template exclude<E...>()
			.template optional<O...>()
			.collection();
	}

}	 // namespace sek::engine