/*
 * Created by switchblade on 19/07/22
 */

#pragma once

#include "component_set.hpp"
#include "component_view.hpp"

namespace sek::engine
{
	template<typename... Cs>
	struct order_by_t
	{
		using type = type_seq_t<Cs...>;

		constexpr order_by_t() noexcept = default;
		constexpr order_by_t(type_seq_t<Cs...>) noexcept {}
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

	/** @brief Helper structure used to store filter predicates for entity query.
	 * @tparam P Predicate stored by the filter.
	 * @tparam Cs Component types captured by the filter. */
	template<typename P, typename... Cs>
	class filter_t : ebo_base_helper<P>
	{
		using ebo_base = ebo_base_helper<P>;

	public:
		filter_t() = delete;
		filter_t(const filter_t &) = delete;
		filter_t &operator=(const filter_t &) = delete;

		constexpr filter_t(filter_t &&) noexcept(std::is_nothrow_move_constructible_v<P>) = default;
		constexpr filter_t &operator=(filter_t &&) noexcept(std::is_nothrow_move_assignable_v<P>) = default;

		constexpr filter_t(P &&p) noexcept(std::is_nothrow_move_constructible_v<P>) : ebo_base{std::forward<P>(p)} {}

		[[nodiscard]] constexpr bool operator()(const Cs *...cs) const
			noexcept(std::is_nothrow_invocable_r_v<bool, P, const Cs *...>)
		{
			return std::invoke(*ebo_base::get(), cs...);
		}

		constexpr void swap(filter_t &other) noexcept { ebo_base::swap(other); }
		friend constexpr void swap(filter_t &a, filter_t &b) noexcept { a.swap(b); }
	};

	namespace detail
	{
		template<typename...>
		struct world_query_handler;

		template<typename World, typename... OrderBy, typename... Required, typename... Optional, typename... Exclude, typename... Filters>
		struct world_query_handler<World, type_seq_t<OrderBy...>, type_seq_t<Required...>, type_seq_t<Optional...>, type_seq_t<Exclude...>, type_seq_t<Filters...>>
		{
			world_query_handler() = delete;

			constexpr world_query_handler(World *parent) noexcept : m_parent(parent) {}
			constexpr world_query_handler(World *parent, std::tuple<Filters...> &&filters)
				: m_parent(parent), m_filters(std::move(filters))
			{
			}

			template<typename... Ts>
			constexpr world_query_handler(world_query_handler<Ts...> &&other)
				: world_query_handler(other.m_parent, std::move(other.m_filters))
			{
			}

			template<typename... Ts, typename P, typename... Cs>
			constexpr world_query_handler(world_query_handler<Ts...> &&other, filter_t<P, Cs...> &&filter)
				: m_parent(other.m_parent),
				  m_filters(std::tuple_cat(std::move(other.m_filters), std::forward_as_tuple(filter)))
			{
			}

			constexpr bool check_include(entity_t);
			constexpr bool check_exclude(entity_t);

			constexpr void swap(world_query_handler &other) noexcept
			{
				std::swap(m_parent, other.m_parent);
				std::swap(m_filters, other.m_filters);
			}

			World *m_parent;
			std::tuple<Filters...> m_filters;
		};

		template<typename W, typename... OB, typename... R, typename... O, typename... E, typename... F, typename P, typename... C>
		world_query_handler(
			world_query_handler<W, type_seq_t<OB...>, type_seq_t<R...>, type_seq_t<O...>, type_seq_t<E...>, type_seq_t<F...>> &&,
			filter_t<P, C...> &&)
			-> world_query_handler<W, type_seq_t<OB...>, type_seq_t<R...>, type_seq_t<O...>, type_seq_t<E...>, type_seq_t<F..., filter_t<P, C...>>>;

	}	 // namespace detail

	/** @brief Query structure used to filter components of a world and build a component collection or a view.
	 *
	 * @tparam World World type used for the query.
	 * @tparam OrderBy Component types to order the query by.
	 * @tparam Included Component types included by the query.
	 * @tparam Excluded Component types excluded from the query.
	 * @tparam Optional Component types optional to the query (must be included).
	 *
	 * @note Included and excluded component sets must not intersect.
	 * @note Ordered-by components cannot be fixed-storage (`component_traits::is_fixed` must not be defined).
	 * @note Ordered-by components are implicitly included. */
	template<typename World, typename... OrderBy, typename... Included, typename... Excluded, typename... Optional>
	class entity_query<World, order_by_t<OrderBy...>, included_t<Included...>, excluded_t<Excluded...>, optional_t<Optional...>>
	{
		template<typename... Append>
		constexpr static bool allow_ordering = !std::is_const_v<World> || (sizeof...(OrderBy) + sizeof...(Append) == 0);

		static_assert(!(is_in_v<std::remove_cv_t<Excluded>, std::remove_cv_t<OrderBy>..., std::remove_cv_t<Included>...> || ...),
					  "Excluded and included component types must not intersect");
		static_assert((is_in_v<std::remove_cv_t<Optional>, std::remove_cv_t<Included>...> && ...),
					  "Optional component types must be included");

		static_assert((!fixed_component<std::remove_cv_t<OrderBy>> && ...), "Cannot order fixed-storage components");
		static_assert(allow_ordering<>, "Cannot order constant world");

		friend class entity_world;

		template<typename T>
		using is_opt = is_in<T, std::remove_cv_t<Optional>...>;
		template<typename S>
		using is_opt_set = is_opt<typename S::component_type>;

		template<typename T>
		constexpr static bool is_opt_v = is_opt<T>::value;
		template<typename S>
		constexpr static bool is_opt_set_v = is_opt_set<S>::value;

	public:
		entity_query() = delete;
		entity_query(const entity_query &) = delete;
		entity_query &operator=(const entity_query &) = delete;

		constexpr entity_query(entity_query &&) noexcept = default;
		constexpr entity_query &operator=(entity_query &&) noexcept = default;

		/** Initializes an entity query for the specified parent world.
		 * @param parent Reference to the parent entity world. */
		constexpr explicit entity_query(World &parent) noexcept : entity_query(parent, included_t<Included...>{}) {}
		/** @copydoc entity_query
		 * @param imc Instance of `included_t` helper structure used to specify the included component types. */
		constexpr entity_query(World &parent, included_t<Included...> inc) noexcept
			: entity_query(parent, inc, excluded_t<Excluded...>{})
		{
		}
		/** @copydoc entity_query
		 * @param exc Instance of `excluded_t` helper structure used to specify the excluded component types. */
		constexpr entity_query(World &parent, included_t<Included...> req, excluded_t<Excluded...> exc) noexcept
			: entity_query(parent, req, exc, optional_t<Optional...>{})
		{
		}
		/** @copydoc entity_query
		 * @param opt Instance of `optional_t` helper structure used to specify the optional component types. */
		constexpr entity_query(World &parent, included_t<Included...> req, excluded_t<Excluded...> exc, optional_t<Optional...> opt) noexcept
			: entity_query(parent, order_by_t<OrderBy...>{}, req, exc, opt)
		{
		}

		// clang-format off
		/** @copydoc entity_query
		 * @param ord Instance of `order_by_t` helper structure used to specify the ordered-by component types.
		 * @note Ordering queries are only allowed for non-const worlds. */
		constexpr entity_query(World &parent,
							  order_by_t<OrderBy...> ord [[maybe_unused]],
							  included_t<Included...> req [[maybe_unused]],
							  excluded_t<Excluded...> exc [[maybe_unused]],
							  optional_t<Optional...> opt [[maybe_unused]]) noexcept
			requires allow_ordering<> : m_parent(&parent)
		{
		}

		/** Returns a new query with `Cs` components added to the order-by components list.
		 * @return New query instance.
		 * @note Old query instance is left in an invalid state.
		 * @note Ordering queries are only allowed for non-const worlds. */
		template<typename... Cs>
		constexpr auto order_by() requires allow_ordering<Cs...>;
		// clang-format on

		/** Returns a new query with `Cs` components added to the required components list.
		 * @return New query instance.
		 * @note Old query instance is left in an invalid state. */
		template<typename... Cs>
		constexpr auto require();
		/** Returns a new query with `Cs` components added to the optional components list.
		 * @return New query instance.
		 * @note Old query instance is left in an invalid state. */
		template<typename... Cs>
		constexpr auto optional();
		/** Returns a new query with `Cs` components added to the excluded components list.
		 * @return New query instance.
		 * @note Old query instance is left in an invalid state. */
		template<typename... Cs>
		constexpr auto exclude();

		/** Returns a new query, with the specified functor added to the filter list.
		 * @tparam Cs Components the filter is applied to.
		 * @param p Predicate used to conditionally select entities of the query.
		 * @return New query instance.
		 * @note Old query instance is left in an invalid state. */
		template<typename... Cs, typename P>
		constexpr auto filter(P &&p);

		/** Returns a component view made using this query. */
		template<typename Alloc = std::allocator<entity_t>>
		[[nodiscard]] constexpr auto view(const Alloc &alloc = Alloc{}) const
		{
			return view({std::addressof(m_parent->template storage<std::remove_cv_t<Included>>())...},
						{std::addressof(m_parent->template storage<std::remove_cv_t<Excluded>>())...},
						alloc);
		}

	private:
		template<typename Alloc, typename... I, typename... E>
		[[nodiscard]] constexpr auto view(std::tuple<I *...> inc_data, std::tuple<E *...> exc_data, const Alloc &alloc) const
		{
			using view_t = component_view<included_t<Included...>, optional_t<Optional...>, Alloc>;
		}

		template<typename... I, typename... E>
		[[nodiscard]] constexpr static bool accept(entity_t entity, std::tuple<I *...> inc_data, std::tuple<E *...> exc_data) noexcept
		{
			return ((is_opt_set_v<I> || get<I>(inc_data)->contains(entity)) && ...) &&
				   !(get<E>(exc_data)->contains(entity) && ...);
		}
		template<typename... I, typename... E>
		[[nodiscard]] constexpr static bool reject(entity_t entity, std::tuple<I *...> inc_data, std::tuple<E *...> exc_data) noexcept
		{
			return (get<E>(exc_data)->contains(entity) || ...) ||
				   (!(is_opt_set_v<I> || get<I>(inc_data)->contains(entity)) && ...);
		}

		World *m_parent;
	};

	template<typename W>
	entity_query(W &) -> entity_query<W, order_by_t<>, included_t<>, excluded_t<>, optional_t<>>;
}	 // namespace sek::engine