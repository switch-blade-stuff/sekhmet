/*
 * Created by switchblade on 19/07/22
 */

#pragma once

#include "component_set.hpp"

namespace sek::engine
{
	template<typename... Cs>
	struct collected_t
	{
		using type = type_seq_t<Cs...>;

		constexpr collected_t() noexcept = default;
		constexpr collected_t(type_seq_t<Cs...>) noexcept {}
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

	/** @brief Query structure used to build a component collection or a view.
	 *
	 * @tparam W World type used for the query.
	 * @tparam C Component types collected by the query.
	 * @tparam I Component types included by the query.
	 * @tparam E Component types excluded from the query.
	 * @tparam O Component types optional to the query (must be included).
	 *
	 * @note Excluded components must not be the same as collected, included and optional.
	 * @note Collected components cannot be fixed-storage (`component_traits::is_fixed` must not be defined).
	 * @note Collecting queries can only be created for non-constant worlds. */
	template<typename W, typename... C, typename... I, typename... E, typename... O>
	class entity_query<W, collected_t<C...>, included_t<I...>, excluded_t<E...>, optional_t<O...>>
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

		static_assert(sizeof...(C) == 0 || is_read_only, "Collections are not available for read-only queries");
		static_assert(!(is_fixed<C> || ...), "Cannot collect fixed-storage components");

		template<typename... Ts>
		using collect_query =
			entity_query<W, collected_t<C..., Ts...>, included_t<I...>, excluded_t<E...>, optional_t<O...>>;
		template<typename... Ts>
		using include_query =
			entity_query<W, collected_t<C...>, included_t<I..., Ts...>, excluded_t<E...>, optional_t<O...>>;
		template<typename... Ts>
		using exclude_query =
			entity_query<W, collected_t<C...>, included_t<I...>, excluded_t<E..., Ts...>, optional_t<O...>>;
		template<typename... Ts>
		using optional_query =
			entity_query<W, collected_t<C...>, included_t<I...>, excluded_t<E...>, optional_t<O..., Ts...>>;

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
			return include_query<Cs...>{*m_parent};
		}
		/** Returns a new query with `Cs` components added to the excluded components list.
		 * @return New query instance. */
		template<typename... Cs>
		constexpr auto exclude() const
		{
			return exclude_query<Cs...>{*m_parent};
		}
		/** Returns a new query with `Cs` components added to the optional components list.
		 * @return New query instance. */
		template<typename... Cs>
		constexpr auto optional() const
		{
			return optional_query<Cs...>{*m_parent};
		}

		/** Returns a new query with `Cs` components added to the collected components list.
		 * @return New query instance.
		 * @note Collecting queries are only allowed for non-const worlds. */
		template<typename... Cs>
		constexpr auto collect() const
		{
			return collect_query<Cs...>{*m_parent};
		}
		/** Returns a component collection made using this query.
		 * @note Collections are only allowed for non-const worlds.
		 * @note Collections sort collected components and track any modifications to component sets. */
		[[nodiscard]] constexpr auto collection() const;

		/** Returns a component view made using this query.
		 * @note Views ignore collected components. */
		[[nodiscard]] constexpr auto view() const;

	private:
		W *m_parent;
	};

	template<typename W>
	entity_query(W &) -> entity_query<W, collected_t<>, included_t<>, excluded_t<>, optional_t<>>;
	template<typename W>
	entity_query(const W &) -> entity_query<const W, collected_t<>, included_t<>, excluded_t<>, optional_t<>>;
}	 // namespace sek::engine